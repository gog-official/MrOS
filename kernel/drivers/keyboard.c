// covers most stuff

#include "keyboard.h"
#include "../interrupts/idt.h"
#include "../interrupts/pic.h"
#include "../core/vga.h"
#include "../sys/statusbar.h"
#include "../sys/reminder.h"

// scancode table
static const char sc_normal[88] = {
	0,
	27,
	'1','2','3','4','5','6','7','8','9','0','-','=',
	'\b',
	'\t',
	'q','w','e','r','t','y','u','i','o','p','[',']',
	'\n',
	0,
	'a','s','d','f','g','h','j','k','l',';','\'','`',
	0,
	'\\',
	'z','x','c','v','b','n','m',',','.','/',
	0,
	'*',
	0,
	' ',
	0,
	// F1-F10 and beyond, not mapped
	0,0,0,0,0,0,0,0,0,0,
	0,
	0,
	0,0,0,0,0,0,0,0,0,0,0
};

static const char sc_shifted[88] = {
	0,
	0,
	'!','@','#','$','%','^','&','*','(',')','_','+',
	'\b',
	'\t',
	'Q','W','E','R','T','Y','U','I','O','P','{','}',
	'\n',
	0,
	'A','S','D','F','G','H','J','k','L',':','"','~',
	0,
	'|',
	'Z','X','C','V','B','N','M','<','>','?',
	0,
	'*',
	0,
	' ',
	0,
	0,0,0,0,0,0,0,0,0,0,
	0,
	0,
	0,0,0,0,0,0,0,0,0,0,0
};

// states
static volatile int shift_held = 0;
static volatile int ctrl_held = 0;
static volatile int extended = 0;
static volatile int caps_lock = 0;

// ring buffer
static volatile key_event_t kb_buf[KB_BUFFER_SIZE];
static volatile uint32_t kb_head = 0;
static volatile uint32_t kb_tail = 0;

//ring_buffer helpers
static inline int buf_full(void) {
	return (((kb_head + 1) & KB_BUFFER_MASK) == kb_tail);
}

static inline int buf_empty(void) {
	return (kb_head == kb_tail);
}

// push one char
static inline void buf_push(uint8_t key, uint8_t mods) {
	if (!buf_full()) {
		kb_buf[kb_head].key = key;
		kb_buf[kb_head].modifiers = mods;
		kb_head = (kb_head + 1) & KB_BUFFER_MASK;
	}
}

// pop one char

static inline key_event_t buf_pop(void) {
	key_event_t e = { KB_EMPTY, 0 };
	if (!buf_empty()) {
		e = kb_buf[kb_tail];
		kb_tail = (kb_tail + 1) & KB_BUFFER_MASK;
	}
	return e;
}

// irq1 handler
extern void irq1_wrapper(void);

void keyboard_irq_handler(void) {
	uint8_t scancode = inb(KEYBOARD_DATA_PORT);

	if (scancode == 0xE0) {
		extended = 1;
		pic_send_eoi(1);
		return;
	}

	uint8_t is_release = scancode & 0x80;
	uint8_t make_code = scancode & 0x7F;

	// handle extended scancode
	if (extended) {
		extended = 0;
		if (!is_release) {
			uint8_t mods = 0;
			if (shift_held) mods |= KB_MOD_SHIFT;
			if (ctrl_held) mods |= KB_MOD_CTRL;

			switch (make_code) {
				case 0x48: buf_push(KEY_UP, mods); break;
				case 0x50: buf_push(KEY_DOWN, mods); break;
				case 0x4B: buf_push(KEY_LEFT, mods); break;
				case 0x4D: buf_push(KEY_RIGHT, mods); break;
				case 0x49: buf_push(KEY_PAGEUP, mods); break;
				case 0x51: buf_push(KEY_PAGEDOWN, mods); break;
				case 0x47: buf_push(KEY_HOME, mods); break;
				case 0x4F: buf_push(KEY_END, mods); break;
				case 0x53: buf_push(KEY_DELETE, mods); break;
				default: break;
			}
		}
		pic_send_eoi(1);
		return;
	}

	if (make_code == 0x1D) {
		ctrl_held = !is_release;
		pic_send_eoi(1);
		return;
	}

	if (make_code == 0x2A || make_code == 0x36) {
		shift_held = !is_release;
		pic_send_eoi(1);
		return;
	}

	if (make_code == 0x3A && !is_release) {
		caps_lock = !caps_lock;
		pic_send_eoi(1);
		return;
	}

	if (is_release) {
		pic_send_eoi(1);
		return;
	}

	if (make_code >= 88) {
		pic_send_eoi(1);
		return;
	}

	// switch between tables
	char c;
	if (shift_held) {
		c = sc_shifted[make_code];
	} else {
		c = sc_normal[make_code];
	}

	if (caps_lock && c >= 'a' && c <= 'z') c = c-'a'+'A';
	if(caps_lock && c>= 'A' && c <= 'Z') c = c-'A'+'a';

	if (c != 0) {
		uint8_t mods = 0;
		if (shift_held) mods |= KB_MOD_SHIFT;
		if (ctrl_held) mods |= KB_MOD_CTRL;
		buf_push((uint8_t)c, mods);
	}
	
	pic_send_eoi(1);
}

//irq1 _wrapper
__attribute__((naked)) void irq1_wrapper(void) {
	__asm__ volatile (
			"pusha\n"
			"call keyboard_irq_handler\n"
			"popa\n"
			"iret\n"
			);
}

// public API
void keyboard_init(void) {
	idt_set_gate(IRQ1_KEYBOARD, (uint32_t)irq1_wrapper, 0x08, IDT_INTERRUPT_GATE);
}

key_event_t keyboard_getkey(void) {
	while (buf_empty()) {
		__asm__ volatile ("hlt");
	}
	return buf_pop();
}

char keyboard_getchar(void) {
	while (buf_empty()) {
		statusbar_update();
		reminder_process(); // play pending reminder sounds while waiting for keyboard input
		key_event_t e = keyboard_getkey();
		if (e.key != KB_EMPTY && e.key < 0x80) return (char)e.key;
		if (e.key == KEY_ESCAPE) return 0x1B;
		if (e.key == KEY_DELETE) return 127;
	}
}

// keyboard readline, reads char until you press return

void keyboard_readline(char* buf, int max) {
	int pos = 0;
	while (1) {
		statusbar_update();
		reminder_process(); // play any pending reminder sounds while waiting for input
		key_event_t e = keyboard_getkey();
		char c = (char)e.key;

		if (c == '\n') {
			if (pos < max) buf[pos] = '\0';
			else if (max > 0) buf[max - 1] = '\0';
			vga_putchar('\n', COLOR_DEFAULT);
			return;
		}

		if (c == '\b') {
			if (pos > 0) {
				pos --;
				// erase on screen
				if (cursor_col > 0) {
					cursor_col --;
					vga_set_cell(cursor_row, cursor_col, ' ', COLOR_DEFAULT);
					vga_set_cursor(cursor_row, cursor_col);
				}
			}
			continue;
		}

		if (e.key >= 0x80) continue;

		// ignore tab inside readline
		if (c == '\t') continue;

		//only printable asscii
		if (c < 0x20) continue;

		//try not to overflow buf
		if (pos >= max - 1) continue;

		buf[pos++] = c;
		vga_set_cursor(cursor_row, cursor_col);
		vga_putchar(c, COLOR_DEFAULT);
	}
}
