#include "alloc.h"

extern char *__bss_end;
extern char *__stack_top;
static char *heap_ptr = NULL;

void* simple_alloc(unsigned int size) {
    void *alloc = NULL;
    if (heap_ptr == NULL) {
        heap_ptr = (char*)&__bss_end;
    }
    if (heap_ptr + size > (char*)&__stack_top) {
        return NULL;
    }
    alloc = (void *)heap_ptr;
    heap_ptr += size;
    return alloc;
}