// it has no dynamic alloc(fixed buffers only)
// all rendering go through vga_set_cell equiv
// we write to vga buuffer directly btw,bypassing vga_puychar
// so we never accidently scroll or advance the main cursor
#include "vi.h"
#include "../core/vga.h"
#include "../fs/vfs.h"
#include "../drivers/keyboard.h"
#include "../lib/string.h"

// editor states
typedef enum {
	MODE_NORMAL = 0,
	MODE_INSERT,
	MODE_COMMAND,
	MODE_SEARCH,
} vi_mode_t;

//text buffer
static char buf[VI_MAX_LINES][VI_MAX_LINE_LEN];
static int line_len[VI_MAX_LINES];
static int num_lines;

//cursor
static int cur_row;
static int cur_col;

//viewport
static int view_top;

//editor state
static vi_mode_t mode;
static int dirty;
static char filename[64];

//command/search input
static char cmd_buf[80];
static int cmd_len;

//status message(shown in row 22 briefly)
static char status_msg[80];
static int status_msg_ticks;

//undo(new learning btw)
static char undo_buf[VI_MAX_LINES][VI_MAX_LINE_LEN];
static int undo_line_len[VI_MAX_LINES];
static int undo_num_lines;
static int undo_cur_row, undo_cur_col;
static int undo_valid;

//last search string
static char last_search[80];

//direct vga write, we passs vga putchar to write at exact row/col without moving cursor row/ cusror col or triggering scroll.

static unsigned short* const vi_vga = (unsigned short*)VGA_ADDRESS;

static inline void vi_set_cell(int row, int col, char c, uint8_t color) {
	vi_vga[row * VI_TEXT_COLS + col] = (unsigned short)((color << 8) | (unsigned char)c);
}

static void vi_write_str(int row, int col, const char* s, uint8_t color) {
	int c = col;
	while (*s && c < VI_TEXT_COLS) {
		vi_set_cell(row, c++, *s++, color);
	}
}

static void vi_clear_row_range(int row, int from_col, int to_col, uint8_t color) {
	for (int c = from_col; c < to_col; c++)
		vi_set_cell(row, c, ' ', color);
}

//buf operation

static void buf_clamp_cursor(void) {
	if (cur_row < 0) cur_row = 0;
	if (cur_row >= num_lines) cur_row = num_lines - 1;
	if (cur_row < 0) cur_row = 0;

	int max_col = line_len[cur_row];
	// in n mode, cursor stops before last char
	if (mode == MODE_NORMAL && max_col > 0) max_col--;
	if (max_col < 0) max_col = 0;

	if (cur_col > max_col) cur_col = max_col;
	if (cur_col < 0) cur_col = 0;
}

static void buf_insert_char(char c) {
	int row = cur_row;
	int col = cur_col;
	int len = line_len[row];

	if (len >= VI_MAX_LINE_LEN - 1) return;

	// shift chars right
	for (int i = len; i > col; i--)
		buf[row][i] = buf[row][i - 1];

	buf[row][col] = c;
	buf[row][len+1] = '\0';
	line_len[row]++;
	cur_col++;
	dirty = 1;
}

static void buf_delete_char(int row, int col) {
	int len = line_len[row];
	if (len == 0 || col >= len) return;

	for (int i = col; i < len-1; i++) 
		buf[row][i] = buf[row][i + 1];

	buf[row][len - 1] = '\0';
	line_len[row]--;
	dirty = 1;
}

static void buf_split_line(void) {
	if (num_lines >= VI_MAX_LINES) return;

	int row = cur_row;
	int col = cur_col;
	int len = line_len[row];

	//shift all lines below dowm
	for (int r = num_lines; r > row+1; r--) {
		memcpy(buf[r], buf[r-1], VI_MAX_LINE_LEN);
		line_len[r] = line_len[r-1];
	}
	//new lines = text from col onward
	int new_len = len - col;
	memcpy(buf[row+1], buf[row]+col, (size_t)new_len);
	buf[row+1][new_len] = '\0';
	line_len[row+1] = new_len;

	// current line = text up to col
	buf[row][col] = '\0';
	line_len[row] = col;

	num_lines++;
	cur_row++;
	cur_col = 0;
	dirty = 1;
}

