// MrOS port of doomgeneric.
//
// foomgeneric is a stripped doom source port designed for bare-metal and embedded targets. so it exposes a small set of functions you must implement, and calls them itselt.
//
// source: https://github.com/ozkl/doomgeneric
//
// we needa implement some functions tho,
//
// so wad loading: doom loads the wad file via standard I/O (fopen/fread).
// we inteercept these by providing our own fopen/fread/fclose.
// that read from a fixed di*k location using ATA driver.
//
// wad location : sector 128 onward(64KB offset from start of disk).
// Embedded bt makefile:
// 	dd if=doom1.wad of=mros.img bs=512 see=128 conv=notrunc you will see me implement those later on,
#include "doomgeneric_mros.h"
#include "../drivers/vga_mode13.h"
#include "../drivers/keyboard.h"
#include "../drivers/timer.h"
#include "../lib/string.h"
#include "../lib/malloc.h"
#include "../core/vga.h"
#include "../interrupts/pic.h"

// WAD I/O: fake stdio backed by ATA PIO disk reads
// ATA PIO driver (simple, polling kinda fine for loading WAD once anyways):
// you guys can search about their ports, i wont be explaining them, its too much comments now
#define ATA_DATA 0x1F0
#define ATA_SECCOUNT 0x1F2
#define ATA_LBA_LO 0x1F
#define ATA_LBA_MID 0x1F4
#define ATA_LBA_HI 0x1F5
#define ATA_LBA_DRIVE 0x1F6
#define ATA_DRIVE 0x1F6
#define ATA_CMD 0x1F7
#define ATA_STATUS 0x1F7
#define ATA_CMD_READ 0x20

#define WAD_START_SECTOR 128 // here we go again

// read 'count' 512-byte sectors starting at LBA 'lba' into 'buf'
static void ata_read_sectors(uint32_t lba, uint8_t count, uint8_t* buf) {
	// wait for drive to be rrrrreddi
	while (inb(ATA_STATUS) & 0x80); // bsy bit :)
	
	outb(ATA_DRIVE, 0xE0 | ((lba >> 24) & 0x0F));
	outb(ATA_SECCOUNT, count);
	outb(ATA_LBA_LO, (uint8_t)(lba));
	outb(ATA_LBA_MID, (uint8_t)(lba >> 8));
	outb(ATA_LBA_HI, (uint8_t)(lba >> 16));
	outb(ATA_CMD, ATA_CMD_READ);

	for (int s = 0; s < count; s++) {
		// wait for DRQ (data request)
		while (!(inb(ATA_STATUS) & 0x08));

		// read 256 16-bit words 
		uint16_t* ptr = (uint16_t*)(buf + s * 512);
		for (int i = 0; i < 256; i++) {
			// inw; read 16bit word from portye
			uint16_t val;
			__asm__ volatile ("inw %1, %0" : "=a"(val) : "Nd"((uint16_t)ATA_DATA));
			ptr[i] = val;
		}
	}
}

// simple fake file, we only support sequential reads of the wad
// doom opens exactly one file (the wad), reads it and boom closes it
typedef struct {
	uint32_t disk_offset_bytes; // current byte position from wad start
	uint32_t wad_size_bytes; // total wad size (read from wad header)
	int valid;
} fake_file_t;

static fake_file_t wad_handle;

// these are called by doom's W_init via the standard C names 
// we ovveride them with our own symbol, yes typical arch user btw
void* fopen (const char* path, const char* mode) {
	(void)path; (void)mode;
	wad_handle.disk_offset_bytes = 0;
	wad_handle.valid = 1;
	return (void*)&wad_handle;
}

int fclose(void* f) {
	(void)f;
	wad_handle.valid = 0;
	return 0;
}

size_t fread(void* buf, size_t size, size_t nmemb, void* f) {
	(void)f;
	size_t total_bytes = size * nmemb;
	if (total_bytes == 0) return 0;
	// conv byte offset to sector-offset.
	// wad starts at it start sector on disk
	// we read whole sectors  and slice the needed byte, like slice them slice slice.....
	uint32_t byte_pos = wad_handle.disk_offset_bytes;
	uint32_t start_lba = WAD_START_SECTOR + (byte_pos / 512);
	uint32_t byte_off = byte_pos % 512;

	// how much to read?
	uint32_t sectors_needed = (byte_off + total_bytes + 511) / 512;
	if (sectors_needed > 255) sectors_needed = 255;

	// tmp sector buffer
	uint8_t* sector_buf = (uint8_t*)kmalloc(sectors_needed * 512);
	if (!sector_buf) return 0;

	ata_read_sectors(start_lba, (uint8_t)sectors_needed, sector_buf);
	memcpy(buf, sector_buf + byte_off, total_bytes);

	wad_handle.disk_offset_bytes += total_bytes;
	return nmemb;
}

int fseek(void* f, long offset, int whence) {
	(void)f;
	if (whence == 0) wad_handle.disk_offset_bytes = (uint32_t)offset; //seek set
	else if (whence == 1) wad_handle.disk_offset_bytes += (uint32_t)offset; // seek cur
	return 0;
}

