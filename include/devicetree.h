#ifndef DEVICETREE_H
#define DEVICETREE_H

#include "utils.h"
#include "uart.h"
#include "string.h"
#include <stdint.h>
#include <stddef.h>

#define FDT_BEGIN_NODE  0x1
#define FDT_END_NODE    0x2
#define FDT_PROP        0x3
#define FDT_NOP         0x4
#define FDT_END         0x9

struct fdt_header {
    uint32_t magic;
    uint32_t totalsize;
    uint32_t off_dt_struct;
    uint32_t off_dt_strings;
    uint32_t off_mem_rsvmap;
    uint32_t version;
    uint32_t last_comp_version;
    uint32_t boot_cpuid_phys;
    uint32_t size_dt_strings;
    uint32_t size_dt_struct;
};

struct fdt_prop {
    uint32_t len;
    uint32_t nameoff;
};

typedef int (*fdt_callback)(int type, const char* name, const void* data, uint32_t size, void* user_data);

int fdt_init(const void* fdt_base);
int fdt_parse_node(const void** ptr, fdt_callback callback);
int fdt_traverse(fdt_callback callback);
void fdt_print_header(const struct fdt_header* header);
int initramfs_callback(int type, const char* name, const void* data, uint32_t size, void* user_data);

#endif /* DEVICETREE_H */