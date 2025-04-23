/**
 * load.c - Load kernel image
 * 
 * This file contains the function to load the kernel image using UART.
 */

#include "load.h"

void load_kernel() {
    unsigned int kernel_size = 0;
    char *kernel_addr = (char*)0x80000;
    unsigned int chksum = 0;

    // Read header
    char magic[4] = "BOOT";
    for (int i=0; i<4; ++i) {
        if (uart_getc() != magic[i]) {
            uart_puts("Invalid kernel image.\r\n");
            return;
        }
    }
    uart_puts("Valid kernel image.\r\n");

    for (int i=0; i<4; i++) {
        kernel_size |= uart_getc() << (i * 8);
    }
    uart_puts("Kernel size: ");
    uart_hex(kernel_size);
    uart_puts("\r\n");

    for (int i=0; i<4; i++) {
        chksum |= uart_getc() << (i * 8);
    }
    uart_puts("Checksum: ");
    uart_hex(chksum);
    uart_puts("\r\n");

    // Read kernel image
    for (int i=0; i<kernel_size; ++i) {
        char ch = uart_getc();
        *(kernel_addr + i) = ch;
    }

    uart_puts("Kernel loaded.\r\n");

    // Verify checksum
    unsigned int sum = 0;
    for (int i=0; i<kernel_size; ++i) {
        sum += *(kernel_addr + i);
    }
    if (sum == chksum) {
        uart_puts("Checksum passed.\r\n");
    }
    else {
        uart_puts("Checksum failed.\r\n");
        return;
    }

    // Jump to kernel
    void (*kernel)(void) = (void (*)(void))kernel_addr;
    kernel();
}