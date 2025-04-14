#ifndef ALLOC_H
#define ALLOC_H

#include <stddef.h>
#include "mm.h"
#include "uart.h"

void* simple_alloc(unsigned int size);
void* alloc(unsigned int size);
void free(void *ptr);

void test_alloc();

#endif /* ALLOC_H */