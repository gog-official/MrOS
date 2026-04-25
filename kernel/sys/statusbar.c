// owns row 23(divider) and 24(actual bar)
// I didn't put much time in its design as this os is not meant for complexity
// I never use vga_putchar and vga_println(as they move the cursor), thats why I used vga_print_at and vga_clear

#include "statusbar.h"
#include "../core/vga.h"
#include "../drivers/timer.h"

// internal state

//current message text and its color
static char msg_buf[64];
static unsigned char msg_color = COLOR_CYAN;
//msg sending tick
static unsigned int msg_set_tick = 0;

// 1 for msg active, 0 for inactive
static int msg_active = 0;

// uptime redraw tick, once/sec
static unsigned int last_draw_tick = 0;

// helpers

// itoa for statusbar
static void sb_itoa(unsigned int v, char* buf) {
	if (v == 0) { buf[0]='0'; buf[1]='\0'; return; }
	char tmp[12];
	int i = 0, j = 0;
	while (v > 0) { tmp[i++] = '0' + (v%10); v/=10; }
	while (i > 0) buf[j++] = tmp[--i];
	buf[j] = '\0';
}

// also strlen
static int sb_strlen(const char* s) {
	int n = 0; while (s[n]) n++; return n;
}

// copy src into dst, return chars written
static int sb_strcpy(char* dst, const char* src) {
	int i = 0;
	while (src[i]) { dst[i] = src[i]; i++; }
	dst[i] = '\0';
	return i;
}

// draw_uptime
// writes the uptime portion of the statusbar starting at col 14
// pads with spaces after the nummer so oldy goldy long strings are eraased
//
// format : h m s

static void draw_uptime(void) {
	unsigned int ticks = timer_get_ticks();
	unsigned int seconds = ticks / 100; // basic frmla
	unsigned int minutes = seconds / 60;
	unsigned int hours = minutes / 60;

	seconds %= 60;
	minutes %= 60;

	char buf[32];
	int pos = 0;
	char num[12];

	if (hours > 0) {
		sb_itoa(hours, num);
		int l = sb_strlen(num); for (int i=0;i<l;i++) buf[pos++]=num[i];
		buf[pos++] = 'h'; buf[pos++] = ' ';
	}
	if (minutes > 0 || hours > 0) {
		sb_itoa(minutes, num);
		int l = sb_strlen(num); for (int i=0; i<l; i++) buf[pos++]=num[i];
		buf[pos++]='m'; buf[pos++]=' ';
	}
	sb_itoa(seconds, num);
	int l = sb_strlen(num); for (int i=0; i<l; i++) buf[pos++]=num[i];
	buf[pos++]='s';

	// pad to 14 chars to wipe previous longer stringy
	while (pos < 14) buf[pos++] = ' ';
	buf[pos] = '\0';

	//uptime stars at col 14 btw, after "[ MrOS ]  up: "
	vga_print_at(STATUSBAR_ROW, 14, buf, COLOR_DEFAULT);
	vga_print_at(STATUSBAR_ROW, 0, "[ MrOS ]  up: ", COLOR_GREEN);
}

// statusbar_init
// draw the divider line and empty status bar once at boot.
// called from kmain before sti

void statusbar_init(void) {
	// draw divider: row 23 filled with '-' (ASCII 196 in CP437,
	// but plain '-' works equally as fine in QEMU's default font)
	for (int c = 0; c < VGA_COLS; c++) {
		// we can just directly write: '-' in grey
		unsigned short* vga = (unsigned short*)VGA_ADDRESS;
		vga[STATUSBAR_DIVIDER_ROW * VGA_COLS + c] = (unsigned short)((COLOR_GREY << 8) | '-');
	}

	//blank status bar roo
	vga_clear_row(STATUSBAR_ROW);

	// left prefix
	vga_print_at(STATUSBAR_ROW, 0, "[ MrOS ]  up: ", COLOR_GREEN);

	draw_uptime();
}

// draw msg
// writes the reminder msg portion starting at colu 30
// a separator ' | ' preceds the msg
static void draw_msg(void) {
	if (!msg_active) {
		// erase msg area (30 chars from col 30)
		char blank[32];
		for (int i = 0; i <  31; i++) blank[i] = ' ';
		blank[31] = '\0';
		vga_print_at(STATUSBAR_ROW, 30, blank, COLOR_DEFAULT);
		return;
	}

	// build " | <msg>"
	char display[64];
	int pos = 0;
	display[pos++] = ' ';
	display[pos++] = '|';
	display[pos++] = ' ';
	for (int i = 0; msg_buf[i] && pos < 62; i++)
		display[pos++] = msg_buf[i];

	// pad rest to clear oldy long msg
	while (pos < 63) display[pos++] = ' ';
	display[pos] = '\0';

	vga_print_at(STATUSBAR_ROW, 30, display, msg_color);
}

// status_update
// called from shell_run() main loop - roughly every interation.
// only redraws once per second to avoid flickering.
// also handles msg timeout tbh
void statusbar_update(void) {
	unsigned int now = timer_get_ticks();

	// redraw uptime once per second (every 100 ticks)
	if (now - last_draw_tick >= 100) {
		last_draw_tick = now;
		draw_uptime();
	}

	// check message timeout
	if (msg_active) {
		unsigned int elapsed_sec = (now - msg_set_tick) / 100;
		if (elapsed_sec >= STATUSBAR_MSG_TIMEOUT_SEC) {
			statusbar_clear_msg();
		}
	}
}

// statusbar_set_msg
// called by reminder.c when it's time to show a msg.
// its kinda safe to call from IRQ context - only writes to our private buffers,
// then calls vga_print_at which is a direct vga write (not a syscall)

void statusbar_set_msg(const char* msg, unsigned char color) {
	sb_strcpy(msg_buf, msg);
	msg_color = color;
	msg_set_tick = timer_get_ticks();
	msg_active = 1;
	draw_msg();
}

// statusbar_clear_msg ->
// erase the msg portion of the bar
void statusbar_clear_msg(void) {
	msg_active = 0;
	msg_buf[0] = '\0';
	draw_msg();
}
