// virtual filesystem switch
#ifndef VFS_H
#define VFS_H

#include <stdint.h>
#include <stddef.h>

// limits
#define VFS_MAX_NAME 12
#define VFS_MAX_PATH 64
#define VFS_MAX_OPEN 8
#define VFS_MAX_DIR_ENTRIED 224

// node types
#define VFS_TYPE_FILE 1
#define VFS_TYPE_DIR 2

typedef struct {
	char name[VFS_MAX_NAME];
	uint8_t type;
	uint32_t size;
	uint32_t first_cluster;
	uint8_t attr;
} vfs_node_t;

typedef struct {
	int valid;
	vfs_node_t node;
	uint32_t position;
} vfs_file_t;

typedef struct {
	int (*mount) (void);
	int (*readdir)(const char* path, vfs_node_t* entries, int max);
	int (*open) (const char* path, vfs_file_t* file);
	int (*read) (vfs_file_t* file, uint8_t* buf, uint32_t len);
	int (*write) (vfs_file_t* file, const uint8_t* buf, uint32_t len);
	int (*create) (const char* path);
	int (*remove) (const char* path);
	int (*mkdir) (const char* path);
	int (*close) (vfs_file_t* file);
} vfs_ops_t;

// vfs public api
int vfs_init(void);
int vfs_readdir(const char* path, vfs_node_t* entries, int max);
int vfs_open(const char* path, vfs_file_t* file);
int vfs_read(vfs_file_t* file, uint8_t* buf, uint32_t len);
int vfs_write(vfs_file_t* file, const uint8_t* buf, uint32_t len);
int vfs_create(const char* path);
int vfs_remove(const char* path);
int vfs_mkdir(const char* path);
int vfs_close(vfs_file_t* file);

extern char vfs_cwd[VFS_MAX_PATH];

#endif
