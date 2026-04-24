// PIT setup and irq0 handler

#include "timer.h"
#include "../interrupts/idt.h"
#include "../interrupts/pic.h"

// global tickie counter
static volatile uint32_t ticks = 0;

// asm wrapper symbol
extern void irq0_wrapper(void);

void timer_irq_handler(void) {
	ticks ++;
	pic_send_eoi(0);
}

// this tells gcc no prologue or epilogue
__attribute__((naked)) void irq0_wrapper(void) {
	__asm__ volatile (
		"pusha\n"
		"call timer_irq_handler\n"
		"popa\n"
		"iret\n"
	);
}

void timer_init(void) {
	outb(PIT_COMMAND, 0x36);

	// send divisor low byte
	uint16_t divisor = (uint16_t)PIT_DIVISOR;
	outb(PIT_CHANNEL0, (uint8_t)(divisor & 0xFF));
	outb(PIT_CHANNEL0, (uint8_t)(divisor >> 8) & 0xFF);

	idt_set_gate(IRQ0_TIMER, (uint32_t)irq0_wrapper, 0x08, IDT_INTERRUPT_GATE);
}

uint32_t timer_get_ticks(void) {
	return ticks;
}

void timer_sleep_ticks(uint32_t t) {
	uint32_t end = ticks + t;
	while (ticks < end) {
		__asm__ volatile ("hlt");
	}
}

void timer_sleep(uint32_t seconds) {
	timer_sleep_ticks(seconds * TICKS_PER_SEC);
}
