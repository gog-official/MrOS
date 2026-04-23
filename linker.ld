ENTRY(kernel_entry)

SECTIONS
{
    . = 0x10000;

    .text : {
	kernel/kernel_entry.o(.text)
        *(.text)
    }

    .rodata : {
        *(.rodata)
	*(.rodata.*)
    }
    .data : {
        *(.data)
    }

    .bss : {
	_bss_start = .;
        *(.bss)
        *(COMMON)
	_bss_end = .;
    }
}
