// terminal scroll buffer for mr0s shell.
//
// Design:
// 	A circular buffer of SCROLLBUF_LINES lines.
// 	each line stroes up to 80 chars + a color attribbute per char
// 	vga_putchar() feeds every chararcter here as well as to the screen
//
// keys:
// 	ctrl + up -> scroll up one line
// 	ctrl + down -> scroll down one line
// 	page up -> scroll up half a screen
// 	page down -> scroll down half a screen
#ifndef SCROLLBUF_H
#define SCROLLBUF_H

#include <stdint.h>
#include <stdlib.h>

#define SCROLLBUF_LINES 500
#define SCROLLBUF_COLS 80
#define SCROLLBUF_VISIBLE 22
#define SCROLLBUF_HALF (SCROLLBUF_VISIBLE / 2)

void scrollbuf_init(void);

void scrollbuf_putchar(char c, uint8_t color);

void scrollbuf_scroll_up(int lines);

void scrollbuf_scroll_down(int lines);

void scrollbuf_snap(void);

int scrollbuf_is_scrolled(void);

void scrollbuf_render(void);

#endif // !SCROLLBUF_H
