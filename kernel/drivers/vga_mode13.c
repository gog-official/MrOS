// so essentially we program VGA hardware registers to enter/exit mode 13h
// register tables taken from FreeVGA docs and verified against the BIOS mode 13h initialisation seq
#include "vga_mode13.h"
#include "../interrupts/pic.h" // out/in b's
#include "../lib/string.h"

// VGA register tables for mode 13h
// each entry is like {indesx, val}

// single write to 0x3C2
#define MODE13_MISC 0x63

// index to 0x3C4, data to 0x3C5
static const uint8_t mode13_seq[][2] = {
	{ 0x00, 0x03 }, // reset
	{ 0x01, 0x01 },	// clocking mode: 8-dot char
	{ 0x02, 0x0F }, // map mask: all planes enabled
	{ 0x03, 0x00 }, // char map select
	{ 0x04, 0x0E }, // memory mode: chain4, extended
};

// CRTC registers (index to 0x3D4, data to 0x3D5)
static const uint8_t mode13_crtc[][2] = {
	{0x00, 0x5F}, {0x01, 0x4F}, {0x02, 0x50}, {0x03, 0x82},
	{0x04, 0x54}, {0x05, 0x80}, {0x06, 0xBF}, {0x07, 0x1F},
	{0x08, 0x00}, {0x09, 0x41}, {0x0A, 0x00}, {0x0B, 0x00},
	{0x0C, 0x00}, {0x0D, 0x00}, {0x0E, 0x00}, {0x0F, 0x00},
	{0x10, 0x9C}, {0x11, 0x8E}, {0x12, 0x8F}, {0x13, 0x28},
	{0x14, 0x40}, {0x15, 0x96}, {0x16, 0xB9}, {0x17, 0xA3},
	{0x18, 0xFF},
};

// Graphics Controller register
static const uint8_t mode13_gc[][2] = {
	{0x00, 0x00}, {0x01, 0x00}, {0x02, 0x00}, {0x03, 0x00},
	{0x04, 0x00}, {0x05, 0x40}, {0x06, 0x05}, {0x07, 0x0F},
	{0x08, 0xFF},
};

// attribute controller registers
static const uint8_t mode13_ac[] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x41,
	0x00, 0x0F, 0x00, 0x00
};

// text mode restoration tables (mode 3 - 80x25 text)
#define MODE3_MISC 0x67

static const uint8_t mode3_seq[][2] = {
	{0x00, 0x03}, {0x01, 0x00}, {0x02, 0x03},
	{0x03, 0x00}, {0x04, 0x02},
};

static const uint8_t mode3_crtc[][2] = {
	{0x00, 0x5F}, {0x01, 0x4F}, {0x02, 0x50}, {0x03, 0x82},
	{0x04, 0x55}, {0x05, 0x81}, {0x06, 0xBF}, {0x07, 0x1F},
	{0x08, 0x00}, {0x09, 0x4F}, {0x0A, 0x0D}, {0x0B, 0x0E},
	{0x0C, 0x00}, {0x0D, 0x00}, {0x0E, 0x00}, {0x0F, 0x00},
	{0x10, 0x9C}, {0x11, 0x8E}, {0x12, 0x8F}, {0x13, 0x28},
	{0x14, 0x1F}, {0x15, 0x96}, {0x16, 0xB9}, {0x17, 0x83},
	{0x18, 0xFF},
};

static const uint8_t mode3_gc[][2] = {
	{0x00, 0x00}, {0x01, 0x00}, {0x02, 0x00}, {0x03, 0x00},
	{0x04, 0x00}, {0x05, 0x10}, {0x06, 0x0E}, {0x07, 0x00},
	{0x08, 0xFF},
};

static const uint8_t mode3_ac[] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x14, 0x07,
	0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
	0x0C, 0x00, 0x0F, 0x08, 0x00
};

// Register write helpers
static void write_regs_seq(const uint8_t regs[][2], int n) {
	for (int i = 0; i < n; i++) {
		outb(VGA_SEQ_INDEX, regs[i][0]);
		outb(VGA_SEQ_DATA, regs[i][1]);
	}
}

static void write_regs_crtc(const uint8_t regs[][2], int n) {
	for (int i = 0; i < n; i++) {
		outb(VGA_CRTC_INDEX, regs[i][0]);
		outb(VGA_CRTC_DATA, regs[i][1]);
	}
}

static void write_regs_gc(const uint8_t regs[][2], int n) {
	for (int i = 0; i < n; i++) {
		outb(VGA_GC_INDEX, regs[i][0]);
		outb(VGA_GC_DATA, regs[i][1]);
	}
}

static void write_regs_ac(const uint8_t* regs, int n) {
	// must read INSTANT_READ first to resent the flippy-floppy to index mode
	inb(VGA_INSTAT_READ);
	for (int i = 0; i < n; i++) {
		outb(VGA_AC_INDEX, (uint8_t)i);
		outb(VGA_AC_INDEX, regs[i]);
	}
	// lock  palette
	outb(VGA_AC_INDEX, 0x20);
}

#define ARRAY_LEN(a) (int)(sizeof(a)/sizeof((a)[0]))

// public api
void  mode13_enter(void) {
	// unlock the CRTC reggies 0-7
	outb(VGA_CRTC_INDEX, 0x11);
	outb(VGA_CRTC_DATA, inb(VGA_CRTC_DATA)& ~ 0x80);

	outb(VGA_MISC_WRITE, MODE13_MISC);
	write_regs_seq (mode13_seq, ARRAY_LEN(mode13_seq));
	write_regs_crtc(mode13_crtc, ARRAY_LEN(mode13_crtc));
	write_regs_gc (mode13_gc, ARRAY_LEN(mode13_gc));
	write_regs_ac (mode13_ac, ARRAY_LEN(mode13_ac));
}

void mode13_exit(void) {
	// unlock the CRTC reggies 0-7
	outb(VGA_CRTC_INDEX, 0x11);
	outb(VGA_CRTC_DATA, inb(VGA_CRTC_DATA)& ~ 0x80);

	outb(VGA_MISC_WRITE, MODE3_MISC);
	write_regs_seq (mode3_seq, ARRAY_LEN(mode3_seq));
	write_regs_crtc(mode3_crtc, ARRAY_LEN(mode3_crtc));
	write_regs_gc (mode3_gc, ARRAY_LEN(mode3_gc));
	write_regs_ac (mode3_ac, ARRAY_LEN(mode3_ac));
}

void mode13_blit(const uint8_t* src) {
	memcpy(MODE13_BUFFER, src, MODE13_PIXELS);
}

void mode13_clear(uint8_t color) {
	memset(MODE13_BUFFER, color, MODE13_PIXELS);
}

void mode13_set_palette(const uint8_t* palette) {
	outb(VGA_PELADDR_WRITE, 0); // start at index 0
	for (int i = 0; i < 256 * 3; i++) {
		outb(VGA_PELDATA, palette[i]);
	}
}
