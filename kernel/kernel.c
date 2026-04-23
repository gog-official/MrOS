// kernel/kernel.c

#define VGA_ADDRESS 0xB8000
#define VGA_COLS 80
#define VGA_ROWS 25
#define VGA_COLOR(bg, fg) ((bg << 4) | fg)
#define COLOR_DEFAULT	VGA_COLOR(0, 15)
#define COLOR_GREEN	VGA_COLOR(0, 10)
#define COLOR_RED	VGA_COLOR(0, 12)
#define COLOR_CYAN	VGA_COLOR(0, 11)
#define COLOR_YEELLOW	VGA_COLOR(0, 14)

static volatile unsigned short* const vga = (unsigned short*)VGA_ADDRESS;

static int cursor_row;
static int cursor_col;

// write char colours to screen position

static void vga_set_cell(int row, int col, char c, unsigned char color) {
    int index = row * VGA_COLS + col;
    vga[index] = (unsigned short)((color << 8) | (unsigned char)c);
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
			vga[row * VGA_COLS + col] = vga[(row + 1) * VGA_COLS + col];
		}
	}

	// blank the last row
	for (int col = 0; col < VGA_COLS; col++) {
		vga_set_cell(VGA_ROWS-1, col, ' ', COLOR_DEFAULT);
	}

	// kep cursor at last
	cursor_row = VGA_ROWS -1;
}

// print one char, \n and scrolling too
void vga_putchar(char c, unsigned char color) {
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

void vga_print(const char* str, unsigned char color) {
	for (int i = 0; str[i] != 0; i++) {
		vga_putchar(str[i], color);
	}
}

// print string then newline
void vga_println(const char* str, unsigned char color) {
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
void vga_print_hex(unsigned int value, unsigned char color) {
	char buf[16];
	vga_print("0x", color);
	itoa((int)value, buf, 16);
	vga_print(buf, color);
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
	
	// Banner for OS
	vga_println("==============================================", COLOR_CYAN);
	vga_println("             MrOS - Keeps you fit             ", COLOR_YEELLOW);
	vga_println("    Now with water timer, workout and infos   ", COLOR_YEELLOW);
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

	// showing how great my itoa is
	vga_print("Kernel loaded at address: ", COLOR_CYAN);
	vga_print_hex(0x10000, COLOR_YEELLOW);
	vga_putchar('\n', COLOR_DEFAULT);

	vga_print("VGA buffer address: ", COLOR_CYAN);
	vga_print_hex(0xB8000, COLOR_YEELLOW);
	vga_putchar('\n', COLOR_DEFAULT);

	vga_print("Test integer (1337): ", COLOR_CYAN);
	vga_print_int(1337, COLOR_YEELLOW);
	vga_putchar('\n', COLOR_DEFAULT);

	vga_putchar('\n', COLOR_DEFAULT);
	vga_println("kernel is alive. Halting GPU.", COLOR_GREEN);

	// halting
	for (;;) {
		__asm__ volatile ("hlt");
	}
}
