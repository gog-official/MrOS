#ifndef _Unistd_H
#define _Unistd_H

#include <stddef.h>
#include <sys/types.h>

#define NULL ((void*)0)

#ifdef __cplusplus
extern "C" {
#endif

ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);
int close(int fd);
int unlink(const char *path);

#ifdef __cplusplus
}
#endif

#endif