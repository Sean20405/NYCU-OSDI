#include "exception.h"

void exception_entry() {
    // Print spsr_el1, elr_el1, and esr_el1
    unsigned long spsr_el1, elr_el1, esr_el1;
    asm volatile(
        "mrs %0, spsr_el1\n"
        "mrs %1, elr_el1\n"
        "mrs %2, esr_el1\n"
        : "=r"(spsr_el1), "=r"(elr_el1), "=r"(esr_el1)
        :
        :
    );

    unsigned long ec = (esr_el1 >> 26) & 0x3f;  // Extract the exception class

    uart_puts("spsr_el1: ");
    uart_hex(spsr_el1);
    uart_puts(" elr_el1: ");
    uart_hex(elr_el1);
    uart_puts(" esr_el1: ");
    uart_hex(esr_el1);
    uart_puts(" EC: ");
    uart_hex(ec);
    uart_puts("\r\n");
}


/**
 * irq_entry - Interrupt handler entry point
 *
 * This function is called when an interrupt occurs. It checks the
 * pending interrupts and calls the appropriate handler.
 *
 * The function handles the core timer interrupt and the UART interrupt.
 */
void irq_entry() {
    unsigned int irq_src = *CORE0_IRQ_SOURCE;
    unsigned int pending_1 = *IRQ_PENDING_1;

    disable_irq_el1();
    if (irq_src & TIMER_IRQ) {  // Timer interrupt
        add_task(core_timer_handler, 0);
        execute_task();
    }
    else if ((irq_src & GPU_IRQ) && (pending_1 & (1 << 29))) {  // UART interrupt
        uart_irq_handler();
    }
    enable_irq_el1();
}

void el0_sync_entry(unsigned long sp, unsigned long esr_el1) {
    struct TrapFrame *trapframe = (struct TrapFrame *)sp;
    unsigned long ec = (esr_el1 >> 26) & 0x3f;  // Extract the exception class

    if (ec == 0x15) {  // System call
        syscall_entry(trapframe);
    }
    else {
        uart_puts("Unknown exception class\r\n");
    }
    return;
}

void syscall_entry(struct TrapFrame *trapframe) {
    unsigned long syscall_num = trapframe->x[8];  // x8 contains the syscall number

    switch (syscall_num) {
        case SYS_GETPID_NUM:
            sys_getpid(trapframe);
            break;
        case SYS_UART_READ_NUM:
            sys_uart_read(trapframe);
            break;
        case SYS_UART_WRITE_NUM:
            sys_uart_write(trapframe);
            break;
        case SYS_EXEC_NUM:
            sys_exec(trapframe);
            break;
        case SYS_FORK_NUM:
            sys_fork(trapframe);
            break;
        case SYS_EXIT_NUM:
            sys_exit(trapframe);
            break;
        case SYS_MBOX_CALL_NUM:
            sys_mbox_call(trapframe);
            break;
        case SYS_KILL_NUM:
            sys_kill(trapframe);
            break;
        default:
            uart_puts("Unknown syscall number: ");
            uart_hex(syscall_num);
            uart_puts("\r\n");
    }
}

void enable_irq_el1() {
    asm volatile("msr daifclr, #0xf\n");
}

void disable_irq_el1() {
    asm volatile("msr daifset, #0xf\n");
}