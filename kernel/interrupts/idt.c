// sets up the IDT and loads it with LIDT. only required are registers, other
// are left as 0
#include "idt.h"
static idt_entry_t idt[256];
static idt_ptr_t idt_ptr;

void idt_set_gate(uint8_t num, uint32_t handler, uint16_t selector, uint8_t type_attr) {
	idt[num].offset_lo = (uint16_t)(handler & 0xFFFF);
	idt[num].offset_hi = (uint16_t)((handler >> 16) & 0xFFFF);
	idt[num].selector = selector;
	idt[num].zero = 0;
	idt[num].type_attr = type_attr;
}

//called from kmain
void idt_init(void) {
	idt_ptr.limit = (sizeof(idt_entry_t) * 256) - 1;
	idt_ptr.base = (uint32_t)&idt;

	// zero out entire table
	uint8_t* p = (uint8_t*)idt;
	for (int i = 0; i < (int)sizeof(idt); i++) p[i] = 0;

	// LIDT: Load the IDT register
	__asm__ volatile ("lidt %0" : : "m"(idt_ptr));
}
