#include "alloc.h"

#define MAX_CHUNK_SIZE  128
#define MIN_CHUNK_SIZE  16
#define MIN_CACHE_ORDER 4
#define MAX_CACHE_ORDER 7
#define CACHE_NUM       4

extern char *__bss_begin;
extern char *__bss_end;
extern char *__stack_top;
static char *heap_ptr = NULL;

extern struct PageInfo page_list[PAGE_NUM];
extern void *memory_start;

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

/*** Dynamic Memory Allocator (kmalloc) ***/
struct kmem_cache kmem_caches[CACHE_NUM];

void kmem_freelist_push(struct kmem_cache_entry *entry, struct kmem_cache *cache) {
    entry->next = cache->free_list;
    if (cache->free_list != NULL) {
        cache->free_list->prev = entry;
    }
    cache->free_list = entry;
    entry->prev = NULL;
}

void kmem_freelist_pop(struct kmem_cache *cache) {
    struct kmem_cache_entry *tmp = cache->free_list;
    cache->free_list = cache->free_list->next;
    if (cache->free_list != NULL) {
        cache->free_list->prev = NULL;
    }
    tmp->next = NULL;
    tmp->prev = NULL;
}

void print_kmem_freelist() {
    uart_puts("========== kmem Free List ==========\r\n");
    for (int i=0; i<CACHE_NUM; i++) {
        struct kmem_cache_entry *entry = kmem_caches[i].free_list;
        uart_puts("Cache size: ");
        uart_puts(itoa(kmem_caches[i].cache_size));
        uart_puts("\r\n");
        while (entry != NULL) {
            uart_hex((unsigned long)entry);
            uart_puts(" -> ");
            entry = entry->next;
        }
        uart_puts("NULL\r\n");
    }
    uart_puts("====================================\r\n");
}

void kmem_cache_init() {
    for (int i=0; i<CACHE_NUM; i++) {
        kmem_caches[i].cache_size = (1 << (i + 4)); // 16, 32, 64, 128
        kmem_caches[i].free_list = NULL;
    }

    for (int i=0; i<CACHE_NUM; i++) {
        void *page = _alloc(PAGE_SIZE);
        if (page == NULL) {
            uart_puts("Failed to allocate memory for cache!\n");
            return;
        }

        int page_idx = (page - memory_start) / PAGE_SIZE;
        page_list[page_idx].cache_order = i + MIN_CACHE_ORDER;

        int chunk_size = kmem_caches[i].cache_size;
        int chunk_num = PAGE_SIZE / (chunk_size + sizeof(struct kmem_cache_entry));
        kmem_caches[i].free_list = NULL;

        // Split chunks and add to the free list
        for (int j = 0; j < chunk_num; j++) {
            struct kmem_cache_entry *new_entry = (struct kmem_cache_entry*)page;
            kmem_freelist_push(new_entry, &kmem_caches[i]);
            page += chunk_size + sizeof(struct kmem_cache_entry);
        }
    }

    print_free_list();
}

void request_page(unsigned int order) {
    void *page = _alloc(PAGE_SIZE);
    if (page == NULL) {
        uart_puts("Failed to allocate memory for cache!\n");
        return;
    }

    int page_idx = (page - memory_start) / PAGE_SIZE;
    page_list[page_idx].cache_order = order + MIN_CACHE_ORDER;

    int chunk_size = kmem_caches[order].cache_size;
    int chunk_num = PAGE_SIZE / (chunk_size + sizeof(struct kmem_cache_entry));

    // Split chunks and add to the free list
    for (int j = 1; j < chunk_num; j++) {
        struct kmem_cache_entry *new_entry = (struct kmem_cache_entry*)page;
        kmem_freelist_push(new_entry, &kmem_caches[order]);
        page += chunk_size + sizeof(struct kmem_cache_entry);
    }
}

// Start from 4 (2^4 = 16) and go up to 7 (2^7 = 128)
unsigned int get_chunk_order(unsigned int size) {
    unsigned int order = 4;
    while ((1 << order) < size) {
        order++;
    }
    return order;
}

/**
 * kmalloc - Allocates memory dynamically
 * 
 * This function handle the small size request for memory. It will request
 * a page using `_alloc` and split it into some common size chunks. If there
 * is no free chunk, request a new page.
 * 
 * @param size: The size of memory to allocate
 * @return Pointer to the allocated memory
 */
