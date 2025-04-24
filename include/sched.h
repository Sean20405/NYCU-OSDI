#ifndef THREAD_H
#define THREAD_H

#include <stddef.h>
#include "alloc.h"
#include "uart.h"

#define MAX_TASKS 64
#define DEFAULT_PRIORITY 10
#define THREAD_SIZE 0x1000  // 4KB stack size
#define TASK_READY 0 // Task state: ready
#define TASK_RUNNING 1 // Task state: running
#define TASK_BLOCKED 2 // Task state: blocked
#define TASK_EXITED 3 // Task state: exited

struct cpu_context {
    unsigned long x19;
    unsigned long x20;
    unsigned long x21;
    unsigned long x22;
    unsigned long x23;
    unsigned long x24;
    unsigned long x25;
    unsigned long x26;
    unsigned long x27;
    unsigned long x28;
    unsigned long fp;
    unsigned long lr;
    unsigned long sp;
};

struct ThreadTask {
    struct cpu_context cpu_context;
    unsigned int id; // Thread ID
    long state;
    long counter;
    long priority;
    long preempt_count;  // Whether this task can be preempted currently, non-zero means cannot.
    struct ThreadTask *next;
};

#ifndef __ASSEMBLER__
extern struct ThreadTask* get_current(void);
extern void cpu_switch_to(struct ThreadTask *prev, struct ThreadTask *next);
extern void ret_from_fork(void);
#endif

void sched_init();
int thread_create(void (*callback)(void));
void _exit();
void schedule();
void kill_zombies();
void idle();

#endif /* THREAD_H */