static void buf_join_lines(int row) {
	if (row >= num_lines - 1) return;

	int len1 = line_len[row];
	int len2 = line_len[row + 1];

	if (len1 + len2 >= VI_MAX_LINE_LEN) return;

	// append next line to current
	memcpy(buf[row] + len1, buf[row+1], (size_t)len2);
	buf[row][len1 + len2] = '\0';
	line_len[row] = len1 + len2;

	//shift remaining lines up
	for (int r = row + 1; r < num_lines - 1; r++) {
		memcpy(buf[r], buf[r+1], VI_MAX_LINE_LEN);
		line_len[r] = line_len[r+1];
	}
	num_lines--;
}

static void buf_delete_line(int row) {
	if (num_lines == 1) {
		// ll- just clear bruh
		buf[0][0] = '\0';
		line_len[0] = 0;
		cur_col = 0;
		dirty = 1;
		return;
	}
	for (int r = row; r < num_lines -1; r++) {
		memcpy(buf[r], buf[r+1], VI_MAX_LINE_LEN);
		line_len[r] = line_len[r+1];
	}
	num_lines--;
	dirty = 1;
	buf_clamp_cursor();
}

static void buf_insert_line_after(int row) {
	if (num_lines >= VI_MAX_LINES) return;
	for (int r = num_lines; r > row + 1; r--) {
		memcpy(buf[r], buf[r-1], VI_MAX_LINE_LEN);
		line_len[r] = line_len[r-1];
	}
	buf[row+1][0] = '\0';
	line_len[row+1] = 0;
	num_lines++;
	cur_row = row + 1;
	cur_col = 0;
	dirty = 1;
}

static void buf_insert_line_before(int row) {
	if (num_lines >= VI_MAX_LINES) return;
	for (int r = num_lines; r > row; r--) {
		memcpy(buf[r], buf[r-1], VI_MAX_LINE_LEN);
		line_len[r] = line_len[r-1];
	}
	buf[row][0] = '\0';
	line_len[row] = 0;
	num_lines++;
	cur_row = row;
	cur_col = 0;
	dirty = 1;
}

// undo, snapshot before destructionnn
static void undo_snapshot(void) {
	for (int r = 0; r < num_lines; r++) {
		memcpy(undo_buf[r], buf[r], VI_MAX_LINE_LEN);
		undo_line_len[r] = line_len[r];
	}

	undo_num_lines = num_lines;
	undo_cur_row = cur_row;
	undo_cur_col = cur_col;
	undo_valid = 1;
}

static void undo_restore(void) {
	if (!undo_valid) {
		memcpy(status_msg, "Nothing to undo", sizeof("Nothing to undo"));
		return;
	}
	for (int r = 0; r < undo_num_lines; r++) {
		memcpy(buf[r], undo_buf[r], VI_MAX_LINE_LEN);
		line_len[r] = undo_line_len[r];
	}
	num_lines = undo_num_lines;
	cur_row = undo_cur_row;
	cur_col = undo_cur_col;
	undo_valid = 0;
	dirty = 1;
	memcpy(status_msg, "Undo.", sizeof("Undo."));
}

// search

static void vi_search_next(void) {
	if (last_search[0] == '\0') return;

	int slen = strlen(last_search);
	int start_row = cur_row;
	int start_col = cur_col + 1;

	for (int pass = 0; pass < 2; pass++) {
		int r_start = (pass == 0) ? start_row : 0;
		int c_start = (pass == 0) ? start_col : 0;

		for (int r = r_start; r < num_lines; r++) {
			int c_begin = (r == r_start && pass == 0) ? c_start : 0;
			for (int c = c_begin; c<= line_len[r] - slen; c++) {
				if (memcmp(buf[r] + c, last_search, (size_t)slen) == 0) {
					cur_row = r;
					cur_col = c;

					// scroll viewport if needed
					if (cur_row < view_top)
						view_top = cur_row;
					if (cur_row >= view_top + VI_TEXT_ROWS)
						view_top = cur_row - VI_TEXT_ROWS+1;
					memcpy(status_msg, "Match found.", sizeof("Match found."));
					return;
				}
			}
		}
		// second pass searches from top
		if (pass == 0 && start_row == 0 && start_col == 1) break;
	}
	memcpy(status_msg, "Pattern not found.", sizeof("Pattern not found"));
}

