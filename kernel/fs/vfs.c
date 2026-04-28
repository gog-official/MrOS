#include "vfs.h"
#include "fat12/fat12.h"
#include "../lib/string.h"
#include "fat12/fat12.h"

static vfs_ops_t ops;
char vfs_cwd[VFS_MAX_PATH] = "/";

int vfs_init(void) {
	fat12_register(&ops);
	return ops.mount();
}

int vfs_readdir(const char* path, vfs_node_t* entries, int max) {
	return ops.readdir(path, entries, max);
}

int vfs_open(const char* path, vfs_file_t* file) {
	return ops.open(path, file);
}
int vfs_read(vfs_file_t* file, uint8_t* buf, uint32_t len) {
	return ops.read(file, buf, len);
}
int vfs_write(vfs_file_t* file, const uint8_t* buf, uint32_t len) {
	return ops.write(file, buf, len);
}

int vfs_create(const char* path) {
	return ops.create(path);
}

int vfs_remove(const char* path) {
	return ops.remove(path);
}

int vfs_mkdir (const char* path) {
	return ops.mkdir(path);
}

int vfs_close(vfs_file_t* file) {
	return ops.close(file);
}
