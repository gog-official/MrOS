#ifndef IDT_H
#define IDT_H

#include <stdint.h>

typedef struct {
	uint16_t offset_lo;
	uint16_t selector;
	uint8_t zero;
	uint8_t type_attr;
	uint16_t offset_hi;
} __attribute__((packed)) idt_entry_t;

typedef struct {
	uint16_t limit;
	uint32_t base;
} __attribute__((packed)) idt_ptr_t;

#define IDT_INTERRUPT_GATE 0x8E
#define IDT_TRAP_GATE 0x8F

void idt_init(void);
void idt_set_gate(uint8_t num, uint32_t handler, uint16_t selector, uint8_t type_attr);

#endif // !IDT_H