// Viewport
static void viewport_scroll_to_cursor(void) {
	if (cur_row < view_top)
		view_top = cur_row;
	if (cur_row >= view_top + VI_TEXT_ROWS)
		view_top = cur_row - VI_TEXT_ROWS + 1;
	if (view_top < 0)
		view_top = 0;
}

// Renderer
static void vi_render(void) {
	// text area
	for (int screen_row = 0; screen_row < VI_TEXT_ROWS; screen_row++){
		int buf_row = view_top + screen_row;

		vi_clear_row_range(screen_row, 0, VI_TEXT_COLS, COLOR_DEFAULT);

		if (buf_row >= num_lines) {
			vi_set_cell(screen_row, 0, '~', COLOR_GREY);
			continue;
		}

		//render line cont
		int len = line_len[buf_row];
		int draw_len = len < VI_TEXT_COLS ? len : VI_TEXT_COLS;

		for (int c = 0; c < draw_len; c++) {
			uint8_t color = COLOR_DEFAULT;

			// highlight the cursor pos
			if (buf_row == cur_row && c == cur_col)
				color = VGA_COLOR(15, 0); // black text on white white stuff(bg)
			vi_set_cell(screen_row, c, buf[buf_row][c], color);
		}

		// cursor at the end of line
		if (buf_row == cur_row && cur_col == len) {
			vi_set_cell(screen_row, cur_col < VI_TEXT_COLS ? cur_col : VI_TEXT_COLS - 1, ' ', VGA_COLOR(15, 0));
		}
	}

	// status bar (row 22)
	vi_clear_row_range(22, 0, VI_TEXT_COLS, VGA_COLOR(7,0));

	if (mode == MODE_COMMAND) {
		// show command being typed
		char cmdline[82];
		cmdline[0] = ':';
		memcpy(cmdline + 1, cmd_buf, (size_t)cmd_len);
		cmdline[cmd_len + 1] = '\0';
		vi_write_str(22, 0, cmdline, VGA_COLOR(7, 0));
	} else if (mode == MODE_SEARCH) {
		char cmdline[82];
		cmdline[0] = '/';
		memcpy(cmdline + 1, cmd_buf, (size_t)cmd_len);
		cmdline[cmd_len + 1] = '\0';
		vi_write_str(22, 0, cmdline, VGA_COLOR(7, 0));
	} else if (status_msg[0]) {
		// show status message
		vi_write_str(22, 0, status_msg, VGA_COLOR(7, 0));
	} else {
		// normal status: filename + dirty + mode + position
		char left[48], right[24];
		int lpos = 0;

		// filename
		for (int i = 0; filename[i] && lpos < 40; i++) {
			left[lpos++] = filename[i];
		}
		if (dirty) {
			left[lpos++] = ' ';
			left[lpos++] = '[';
			left[lpos++] = '+';
			left[lpos++] = ']';
		}
		left[lpos] = '\0';

		// mode indicator
		const char* mode_str = "";
		if (mode == MODE_INSERT) mode_str = " -- INSERT -- ";
		vi_write_str(22, 0, left, VGA_COLOR(7, 0));
		vi_write_str(22, lpos, mode_str, VGA_COLOR(7, 0));

		// line:col on the right like vim
		char lnum[8], cnum[8];
		itoa(cur_row + 1, lnum, 10);
		itoa(cur_col + 1, cnum, 10);
		int rlen = 0;
		char tmp[8];
		for (int i=0; lnum[i]; i++) {
			right[rlen++] = lnum[i];
		}
		right[rlen++] = ':';
		for (int i=0; cnum[i]; i++) {
			right[rlen++] = cnum[i];
		}
		right[rlen] = '\0';
		// also show total lines
		right[rlen++]=' '; right[rlen++]='/'; right[rlen++]=' ';
		itoa(num_lines, tmp, 10);
		for (int i=0; tmp[i]; i++) right[rlen++] = tmp[i];
		right[rlen] = '\0';

		vi_write_str(22, VI_TEXT_COLS - rlen - 1, right, VGA_COLOR(7, 0));
	}
}

