#ifndef PIC_H
#define PIC_H

#include <stdint.h>

#define PIC1_COMMAND 0x20
#define PIC1_DATA 0x21
#define PIC2_COMMAND 0xA0 // slave
#define PIC2_DATA 0xA1 // slave :(

#define PIC_EOI 0x20 // end of interrupt command

#define IRQ0_TIMER 0x20
#define IRQ1_KEYBOARD 0x21

void pic_remap(void);
void pic_send_eoi(uint8_t irq);

static inline void outb(uint16_t port, uint8_t val) {
	__asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
	uint8_t ret;
	__asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
	return ret;
}

static inline void io_wait(void) {
	outb(0x80, 0);
}

#endif // !PIC_H
