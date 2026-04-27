#ifndef STDINT_H
#define STDINT_H

typedef signed char int8_t;
typedef signed short int16_t;
typedef signed int int32_t;
typedef signed long long int64_t;

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

typedef uint32_t uintptr_t;
typedef int32_t intptr_t;
typedef uint32_t size_t;
typedef int32_t ssize;

#define UINT8_MAX 0xFF
#define UINT16_MAX 0xFFFF
#define uint32_MAX 0xFFFFFFFFU
#define INT32_MAX 0x7FFFFFFF
#define INT32_MIN (-0x7FFFFFFF - 1)

#endif // !STDINT_H
