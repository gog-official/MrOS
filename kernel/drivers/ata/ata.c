// das ist polling mode(no  irq)
#include "ata.h"
#include "../../interrupts/pic.h"
#include "../../lib/string.h"

// low lvl helpers

static inline uint16_t inw(uint16_t port) {
	uint16_t val;
	__asm__ volatile ("inw %1, %0" : "=a"(val) : "Nd"(port));
	return val;
}

static inline void outw(uint16_t port, uint16_t val) {
	__asm__ volatile ("outw %0, %1" : : "a"(val), "Nd"(port));
}

static ata_error_t ata_wait_bsy(void) {
	for (uint32_t i = 0; i < 0x1000000; i++) {
		if (!(inb(ATA_ALT_STATUS) & ATA_SR_BSY)) return ATA_OK;
	}
	return ATA_ERR_BSY;
}

static ata_error_t ata_wait_drq(void) {
	for (uint32_t i = 0; i < 0x1000000; i++) {
		uint8_t status = inb(ATA_ALT_STATUS);
		if (status & ATA_SR_ERR) return ATA_ERR_FAULT;
		if (status & ATA_SR_DRQ) return ATA_OK;
	}
	return ATA_ERR_DRQ;
}

static void ata_select(uint32_t lba) {
	outb(ATA_DRIVE_HEAD, ATA_DRIVE_MASTER | ((lba >> 24) & 0x0F ));
	inb(ATA_ALT_STATUS); inb(ATA_ALT_STATUS);
	inb(ATA_ALT_STATUS); inb(ATA_ALT_STATUS);
}

static void ata_send_lba(uint32_t lba, uint8_t count) {
	outb(ATA_SECCOUNT, count);
	outb(ATA_LBA_LO, (uint8_t)(lba));
	outb(ATA_LBA_MID, (uint8_t)(lba >> 8));
	outb(ATA_LBA_HI, (uint8_t)(lba >> 16));
}

// public API
ata_error_t ata_init(void) {
	outb(ATA_DEV_CTRL, 0x02);
	ata_select(0);
	outb(ATA_COMMAND, ATA_CMD_IDENTIFY);
	uint8_t status = inb(ATA_STATUS);
	if (status == 0x00) return ATA_ERR_FAULT;

	ata_error_t err = ata_wait_bsy();
	if (err != ATA_OK) return err;

	err = ata_wait_drq();
	if (err != ATA_OK) return err;
	for (int i = 0; i < 256; i++) inw(ATA_DATA);

	return ATA_OK;
}

ata_error_t ata_read_sectors(uint32_t lba, uint8_t count, uint8_t* buf) {
	if (count == 0) return ATA_ERR_RANGE;

	ata_error_t err = ata_wait_bsy();
	if (err != ATA_OK) return err;

	ata_select(lba);
	ata_send_lba(lba, count);
	outb(ATA_COMMAND, ATA_CMD_READ_SECTORS);

	for (int s = 0; s < count; s++) {
		err = ata_wait_bsy();
		if (err != ATA_OK) return err;
		err = ata_wait_drq();
		if (err != ATA_OK) return err;

		uint16_t*  ptr = (uint16_t*)(buf + s * ATA_SECTOR_SIZE);
		for (int i = 0; i < 256; i ++) {
			ptr[i] = inw(ATA_DATA);
		}
	}
	return ATA_OK;
}

ata_error_t ata_write_sectors(uint32_t lba, uint8_t count, const uint8_t* buf) {
	if (count == 0) return ATA_ERR_RANGE;

	ata_error_t err = ata_wait_bsy();
	if (err != ATA_OK) return err;

	ata_select(lba);
	ata_send_lba(lba, count);
	outb(ATA_COMMAND, ATA_CMD_WRITE_SECTORS);

	for (int s = 0; s < count; s++) {
		err = ata_wait_drq();
		if (err != ATA_OK) return err;

		const uint16_t* ptr = (const uint16_t*) (buf + s * ATA_SECTOR_SIZE);

		for (int i = 0; i < 256; i ++) {
			outw(ATA_DATA, ptr[i]);
		}
	}

	outb(ATA_COMMAND, ATA_CMD_FLUSH);
	ata_wait_bsy();

	return ATA_OK;
}

ata_error_t ata_read_byte_range(uint32_t byte_offset, uint32_t len, uint8_t* buf) {
	if (len == 0) return ATA_OK;

	uint32_t start_lba = byte_offset / ATA_SECTOR_SIZE;
	uint32_t start_off = byte_offset % ATA_SECTOR_SIZE;
	uint32_t end_byte = byte_offset + len - 1;
	uint32_t end_lba = end_byte / ATA_SECTOR_SIZE;
	uint32_t sectors = end_lba - start_lba + 1;

	static uint8_t tmp[255 * ATA_SECTOR_SIZE];
	if (sectors > 255) return ATA_ERR_RANGE;

	ata_error_t err = ata_read_sectors(start_lba, (uint8_t)sectors, tmp);
	if (err != ATA_OK) return err;

	memcpy(buf, tmp + start_off, len);
	return ATA_OK;
}

ata_error_t ata_write_byte_range(uint32_t byte_offset, uint32_t len, const uint8_t* buf) {
	if (len == 0) return ATA_OK;

	uint32_t start_lba = byte_offset / ATA_SECTOR_SIZE;
	uint32_t start_off = byte_offset % ATA_SECTOR_SIZE;
	uint32_t end_lba = (byte_offset + len - 1) / ATA_SECTOR_SIZE;
	uint32_t sectors = end_lba - start_lba + 1;

	if (sectors > 255) return ATA_ERR_RANGE;

	static uint8_t tmp[255 * ATA_SECTOR_SIZE];

	ata_error_t err = ata_read_sectors(start_lba, (uint8_t)sectors, tmp);
	if (err != ATA_OK) return err;

	memcpy(tmp + start_off, buf, len);

	return ata_write_sectors(start_lba, (uint8_t)sectors, tmp);
}
