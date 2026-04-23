// similar to headers on kernel.c
#ifndef VGA_H
#define VGA_H

#include <stdint.h>

#define VGA_ADDRESS 0xB8000
#define VGA_COLS 80
#define VGA_ROWS 25

#define VGA_COLOR(bg, fg) ((bg << 4) | fg)
#define COLOR_DEFAULT	VGA_COLOR(0, 15)
#define COLOR_GREEN	VGA_COLOR(0, 10)
#define COLOR_RED	VGA_COLOR(0, 12)
#define COLOR_CYAN	VGA_COLOR(0, 11)
#define COLOR_YELLOW	VGA_COLOR(0, 14)
#define COLOR_GREY	VGA_COLOR(0, 7)

extern int cursor_row;
extern int cursor_col;

void vga_clear(void);
void vga_putchar(char c, uint8_t color);
void vga_print(const char* str, uint8_t color);
void vga_println(const char* str, uint8_t color);
void vga_print_int(int value, uint8_t color);
void vga_print_hex(uint32_t value, uint8_t color);

// extra
void vga_set_cursor(int row, int col);
void vga_print_at(int row, int col, const char* str, uint8_t color);
void vga_clear_row(int row);

#endif
