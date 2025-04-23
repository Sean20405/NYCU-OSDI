#include "exec.h"
#include "uart.h"

void exec(char* filename) {
    char *exec_addr = cpio_get_exec(filename);
    if (exec_addr == NULL) return;

    // Change to EL0 and jump to the entry point
    char *program_stack = simple_alloc(PROGRAM_STACK_SIZE);
    char *stack_top = program_stack + PROGRAM_STACK_SIZE;

    uart_puts("exec_addr: ");
    uart_hex((unsigned long)exec_addr);
    uart_puts("\r\n");
    uart_puts("stack_top: ");
    uart_hex((unsigned long)stack_top);
    uart_puts("\r\n");

    asm volatile(
       "mov x5, 0x3c0\n"
       "msr spsr_el1, x5\n"
       "msr elr_el1, %0\n"
       "msr sp_el0, %1\n"
       "eret"
       :
       : "r"(exec_addr), "r"((unsigned long)program_stack)
       : "x5"
    );
}
