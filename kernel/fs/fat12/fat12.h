// FAT12 filesystem driver
#ifndef FAT12_H
#define FAT12_H

#include <stdint.h>
#include "../../fs/vfs.h"

typedef struct {
	uint8_t jump[3];
	uint8_t oem_name[8];
	uint16_t bytes_per_sector;
	uint8_t sectors_per_cluster;
	uint16_t reserved_sectors;
	uint8_t num_fats;
	uint16_t root_entry_count;
	uint16_t total_sectors_16;
	uint8_t media_descriptor;
	uint16_t sectors_per_fat;
	uint16_t sectors_per_track;
	uint16_t num_heads;
	uint32_t hidden_sectors;
	uint32_t total_sectors_32;
	uint8_t drive_num;
	uint8_t reserved1;
	uint8_t boot_signature;
	uint32_t volume_id;
	uint8_t volume_label[11];
	uint8_t fs_type[8];
} __attribute__((packed)) fat12_bpb_t;

typedef struct {
	uint8_t name[8];
	uint8_t ext[3];
	uint8_t attr;
	uint8_t reserved[10];
	uint16_t time;
	uint16_t date;
	uint16_t first_cluster;
	uint32_t file_size;
} __attribute__ ((packed)) fat12_dirent_t;

#define FAT_ATTR_READONLY 0x01
#define FAT_ATTR_HIDDEN 0x02
#define FAT_ATTR_SYSTEM 0x04
#define FAT_ATTR_VOLUMEID 0x08
#define FAT_ATTR_DIRECTORY 0x10
#define FAT_ATTR_ARCHIVE 0x20
#define FAT_ATTR_LFN 0x0F

#define FAT12_EOC 0xFF8

typedef struct {
	uint32_t fat_start_sector;
	uint32_t root_start_sector;
	uint32_t data_start_sector;
	uint32_t root_dir_sectors;
	uint8_t sectors_per_cluster;
	uint16_t bytes_per_sector;
	uint32_t total_sectors;
	int mounted;
} fat12_fs_t;

void fat12_register(vfs_ops_t* ops);

#endif // !FAT12_H
