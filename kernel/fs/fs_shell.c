//filesystem shell command
#include "vfs.h"
#include "../core/vga.h"
#include "../lib/string.h"
#include <stdio.h>

//ls
void cmd_ls(int argc, char** argv) {
	const char* path = (argc >= 2) ? argv[1] : vfs_cwd;
	static vfs_node_t entries[VFS_MAX_DIR_ENTRIED];
	int count = vfs_readdir(path, entries, VFS_MAX_DIR_ENTRIED);

	if (count < 0) {
		vga_println("ls: cannot read directory", COLOR_RED);
		return;
	}

	vga_putchar('\n', COLOR_DEFAULT);
	for (int i = 0; i < count; i++) {
		if (entries[i].type == VFS_TYPE_DIR) {
			vga_print("  [DIR]  ", COLOR_CYAN);
			vga_println(entries[i].name, COLOR_CYAN);
		} else {
			vga_print("  [FILE] ", COLOR_DEFAULT);
			int namelen = strlen(entries[i].name);
			vga_print(entries[i].name, COLOR_DEFAULT);
			for (int p = namelen; p < 14; p++) vga_putchar(' ', COLOR_DEFAULT);

			char szbuf[12];
			itoa((int)entries[i].size, szbuf, 10);
			vga_print(szbuf, COLOR_GREY);
			vga_println(" bytes", COLOR_GREY);
		}
	}
	if (count == 0) vga_println("  (empty like you)", COLOR_GREY);
	vga_putchar('\n', COLOR_DEFAULT);
}

//meow
void cmd_cat(int argc, char** argv) {
	if (argc < 2) {
		vga_println("usage: cat <file>", COLOR_YELLOW);
		return;
	}

	vfs_file_t file;
	if (vfs_open(argv[1], &file) < 0) {
		vga_print("meow: :( do some planks and maybe they instantly appear? file not found: ", COLOR_RED);
		vga_println(argv[1], COLOR_DEFAULT);
		return;
	}

	static uint8_t buf[512];
	int n;
	vga_putchar('\n', COLOR_DEFAULT);
	while ((n = vfs_read(&file, buf, sizeof(buf)-1)) > 0) {
		buf[n] = '\0';
		vga_print((char*)buf, COLOR_DEFAULT);
	}

	vga_putchar('\n', COLOR_DEFAULT);
	vfs_close(&file);
}

//cd
void cmd_cd(int argc, char** argv) {
	if (argc < 2) {
		vga_println("usage: cd <path>", COLOR_YELLOW);
		return;
	}

	if (strcmp(argv[1], "/") == 0 || strcmp(argv[1], "..") == 0) {
		strcpy(vfs_cwd, "/");
	} else {
		static vfs_node_t entries[VFS_MAX_DIR_ENTRIED];
		int count = vfs_readdir("/", entries, VFS_MAX_DIR_ENTRIED);
		int found = 0;
		for (int i = 0; i < count; i++) {
			if(entries[i].type == VFS_TYPE_DIR && strcmp(entries[i].name, argv[1]) == 0) {
				found = 1; break;
			}
		}
		if  (!found) {
			vga_print("cd: maybe some planks may restore them? no such directory: ", COLOR_RED);
			vga_println(argv[1], COLOR_DEFAULT);
			return;
		}
		strcpy(vfs_cwd, "/");
		strcat(vfs_cwd, argv[1]);
	}
	vga_print("cwd: ", COLOR_GREY);
	vga_println(vfs_cwd, COLOR_DEFAULT);
}

//write
void cmd_write(int argc, char** argv) {
	if (argc < 3) {
		vga_println("usage: write <file> <text>", COLOR_YELLOW);
		vga_println("  eg: write GLUTES.md hello", COLOR_GREY);
		return;
	}

	vfs_create(argv[1]);
	vfs_file_t file;
	if (vfs_open(argv[1], &file) < 0) {
		vga_print("write: cannot open:", COLOR_RED);
		vga_println(argv[1], COLOR_DEFAULT);
		return;
	}

	static char content[256];
	int pos = 0;
	for (int i = 2; i < argc && pos < 250; i++) {
		int l = strlen(argv[i]);
		for (int j = 0; j < l && pos < 250; j++) {
			content[pos++] = argv[i][j];
		}
		if (i < argc -1) content [pos++] = ' ';
	}
	content[pos++] = '\n';
	content[pos] = '\0';

	int written = vfs_write(&file, (uint8_t*)content, (uint32_t)pos);
	vfs_close(&file);

	vga_print("Wrote ", COLOR_GREEN);
	char buf[12]; itoa(written, buf, 10);
	vga_print(buf, COLOR_YELLOW);
	vga_print(" bytes to ", COLOR_GREEN);
	vga_println(argv[1], COLOR_DEFAULT);
}

//mkdir
void cmd_mkdir(int argc, char** argv) {
	if (argc < 2) {
		vga_println("usage: mkdir <name>", COLOR_YELLOW);
		return;
	}
	if (vfs_mkdir(argv[1]) == 0) {
		vga_print("Created directory: ", COLOR_GREEN);
		vga_println(argv[1], COLOR_DEFAULT);
	} else {
		vga_print("mkdir: failed (exists or dir full, i guess doing some plank may do smth idk?): ", COLOR_RED);
		vga_println(argv[1], COLOR_DEFAULT);
	}
}

//rm
void cmd_rm(int argc, char** argv) {
	if (argc < 2) {
		vga_println("usage: rm <file>", COLOR_YELLOW);
		return;
	}
	if (vfs_remove(argv[1]) == 0) {
		vga_print("Removed: ", COLOR_GREEN);
		vga_println(argv[1], COLOR_DEFAULT);
	} else {
		vga_print("rm: failed (file not found or read-only, maybe do some planks?): ", COLOR_RED);
		vga_println(argv[1], COLOR_DEFAULT);
	}
}

//stats
void cmd_stat(int argc, char** argv) {
	if (argc < 2) {
		vga_println("usage: stat <file>", COLOR_YELLOW);
		return;
	}
	vfs_file_t file;
	if (vfs_open(argv[1], &file) < 0) {
		vga_print("stat: not found: ", COLOR_RED);
		vga_println(argv[1], COLOR_DEFAULT);
		return;
	}
	char buf[32];
	vga_putchar('\n', COLOR_DEFAULT);
	vga_print("  Name   : ", COLOR_GREY); vga_println(file.node.name, COLOR_DEFAULT);
	vga_print("  Type   : ", COLOR_GREY); 
	vga_println(file.node.type == VFS_TYPE_DIR ? "directory" : "file", COLOR_DEFAULT);
	vga_print("  Size   : ", COLOR_GREY);
	itoa((int)file.node.size, buf, 10);
	vga_print(buf, COLOR_YELLOW);  vga_println(" bytes:)", COLOR_GREY);
	vga_print("  Clustr : ", COLOR_GREY);
	itoa((int)file.node.first_cluster, buf, 10);
	vga_println(buf, COLOR_DEFAULT);
	vga_putchar('\n', COLOR_DEFAULT);
	vfs_close(&file);
}
