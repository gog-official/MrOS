#ifndef _STRING_H
#define _STRING_H

#include <stddef.h>
#include <stdint.h>

void* memset (void* dst, int c, size_t n);
void* memcpy (void* dst, const void* src, size_t n);
void* memmove(void* dst, const void* src, size_t n);
int memcmp (const void* a, const void* b, size_t n);
int strlen (const char* s);
int strcmp (const char* a, const char* b);
int strncmp(const char* a, const char* b, int n);
char* strcpy(char* dst, const char* src);
char* strncpy(char* dst, const char* src, int n);
char* strcat (char* dst, const char* src);
char* strchr(const char* s, int c);
char* strstr(const char* haystack, const char* needle);

#include "../kernel/lib/malloc.h"
static inline char* strdup(const char* s) {
	int len = strlen(s) + 1;
	char* copy = (char*)kmalloc((size_t)len);
	if (copy) memcpy(copy, s, (size_t)len);
	return copy;
}

#endif
