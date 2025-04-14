#include "alloc.h"

#define MAX_CHUNK_SIZE 1024

extern char *__bss_begin;
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

void* alloc(unsigned int size) {
    if (size == 0) return NULL;

    void *alloc = NULL;
    if (size > MAX_CHUNK_SIZE) {
        alloc = _alloc(size);
        print_free_list();
    }
    else return NULL;  // To be implemented

    if (alloc == NULL) {
        uart_puts("Failed to allocate memory!\n");
        return NULL;
    }
    return alloc;
}

void free(void *ptr) {
    if (ptr == NULL) return;
    if (ptr >= (void*)&__bss_end && ptr < (void*)&__stack_top) return;  // Out of bounds
    
    _free(ptr);
    print_free_list();
    return;
}

void test_alloc() {
    uart_puts("Testing memory allocation...\n");
    char *ptr1 = (char *)alloc(4096);
    char *ptr2 = (char *)alloc(4096);
    char *ptr3 = (char *)alloc(4096);
    char *ptr4 = (char *)alloc(4096);

    free(ptr1);
    free(ptr3);
    free(ptr2);
    free(ptr4);
}