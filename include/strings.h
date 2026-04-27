#ifndef _STRINGS_H
#define _STRINGS_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline int strcasecmp(const char *s1, const char *s2) {
    unsigned char c1, c2;
    while (1) {
        c1 = (unsigned char) *s1++;
        c2 = (unsigned char) *s2++;
        if (c1 == '\0' || c2 == '\0')
            break;
        if (c1 >= 'A' && c1 <= 'Z')
            c1 += 'a' - 'A';
        if (c2 >= 'A' && c2 <= 'Z')
            c2 += 'a' - 'A';
        if (c1 != c2)
            break;
    }
    return c1 - c2;
}

static inline int strncasecmp(const char *s1, const char *s2, size_t n) {
    if (n == 0)
        return 0;
    unsigned char c1, c2;
    while (n-- != 0) {
        c1 = (unsigned char) *s1++;
        c2 = (unsigned char) *s2++;
        if (c1 == '\0' || c2 == '\0')
            break;
        if (c1 >= 'A' && c1 <= 'Z')
            c1 += 'a' - 'A';
        if (c2 >= 'A' && c2 <= 'Z')
            c2 += 'a' - 'A';
        if (c1 != c2)
            break;
    }
    return c1 - c2;
}

#ifdef __cplusplus
}
#endif

#endif /* _STRINGS_H */