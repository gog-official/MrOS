#ifndef _STDLIB_H
#define _STDLIB_H

#include <stddef.h>
#include "../kernel/lib/malloc.h"
#include "../kernel/lib/string.h"

static inline void* malloc(size_t size) {
	return kmalloc(size);
}

static inline void free(void* ptr) {
	kfree(ptr);
}

static inline void* realloc(void* ptr, size_t size) {
	return krealloc(ptr, size);
}

static inline void* calloc(size_t n, size_t size) {
	void* p = kmalloc(n * size);
	if (p) memset(p, 0, n * size);
	return p;
}

int atoi(const char* s);
char* itoa(int value, char* buf, int base);

static inline int abs(int x) {
	return x < 0 ? -x : x;
}

static inline void exit(int code) {
	(void)code;
	__asm__ volatile ("cli; hlt");
	for(;;);
}

static inline void abort(void) {
	exit(1);
}

#endif // !_STDLIB_H
