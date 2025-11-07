#ifndef TIMER_H
#define TIMER_H

#include "uart.h"
#include "alloc.h"
#include "exception.h"
#include "string.h"
#include "sched.h"
#include <stddef.h>

#define CORE0_TIMER_IRQ_CTRL ((volatile unsigned int *)0x40000040)

typedef void (*timer_callback)(char*);

void timer_enable_irq();
void timer_disable_irq();
void set_timer_irq(unsigned long long tick);
void timer_init();
void core_timer_handler();
void print_msg(char* msg);
void print_uptime(char* _);
unsigned long long get_tick();
unsigned long long get_freq();
unsigned long long get_time();
void set_timeout(char* msg, int sec);
void add_timer(timer_callback callback, char* msg, unsigned long long tick);

#endif /* TIMER_H */