#ifndef MM_H
#define MM_H

#include "uart.h"
#include "alloc.h"
#include "utils.h"

#define MAX_ORDER       14
#define PAGE_SIZE       4096
// #define MEMORY_SIZE     0x3C000000  // Unit: byte
#define MEMORY_SIZE     0x10000000  // Unit: byte, TODO: for testing
#define PAGE_NUM        (MEMORY_SIZE / PAGE_SIZE)
#define MAX_BLOCK_SIZE  (1 << (MAX_ORDER - 1))  // Max number of pages in a block

// The entry of the free list
struct Block {
    int idx;            // Index in the frame array
    struct Block *prev;
    struct Block *next;
};

struct PageInfo {
    int order;  // Use for `mm`, represent the order of the page
    int cache_order;  // Use for `kmem`, represent the order of the cache. -1 if not in cache
    struct Block *entry_in_list; // Pointer to the entry in the free list
};

// Utility functions
int round(int size);
int get_order(int size);
int get_buddy(int idx, int order);

// Printing functions
void print_add_msg(int idx, int order);
void print_rm_msg(int idx, int order);
void print_alloc_page_msg(void* addr, int idx, int order);
void print_free_page_msg(void* addr, int idx, int curr_idx, int order);
void print_free_list();

// Memory management functions
void add_to_free_list(struct Block *entry, int order);
void mm_init();
void* _alloc(unsigned int size);
void _free(void *ptr);

#endif