// clear status message after rendering them
static void vi_clear_status(void) {
	status_msg[0] = '\0';
}

// file i/o
static void vi_load_file(void) {
	// init buffer to one empty line
	memset(buf, 0, sizeof(buf));
	memset(line_len, 0, sizeof(line_len));
	num_lines = 1;
	buf[0][0] = '\0';
	line_len[0] = 0;

	vfs_file_t file;
	if (vfs_open(filename, &file) < 0) {
		// newfile
		memcpy(status_msg, "New file.", sizeof("New file."));
		return;
	}

	static uint8_t raw[VI_MAX_LINES * VI_MAX_LINE_LEN];
	int total = vfs_read(&file, raw, sizeof(raw) - 1);
	vfs_close(&file);

	if (total <= 0)
		return;
	raw[total] = '\0';

	// split into lines
	num_lines = 0;
	int col = 0;
	for (int i = 0; i <= total && num_lines < VI_MAX_LINES; i++) {
		char c = (char)raw[i];
		if (c == '\n' || c == '\0') {
			buf[num_lines][col] = '\0';
			line_len[num_lines] = col;
			num_lines++;
			col = 0;
			if (c == '\0') break;
		} else if (c == '\r') {
			continue;
		} else {
			if (col < VI_MAX_LINE_LEN - 1) {
				buf[num_lines][col++] = c;
			}
		}
	}
	if (num_lines == 0) {
		num_lines = 1;
	}
	char msg[32];
	memcpy(msg, "Loaded ", 7);
	char lbuf[8]; itoa(num_lines, lbuf, 10);
	int pos = 7;
	for (int i=0;lbuf[i];i++) msg[pos++]=lbuf[i];
	memcpy(msg+pos, " lines.", 7); msg[pos+7]='\0';
	memcpy(status_msg, msg, sizeof(msg));
}

static int vi_save_file(void) {
	// build raw content: join linues with \n
	static uint8_t raw[VI_MAX_LINES * VI_MAX_LINE_LEN];
	int pos = 0;
	for (int r = 0; r < num_lines; r ++) {
		for (int c= 0; c < line_len[r] && pos < (int)sizeof(raw) - 1; c++)
			raw[pos++] = (uint8_t)buf[r][c];
		if (pos < (int)sizeof(raw)-1) raw[pos++] = '\n';
	}

	// delete + recreated(overwrite)
	vfs_remove(filename);
	vfs_create(filename);

	vfs_file_t file;
	if (vfs_open(filename, &file) < 0) {
		memcpy(status_msg, "ERROR: cannot write file!", 26);
		return -1;
	}
	vfs_write(&file, raw, (uint32_t)pos);
	vfs_close(&file);
	dirty = 0;

	char msg[48];
	int mp = 0;
	memcpy(msg, "Written: ", 9); mp=9;
	char pbuf[8]; itoa(pos, pbuf, 10);
	for (int i = 0; pbuf[i]; i++) msg[mp++] = pbuf[i];
	memcpy(msg+mp, " bytes.", 7); msg[mp+7] = '\0';
	memcpy(status_msg, msg, mp+8);
	return 0;
}

// input handlers- one per mode
static int vi_runnint; // 0 to exit

