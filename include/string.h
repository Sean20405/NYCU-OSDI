#ifndef STRING_H
#define STRING_H

#include "alloc.h"
#include <stddef.h>

int strcmp(const char *s1, const char *s2);
char *strtok(char *str, char delim);
unsigned int strlen(const char *s);
char *strdup(const char *s);
void *memset(void *s, int c, unsigned int n);
void memcpy(void *dest, void *src, unsigned int n);

#endif /* STRING_H */