long ftell(void* f) {
	(void)f;
	return (long)wad_handle.disk_offset_bytes;
}

int feof(void* f) { (void)f; return 0; }
int ferror(void* f) { (void)f; return 0; }

// key mapping
// map our scancodes with doom codes
// doom generic defines the keys in doomkeys.h
// we use the asscii values our keyboard driver produces

#define DOOM_KEY_RIGHTARROW 0xae
#define DOOK_KEY_LEFTARROW 0xac
#define DOOM_KEY_UPARROW 0xad
#define DOOM_KEY_DOWNARROW  0xaf
#define DOOM_KEY_FIRE ' '
#define DOOM_KEY_USE 'e'
#define DOOM_KEY_ESCAPE 27

// simple key event queue
#define KEY_QUEUE_SIZE 16
static struct { int pressed; unsigned char key; } key_queue[KEY_QUEUE_SIZE];
static int kq_head = 0, kq_tail = 0;

static void kq_push(int pressed, unsigned char key) {
	int next = (kq_head + 1) % KEY_QUEUE_SIZE;
	if (next != kq_tail) {
		key_queue[kq_head].pressed = pressed;
		key_queue[kq_head].key = key;
		kq_head = next;
	}

}

// called from keyboard IRQ handler when doom is sprinting
void doom_key_event(unsigned char ascii, int pressed) {
	kq_push(pressed, ascii);
}

// interface implentation
void DG_Init(void) {
	// DG_screenbuffer is a global defined by doomgeneric 4 us
	extern uint8_t* DG_ScreenBuffer;
	mode13_blit(DG_ScreenBuffer);
}

void DG_SleepMs(uint32_t ms) {
	uint32_t ticks = ms / 10;
	if (ticks == 0) ticks = 1;
	timer_sleep_ticks(ticks);
}

uint32_t DG_GetKet(int* pressed, unsigned char* doomKey) {
	if (kq_head == kq_tail) return 0; // empty
	*pressed = key_queue[kq_tail].pressed;
	*doomKey = key_queue[kq_tail].key;
	kq_tail = (kq_tail + 1) % KEY_QUEUE_SIZE;
	return 1;
}

void DG_SetWindowTitle(const char* title) {
	(void)title; // no wm
}

// Stub for missing functions
void I_BindJoystickVariables(void) {}
void I_InitJoystick(void) {}

int DG_GetKey(int* pressed, unsigned char* doomKey) {
	if (kq_head == kq_tail) return 0; // empty
	*pressed = key_queue[kq_tail].pressed;
	*doomKey = key_queue[kq_tail].key;
	kq_tail = (kq_tail + 1) % KEY_QUEUE_SIZE;
	return 1;
}

char* strrchr(const char* s, int c) {
	const char* found = NULL;
	while (*s) {
		if (*s == (char)c) found = s;
		s++;
	}
	return (char*)found;
}

void kfree(void* p) { (void)p; }

int remove(const char* path) { (void)path; return 0; }
int rename(const char* old, const char* new) { (void)old; (void)new; return 0; }
int system(const char* cmd) { (void)cmd; return 0; }

void DG_DrawFrame(void) {
	extern uint8_t* DG_ScreenBuffer;
	mode13_blit(DG_ScreenBuffer);
}

void DG_SetPalette(const uint8_t* palette) {
	mode13_set_palette(palette);
}

uint32_t DG_GetTicksMs(void) {
	return timer_get_ticks() * 10;
}

double atof(const char* s) {
	double result = 0.0;
	int sign = 1;
	if (*s == '-') { sign = -1; s++; }
	while (*s >= '0' && *s <= '9') {
		result = result * 10 + (*s - '0');
		s++;
	}
	if (*s == '.') {
		s++;
		double div = 10.0;
		while (*s >= '0' && *s <= '9') {
			result += (*s - '0') / div;
			div *= 10.0;
			s++;
		}
	}
	return sign * result;
}

int mkdir(const char* path, int mode) { (void)path; (void)mode; return 0; }

long long __divdi3(long long a, long long b) {
	return a / b;
}

// doom launch and exit

//doomgeneric main: declare extern, defined in doomgeneric source
extern int doomgeneric_Create(int argc, char** argv);
extern void doomgeneric_Tick(void);

static int doom_running = 0;

void doom_launch(void) {
	doom_running = 1;
	vga_println("Initializing Doom...", COLOR_YELLOW);
	vga_println("Loading WAD from disk sector 128...", COLOR_GREY);

	mm_init(); // reset heap so Doom gets a fresh 8MB
	
	//argv for Doom: just the program name
	static char* doom_argv[] = {"doom", 0};
	doomgeneric_Create(1, doom_argv);

	//main loop: runs untill player quits
	while (doom_running) {
		doomgeneric_Tick();

		// check for doom exit: if player presses esc on main menu
		// doom generic sets a quit flag, we detect it by checking
		// if dg)getkey returns a quit event
		// for now, also allow Ctrl+Q from shell context to exit

	}

	//restore text mode and return to shell
	mode13_exit();
	vga_clear();
	doom_running = 0;
	vga_println("Returned from DOOM.", COLOR_GREEN);
}

void doom_quit(void) {
	doom_running = 0;
}
