#include "fat12.h"
#include "../vfs.h"
#include "../../drivers/ata/ata.h"
#include "../../lib/string.h"
#include <stdint.h>
#include <stdio.h>

//global state
static fat12_fs_t fs;
static fat12_bpb_t bpb;

#define FAT_CACHE_SIZE (18 * 512)
static uint8_t fat_cache[FAT_CACHE_SIZE];
static int fat_dirty = 0;

// internal Helpers

static int fat12_load_fat(void) {
	uint32_t fat_bytes = bpb.sectors_per_fat * bpb.bytes_per_sector;
	if (fat_bytes > FAT_CACHE_SIZE) return -1;
	return ata_read_byte_range(
			fs.fat_start_sector * bpb.bytes_per_sector,
			fat_bytes, fat_cache);
}

static int fat12_flush_fat(void) {
	if (!fat_dirty) return 0;
	uint32_t fat_bytes = bpb.sectors_per_fat * bpb.bytes_per_sector;

	ata_write_byte_range(
		fs.fat_start_sector * bpb.bytes_per_sector,
		fat_bytes, fat_cache);

	if (bpb.num_fats > 1) {
		ata_write_byte_range(
			(fs.fat_start_sector + bpb.sectors_per_fat) * bpb.bytes_per_sector, fat_bytes, fat_cache);
	}
	fat_dirty = 0;
	return 0;
}

static uint16_t fat12_get_entry(uint16_t cluster) {
	uint32_t offset = cluster + (cluster / 2);
	uint16_t val = *(uint16_t*)(fat_cache + offset);
	if (cluster & 1)
		return val >> 4;
	else
	 	return val & 0x0FFF;
}

static void fat12_set_entry(uint16_t cluster, uint16_t value) {
	uint32_t offset = cluster + (cluster / 2);
	if (cluster & 1) {
		fat_cache[offset] = (fat_cache[offset] & 0x0F) | ((value & 0x0F) << 4);
		fat_cache[offset + 1] = (value >> 4) & 0xFF;
	} else {
		fat_cache[offset] = value & 0xFF;
		fat_cache[offset + 1] = (fat_cache[offset + 1] & 0xF0) | ((value >> 8) & 0x0F);
	}
	fat_dirty = 1;
}

static uint16_t fat12_find_free_cluster(void) {
	uint32_t total_clusters = (fs.total_sectors - fs.data_start_sector) / bpb.sectors_per_cluster;
	for (uint16_t c = 2; c < (uint16_t)total_clusters+2; c++) {
		if (fat12_get_entry(c) ==  0x000) return c;
	}
	return 0;
}

static uint16_t cluster_to_lba(uint16_t cluster) {
	return fs.data_start_sector + (uint32_t)(cluster - 2) * bpb.sectors_per_cluster;
}

static void fat12_83_to_str(const uint8_t* name8, const uint8_t* ext3, char* out) {
	int i = 0, j = 0;
	for (i = 7; i >= 0 && name8[i] == ' '; i--);
	for (int k = 0; k <= i; k++) out[j++] = (char)name8[k];
	if (ext3[0] != ' ') {
		out[j++] = '.';
		for (int k = 0; k < 3 && ext3[k] != ' '; k++) out[j++] = (char)ext3[k];
	}
	out[j] = '\0';
}

static void fat12_str_to_83(const char* str, uint8_t* name8, uint8_t* ext3) {
	memset(name8, ' ', 8);
	memset(ext3, ' ', 3);
	int i = 0, j = 0;
	while (str[j] && str[j] != '.' && i < 8) {
		char c = str[j++];
		if (c >= 'a' && c <= 'z') c -= 32;
		name8[i++] = (uint8_t)c;
	}
	if (str[j] == '.') {
		j++;
		i = 0;
		while (str[j] && i < 3) {
			char c = str[j++];
			if (c >= 'a' && c <= 'z') c -= 32;
			ext3[i++] = (uint8_t)c;
		}
	}
}

