// circular scroll buffer implementation
#include "scrollbuf.h"
#include "statusbar.h"
#include "../core/vga.h"
#include "../lib/string.h"
#include <sys/types.h>

// STORAGE
typedef struct {
	char ch;
	uint8_t color;
} sb_cell_t;

static sb_cell_t lines[SCROLLBUF_LINES][SCROLLBUF_COLS];
static int line_fill[SCROLLBUF_LINES]; // how many cells used in each line

static int write_line = 0; // 0 based index into lines[]
static int write_col = 0;

static int total_lines = 1;

// viewport: 0 = live, N = N lines scrolled back
static int viewport_offset = 0;

// direct vga pointer, bypasses vga_putchar to avoid recursion
static unsigned short* const sb_vga = (unsigned short*)VGA_ADDRESS;

// INTERNAL HELPERS

static inline void sb_set_cell(int row, int col, char c, uint8_t color) {
	sb_vga[row * SCROLLBUF_COLS + col] = (unsigned short)((color << 8) | (unsigned char)c);
}

static void sb_clear_screen_row(int row) {
	for (int c = 0; c < SCROLLBUF_COLS; c++)
		sb_set_cell(row, c, ' ', COLOR_DEFAULT);
}

static int oldest_line_idx(void) {
	if (total_lines <= SCROLLBUF_LINES) return 0;
	return (write_line + 1) % SCROLLBUF_LINES;
}

static int get_line_idx(int logical) {
	int stored = total_lines < SCROLLBUF_LINES ? total_lines : SCROLLBUF_LINES;
	(void)stored;
	int oldest = oldest_line_idx();
	return (oldest + logical) % SCROLLBUF_LINES;
}

static int stored_line_count(void) {
	return total_lines < SCROLLBUF_LINES ? total_lines : SCROLLBUF_LINES;
}

// scroll indicatoor
static void sb_update_indicator(void) {
	if (viewport_offset > 0) {
		char msg[40];
		int pos = 0;
		const char* prefix = "SCROLL ^";
		for (int i = 0; prefix[i]; i++) msg[pos++] = prefix[i];
		char num[8]; itoa(viewport_offset, num, 10);
		for (int i = 0; num[i]; i++) msg[pos++] = num[i];
		const char* suffix = " lines back  (^D snap)";
		for (int i = 0; suffix[i]; i++) msg[pos++] = suffix[i];
		msg[pos] = '\0';
		statusbar_set_msg(msg, COLOR_YELLOW);
	} else {
		statusbar_clear_msg();
	}
}

// public api
void scrollbuf_init(void) {
	for (int r = 0; r < SCROLLBUF_LINES; r++) {
		line_fill[r] = 0;
		for (int c = 0; c < SCROLLBUF_COLS; c++) {
			lines[r][c].ch = ' ';
			lines[r][c].color = COLOR_DEFAULT;
		}
	}
	write_line = 0;
	write_col = 0;
	total_lines = 1;
	viewport_offset = 0;
}

void scrollbuf_putchar(char c, uint8_t color) {
	if (c == '\n' || c == '\r') {
		if (c == '\n') {
			// finalize current line and move to the next one
			line_fill[write_line] = write_col;
			write_line = (write_line + 1) % SCROLLBUF_LINES;
			write_col = 0;
			total_lines++;
			// clear new line
			for (int i = 0; i < SCROLLBUF_COLS; i++) {
				lines[write_line][i].ch = ' ';
				lines[write_line][i].color =  COLOR_DEFAULT;
			}
			line_fill[write_line] = 0;
		}
		return;
	}

	if (c == '\b') {
		if (write_col > 0) write_col--;
		return;
	}

	if (c == '\t') {
		write_col = (write_col + 4) & ~3;
		if (write_col >= SCROLLBUF_COLS) write_col = SCROLLBUF_COLS - 1;
		return;
	}

	if (c < 0x20) return; // ignore contrrol chars
	
	if (write_col < SCROLLBUF_COLS) {
		lines[write_line][write_col].ch = c;
		lines[write_line][write_col].color = color;
		write_col++;
		line_fill[write_line] = write_col;
	}
	// line overflow, silenty discard . this is what good terminals do. we are good guy
}

void scrollbuf_render(void) {
	int count = stored_line_count();
	int visible = SCROLLBUF_VISIBLE;

	// clamping
	int lines_to_show = count < visible ? count : visible;

	//start rendering from the bottom of the visible area
	int start_scree_row = visible - lines_to_show;
	int start_logical = count - lines_to_show - viewport_offset;
	if (start_logical < 0) start_logical = 0;

	// clear only the rows we'll use for content
	for (int screen_row = start_scree_row; screen_row < visible; screen_row++) {
		int logical = start_logical + (screen_row - start_scree_row);
		int idx = get_line_idx(logical);

		for (int col = 0; col < SCROLLBUF_COLS; col++) {
			sb_set_cell(screen_row, col, lines[idx][col].ch, lines[idx][col].color);
		}
	}

	if (start_scree_row > 0 && viewport_offset == 0) {
		return;
	}
}

void scrollbuf_scroll_up(int n) {
	int count = stored_line_count();

	// maximum we can scroll back
	int max_offset = count - SCROLLBUF_VISIBLE;
	if (max_offset < 0) return;

	viewport_offset += n;
	if (viewport_offset > max_offset) viewport_offset = max_offset;

	scrollbuf_render();
	sb_update_indicator();
}

void scrollbuf_scroll_down(int n) {
	viewport_offset -= n;
	if (viewport_offset < 0)
		viewport_offset = 0;

	scrollbuf_render();
	sb_update_indicator();
}

void scrollbuf_snap(void) {
	if (viewport_offset == 0) return; // already live
	viewport_offset = 0;
	scrollbuf_render();
	sb_update_indicator();
}

int scrollbuf_is_scrolled(void) {
	return viewport_offset > 0;
}
