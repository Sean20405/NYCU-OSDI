#include "uart.h"
#include "shell.h"
#include "devicetree.h"
#include "timer.h"
#include "mm.h"

extern char *__stack_top;
extern uint32_t cpio_addr;
extern uint32_t cpio_end;
extern uint32_t fdt_total_size;

void main() {
    // Get the address of the device tree blob
    uint32_t dtb_address;
    asm volatile ("mov %0, x20" : "=r" (dtb_address));

    int ret = fdt_init((void*)dtb_address);
    if (ret) {
        uart_puts("Failed to initialize the device tree blob!\n");
        return;
    }
    ret = fdt_traverse(initramfs_callback);
    if (ret) {
        uart_puts("Failed to traverse the device tree blob!\n");
        return;
    }

    mm_init();
    reserve(0x0000, 0x1000);                                    // Spin tables for multicore boot
    reserve(0x80000, (void*)&__stack_top);                      // Kernel image & startup allocator
    reserve((void*)cpio_addr, (void*)cpio_end);                 // Initramfs
    reserve((void*)dtb_address, (void*)dtb_address + be2le_u32(fdt_total_size));   // Devicetree 

    kmem_cache_init();

    enable_irq_el1();

    // Lab3 Basic 2: Print uptime every 2 seconds
    // add_timer(print_uptime, NULL, 2 * get_freq());

    shell();
}