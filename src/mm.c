#include "mm.h"

struct PageInfo *free_list[MAX_ORDER];  // An array of double linked lists, where each index corresponds to a different order of blocks
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

int get_lsb(int x) {
    int lsb = 0;
    while (x > 1) {
        x >>= 1;
        lsb++;
    }
    return lsb;
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
        struct PageInfo *entry = free_list[i];
        int cnt = 0;
        uart_puts("Order ");
        uart_puts(itoa(i));
        uart_puts(": ");
        while (entry != NULL) {
            cnt++;
            uart_puts(itoa(entry->idx));
            uart_puts(" -> ");
            entry = entry->next;
        }
        uart_puts("NULL\t[");
        uart_puts(itoa(cnt));
        uart_puts("]\r\n");
    }
    uart_puts("===============================\r\n\r\n");
}

// Add the entry to the front of the free list for the given order
void add_to_free_list(struct PageInfo *entry, int order) {
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

    // print_add_msg(entry->idx, order);
}

void rm_from_free_list(struct PageInfo *entry, int order) {
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

    // print_rm_msg(entry->idx, order);
}

void mm_init() {
    // Initialize the free list
    for (int i = 0; i < MAX_ORDER; i++) {
        free_list[i] = NULL;
    }

    // Allocate the frame array
    memory_start = 0x00000000;  // TODO: For testing
    memset(page_list, 0, sizeof(page_list));

    // Initialize the free list and page list
    for (int i=0; i<PAGE_NUM; i+=MAX_BLOCK_SIZE) {
        struct PageInfo *entry = page_list + i;
        entry->idx = i;
        add_to_free_list(entry, MAX_ORDER - 1);  // Add to the free list with the maximum order
        
        page_list[i].order = MAX_ORDER - 1;
        // page_list[i].allocated = 0;  // Mark as free
        page_list[i].entry_in_list = entry;  // Link to the free list entry
    }

    for (int i=0; i<PAGE_NUM; i++) page_list[i].cache_order = -1;

    // print_free_list();
}

void* _alloc(unsigned int size) {
    if (size == 0 || size > MAX_ALLOC_SIZE) {
        uart_puts("The requested size is invalid!\n");
        return NULL;
    }

    size = round(size);  // Round up to the nearest page size

    // Calculate the order of the block
    int order = get_order(size);

    // Find a free block of the required size
    for (int i = order; i < MAX_ORDER; i++) {
        if (free_list[i] != NULL) {
            struct PageInfo *block = free_list[i];
            rm_from_free_list(block, i);  // Remove from the free list          

            // Found a block in higher order, split it into smaller blocks
            while (i > order) {
                i--;
                struct PageInfo *new_entry = block + (1 << i);
                new_entry->idx = block->idx + (1 << i);
                add_to_free_list(new_entry, i);

                page_list[new_entry->idx].order = i;
                page_list[new_entry->idx].entry_in_list = new_entry;  // Link to the free list entry
            }

            // Mark the block as allocated
            page_list[block->idx].order = order;
            // page_list[block->idx].allocated = 1;  // Mark as allocated
            page_list[block->idx].entry_in_list = NULL;  // Unlink from the free list
            
            void *addr = memory_start + block->idx * PAGE_SIZE;
            // print_alloc_page_msg(addr, block->idx, order);
            // print_free_list();
            return addr;
        }
    }
    return NULL;  // No suitable block found
}

