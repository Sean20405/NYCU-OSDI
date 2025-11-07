#ifndef EXCEPTION_H
#define EXCEPTION_H

#include "uart.h"
#include "timer.h"
#include "syscall.h"

#define IRQ_BASE            0x3f00b000

#define IRQ_BASIC_PENDING   ((volatile unsigned int*)(IRQ_BASE + 0x200))
#define IRQ_PENDING_1       ((volatile unsigned int*)(IRQ_BASE + 0x204))
#define IRQ_PENDING_2       ((volatile unsigned int*)(IRQ_BASE + 0x208))
#define FIQ_CONTROL         ((volatile unsigned int*)(IRQ_BASE + 0x20c))
#define ENABLE_IRQS_1       ((volatile unsigned int*)(IRQ_BASE + 0x210))
#define ENABLE_IRQS_2       ((volatile unsigned int*)(IRQ_BASE + 0x214))
#define ENABLE_BASIC_IRQS   ((volatile unsigned int*)(IRQ_BASE + 0x218))
#define DISABLE_IRQS_1      ((volatile unsigned int*)(IRQ_BASE + 0x21c))
#define DISABLE_IRQS_2      ((volatile unsigned int*)(IRQ_BASE + 0x220))
#define DISABLE_BASIC_IRQS  ((volatile unsigned int*)(IRQ_BASE + 0x224))

#define CORE0_IRQ_SOURCE    ((volatile unsigned int *)0x40000060)
#define TIMER_IRQ           (1 << 1)
#define GPU_IRQ             (1 << 8)  // mini UART IRQ bit

struct TrapFrame {
    unsigned long x[31];    // x0-x30
    unsigned long sp_el0;   // stack pointer for EL0
    unsigned long spsr_el1; // saved program status register
    unsigned long elr_el1;  // exception link register
};

void exception_entry();
void irq_entry();
void enable_irq_el1();
void disable_irq_el1();

#endif /* EXCEPTION_H */