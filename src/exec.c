#include "exec.h"

void _exec(char* filename) {
    unsigned int exec_size = cpio_get_file_size(filename);
    if (exec_size == 0) {
        uart_puts(filename);
        uart_puts(":  File not found\r\n");
        return;
    }

    uart_puts("File: ");
    uart_puts(filename);
    uart_puts(", size: ");
    uart_puts(itoa(exec_size));
    uart_puts("\r\n");

    char *exec_addr = alloc(exec_size);
    if (exec_addr == NULL) return;

    cpio_get_exec(filename, exec_addr);
    if (exec_addr == NULL) {
        uart_puts("Failed to load executable\r\n");
        return;
    }

    struct ThreadTask* new_thread = thread_create(exec_addr);
    asm volatile(
        "msr tpidr_el1, %0\n"
        "mov x5, 0x0\n"
        "msr spsr_el1, x5\n"
        "msr elr_el1, %1\n"
        "msr sp_el0, %2\n"
        "mov sp, %3\n"
        "eret"
        :
        : "r"(new_thread), "r"(new_thread->cpu_context.lr), "r"(new_thread->cpu_context.sp), 
          "r"(new_thread->kernel_stack + THREAD_STACK_SIZE)
        : "x5"
    );
}
