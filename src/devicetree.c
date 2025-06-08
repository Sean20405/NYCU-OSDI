#include "devicetree.h"
#include "uart.h"

static const void* g_fdt_base = NULL;
static const char* g_fdt_strings = NULL;
static const void* g_fdt_structure = NULL;
uint32_t fdt_total_size = 0;
static uint32_t g_fdt_strings_size = 0;
static uint32_t g_fdt_structure_size = 0;

extern uint32_t cpio_addr;
extern uint32_t cpio_end;

int fdt_init(const void* fdt_base) {
    if (!fdt_base) return 0;

    g_fdt_base = fdt_base;
    const struct fdt_header* header = (const struct fdt_header*)fdt_base;

    // Check the magic number
    if (be2le_u32(header->magic) != 0xd00dfeed) {
        uart_puts("Invalid FDT magic number!\n");
        return -1;
    }

    fdt_total_size = header->totalsize;
    g_fdt_structure = (const char*)fdt_base + be2le_u32(header->off_dt_struct);
    g_fdt_strings = (const char*)fdt_base + be2le_u32(header->off_dt_strings);
    g_fdt_strings_size = be2le_u32(header->size_dt_strings);
    g_fdt_structure_size = be2le_u32(header->size_dt_struct);

    return 0;
}

/**
 * fdt_parse_node - Parse a node in the device tree continuously
 * 
 * This function parses a node in the device tree and calls the callback function
 * for each node to do corresponding operations.
 * 
 * @param ptr: Pointer to the pointer to the current position in the device tree
 * @param callback: Callback function to be called for each node
 * @return 0 for success, others for error
 * 
 * Note: It does not support recursive parsing. Only parse each node individually.
 */
int fdt_parse_node(const void** ptr, fdt_callback callback) {
    const char* p = *ptr;

    while (1) {
        uint32_t token = be2le_u32(*(uint32_t*)p);
        p += sizeof(uint32_t);

        if (token == FDT_BEGIN_NODE) {
            const char* name = (const char*)p;
            p += align(strlen(name) + 1, 4);
            if (callback && callback(FDT_BEGIN_NODE, name, NULL, 0, NULL)) {
                uart_puts("Failed to parse the node! Skip it.\r\n");
                *ptr = p;
                return -1;
            }
        }
        else if (token == FDT_END_NODE) {
            if (callback && callback(FDT_END_NODE, NULL, NULL, 0, NULL)) {
                uart_puts("Failed to parse the node! Skip it.\r\n");
                *ptr = p;
                return -1;
            }
        }
        else if (token == FDT_PROP) {
            struct fdt_prop* prop = (struct fdt_prop*)p;
            uint32_t len = be2le_u32(prop->len);
            uint32_t nameoff = be2le_u32(prop->nameoff);

            char* name = (char*)g_fdt_strings + nameoff;
            const void* data = p + sizeof(struct fdt_prop);

            // Move to the next token (align to 4 bytes)
            p += sizeof(struct fdt_prop) + align(len, 4);

            if (callback && callback(FDT_PROP, name, data, len, NULL)) {
                uart_puts("Failed to parse the node! Skip it.\r\n");
                *ptr = p;
                return -1;
            }
        }
        else if (token == FDT_NOP) {
            continue;
        }
        else if (token == FDT_END) {
            *ptr = p;
            return 0;
        }
        else {
            uart_puts("Invalid FDT token!\r\n");
            *ptr = p;
            return -1;
        }
    }
}

int fdt_traverse(fdt_callback callback) {
    if (!g_fdt_base | !g_fdt_structure) return -1;

    const void* ptr = g_fdt_structure;
    return fdt_parse_node(&ptr, callback);
}

void fdt_print_header(const struct fdt_header* header) {
    uart_puts("FDT Header:\n");
    uart_puts("  magic: ");
    uart_hex(be2le_u32(header->magic));
    uart_puts("\r\n");
    uart_puts("  totalsize: ");
    uart_hex(be2le_u32(header->totalsize));
    uart_puts("\r\n");
    uart_puts("  off_dt_struct: ");
    uart_hex(be2le_u32(header->off_dt_struct));
    uart_puts("\r\n");
    uart_puts("  off_dt_strings: ");
    uart_hex(be2le_u32(header->off_dt_strings));
    uart_puts("\r\n");
    uart_puts("  off_mem_rsvmap: ");
    uart_hex(be2le_u32(header->off_mem_rsvmap));
    uart_puts("\r\n");
    uart_puts("  version: ");
    uart_hex(be2le_u32(header->version));
    uart_puts("\r\n");
    uart_puts("  last_comp_version: ");
    uart_hex(be2le_u32(header->last_comp_version));
    uart_puts("\r\n");
    uart_puts("  boot_cpuid_phys: ");
    uart_hex(be2le_u32(header->boot_cpuid_phys));
    uart_puts("\r\n");
    uart_puts("  size_dt_strings: ");
    uart_hex(be2le_u32(header->size_dt_strings));
    uart_puts("\r\n");
    uart_puts("  size_dt_struct: ");
    uart_hex(be2le_u32(header->size_dt_struct));
    uart_puts("\r\n");
}

/*** Callback functions ***/

int initramfs_callback(int type, const char* name, const void* data, uint32_t size, void* user_data) {
    if (strcmp(name, "linux,initrd-start") == 0) {
        cpio_addr = be2le_u32(*(uint32_t*)data);
    }
    else if (strcmp(name, "linux,initrd-end") == 0) {
        cpio_end = be2le_u32(*(uint32_t*)data);
    }
    return 0;
}