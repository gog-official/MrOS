#include "speaker.h"
#include "timer.h"
#include "../interrupts/pic.h" // for outb/inb

// low level hardware

// here's a revision for you
// we will program PIT channel 2 to generate a square wave at freq_hz.
//
// PIT command byte for channel 2:
// 	bits 7-6 = 10  - channel 2
// 	bits 5-4 = 11  - access mode lobyte/hibyte
// 	bits 3-1 = 011 - mode 3 (square wave)
// 	bit 0 = 0 - binary not BCD
// 	= 1011 0110 = 0xB6, so yeah we'll use it

static void pit_set_channel2(uint32_t freq_hz) {
	if (freq_hz == 0) return;

	uint32_t divisor = 1193180 / freq_hz;

	// clamp: divisor must tightly fit in 16 bits(max ~18hz), min 1
	if (divisor > 0xFFFF) divisor = 0xFFFF;
	if (divisor < 1) divisor = 1;

	// send command then divisor lo/hi
	outb(PIT_CMD_PORT, 0xB6);
	outb(PIT_CHANNEL2_PORT, (uint8_t)(divisor & 0xFF));
	outb(PIT_CHANNEL2_PORT, (uint8_t)((divisor >> 8) & 0xFF));

}
