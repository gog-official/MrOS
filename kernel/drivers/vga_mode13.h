// VGA Mode 13h, 320x200 pixels, 256 colors
// hardware details:
// 	video mem: 0xA0000 (linear, 64000 bytes = 320*200)
// 	each byte = one pixel, value = index into 256-color palette
// 	palette: 256 entries of (R, G, B) each 0-63 (6-bit DAC)
//
// Entering mode 13h:
// 	we can't use BIOS int 0x10 (we're in protected mode).
// 	we must program the VGA registers directly:
// 		-sequencer, crtc, graphics controller, attrivute controller and stuffs
//	Das ist the same register sequence BIOS uses for int 0x10 AX=0x13.
#ifndef VGA_MODE13_H
#define VGA_MODE13_H
#include <stdint.h>

#define MODE13_WIDTH 320
#define MODE13_HEIGHT 200
#define MODE13_PIXELS (MODE13_WIDTH * MODE13_HEIGHT)
#define MODE13_BUFFER ((uint8_t*)0xA0000)

// VGA register ports
#define VGA_MISC_WRITE 0x3C2
#define VGA_SEQ_INDEX 0x3C4
#define VGA_SEQ_DATA 0x3C5
#define VGA_PELADDR_WRITE 0x3C8 // palette write addr
#define VGA_PELDATA 0x3C9 // palette data (R then G then B)
#define VGA_GC_INDEX 0x3CE
#define VGA_GC_DATA 0x3CF
#define VGA_AC_INDEX 0x3C0
#define VGA_AC_READ 0x3C1
#define VGA_CRTC_INDEX 0x3D4
#define VGA_CRTC_DATA 0x3D5
#define VGA_INSTAT_READ 0x3DA

void mode13_enter(void);
void mode13_exit(void); // returns to VGA text mode 3

// this one writes directly to video mem
// and in it x must be 0-319, y must be 0-199
// color = palette index 0-255, boom!
static inline void mode13_set_pixel(int x, int y, uint8_t color) {
	MODE13_BUFFER[y * MODE13_WIDTH + x] = color;
}

// and this one will copy a 320x200 framebuffer to video memory in one shot.
// doom calls this every frame from DG_DrawFrame().
// src must ponit to MODE13_Pixels bytes
void mode13_blit(const uint8_t* src);

//so this one will fill entire screen with one color (default 0 = black)

void mode13_clear(uint8_t color);

// this one will upload 256 palette entries from a flat array.
// Array layout is [R0,G0,B0, R1,G1,B1, ..., R255,G255,B255]
// each comp is 0-63 (VGA DAC is 6-bit).
// Doom calls this once during startal with its playpal lump(not paypal:( )
void mode13_set_palette(const uint8_t* palette);

#endif // !VGA_MODE13_H
