#include "string.h"

int strcmp (const char *s1, const char *s2) {
    while (*s1 && *s2 && *s1 == *s2) {
        s1++;
        s2++;
    }
    return *s1 - *s2;
}


void *memset(void *s, int c, unsigned int n) {
    unsigned char *p = s;
    while (n--) {
        *p++ = (unsigned char)c;
    }
    return s;
}

void memcpy(void *dest, void *src, unsigned int n) {
    unsigned char *d = dest;
    unsigned char *s = src;
    while (n--) {
        *d++ = *s++;
    }
}