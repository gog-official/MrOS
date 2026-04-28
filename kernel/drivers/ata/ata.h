// ATA PIO driver
// PIO means the CPU read/writes data direcctly via Input output ports.
// slower than dma but simple and perfectly fine for fs.
#ifndef ATA_H
#define ATA_H

#include <stdint.h>

//IO PORTS
#define ATA_DATA 0x1F0
#define ATA_ERROR 0x1F1
#define ATA_FEATURES 0x1F1
#define ATA_SECCOUNT 0x1F2
#define ATA_LBA_LO 0x1F3
#define ATA_LBA_MID 0x1F4
#define ATA_LBA_HI 0x1F5
#define ATA_DRIVE_HEAD 0x1F6
#define ATA_STATUS 0x1F7
#define ATA_COMMAND 0x1F7
#define ATA_ALT_STATUS 0x3F6
#define ATA_DEV_CTRL 0x3F6

// status register bits
#define ATA_SR_BSY 0x80
#define ATA_SR_DRDY 0x40
#define ATA_SR_DRQ 0x08
#define ATA_SR_ERR 0x01

// cmd
#define ATA_CMD_READ_SECTORS 0x20
#define ATA_CMD_WRITE_SECTORS 0x30
#define ATA_CMD_IDENTIFY 0xEC
#define ATA_CMD_FLUSH 0xE7

// drive select
#define ATA_DRIVE_MASTER 0xE0
#define ATA_DRIVE_SLAVE 0xF0

// sector size
#define ATA_SECTOR_SIZE 512

typedef enum {
	ATA_OK = 0,
	ATA_ERR_BSY = -1,
	ATA_ERR_DRQ = -2,
	ATA_ERR_FAULT = -3,
	ATA_ERR_RANGE = -4
} ata_error_t;

ata_error_t ata_init(void);
ata_error_t ata_read_sectors(uint32_t lba, uint8_t count, uint8_t* buf);
ata_error_t ata_write_sectors(uint32_t lba, uint8_t count, const uint8_t* buf);
ata_error_t ata_read_byte_range(uint32_t byte_offset, uint32_t len, uint8_t* buf);
ata_error_t ata_write_byte_range(uint32_t byte_offset, uint32_t len, const uint8_t* buf);

#endif // !ATA_H
