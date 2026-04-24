#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>
#define KEYBOARD_DATA_PORT 0x60

#define KB_BUFFER_SIZE 256
#define KB_BUFFER_MASK (KB_BUFFER_SIZE - 1)

#define KB_EMPTY 0xFF

void keyboard_init(void);
void keyboard_irq_handler(void);

char keyboard_getchar(void);

void keyboard_readline(char* buf, int max);

#endif // !KEYBOARD_H
