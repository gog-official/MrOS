#include "../core/vga.h"
#include "string.h"
#include "../../include/stdio.h"

// Forward declaration for itoa from string.c
extern char* itoa(int value, char* buf, int base);

int errno = 0;

int vprintf(const char* fmt, va_list args) {
	char buf[1024];
	int n = vsprintf(buf, fmt, args);
	vga_print(buf, COLOR_DEFAULT);
	return n;
}

int vfprintf(FILE* f, const char* fmt, va_list args) {
	(void)f;
	return vprintf(fmt, args);
}

int vsprintf(char* buf, const char* fmt, va_list args) {
	char* out = buf;
	char tmp[32];

	while (*fmt) {
		if (*fmt != '%') {
			*out++ = *fmt++;
			continue;
		}
		fmt++;
		switch (*fmt) {
			case 'd': case 'i': {
				int v = va_arg(args, int);
				itoa(v, tmp, 10);
				char* t = tmp; while (*t) *out++ = *t++;
				break;
			}
			case 'u': {
				unsigned int v = va_arg(args, unsigned int);
				// unsigned itoa
				if (v == 0) { *out++ = '0'; break; }
				char utmp[12]; int ui = 0;
				while (v > 0) { utmp[ui++] = '0' + (v%10); v/=10;}
				while (ui > 0) *out++ = utmp[--ui];
				break;
			}
			case 'x': {
				unsigned int v = va_arg(args, unsigned int);
				itoa((int)v, tmp, 16);

				// lowercaseeeeee
				char* t = tmp;
				while (*t) { *out++ = (*t>='A'&&*t<='F') ? *t+32 : *t; t++; } // little compat
				break;							      			
			}
			case 'X': {
				unsigned int v = va_arg(args, unsigned int);
				itoa((int)v, tmp, 16);
				char* t = tmp; while (*t) *out++ = *t++;
				break;
			}
			case 's': {
				const char* s = va_arg(args, const char*);
				if (!s) s = "(null)";
				while (*s) *out++ = *s++;
				break;
			}
			case 'c': {
				char c = (char)va_arg(args, int);
				*out++ = c;
				break;
			}
			case '%': {
				*out++ = '%';
				break;
			}
			default: {
				*out++ = '%';
				*out++ = *fmt;
				break;
			}
		}
		fmt++;
	}

	*out = '\0';
	return (int)(out - buf);
}

int vsnprintf(char* buf, size_t n, const char* fmt, va_list args) {
	char tmp[2048];
	int len = vsprintf(tmp, fmt, args);
	if ((size_t)len >= n) len = (int)n - 1;
	memcpy(buf, tmp, (size_t)len);
	buf[len] = '\0';
	return len;
}

int printf(const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	int n = vprintf(fmt, args);
	va_end(args);
	return n;
}

int fprintf(FILE* f, const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	int n = vfprintf(f, fmt, args);
	va_end(args);
	return n;
}

int snprintf(char* buf, size_t n, const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	int len = vsnprintf(buf, n, fmt, args);
	va_end(args);
	return len;
}

int puts(const char* s) {
	vga_print(s, COLOR_DEFAULT);
	vga_putchar('\n', COLOR_DEFAULT);
	return 0;
}
int fputs(const char* s, FILE* f) {
	(void)f;
	vga_print(s, COLOR_DEFAULT);
	return 0;
}
int fputc(int c, FILE* f) {
	(void)f;
	vga_putchar((char)c, COLOR_DEFAULT);
	return c;
}
int putchar(int c) {
	vga_putchar((char)c, COLOR_DEFAULT);
	return c;
}
int fflush(FILE* f) {
	(void)f;
	return 0; // nth to flush
}

size_t fwrite(const void* buf, size_t size, size_t nmemb, FILE* f) {
	(void)f;
	const char* p = (const char*)buf;
	size_t total = size * nmemb;
	for (size_t i = 0; i < total; i++) {
		vga_putchar(p[i], COLOR_DEFAULT);
	}
	return nmemb;
}

int sscanf(const char* str, const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);

	int matched = 0;
	const char* s = str;
	const char* f = fmt;

	while (*f && *s) {
		if (*f == '%') {
			f++;
			if (*f == 'd' || *f == 'i') {
				while (*s == ' ' || *s == '\t') s++;
				int neg = 0;
				if (*s == '-') {
					neg = 1; s++;
				}
				int val = 0;
				while (*s >= '0' && *s <= '9') {
					val = val * 10 + (*s - '0');
					s++;
				}
				*va_arg(args, int*) = neg ? -val : val;
				matched ++;
			} else if (*f == 's') {
				while (*s == ' ' || *s == '\t') s++;
				char* dst = va_arg(args, char*);
				while (*s && *s != ' ' && *s != '\t' && *s != '\n')
					*dst++ = *s++;
				*dst = '\0';
				matched ++;

			}
			f++;
		} else if (*f == *s) {
			f++; s++;
		} else {
			break;
		}
	}
	va_end(args);
	return matched;
}
