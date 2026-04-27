#include "../core/vga.h"
#include "../drivers/timer.h"

// stub for when doom is not included
void doom_launch(void) {
    vga_println("DOOM support not compiled in.", COLOR_RED);
}

void doom_quit(void) {}

void doom_key_event(unsigned char ascii, int pressed) {
    (void)ascii; (void)pressed;
}