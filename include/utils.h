#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>

unsigned int hex_to_uint(char *hex_str, unsigned int len);
int atoi(char *str);
uint32_t be2le_u32(uint32_t be);
unsigned int align(unsigned int n, unsigned int alignment);

#endif /* UTILS_H */