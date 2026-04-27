#include "../core/vga.h"

void k_assert_fail(const char* expr, const char* file, int line) {
	vga_println("ASSERTION FAILED BOOOO:", COLOR_RED);
	vga_println(expr, COLOR_RED);
	vga_println(file, COLOR_GREY);
	vga_print_int(line, COLOR_GREY);
	vga_putchar('\n', COLOR_DEFAULT);
	__asm__ volatile ("cli: hlt");
	for(;;);
}
