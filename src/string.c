#include "string.h"

int strcmp (const char *s1, const char *s2) {
    while (*s1 && *s2 && *s1 == *s2) {
        s1++;
        s2++;
    }
    return *s1 - *s2;
}

char *strtok(char *str, char delim) {
    /**
     * strtok() is a function that splits a string into tokens based on a delimiter.
     * It maintains a static pointer to the next token in the string.
     *
     * @param str The string to split. If NULL, the function will continue splitting the previous string.
     * @param delim The delimiter to split the string by.
     * @return The next token in the string.
    */
    static char *p = NULL;
    if (str != NULL) p = str;
    if (p == NULL || *p == '\0') return NULL;

    char *ret = p;
    while (*p != delim && *p != '\0') p++;
    if (*p == delim) {
        *p = '\0';
        p++;
    }
    return ret;
}

unsigned int strlen(const char *s) {
    unsigned int len = 0;
    while (*s++) len++;
    return len;
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