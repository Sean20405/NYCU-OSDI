#ifndef SCHED_H
#define SCHED_H

#include <stddef.h>
#include "alloc.h"
#include "uart.h"
#include "exec.h"
#include "signal.h"
#include "exception.h"

#define MAX_TASKS 64
#define DEFAULT_PRIORITY 10
#define THREAD_STACK_SIZE 0x1000  // 4KB stack size
#define TASK_READY 0
#define TASK_RUNNING 1
#define TASK_BLOCKED 2
#define TASK_EXITED 3

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
    void* kernel_stack;
    void* user_stack;

    // Signal handling
    unsigned int pending_sig;           // A binary mask of pending signals
    sighandler_t sig_handlers[SIG_NUM]; // Signal handlers
    struct TrapFrame *sig_frame;        // Saved context before jumping to signal handler
    
    // Linked list pointers
    struct ThreadTask *next;
};

#ifndef __ASSEMBLER__
extern struct ThreadTask* get_current(void);
extern void set_current(struct ThreadTask *task);
extern void cpu_switch_to(struct ThreadTask *prev, struct ThreadTask *next);
extern void ret_from_fork(void);
#endif

extern struct ThreadTask *ready_queue;
extern struct ThreadTask *wait_queue;
extern struct ThreadTask *zombie_queue;
extern unsigned int thread_cnt;

void sched_init();
struct ThreadTask* thread_create(void (*callback)(void));
struct ThreadTask* get_thread_task_by_id(int pid);
void _exit();
int _kill(unsigned int pid);
void schedule();
void kill_zombies();
void idle();

#endif /* SCHED_H */