// normal mode
static void handle_normal(char c) {
	vi_clear_status();
	switch (c) {
		// coconut oil
		case 'h': cur_col--; break;
		case 'l': cur_col++; break;
		case 'k': cur_row--; break;
		case 'j': cur_row++; break;

		case '0': cur_col = 0; break;
		case '$': cur_col = line_len[cur_row] > 0 ? line_len[cur_row] - 1: 0; break;
		case 'w': {
				  // move to start of next word
				  int r = cur_row, c2 = cur_col;
				  while (r < num_lines) {
					  while (c2 < line_len[r] && buf[r][c2] != ' ') c2++;
					  while (c2 < line_len[r] && buf[r][c2] == ' ') c2++;
					  if (c2 < line_len[r]) {
						  cur_row=r; cur_col=c2; goto done_w;
					  }
					  r++; c2=0;
				  }
				  done_w: break;
			  }
		case 'b': {
				  // move to start of the previous word
				  int r = cur_row, c2 = cur_col - 1;
				  if (c2 < 0 && r > 0) { r--; c2 = line_len[r] - 1; }
				  while (r >= 0) {
					  while (c2 > 0 && buf[r][c2] == ' ') c2--;
					  while (c2 > 0 && buf[r][c2-1] != ' ') c2--;
					  if (c2 >= 0) { cur_row = r; cur_col=c2; goto done_b; }
					  r--; if(r>=0) c2=line_len[r]-1;
				  }
				  done_b: break;
			  }
		case 'g': {
				  // gg = go to firt line
				  // peek next key like peek
				  char c2 = keyboard_getchar();
				  if (c2 == 'g') { cur_row = 0; cur_col = 0; }
			  }
			  break;
		case 'G': 
			  cur_row = num_lines - 1;
			  cur_col = 0;
			  break;

		// mode changer
		case 'i': 
			  mode = MODE_INSERT;
			  memcpy(status_msg, "-- INSERT --", 13);
			  return;
		case 'a':
			  if (line_len[cur_row] > 0) cur_col++;
			  mode = MODE_INSERT;
			  memcpy(status_msg, "-- INSERT --", 13);
			  return;
		case 'A':
			  cur_col = line_len[cur_row];
			  mode = MODE_INSERT;
			  memcpy(status_msg, "-- INSERT --", 13);
			  return;
		case 'o':
			  undo_snapshot();
			  buf_insert_line_after(cur_row);
			  mode = MODE_INSERT;
			  return;

		case 'O': 
			  undo_snapshot();
			  buf_insert_line_before(cur_row);
			  mode = MODE_INSERT;
			  return;

		// del stuff
		case 'x':
			  undo_snapshot();
			  buf_delete_char(cur_row, cur_col);
			  break;
		case 'd': {
				  //peeky pookey
				  char c2 = keyboard_getchar();
				  if (c2 == 'd') {
					  undo_snapshot();
					  buf_delete_line(cur_row);
				  }
				  break;
			  }
		// undo
		case 'u':
			  undo_restore();
			  break;
		case '/':
			  mode = MODE_SEARCH;
			  cmd_len = 0;
			  cmd_buf[0] = '\0';
			  return;
		case 'n':
			  vi_search_next();
			  break;

		// command mode
		case ':':
			  mode = MODE_COMMAND;
			  cmd_len = 0;
			  cmd_buf[0] = '\0';
			  return;
		default: break;
	}

	buf_clamp_cursor();
	viewport_scroll_to_cursor();
}

