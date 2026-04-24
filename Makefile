# MrOS Makefile
CC = i686-elf-gcc
LD = i686-elf-ld
NASM = nasm
QEMU = qemu-system-i386
OBJCOPY = i686-elf-objcopy

INCLUDES = -Ikernel/core -Ikernel/interrupts -Ikernel/drivers -Ikernel/fitness -Ikernel/shell
CFLAGS = -D__STDC_HOSTED__=0 -Ikernel/include -m32 -ffreestanding \
          -fno-builtin -fno-stack-protector -nostdlib \
          -Wall -Wextra -O2 $(INCLUDES) -Ikernel \
          -fno-merge-constants -fno-jump-tables -fno-pic
LDFLAGS = -m elf_i386 -nostdlib -T linker.ld

# O files
BOOT_BIN = boot/boot.bin
ENTRY_OBJ = kernel/core/kernel_entry.o


SRCDIRS = \
	 kernel/core \
	 kernel/interrupts \
	 kernel/shell \
	 kernel/drivers \
	 kernel/fitness

KERNEL_BIN = kernel/kernel.bin
OS_IMAGE = mros.img

C_SRCS := $(foreach dir,$(SRCDIRS),$(wildcard $(dir)/*.c))
C_OBJS := $(patsubst %.c,%.o,$(C_SRCS))

all: $(OS_IMAGE)

# RAW BIN
$(BOOT_BIN): boot/boot_sect.asm
	$(NASM) -f bin $< -o $@

$(ENTRY_OBJ): kernel/core/kernel_entry.asm
	$(NASM) -f elf32 $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL_BIN): $(ENTRY_OBJ) $(C_OBJS)
	$(LD) $(LDFLAGS) -o kernel/kernel.elf $(ENTRY_OBJ) $(C_OBJS)
	$(OBJCOPY) -O binary kernel/kernel.elf $@

$(OS_IMAGE): $(BOOT_BIN) $(KERNEL_BIN)
	dd if=/dev/zero of=$(OS_IMAGE) bs=512 count=2048 2>/dev/null
	dd if=$(BOOT_BIN) of=$(OS_IMAGE) conv=notrunc 2>/dev/null
	dd if=$(KERNEL_BIN) of=$(OS_IMAGE) bs=512 seek=1 conv=notrunc 2>/dev/null

run: $(OS_IMAGE)
	$(QEMU) -drive format=raw,file=$(OS_IMAGE) -nographic

run-gui: $(OS_IMAGE)
	$(QEMU) -drive format=raw,file=$(OS_IMAGE)

clean:
	rm -f $(BOOT_BIN) $(ENTRY_OBJ) $(C_OBJS) $(KERNEL_BIN) kernel/kernel.elf $(OS_IMAGE)

.PHONY: all run run-gui clean