static int fat12_read_root_dir(fat12_dirent_t* buf, int max) {
	uint32_t root_bytes = bpb.root_entry_count * sizeof(fat12_dirent_t);
	uint8_t* raw = (uint8_t*)buf;
	ata_read_byte_range(fs.root_start_sector * bpb.bytes_per_sector, root_bytes, raw);
	int count = 0;
	for (int i = 0; i < bpb.root_entry_count && count < max; i++) {
		fat12_dirent_t* e = &buf[i];
		if (e->name[0] == 0x00) break;
		if (e->name[0] == 0xE5) continue;
		if (e->attr == FAT_ATTR_LFN) continue;
		if (e->attr & FAT_ATTR_VOLUMEID) continue;
		buf[count++] = *e;
	}
	return count;
}

static void fat12_write_root_dir(fat12_dirent_t* buf) {
	uint32_t root_bytes = bpb.root_entry_count * sizeof(fat12_dirent_t);
	ata_write_byte_range(fs.root_start_sector * bpb.bytes_per_sector, root_bytes, (uint8_t*)buf);
}

static int fat12_find_entry(const char* name, fat12_dirent_t* out) {
	static fat12_dirent_t dir[VFS_MAX_DIR_ENTRIED];
	int count = fat12_read_root_dir(dir, VFS_MAX_DIR_ENTRIED);

	uint8_t name8[8], ext3[3];
	fat12_str_to_83(name, name8, ext3);

	for (int i = 0; i < count; i++) {
		if (memcmp(dir[i].name, name8, 8) == 0 && memcmp(dir[i].ext, ext3, 3) == 0) {
			if (out) *out = dir[i];
			return i;
		}
	}
	return -1;
}

// VFS OPS IMPLEMENTATION!!!!!!
static int fat12_mount(void) {
	uint8_t boot[512];
	if (ata_read_sectors(0, 1, boot) != ATA_OK) return -1;
	memcpy(&bpb, boot, sizeof(fat12_bpb_t));

	if (bpb.bytes_per_sector != 512) return -1;
	
	fs.fat_start_sector = bpb.reserved_sectors;
	fs.root_start_sector = fs.fat_start_sector + (uint32_t)bpb.num_fats * bpb.sectors_per_fat;
	fs.root_dir_sectors = (bpb.root_entry_count * 32 + 511) / 512;
	fs.data_start_sector = fs.root_start_sector + fs.root_dir_sectors;
	fs.sectors_per_cluster = bpb.sectors_per_cluster;
	fs.bytes_per_sector =  bpb.bytes_per_sector;
	fs.total_sectors = bpb.total_sectors_16 ? bpb.total_sectors_16 : bpb.total_sectors_32;
	fs.mounted = 1;

	if (fat12_load_fat() != ATA_OK) return - 1;
	return 0;
}

static int fat12_readdir(const char* path, vfs_node_t* entries, int max) {
	(void)path;
	static fat12_dirent_t dir[VFS_MAX_DIR_ENTRIED];
	int count = fat12_read_root_dir(dir, VFS_MAX_DIR_ENTRIED);

	int n = 0;
	for (int i = 0; i < count && n < max; i++) {
		fat12_83_to_str(dir[i].name, dir[i].ext, entries[n].name);
		entries[n].size = dir[i].file_size;
		entries[n].first_cluster = dir[i].first_cluster;
		entries[n].attr = dir[i].attr;
		entries[n].type = (dir[i].attr & FAT_ATTR_DIRECTORY) ? VFS_TYPE_DIR : VFS_TYPE_FILE;
		n++;
	}
	return n;
}

static int fat12_open(const char* path, vfs_file_t* file) {
	fat12_dirent_t dirent;
	if (fat12_find_entry(path, &dirent) < 0) return -1;

	fat12_83_to_str(dirent.name, dirent.ext, file->node.name);
	file->node.size = dirent.file_size;
	file->node.first_cluster = dirent.first_cluster;
	file->node.attr = dirent.attr;
	file->node.type = (dirent.attr & FAT_ATTR_DIRECTORY) ? VFS_TYPE_DIR : VFS_TYPE_FILE;
	file->position = 0;
	file->valid = 1;
	return 0;
}

