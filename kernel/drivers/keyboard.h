#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>
#define KEYBOARD_DATA_PORT 0x60

#define KB_BUFFER_SIZE 256
#define KB_BUFFER_MASK (KB_BUFFER_SIZE - 1)

#define KB_EMPTY 0xFF

// special key codes
#define KEY_UP 0x80
#define KEY_DOWN 0x81
#define KEY_LEFT 0x82
#define KEY_RIGHT 0x83
#define KEY_PAGEUP 0x84
#define KEY_PAGEDOWN 0x85
#define KEY_HOME 0x86
#define KEY_END 0x87
#define KEY_DELETE 0x7F
#define KEY_F1 0x90
#define KEY_ESCAPE 0x1B

//modifier flags
#define KB_MOD_SHIFT 0x01
#define KB_MOD_CTRL 0x02
#define KB_MOD_ALT 0x04

typedef struct {
	uint8_t key; // ASCII or KEY_* constant
	uint8_t modifiers; // bitmask of KB_MOD_*
} key_event_t;

void keyboard_init(void);
void keyboard_irq_handler(void);

char keyboard_getchar(void);
key_event_t keyboard_getkey(void);

void keyboard_readline(char* buf, int max);

#endif // !KEYBOARD_H
