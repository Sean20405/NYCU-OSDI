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

void enable_irq_el1() {
    asm volatile("msr daifclr, #0xf\n");
}

void disable_irq_el1() {
    asm volatile("msr daifset, #0xf\n");
}