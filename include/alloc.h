#ifndef ALLOC_H
#define ALLOC_H

#include <stddef.h>
#include "mm.h"
#include "uart.h"

struct kmem_cache_entry {
    struct kmem_cache_entry *prev;
    struct kmem_cache_entry *next;
};
struct kmem_cache {
    int cache_size;   // The size of each chunk in this cache
    struct kmem_cache_entry *free_list;
};

void* simple_alloc(unsigned int size);
void kmem_freelist_push(struct kmem_cache_entry *entry, struct kmem_cache *cache);
void kmem_freelist_pop(struct kmem_cache *cache);
void print_kmem_freelist();
void kmem_cache_init();
void request_page(unsigned int order);
unsigned int get_chunk_order(unsigned int size);
void* kmalloc(unsigned int size);
void kfree(void *ptr);
void* alloc(unsigned int size);
void free(void *ptr);

void test_alloc();

#endif /* ALLOC_H */