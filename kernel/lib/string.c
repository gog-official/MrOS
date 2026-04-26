#include "string.h"

// ()()()()
// mem
// ()()()() cool
void* memset(void*dst, int c, size_t n) {
	uint8_t* p = (uint8_t*)dst;
	while (n--) *p++ = (uint8_t)c;
	return dst;
}

void* memcpy(void* dst, const void* src, size_t n) {
	uint8_t* d = (uint8_t*)dst;
	const uint8_t* s = (const uint8_t*)src;
	while (n--) *d++ = *s++;
	return dst;
}

void* memmove(void* dst, const void* src, size_t n) {
	uint8_t* d = (uint8_t*)dst;
	const uint8_t* s = (const uint8_t*)src;
	if (d < s) {
		while (n--) *d++ = *s++;
	} else {
		d += n; s += n;
		while (n--) *--d = *--s;
	}
	return dst;
}

int memcmp(const void* a, const void* b, size_t n) {
	const uint8_t* p = (const uint8_t*)a;
	const uint8_t* q = (const uint8_t*)b;
	while (n--) {
		if (*p != *q)
			return (int) *p - (int)*q;
		p++; q++;
	}
	return 0;
}

// ()()()()
// strings
// ()()()() cooler
int strlen(const char* s) {
	int n = 0; while (s[n]) n++; return n;
}

int strcmp(const char* a, const char* b) {
	while (*a && (*a == *b)) { a++; b++; }
	return (unsigned char)*a - (unsigned char)*b;
}

int strncmp(const char* a, const char* b, int n) {
	while (n-- && *a && (*a == *b)) { a++; b++; }
	if (n < 0)
		return 0;
	return (unsigned char)*a - (unsigned char)*b;
}

char* strcpy(char* dst, const char* src) {
	char* d = dst;
	while ((*d++ = *src++));
	return dst;
}

char* strncpy(char* dst, const char* src, int n) {
	char* d = dst;
	while (n-- && (*d++ = *src++));
	while (n-- > 0) *d++ = '\0';
	return dst;
}

char* strcat(char* dst, const char* src) {
	char* d = dst;
	while (*d) d++;
	while ((*d++ = *src++));
	return dst;
}

char* strchr(const char* s,  int c) {
	while (*s) {
		if (*s == (char)c) return (char*)s;
		s++;
	}
	return NULL;
}

char* strstr(const char* haystack, const char* needle) {
	if (!*needle) return (char*)haystack;
	while (*haystack) {
		const char* h = haystack;
		const char* n = needle;
		while (*h && *n && *h == *n) {
			h++; n++;
		}
		if  (!*n) return (char*)haystack;
	}
	return NULL;
}

//()()()()()()()()
// nummer conversion
//()()()()()()()() coolest

char* itoa(int value, char* buf, int base) {
	char digits[] = "0123456789ABCDEF";
	char tmp[32];
	int i = 0, j = 0, neg = 0;
	if (value == 0) { buf[0]='0'; buf[1]='\0'; return buf; }
	if (value < 0 && base == 10) { neg=1; value=-value; }
	while (value > 0) { tmp[i++] = digits[value%base]; value /= base; }
	if (neg) buf[j++] = '-';
	while (i > 0) buf[j++] = tmp[--i];
	buf[j] = '\0';
	return buf;
}

int atoi(const char* s) {
	int result = 0, neg = 0;
	while (*s == ' ') s++;
	if (*s == '-') { neg=1; s++; }
	else if (*s == '+') s++;
	while (*s >= '0' && *s <= '9') {
		result = result * 10 + (*s - '0');
		s++;
	}
	return neg ? -result : result;
}

int sprintf(char* buf, const char* fmt, ...) {
	// me just using gcc built-ins lol
	__builtin_va_list args;
	__builtin_va_start(args, fmt);

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
				int v = __builtin_va_arg(args, int);
				itoa(v, tmp, 10);
				char* t = tmp; while (*t) *out++ = *t++;
				break;
			}
			case 'u': {
				unsigned int v = __builtin_va_arg(args, unsigned int);
				// unsigned itoa
				if (v == 0) { *out++ = '0'; break; }
				char utmp[12]; int ui = 0;
				while (v > 0) { utmp[ui++] = '0' + (v%10); v/=10;}
				while (ui > 0) *out++ = utmp[--ui];
				break;
			}
			case 'x': {
				unsigned int v = __builtin_va_arg(args, unsigned int);
				itoa((int)v, tmp, 16);

				// lowercaseeeeee
				char* t = tmp;
				while (*t) { *out++ = (*t>='A'&&*t<='F') ? *t+32 : *t; t++; } // little compat
				break;							      			
			}
			case 'X': {
				unsigned int v = __builtin_va_arg(args, unsigned int);
				itoa((int)v, tmp, 16);
				char* t = tmp; while (*t) *out++ = *t++;
				break;
			}
			case 's': {
				const char* s = __builtin_va_arg(args, const char*);
				if (!s) s = "(null)";
				while (*s) *out++ = *s++;
			}
			case 'c': {
				char c = (char)__builtin_va_arg(args, int);
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
	__builtin_va_end(args);
	return (int)(out - buf);
}
