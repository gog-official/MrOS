// kernel/kernel.c

#include "vga.h"
#include "../interrupts/idt.h"
#include "../interrupts/pic.h"
#include "../drivers/timer.h"
#include "../fitness/fitness.h"
#include "../sys/statusbar.h"
#include "../sys/reminder.h"
#include "../drivers/keyboard.h"
#include "../shell/shell.h"

static volatile unsigned short* const vga_buf = (unsigned short*)VGA_ADDRESS;

int cursor_col = 0;
int cursor_row = 0;

// write char colours to screen position

void vga_set_cell(int row, int col, char c, uint8_t color) {
	// Bounds check to prevent crashes
	if (row < 0 || row >= VGA_ROWS || col < 0 || col >= VGA_COLS) {
		return;
	}
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

static void vga_scroll(void) {
	// scroll ceiling is statusbar_divider_row -1 = row 22
	// row 23-24 never touched ;) by the scroll path
	//always keep statusbar safe
	int scroll_limit = STATUSBAR_DIVIDER_ROW - 1;
	for (int row = 0; row < scroll_limit; row++) {
		for (int col = 0; col < VGA_COLS; col++) {
			vga_buf[row * VGA_COLS + col] = vga_buf[(row + 1) * VGA_COLS + col];
		}
	}

	for (int col = 0; col < VGA_COLS; col++) {
		vga_set_cell(scroll_limit, col, ' ', COLOR_DEFAULT);
	}


	cursor_row = scroll_limit -1;
	cursor_col = 0;
	vga_update_cursor(cursor_row, cursor_col);
}

void vga_set_cursor(int row, int col) {
	cursor_row = row;
	cursor_col = col;
	vga_update_cursor(row, col);
}

void vga_update_cursor(int row, int col) {
	uint16_t pos = row *VGA_COLS + col;

	outb(0x3D4, 0x0F);
	outb(0x3D5, (uint8_t)(pos & 0xFF));

	outb(0x3D4, 0x0E);
	outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
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

	if (cursor_col >= VGA_COLS) {
		cursor_col = 0;
		cursor_row ++;
	}
	
	if (cursor_row >= STATUSBAR_DIVIDER_ROW - 1) {
		vga_scroll();
	}
	vga_update_cursor(cursor_row, cursor_col);
}

void vga_print(const char* str, uint8_t color) {
	for (int i = 0; str[i] != 0; i++) {
		vga_putchar(str[i], color);
	}
}

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

	while (i > 0) {
		buf[j++] = tmp[--i];
	}
	buf[j] = '\0';
	return buf;
}

void vga_print_int(int value, unsigned char color) {
	char buf[32];
	itoa(value, buf, 10);
	vga_print(buf, color);
}

void vga_print_hex(uint32_t v, uint8_t color) {
    char buf[16]; vga_print("0x", color); itoa((int)v, buf, 16); vga_print(buf, color);
}

void dynamic_sleep(int interval) {
	for (int i = 0; i < interval; i ++) {
		statusbar_update();
		timer_sleep(1);
	}
}

int k_atoi(char *s) {
	int result = 0;
	int i = 0;

	while (s[i] >= '0' && s[i] <= '9') {
		result = result * 10 + (s[i] - '0');
		i++;
	}
	return result;
}

// ===============================================

// ===============================================

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
	keyboard_init();

	// draws rows 23-24 while interrupts are off. guarantees bar is visible before first irq fires
	statusbar_init();
	
	// Enable interrupts
	__asm__ volatile ("sti");
	
	// prompting user to enter their choice of time for water break interval
	char buf[16];
	vga_print("Enter water reminder interval(minutes) [default - 90]: ", COLOR_YELLOW);
	keyboard_readline(buf, sizeof(buf));
	int minutes = k_atoi(buf);
	if (minutes <= 0) minutes = 90;
	reminder_set_interval(minutes * 60 * 100);

	// IMPORTANT: reminder_init is set after sti:
	// 	records the current tick as the baseline for the 90min countdown.
	// 	Timer must be running (sti done) so timer_get_ticks() is meaningful.
	reminder_init();


	vga_println("==============================================", COLOR_CYAN);
	vga_println("             MrOS - Keeps you fit             ", COLOR_YELLOW);
	vga_println("   Now with water reminder, workout and infos ", COLOR_YELLOW);
	vga_println("==============================================", COLOR_CYAN);
	vga_putchar('\n', COLOR_DEFAULT);


	vga_print("[OK] ", COLOR_GREEN);
	vga_println("Bootloader complete, kernel loaded", COLOR_DEFAULT);

	vga_print("[OK] ", COLOR_GREEN);
	vga_println("Running in 32-bit protected mode", COLOR_DEFAULT);

	vga_print("[OK] ", COLOR_GREEN);
	vga_println("VGA text driver initialized (80x25)", COLOR_DEFAULT);
	vga_println("\nLOOK AT YOUR BODY, then think about cheating in next stage", COLOR_YELLOW);
	
	vga_putchar('\n', COLOR_DEFAULT);
	
	// Wait 5 seconds so user can read the boot messages
	dynamic_sleep(5);
	
	vga_clear();
	statusbar_init();
	

	dynamic_sleep(3);
	vga_clear();
	statusbar_init();

	shell_run();

	for (;;) {
		__asm__ volatile ("hlt");
	}
}
