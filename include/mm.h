#ifndef MM_H
#define MM_H

#include "uart.h"
#include "alloc.h"
#include "utils.h"

// The entry of the free list
struct Block {
    int idx;            // Index in the frame array
    struct Block *prev;
    struct Block *next;
};

struct PageInfo {
    int order;
    int allocated;
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