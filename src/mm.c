#include "mm.h"

#define MAX_ORDER       14
#define PAGE_SIZE       4096
// #define MEMORY_SIZE     0x3C000000  // Unit: byte
#define MEMORY_SIZE     0x10000000  // Unit: byte, TODO: for testing
#define PAGE_NUM        (MEMORY_SIZE / PAGE_SIZE)
#define MAX_BLOCK_SIZE  (1 << (MAX_ORDER - 1))  // Max number of pages in a block

struct Block *free_list[MAX_ORDER];  // An array of double linked lists, where each index corresponds to a different order of blocks
struct PageInfo page_list[PAGE_NUM]; // Array to store the status of each page

void *memory_start = NULL;

// Round up to the multiple of `PAGE_SIZE`
int round(int size) {
    if (size % PAGE_SIZE == 0) {
        return size;
    }
    return ((size / PAGE_SIZE) + 1) * PAGE_SIZE;
}

// Calculate the order of the block based on its size
int get_order(int size) {
    size /= PAGE_SIZE;  // Convert size to number of pages
    int order = 0;
    while ((1 << order) < size) {
        order++;
    }
    return order;
}

int get_buddy(int idx, int order) {
    return idx ^ (1 << order);
}

void print_add_msg(int idx, int order) {
    uart_puts("[+] Add page ");
    uart_puts(itoa(idx));
    uart_puts(" to order ");
    uart_puts(itoa(order));
    uart_puts(".\t\tRange of page: [");
    uart_puts(itoa(idx));
    uart_puts(", ");
    uart_puts(itoa(idx + (1 << order) - 1));
    uart_puts("]\r\n");
}

void print_rm_msg(int idx, int order) {
    uart_puts("[-] Remove page ");
    uart_puts(itoa(idx));
    uart_puts(" from order ");
    uart_puts(itoa(order));
    uart_puts(".\tRange of page: [");
    uart_puts(itoa(idx));
    uart_puts(", ");
    uart_puts(itoa(idx + (1 << order) - 1));
    uart_puts("]\r\n");
}

void print_alloc_page_msg(void* addr, int idx, int order) {
    uart_puts("[Page] Allocated page ");
    uart_puts(itoa(idx));
    uart_puts(" at address ");
    uart_hex((unsigned long)addr);
    uart_puts(" with order ");
    uart_puts(itoa(order));
    uart_puts("\r\n\r\n");
}

void print_free_page_msg(void* addr, int idx, int curr_idx, int order) {
    uart_puts("[Page] Freed page ");
    uart_puts(itoa(idx));
    uart_puts(" at address ");
    uart_hex((unsigned long)addr);
    uart_puts(". Add back to free list in order ");
    uart_puts(itoa(order));
    if (curr_idx != -1) {
        uart_puts(", page ");
        uart_puts(itoa(curr_idx));
    }
    uart_puts("\r\n\r\n");
}

void print_free_list() {
    uart_puts("========== Free List ==========\n");
    for (int i = 0; i < MAX_ORDER; i++) {
        struct Block *entry = free_list[i];
        uart_puts("Free list for order ");
        uart_puts(itoa(i));
        uart_puts(": ");
        while (entry != NULL) {
            uart_puts(itoa(entry->idx));
            uart_puts(" -> ");
            entry = entry->next;
        }
        uart_puts("NULL\n");
    }
    uart_puts("===============================\r\n\r\n");
}

// Add the entry to the front of the free list for the given order
void add_to_free_list(struct Block *entry, int order) {
    if (entry == NULL || order < 0 || order >= MAX_ORDER) return;
    if (free_list[order] == NULL) {
        free_list[order] = entry;
        entry->next = NULL;
        entry->prev = NULL;
    }
    else {
        free_list[order]->prev = entry;
        entry->prev = NULL;
        entry->next = free_list[order];
        free_list[order] = entry;
    }

    print_add_msg(entry->idx, order);
}

void rm_from_free_list(struct Block *entry, int order) {
    if (entry == NULL || order < 0 || order >= MAX_ORDER) return;
    if (free_list[order] == entry) {  // At the front
        free_list[order] = entry->next;
        if (entry->next != NULL) {
            entry->next->prev = NULL;
        }
    }
    else {  // In the middle
        if (entry->prev != NULL) entry->prev->next = entry->next;
        if (entry->next != NULL) entry->next->prev = entry->prev;
    }

    entry->next = NULL;
    entry->prev = NULL;
    // simple_free(buddy_entry);  // TODO: Maybe used a circular linked list to do simple_alloc and simple_free

    print_rm_msg(entry->idx, order);
}

void mm_init() {
    // Initialize the free list
    for (int i = 0; i < MAX_ORDER; i++) {
        free_list[i] = NULL;
    }

    // Allocate the frame array
    memory_start = 0x10000000;  // TODO: For testing
    memset(page_list, 0, sizeof(page_list));

    // Initialize the free list and page list
    uart_puts("PAGE_NUM: ");
    uart_puts(itoa(PAGE_NUM));
    uart_puts(" MAX_BLOCK_SIZE: ");
    uart_puts(itoa(MAX_BLOCK_SIZE));
    uart_puts("\r\n");

    for (int i=0; i<PAGE_NUM; i+=MAX_BLOCK_SIZE) {
        struct Block *entry = (struct Block*)simple_alloc(sizeof(struct Block));
        entry->idx = i;
        add_to_free_list(entry, MAX_ORDER - 1);  // Add to the free list with the maximum order
        
        page_list[i].order = MAX_ORDER - 1;
        page_list[i].allocated = 0;  // Mark as free
        page_list[i].entry_in_list = entry;  // Link to the free list entry
    }

    print_free_list();
}

