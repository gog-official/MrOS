// kernel/kernel.c

#include "vga.h"
#include "idt.h"
#include "pic.h"
#include "timer.h"
#include "fitness.h"

static volatile unsigned short* const vga_buf = (unsigned short*)VGA_ADDRESS;

int cursor_col = 0;
int cursor_row = 0;

// write char colours to screen position

static void vga_set_cell(int row, int col, char c, uint8_t color) {
	vga_buf[row * VGA_COLS + col] = (unsigned short)((color << 8) | (unsigned char)c);
}

// fill entire screen with spaces
void vga_clear(void) {
	for (int row = 0; row < VGA_ROWS; row++) {
		for (int col = 0; col < VGA_COLS; col++) {
			vga_set_cell(row,col,' ',COLOR_DEFAULT);
		}
	}
	cursor_row = 0;
	cursor_col = 0;
}

// shift every row up by one, blank the last row
static void vga_scroll(void) {
	// move up
	for (int row = 0; row < VGA_ROWS -1; row++) {
		for (int col = 0; col < VGA_COLS; col++) {
			vga_buf[row * VGA_COLS + col] = vga_buf[(row + 1) * VGA_COLS + col];
		}
	}

	// blank the last row
	for (int col = 0; col < VGA_COLS; col++) {
		vga_set_cell(VGA_ROWS-1, col, ' ', COLOR_DEFAULT);
	}

	// kep cursor at last
	cursor_row = VGA_ROWS -1;
}

void vga_set_cursor(int row, int col) {
	cursor_row = row;
	cursor_col = col;
}

void vga_print_at(int row, int col, const char* str, uint8_t color) {
	int c = col;
	for (int i = 0; str[i] && c < VGA_COLS; i++, c++)
		vga_set_cell(row, c, str[i], color);
}

void vga_clear_row(int row) {
	for (int c = 0; c < VGA_COLS; c++)
		vga_set_cell(row, c, ' ', COLOR_DEFAULT);
}

// print one char, \n and scrolling too
void vga_putchar(char c, uint8_t color) {
	if (c == '\n') {
		cursor_col = 0;
		cursor_row++;
	} else if (c == '\r') {
		cursor_col = 0;
	} else if (c == '\t') {
		cursor_col = (cursor_col + 4) & ~3;
	} else {
		vga_set_cell(cursor_row, cursor_col, c, color);
		cursor_col++;
	}

	//wrap at end
	if (cursor_col >= VGA_COLS) {
		cursor_col = 0;
		cursor_row ++;
	}
	
	// scroll if last row exceeded
	if (cursor_row >= VGA_ROWS) {
		vga_scroll();
	}
}

// print nul terminated string

void vga_print(const char* str, uint8_t color) {
	for (int i = 0; str[i] != 0; i++) {
		vga_putchar(str[i], color);
	}
}

// print string then newline
void vga_println(const char* str, uint8_t color) {
	vga_print(str, color);
	vga_putchar('\n', color);
}

// utility, minimal no stdlib

static char* itoa(int value, char* buf, int base) {
	static char digits[] = "0123456789ABCDEF";
	char tmp[32];
	int i = 0;
	int negative = 0;

	if (value == 0) {
		buf[0] = '0';
		buf[1] = '\0';
		return buf;
	}

	if (value < 0 && base == 10) {
		negative = -1;
		value = -value;
	}

	while (value > 0) {
		tmp[i++] = digits[value % base];
		value /= base;
	}

	int j = 0;
	if (negative) buf[j++] = '-';

	//reverse into o buffer
	while (i > 0) {
		buf[j++] = tmp[--i];
	}
	buf[j] = '\0';
	return buf;
}

// print a int directly to screen
void vga_print_int(int value, unsigned char color) {
	char buf[32];
	itoa(value, buf, 10);
	vga_print(buf, color);
}

//print hex with 0x prefix
void vga_print_hex(uint32_t v, uint8_t color) {
    char buf[16]; vga_print("0x", color); itoa((int)v, buf, 16); vga_print(buf, color);
}

// after all screen setup, finally kmain
void kmain(void) {
	__asm__ volatile (
		"movw $0x3C2, %%dx\n"
		"movb $0x63, %%al\n"
	        "outb %%al, %%dx\n"
         	::: "eax", "edx"
    	);

    	vga_clear();
	idt_init();
	pic_remap();
	timer_init();
	
	// Enable interrupts
	__asm__ volatile ("sti");

	// Banner for OS
	vga_println("==============================================", COLOR_CYAN);
	vga_println("             MrOS - Keeps you fit             ", COLOR_YELLOW);
	vga_println("    Now with water timer, workout and infos   ", COLOR_YELLOW);
	vga_println("==============================================", COLOR_CYAN);
	vga_putchar('\n', COLOR_DEFAULT);

	// sysinfo
	vga_print("[OK] ", COLOR_GREEN);
	vga_println("Bootloader complete, kernel loaded", COLOR_DEFAULT);

	vga_print("[OK] ", COLOR_GREEN);
	vga_println("Running in 32-bit protected mode", COLOR_DEFAULT);

	vga_print("[OK] ", COLOR_GREEN);
	vga_println("VGA text driver initialized (80x25)", COLOR_DEFAULT);
	
	vga_putchar('\n', COLOR_DEFAULT);
	
	// Wait 5 seconds so user can read the boot messages
	timer_sleep(5);
	
	// Clear screen before workout
	vga_clear();
	
	// fitness
	run_fitness_sequence();

	vga_println("\nLOOK AT YOUR BODY, then think about cheating in next stage", COLOR_YELLOW);

	// halting
	for (;;) {
		__asm__ volatile ("hlt");
	}
}
