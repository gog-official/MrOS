#ifndef _STDIO_H
#define _STDIO_H

#include <stddef.h>
#include <stdarg.h>
#include "../kernel/lib/string.h"

typedef void FILE;

#define stdout ((FILE*)1)
#define stderr ((FILE*)2)
#define EOF (-1)

FILE* fopen (const char* path, const char* mode);
int fclose (FILE* f);
size_t fread (void* buf, size_t size, size_t nmemb, FILE* f);
size_t fwrite (const void* buf, size_t size, size_t nmemb, FILE* f);
int fseek (FILE* f, long offset, int whence);
long ftell (FILE* f);
int feof (FILE* f);
int ferror (FILE* f);
int fflush (FILE* f);

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

int sprintf (char* buf, const char* fmt, ...);
int snprintf(char* buf, size_t n, const char* fmt, ...);
int vsprintf (char* buf, const char* fmt, va_list args);
int vnsprintf(char* buf, size_t n, const char* fmt, va_list args);

int printf(const char* fmt, ...);
int fprintf(FILE* f, const char* fmt, ...);
int vprintf (const char* fmt, va_list args);
int vfprintf(FILE* f, const char* fmt, va_list args);

int puts (const char* s);
int fputs (const char* s, FILE* f);
int fputc(int c, FILE* f);
int putchar(int c);

int sscanf(const char* str, const char* fmt, ...);

#endif // !_STDIO_H
