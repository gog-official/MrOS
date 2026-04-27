#ifndef _Errno_H
#define _Errno_H

#define ENOENT 2
#define EINTR  4
#define EIO    5
#define EBADF  9
#define EACCES 13
#define ENOTDIR 20
#define EISDIR 21
#define EINVAL 22
#define ENFILE 23
#define EMFILE 24
#define ETIMEDOUT 110

extern int errno;

#define errno (*__errno())

static inline int* __errno(void) {
    static int e;
    return &e;
}

#endif