static int fat12_read(vfs_file_t* file, uint8_t* buf, uint32_t len) {
	if (!file->valid) return -1;
	if (file->position >= file->node.size) return 0;

	uint32_t remaining = file->node.size - file->position;
	if (len > remaining) len = remaining;

	uint32_t bytes_read = 0;
	uint32_t cluster_sz = bpb.sectors_per_cluster * bpb.bytes_per_sector;
	uint32_t pos = file->position;

	uint16_t cluster = (uint16_t)file->node.first_cluster;
	uint32_t cluster_idx = pos / cluster_sz;
	uint32_t cluster_off = pos % cluster_sz;

	for (uint32_t i = 0; i < cluster_idx; i++) {
		cluster = fat12_get_entry(cluster);
		if (cluster >= FAT12_EOC) return (int)bytes_read;
	}
	static uint8_t cluster_buf[16 * 512];
	while (bytes_read < len && cluster < FAT12_EOC) {
		uint32_t lba = cluster_to_lba(cluster);
		ata_read_sectors((uint8_t)lba, bpb.sectors_per_cluster, cluster_buf);

		uint32_t to_copy = cluster_sz - cluster_off;
		if (to_copy > len - bytes_read) to_copy = len - bytes_read;

		memcpy(buf + bytes_read, cluster_buf + cluster_off, to_copy);
		bytes_read += to_copy;
		cluster_off = 0;
		cluster = fat12_get_entry(cluster);
	}

	file->position += bytes_read;
	return (int)bytes_read;
}

static int fat12_write(vfs_file_t* file, const uint8_t* buf, uint32_t len) {
	if (!file->valid || len == 0) return -1;
	uint32_t cluster_sz = bpb.sectors_per_cluster * bpb.bytes_per_sector;
	uint32_t bytes_written = 0;
	static uint8_t cluster_buf[16 * 512];

	uint16_t cluster = (uint16_t)file->node.first_cluster;
	uint32_t pos = file->position;

	uint32_t cluster_idx = pos / cluster_sz;
	uint32_t cluster_off = pos % cluster_sz;
	uint16_t prev_cluster = 0;

	for (uint32_t i = 0; i < cluster_idx; i++) {
		uint16_t next = fat12_get_entry(cluster);
		if (next >= FAT12_EOC) {
			uint16_t new_c = fat12_find_free_cluster();
			if (!new_c) return -1;
			fat12_set_entry(cluster, new_c);
			fat12_set_entry(new_c, 0xFFF);
			cluster = new_c;
			break;
		}
		prev_cluster = cluster;
		cluster = next;
	}
	while (bytes_written < len) {
		if (cluster == 0 || cluster >= FAT12_EOC) {
			uint16_t new_c = fat12_find_free_cluster();
			if (!new_c) break;
			fat12_set_entry(new_c, 0xFFF);
			if (prev_cluster) fat12_set_entry(prev_cluster, new_c);
			else file->node.first_cluster = new_c;
			cluster = new_c;
		}

		uint32_t lba = cluster_to_lba(cluster);

		ata_read_sectors((uint8_t)lba, bpb.sectors_per_cluster, cluster_buf);

		uint32_t to_copy = cluster_sz - cluster_off;
		if (to_copy > len - bytes_written) to_copy = len - bytes_written;
		memcpy(cluster_buf + cluster_off, buf + bytes_written, to_copy);
		ata_write_sectors((uint8_t)lba, bpb.sectors_per_cluster, cluster_buf);

		bytes_written += to_copy;
		cluster_off = 0;
		prev_cluster = cluster;
		cluster = fat12_get_entry(cluster);
	}

	file->position += bytes_written;
	if (file->position > file->node.size)
		file->node.size = file->position;

	static fat12_dirent_t dir[VFS_MAX_DIR_ENTRIED];
	int idx = fat12_find_entry(file->node.name, NULL);
	if (idx >= 0) {
		fat12_read_root_dir(dir, VFS_MAX_DIR_ENTRIED);
		dir[idx].file_size = file->node.size;
		dir[idx].first_cluster = (uint16_t)file->node.first_cluster;
		fat12_write_root_dir(dir);
	}

	fat12_flush_fat();
	return (int)bytes_written;
}