void* _alloc(unsigned int size) {
    if (size == 0 || size > MEMORY_SIZE) {
        return NULL;
    }

    size = round(size);  // Round up to the nearest page size

    // Calculate the order of the block
    int order = get_order(size);

    // Find a free block of the required size
    for (int i = order; i < MAX_ORDER; i++) {
        if (free_list[i] != NULL) {
            struct Block *block = free_list[i];
            rm_from_free_list(block, i);  // Remove from the free list          

            // Found a block in higher order, split it into smaller blocks
            while (i > order) {
                i--;
                struct Block *new_entry = (struct Block*)simple_alloc(sizeof(struct Block));
                new_entry->idx = block->idx + (1 << i);
                add_to_free_list(new_entry, i);

                page_list[new_entry->idx].order = i;
                page_list[new_entry->idx].allocated = 0;  // Mark as free
                page_list[new_entry->idx].entry_in_list = new_entry;  // Link to the free list entry
            }

            // Mark the block as allocated
            page_list[block->idx].order = order;
            page_list[block->idx].allocated = 1;  // Mark as allocated
            page_list[block->idx].entry_in_list = NULL;  // Unlink from the free list
            
            void *addr = memory_start + block->idx * PAGE_SIZE;
            print_alloc_page_msg(addr, block->idx, order);
            return addr;
        }
    }

    return NULL;  // No suitable block found
}

void _free(void *ptr) {
    if (ptr == NULL) return;

    int original_idx = (ptr - memory_start) / PAGE_SIZE;

    // Check if the pointer is valid
    if (original_idx < 0 || original_idx >= PAGE_NUM || !page_list[original_idx].allocated) return;

    // Merge with the buddy block if it is free
    int order = page_list[original_idx].order;
    int curr_idx = original_idx, buddy_idx = get_buddy(original_idx, order);
    int smaller_idx = -1, bigger_idx = -1;
    int have_merged = 0;
    
    while (order < MAX_ORDER - 1) {
        struct Block *buddy_entry = page_list[buddy_idx].entry_in_list;

        // uart_puts("[*] buddy idx: ");
        // if (buddy_entry != NULL) uart_puts(itoa(buddy_entry->idx));
        // else uart_puts("NULL");
        // uart_puts(" for page ");
        // uart_puts(itoa(curr_idx));
        // uart_puts(" with order ");
        // uart_puts(itoa(order));
        // uart_puts("\r\n");

        if (buddy_entry && page_list[buddy_idx].order == page_list[curr_idx].order) {  // Have buddy
            uart_puts("[*] Buddy found! buddy idx: ");
            uart_puts(itoa(buddy_entry->idx));
            uart_puts(" for page ");
            uart_puts(itoa(curr_idx));
            uart_puts(" with order ");
            uart_puts(itoa(order));
            uart_puts("\r\n");

            // rm_from_free_list(page_list[curr_idx].entry_in_list, order);
            rm_from_free_list(buddy_entry, order);

            bigger_idx = curr_idx > buddy_idx ? curr_idx : buddy_idx;
            smaller_idx = curr_idx < buddy_idx ? curr_idx : buddy_idx;

            order++;

            // Add the merged block to the free list
            // struct Block *new_entry = (struct Block*)simple_alloc(sizeof(struct Block));
            // new_entry->idx = smaller_idx;
            // add_to_free_list(new_entry, order);

            // Update the page list
            // page_list[smaller_idx].allocated = 0;
            page_list[bigger_idx].allocated = 0;
            page_list[smaller_idx].order = order;
            page_list[bigger_idx].order = -1;
            // page_list[smaller_idx].entry_in_list = new_entry;
            page_list[bigger_idx].entry_in_list = NULL;

            curr_idx = smaller_idx;
            buddy_idx = get_buddy(curr_idx, order);

            // have_merged = 1;
        }
        else {
            struct Block *new_entry = (struct Block*)simple_alloc(sizeof(struct Block));
            new_entry->idx = curr_idx;
            add_to_free_list(new_entry, order);

            page_list[curr_idx].entry_in_list = new_entry;  // Link to the free list entry
            page_list[curr_idx].order = order;  // Update the order
            page_list[curr_idx].allocated = 0;  // Mark as free
            break;
        }
    }

    // if (!have_merged) {
    //     struct Block *new_entry = (struct Block*)simple_alloc(sizeof(struct Block));
    //     new_entry->idx = curr_idx;
    //     add_to_free_list(new_entry, order);

    //     page_list[curr_idx].entry_in_list = new_entry;  // Link to the free list entry
    //     page_list[curr_idx].order = order;  // Update the order
    //     page_list[curr_idx].allocated = 0;  // Mark as free
    // }

    print_free_page_msg(ptr, original_idx, curr_idx, order);
}