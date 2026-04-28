#include "vfs.h"
#include "fat12/fat12.h"
#include "../lib/string.h"
#include "../../include/stdio.h"

static vfs_ops_t ops;
char vfs_cwd[VFS_MAX_PATH] = "/";

// Resolve relative path to absolute path using vfs_cwd
void vfs_resolve_path(const char* path, char* resolved) {
	if (path[0] == '/') {
		strcpy(resolved, path);
		return;
	}
	if (strcmp(vfs_cwd, "/") == 0) {
		resolved[0] = '/';
		strcpy(resolved + 1, path);
	} else {
		strcpy(resolved, vfs_cwd);
		strcat(resolved, "/");
		strcat(resolved, path);
	}
}

int vfs_init(void) {
	fat12_register(&ops);
	return ops.mount();
}

int vfs_readdir(const char* path, vfs_node_t* entries, int max) {
	char resolved[VFS_MAX_PATH];
	vfs_resolve_path(path, resolved);
	return ops.readdir(resolved, entries, max);
}

int vfs_open(const char* path, vfs_file_t* file) {
	char resolved[VFS_MAX_PATH];
	vfs_resolve_path(path, resolved);
	return ops.open(resolved, file);
}
int vfs_read(vfs_file_t* file, uint8_t* buf, uint32_t len) {
	return ops.read(file, buf, len);
}
int vfs_write(vfs_file_t* file, const uint8_t* buf, uint32_t len) {
	return ops.write(file, buf, len);
}

int vfs_create(const char* path) {
	char resolved[VFS_MAX_PATH];
	vfs_resolve_path(path, resolved);
	return ops.create(resolved);
}

int vfs_remove(const char* path) {
	char resolved[VFS_MAX_PATH];
	vfs_resolve_path(path, resolved);
	return ops.remove(resolved);
}

int vfs_mkdir (const char* path) {
	char resolved[VFS_MAX_PATH];
	vfs_resolve_path(path, resolved);
	return ops.mkdir(resolved);
}

int vfs_close(vfs_file_t* file) {
	return ops.close(file);
}
