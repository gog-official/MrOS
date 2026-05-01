# MrOS Makefile
CC = i686-elf-gcc
LD = i686-elf-ld
NASM = nasm
QEMU = qemu-system-i386
OBJCOPY = i686-elf-objcopy

INCLUDES = -Ikernel/core -Ikernel/interrupts -Ikernel/drivers -Ikernel/driver/ata -Ikernel/fitness -Ikernel/shell -Ikernel/lib -Ikernel/fs -Ikernel/fs/fat32 -Ikernel/crypto -Ikernel/auth -Iinclude
CFLAGS = -D__STDC_HOSTED__=0 -Ikernel/include -m32 -ffreestanding \
          -fno-builtin -fno-stack-protector -nostdlib \
          -Wall -Wextra -O2 $(INCLUDES) -Ikernel \
          -fno-merge-constants -fno-jump-tables -fno-pic \
          -include stdint.h
LDFLAGS = -m elf_i386 -nostdlib -T linker.ld

# O files
BOOT_BIN = boot/boot.bin
ENTRY_OBJ = kernel/core/kernel_entry.o


SRCDIRS = \
	 kernel/core \
	 kernel/interrupts \
	 kernel/shell \
	 kernel/drivers \
	 kernel/drivers/ata \
	 kernel/sys	\
	 kernel/fitness \
	 kernel/lib \
	 kernel/fs \
	 kernel/fs/fat12 \
	 kernel/crypto \
	 kernel/auth 

KERNEL_BIN = kernel/kernel.bin
OS_IMAGE = mros.img

C_SRCS := $(foreach dir,$(SRCDIRS),$(wildcard $(dir)/*.c))
C_OBJS := $(patsubst %.c,%.o,$(C_SRCS))

FS_IMAGE = mros_fs.img
FS_IMAGE_BPB = mros_fs_pbp.bin
FS_SECTORS = 2880
FS_OFFSET = 256

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

$(FS_IMAGE_BPB):
	python3 scripts/make_fs_bpb.py $@

$(FS_IMAGE): $(FS_IMAGE_BPB)
	dd if=/dev/zero of=$(FS_IMAGE) bs=512 count=$(FS_SECTORS) 2>/dev/null
	dd if=$(FS_IMAGE_BPB) of=$(FS_IMAGE) bs=512 count=1 conv=notrunc 2>/dev/null

check_kernel_size: $(KERNEL_BIN)
	@SIZE=$$(wc -c < $(KERNEL_BIN)); \
	MAX=$$((255 * 512)); \
	if [ $$SIZE -gt $$MAX ]; then \
	    echo "ERROR: kernel.bin ($$SIZE bytes) exceeds 255-sector limit ($$MAX bytes)"; \
	    echo "Increase sector gap between kernel and FS, or move FS_OFFSET"; \
	    exit 1; \
	fi
	@echo "Kernel OK: $$(wc -c < $(KERNEL_BIN)) bytes / $$((255 * 512)) max"
$(OS_IMAGE): $(BOOT_BIN) $(KERNEL_BIN) $(FS_IMAGE)
	dd if=/dev/zero of=$(OS_IMAGE) bs=512 count=4096 2>/dev/null
	dd if=$(BOOT_BIN) of=$(OS_IMAGE) conv=notrunc 2>/dev/null
	dd if=$(KERNEL_BIN) of=$(OS_IMAGE) bs=512 seek=1 conv=notrunc 2>/dev/null
	dd if=$(FS_IMAGE) of=$(OS_IMAGE) bs=512 seek=$(FS_OFFSET) conv=notrunc 2>/dev/null

iso: mros.img
	# Create a proper bootable ISO using grub-mkrescue style approach
	mkdir -p iso_staging/boot/grub
	cp mros.img iso_staging/boot/
	echo 'set timeout=0' > iso_staging/boot/grub/grub.cfg
	echo 'set default=0' >> iso_staging/boot/grub/grub.cfg
	echo 'menuentry "MrOS" {' >> iso_staging/boot/grub/grub.cfg
	echo '  multiboot2 /boot/mros.img' >> iso_staging/boot/grub/grub.cfg
	echo '  boot' >> iso_staging/boot/grub/grub.cfg
	echo '}' >> iso_staging/boot/grub/grub.cfg
	grub-mkrescue -o mros.iso iso_staging 2>/dev/null || \
	xorriso -as mkisofs -o mros.iso -b mros.img -c boot.cat -boot-info-table -no-emul-boot -boot-load-size 4 -V "MROS" iso_staging
	rm -rf iso_staging

run: $(OS_IMAGE)
	$(QEMU) -drive format=raw,file=$(OS_IMAGE) -nographic

run-gui: $(OS_IMAGE)
	$(QEMU) -drive format=raw,file=$(OS_IMAGE) -audiodev pa,id=snd -machine pcspk-audiodev=snd

run-iso: mros.iso
	$(QEMU) -cdrom mros.iso -boot d -nographic
clean:
	rm -f $(BOOT_BIN) $(ENTRY_OBJ) $(C_OBJS) $(KERNEL_BIN) kernel/kernel.elf $(OS_IMAGE) $(FS_IMAGE) $(FS_IMAGE_BPB)

.PHONY: all run run-gui clean check_kernel_size
