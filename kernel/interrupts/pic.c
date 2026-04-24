// remap both PICs and provide EOI helper

#include "pic.h"

void pic_remap(void) {
	uint8_t mask1 = inb(PIC1_DATA);
	uint8_t mask2 = inb(PIC2_DATA);

	// begin init sequence (0x11 = init + expect ICW4)
	outb(PIC1_COMMAND, 0x11); io_wait();
	outb(PIC2_COMMAND, 0x11); io_wait();

	//set vector offsets
	outb(PIC1_DATA, 0x20); io_wait();
	outb(PIC2_DATA, 0x28); io_wait();

	// tell master slave is on irq2 pin
	outb(PIC1_DATA, 0x04); io_wait();
	outb(PIC2_DATA, 0x02); io_wait();

	// ICW4: 8086 mode
	outb(PIC1_DATA, 0x01); io_wait();
	outb(PIC2_DATA, 0x01); io_wait();

	// restore saved masks
	outb(PIC1_DATA, mask1);
	outb(PIC2_DATA, mask2);
}
// tells PIC the interrupt has been handled

void pic_send_eoi(uint8_t irq) {
	if (irq >= 8) {
		outb(PIC2_COMMAND, PIC_EOI);
	}
	outb(PIC1_COMMAND, PIC_EOI);
}

