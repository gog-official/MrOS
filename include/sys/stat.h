#ifndef _Sys_Stat_H
#define _Sys_Stat_H

#define S_IFMT  0170000
#define S_IFREG 0100000
#define S_IFDIR 0040000
#define S_IFIFO 0010000

struct stat {
    unsigned long st_dev;
    unsigned long st_ino;
    unsigned short st_mode;
    unsigned short st_nlink;
    unsigned long st_uid;
    unsigned long st_gid;
    int __pad0;
    unsigned long st_rdev;
    long st_size;
    long st_blksize;
    long st_blocks;
    long st_atime;
    long st_mtime;
    long st_ctime;
};

int stat(const char *path, struct stat *buf);
int fstat(int fd, struct stat *buf);

#endif