static int fat12_create(const char* path) {
	if (fat12_find_entry(path, NULL) >= 0) return -1;
	static fat12_dirent_t dir[VFS_MAX_DIR_ENTRIED];
	ata_read_byte_range(fs.root_start_sector * bpb. bytes_per_sector, bpb.root_entry_count * 32, (uint8_t*)dir);
	int slot = -1;
	for (int i = 0; i < bpb.root_entry_count; i++) {
		if (dir[i].name[0] == 0x00 || dir[i].name[0] == 0xE5) {
			slot = i; break;
		}
	}
	if (slot < 0) return -1;

	memset(&dir[slot], 0, sizeof(fat12_dirent_t));
	fat12_str_to_83(path, dir[slot].name, dir[slot].ext);
	dir[slot].attr = FAT_ATTR_ARCHIVE;
	dir[slot].file_size = 0;
	dir[slot].first_cluster = 0;

	fat12_write_root_dir(dir);
	return 0;
}

static int fat12_remove(const char* path) {
	static fat12_dirent_t dir[VFS_MAX_DIR_ENTRIED];
	ata_read_byte_range(fs.root_start_sector * bpb.bytes_per_sector, bpb.root_entry_count * 32, (uint8_t*)dir);
	uint8_t name8[8], ext3[3];
	fat12_str_to_83(path, name8, ext3);

	for (int i = 0; i < bpb.root_entry_count; i++) {
		if (dir[i].name[0] == 0x00) break;
		if (dir[i].name[0] == 0xE5) continue;
		if (memcmp(dir[i].name, name8, 8) == 0 && memcmp(dir[i].ext, ext3, 3) == 0) {
			uint16_t c = dir[i].first_cluster;
			while (c >= 2 && c < FAT12_EOC) {
				uint16_t next = fat12_get_entry(c);
				fat12_set_entry(c, 0x000);
				c = next;
			}
			dir[i].name[0] = 0xE5;
			fat12_write_root_dir(dir);
			fat12_flush_fat();
			return 0;
		}
	}
	return -1;
}

static int fat12_mkdir(const char* path) {
	if (fat12_find_entry(path, NULL) >= 0) return -1;

	static fat12_dirent_t dir[VFS_MAX_DIR_ENTRIED];
	ata_read_byte_range(fs.root_start_sector * bpb.bytes_per_sector, bpb.root_entry_count * 32, (uint8_t*)dir);
	int slot = -1;
	for (int i = 0; i < bpb.root_entry_count; i++) {
		if (dir[i].name[0] == 0x00 || dir[i].name[0] == 0xE5) {
			slot = i; break;
		}
	}
	if (slot < 0) return -1;
	memset(&dir[slot], 0, sizeof(fat12_dirent_t));
	fat12_str_to_83(path, dir[slot].name, dir[slot].ext);
	dir[slot].attr = FAT_ATTR_DIRECTORY;
	fat12_write_root_dir(dir);
	return 0;
}

static int fat12_close(vfs_file_t* file) {
	file->valid = 0;
	fat12_flush_fat();
	return 0;
}

//registerrrrrrrrrr
void fat12_register(vfs_ops_t* ops) {
	ops->mount = fat12_mount;
	ops->readdir = fat12_readdir;
	ops->open = fat12_open;
	ops->read = fat12_read;
	ops->write = fat12_write;
	ops->create = fat12_create;
	ops->remove = fat12_remove;
	ops->mkdir = fat12_mkdir;
	ops->close = fat12_close;
}