void _free(void *ptr) {
    if (ptr == NULL) return;

    int original_idx = (ptr - memory_start) / PAGE_SIZE;

    // Check if the pointer is valid
    if (original_idx < 0 || original_idx >= PAGE_NUM || page_list[original_idx].entry_in_list != NULL) {
        return;
    }

    // Merge with the buddy block if it is free
    int order = page_list[original_idx].order;
    int curr_idx = original_idx, buddy_idx = get_buddy(original_idx, order);
    int smaller_idx = -1, bigger_idx = -1;
    int have_merged = 0;
    
    while (order < MAX_ORDER - 1) {
        struct PageInfo *buddy_entry = page_list[buddy_idx].entry_in_list;

        // uart_puts("[*] buddy idx: ");
        // if (buddy_entry != NULL) uart_puts(itoa(buddy_entry->idx));
        // else uart_puts("NULL");
        // uart_puts(" for page ");
        // uart_puts(itoa(curr_idx));
        // uart_puts(" with order ");
        // uart_puts(itoa(order));
        // uart_puts("\r\n");

        if (buddy_entry && page_list[buddy_idx].order == page_list[curr_idx].order) {  // Have buddy
            // uart_puts("[*] Buddy found! buddy idx: ");
            // uart_puts(itoa(buddy_entry->idx));
            // uart_puts(" for page ");
            // uart_puts(itoa(curr_idx));
            // uart_puts(" with order ");
            // uart_puts(itoa(order));
            // uart_puts("\r\n");

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
            // page_list[bigger_idx].allocated = 0;
            page_list[smaller_idx].order = order;
            page_list[bigger_idx].order = -1;
            // page_list[smaller_idx].entry_in_list = new_entry;
            page_list[bigger_idx].entry_in_list = NULL;

            curr_idx = smaller_idx;
            buddy_idx = get_buddy(curr_idx, order);

            // have_merged = 1;
        }
        else {
            struct PageInfo *new_entry = page_list + curr_idx;
            new_entry->idx = curr_idx;
            add_to_free_list(new_entry, order);

            page_list[curr_idx].entry_in_list = new_entry;  // Link to the free list entry
            page_list[curr_idx].order = order;  // Update the order
            // page_list[curr_idx].allocated = 0;  // Mark as free
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

    // print_free_page_msg(ptr, original_idx, curr_idx, order);
    // print_free_list();
}

void split(unsigned int idx, unsigned int order) {
    if (order >= MAX_ORDER) return;
    struct PageInfo *entry = page_list + idx;
    rm_from_free_list(entry, order);
    page_list[idx].entry_in_list = NULL;

    order--;
    entry->idx = idx;
    entry->order = order;
    add_to_free_list(entry, order);
    page_list[idx].entry_in_list = entry;

    int buddy_idx = get_buddy(idx, order);
    struct PageInfo *buddy_entry = page_list + buddy_idx;
    buddy_entry->idx = buddy_idx;
    buddy_entry->order = order;
    add_to_free_list(buddy_entry, order);
    page_list[buddy_idx].entry_in_list = buddy_entry;
}

void reserve(void *start, void *end) {
    end--;  // Exclude the end address
    unsigned int start_idx = (start - memory_start) / PAGE_SIZE;
    unsigned int end_idx = (end - memory_start) / PAGE_SIZE;

    // uart_puts("[x] Reserve memory from ");
    // uart_hex((unsigned long)start);
    // uart_puts(" (");
    // uart_puts(itoa(start_idx));
    // uart_puts(") to ");
    // uart_hex((unsigned long)end);
    // uart_puts(" (");
    // uart_puts(itoa(end_idx));
    // uart_puts(")\r\n");

    // Split the chunk
    for (int curr_order=MAX_ORDER-1; curr_order>0; curr_order--) {
        unsigned int start_block_idx = start_idx - (start_idx & (1 << curr_order) - 1);
        unsigned int end_block_idx = end_idx - (end_idx & (1 << curr_order) - 1);
        // uart_puts("start_block_idx: ");
        // uart_puts(itoa(start_block_idx));
        // uart_puts(" end_block_idx: ");
        // uart_puts(itoa(end_block_idx));
        // uart_puts(" curr_order: ");
        // uart_puts(itoa(curr_order));
        // uart_puts("\r\n");

        if(start_block_idx == end_block_idx) {  // The whole interval is in the same block
            // uart_puts("[*] The whole interval is in the same block: ");
            // uart_puts(itoa(start_block_idx));
            // uart_puts(" with order ");
            // uart_puts(itoa(curr_order));
            // uart_puts("\r\n");

            if (page_list[start_block_idx].order == curr_order) {
                split(start_block_idx, curr_order);
            }
        }
        else {
            // uart_puts("[*] The whole interval is in different blocks: ");
            // uart_puts(itoa(start_block_idx));
            // uart_puts(" with order ");
            // uart_puts(itoa(curr_order));
            // uart_puts(" and ");
            // uart_puts(itoa(end_block_idx));
            // uart_puts(" with order ");
            // uart_puts(itoa(curr_order));
            // uart_puts("\r\n");

            if (page_list[start_block_idx].order == curr_order) {
                split(start_block_idx, curr_order);
            }
            if (page_list[end_block_idx].order == curr_order) {
                split(end_block_idx, curr_order);
            }
            // split(start_block_idx, curr_order);
            // split(end_block_idx, curr_order);
        }
    }

    // Reserve the page
    for (int i=start_idx; i<=end_idx;) {
        struct PageInfo *entry = page_list + i;
        if (page_list[i].entry_in_list != NULL) {
            rm_from_free_list(entry, entry->order);
            page_list[i].entry_in_list = NULL;
        }
        // uart_puts(itoa(entry->order));
        // uart_puts("\r\n");
        i += (1 << entry->order);
    }

    // print_free_list();
}