// insert mode
static void handle_insert(char c) {
	if (c == 27) {
		// esc
		mode = MODE_NORMAL;
		if (cur_col > 0) cur_col--;
		buf_clamp_cursor();
		vi_clear_status();
		return;
	}

	if (c == '\n') {
		undo_snapshot();
		buf_split_line();
		viewport_scroll_to_cursor();
		return;
	}

	if (c == '\b') {
		if (cur_col > 0) {
			undo_snapshot();
			cur_col--;
			buf_delete_char(cur_row, cur_col);
		} else if (cur_row > 0) {
			// backspace at start of line, we will go back one row easy peasy
			undo_snapshot();
			int prev_len = line_len[cur_row - 1];
			buf_join_lines(cur_row - 1);
			cur_row--;
			cur_col = prev_len;
			viewport_scroll_to_cursor();
		}
		return;
	}

	if (c == 127) {
		// del
		if (cur_col < line_len[cur_row]) {
			undo_snapshot();
			buf_delete_char(cur_row, cur_col);
		}
		return;
	}
	if (c >= 0x20 && c <= 0x7E) {
		buf_insert_char(c);
		viewport_scroll_to_cursor();
	}
}

// CMD Mode
static int handle_command_execute(void) {
	// returns 1 if vi should quit
	if (strcmp(cmd_buf, "w") == 0) {
		vi_save_file();
		return 0;
	}
	if (strcmp(cmd_buf, "wq") == 0 || strcmp(cmd_buf, "x") == 0) {
		vi_save_file();
		return 1;
	}
	if (strcmp(cmd_buf, "q") == 0) {
		if (dirty) {
			memcpy(status_msg, "No write since last change (use :q! to override)", 49);
			return 0;
		}
		return 1;
	}
	if (strcmp(cmd_buf, "q!") == 0) {
		return 1;
	}

	// unknown commmand
	memcpy(status_msg, "Unknown command.", 17);
	return 0;
}

static void handle_command(char c) {
	if (c == 27) {
		mode = MODE_NORMAL;
		cmd_len = 0;
		vi_clear_status();
		return;
	}
	if (c == '\n') {
		int quit = handle_command_execute();
		mode = MODE_NORMAL;
		cmd_len = 0;
		if (quit) vi_runnint = 0;
		return;
	}
	if (c == '\b') {
		if (cmd_len > 0) { cmd_len--; cmd_buf[cmd_len] = '\0'; }
		else { mode = MODE_NORMAL; }
		return;
	}
	if (c >= 0x20 && c <= 0x7E && cmd_len < 78) {
		cmd_buf[cmd_len++] = c;
		cmd_buf[cmd_len] = '\0';
	}
}

//search mode
static void handle_search(char c) {
	if (c == 27) {
		mode = MODE_NORMAL;
		cmd_len = 0;
		vi_clear_status();
		return;
	}
	if (c == '\n') {
		memcpy(last_search, cmd_buf, (size_t)(cmd_len + 1));
		mode = MODE_NORMAL;
		cmd_len = 0;
		vi_search_next();
		return;
	}
	if (c == '\b') {
		if (cmd_len > 0) { cmd_len--; cmd_buf[cmd_len] = '\0'; }
		return;
	}
	if (c >= 0x20 && c <= 0x7E && cmd_len < 78) {
		cmd_buf[cmd_len++] = c;
		cmd_buf[cmd_len] = '\0';
	}
}

// entry point
int vi_open(const char* fname) {
	// init state
	memset(buf, 0, sizeof(buf));
	memset(line_len, 0, sizeof(line_len));
	strncpy(filename, fname, 63);
	filename[63] = '\0';
	cur_row = 0;
	cur_col = 0;
	view_top = 0;
	mode = MODE_NORMAL;
	dirty = 0;
	cmd_len = 0;
	undo_valid = 0;
	vi_runnint = 1;
	status_msg[0] =  '\0';
	last_search[0] = '\0';

	// load
	vi_load_file();

	// clear text area
	for (int r = 0; r < VI_TEXT_ROWS; r++) {
		vi_clear_row_range(r, 0, VI_TEXT_COLS, COLOR_DEFAULT);
	}

	// main loop
	while (vi_runnint) {
		vi_render();

		char c = keyboard_getchar();

		switch (mode) {
			case MODE_NORMAL: handle_normal(c); break;
			case MODE_INSERT: handle_insert(c); break;
			case MODE_COMMAND: handle_command(c); break;
			case MODE_SEARCH: handle_search(c); break;
		}
	}

	vga_clear();
	return 0;
}
