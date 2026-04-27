#ifndef _INTTYPES_H
#define _INTTYPES_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Macros for printf format specifiers */
#define PRId8     "d"
#define PRIi8     "i"
#define PRIo8     "o"
#define PRIu8     "u"
#define PRIx8     "x"
#define PRIX8     "X"

#define PRId16    "d"
#define PRIi16    "i"
#define PRIo16    "o"
#define PRIu16    "u"
#define PRIx16    "x"
#define PRIX16    "X"

#define PRId32    "d"
#define PRIi32    "i"
#define PRIo32    "o"
#define PRIu32    "u"
#define PRIx32    "x"
#define PRIX32    "X"

#define PRId64    "lld"
#define PRIi64    "lli"
#define PRIo64    "llo"
#define PRIu64    "llu"
#define PRIx64    "llx"
#define PRIX64    "llX"

/* Macros for scanf format specifiers */
#define SCNd8     "hhd"
#define SCNi8     "hhi"
#define SCNo8     "hho"
#define SCNu8     "hhu"
#define SCNx8     "hhx"

#define SCNd16    "hd"
#define SCNi16    "hi"
#define SCNo16    "ho"
#define SCNu16    "hu"
#define SCNx16    "hx"

#define SCNd32    "d"
#define SCNi32    "i"
#define SCNo32    "o"
#define SCNu32    "u"
#define SCNx32    "x"

#define SCNd64    "lld"
#define SCNi64    "lli"
#define SCNo64    "llo"
#define SCNu64    "llu"
#define SCNx64    "llx"

#ifdef __cplusplus
}
#endif

#endif /* _INTTYPES_H */