void* kmalloc(unsigned int size) {
    unsigned int order = get_chunk_order(size);
    size = (1 << order);  // Round up to the nearest power of 2
    order -= MIN_CACHE_ORDER;  // Adjust order to match cache level

    if (size == 0) return NULL;
    if (size > MAX_CHUNK_SIZE) {
        uart_puts("Invalid size for kmalloc!\n");
        return NULL;
    }
    if (kmem_caches[order].free_list == NULL) {
        request_page(order);
    }
    struct kmem_cache_entry *entry = kmem_caches[order].free_list;
    if (entry == NULL) {
        uart_puts("No free chunk available!\n");
        return NULL;
    }

    // Remove from free list
    kmem_freelist_pop(&kmem_caches[order]);

    void *ptr = (void*)entry + sizeof(struct kmem_cache_entry);  // Return the memory after the entry
    uart_puts("[Chunk] Allocated chunk size ");
    uart_puts(itoa(1 << (order + MIN_CACHE_ORDER)));
    uart_puts(" at address ");
    uart_hex((unsigned long)ptr);
    uart_puts(" (header starts at ");
    uart_hex((unsigned long)entry);
    uart_puts(")\r\n");

    return ptr;
}

void kfree(void *ptr) {
    if (ptr == NULL) return;
    if (ptr >= (void*)&__bss_end && ptr < (void*)&__stack_top) return;  // Out of bounds

    int page_idx = (ptr - memory_start) / PAGE_SIZE;
    int order = page_list[page_idx].cache_order;
    if (order < MIN_CACHE_ORDER || order > MAX_CACHE_ORDER) {
        uart_puts("Invalid pointer to free!\n");
        return;
    }
    struct kmem_cache_entry *entry = (struct kmem_cache_entry*)(ptr - sizeof(struct kmem_cache_entry));
    kmem_freelist_push(entry, &kmem_caches[order - MIN_CACHE_ORDER]);

    uart_puts("[Chunk] Freed chunk size ");
    uart_puts(itoa(1 << (order)));
    uart_puts(" at address ");
    uart_hex((unsigned long)ptr);
    uart_puts(" (header starts at ");
    uart_hex((unsigned long)entry);
    uart_puts(")\r\n");

    return;
}

void* alloc(unsigned int size) {
    if (size == 0) return NULL;

    void *alloc = NULL;
    if (size > MAX_CHUNK_SIZE) {
        alloc = _alloc(size);
    }
    else {
        alloc = kmalloc(size);
        // print_kmem_freelit();
    };

    if (alloc == NULL) {
        uart_puts("Failed to allocate memory!\n");
        return NULL;
    }
    return alloc;
}

void free(void *ptr) {
    if (ptr == NULL) return;
    if (ptr >= (void*)&__bss_end && ptr < (void*)&__stack_top) return;  // Out of bounds

    // Find the corresponding page index
    int page_idx = (ptr - memory_start) / PAGE_SIZE;

    if (page_idx < 0 || page_idx >= PAGE_NUM) {
        uart_puts("Invalid pointer to free!\n");
        return;
    }

    if (page_list[page_idx].cache_order != -1) {  // This address is in kmem cache
        kfree(ptr);
        // print_kmem_freelit();
    }
    else {
        _free(ptr);
    }
    return;
}

void test_alloc() {
    uart_puts("Testing memory allocation...\n");
    char *ptr1 = (char *)alloc(4000);
    char *ptr2 = (char *)alloc(8000);
    char *ptr3 = (char *)alloc(4000);
    char *ptr4 = (char *)alloc(4000);

    free(ptr1);
    free(ptr2);
    free(ptr3);
    free(ptr4);

    /* Test kmalloc */
    uart_puts("Testing dynamic allocator...\n");
    char *kmem_ptr1 = (char *)alloc(16);
    char *kmem_ptr2 = (char *)alloc(32);
    char *kmem_ptr3 = (char *)alloc(64);
    char *kmem_ptr4 = (char *)alloc(128);

    free(kmem_ptr1);
    free(kmem_ptr2);
    free(kmem_ptr3);
    free(kmem_ptr4);

    char *kmem_ptr5 = (char *)alloc(16);
    char *kmem_ptr6 = (char *)alloc(32);

    free(kmem_ptr5);
    free(kmem_ptr6);

    // Test allocate new page if the cache is not enough
    void *kmem_ptr[102];
    for (int i=0; i<100; i++) {
        kmem_ptr[i] = (char *)alloc(128);
    }
    for (int i=0; i<100; i++) {
        free(kmem_ptr[i]);
    }
}