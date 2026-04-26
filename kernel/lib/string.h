// minimal string and memory functions.
// i will implement every one doom actually uses
#ifndef STRING_H
#define STRING_H

#include <stdint.h>
#include <stddef.h>

// if cross-compiler doesnt provide stddef, we do it manually :)
#ifndef NULL
#define NULL ((void*)0)
#endif // !NULL

void* memset (void* dst, int c, size_t n);
void* memcpy (void* dst, const void* src, size_t n);
void* memmove(void* dst, const void* src, size_t n);
int memcmp (const void* a, const void* b, size_t n);

int strlen (const char* s);
int strcmp (const char* a, const char* b);
int strncmp(const char* a, const char* b, int n);
char* strcpy (char* dst, const char* src);
char* strncpy (char* dst, const char* src, int n);
char* strcat (char* dst, const char* src);
char* strchr(const char* s, int c);
char* strstr(const char* haystack, const char* needle);

// these belo will be used by dooooom
char* itoa(int val, char* buf, int base);
int atoi(const char* s);

// this one doesnt support floats, width/precision specifiers
int sprintf(char* buf, const char* fmt, ...); // supports %(d, i, u, x, X, s, c,%)

#endif // !STRING_H
