#include "syscall.h"

extern struct ThreadTask *ready_queue;
extern unsigned int thread_cnt;

void sys_getpid(struct TrapFrame *trapframe) {
    // uart_puts("sys_getpid called\r\n");
    struct ThreadTask *curr = get_current();
    if (curr == NULL) {
        uart_puts("Current task is NULL\r\n");
        return -1;
    }

    trapframe->x[0] = curr->id;
}

void sys_uart_read(struct TrapFrame *trapframe) {
    // uart_puts("sys_uart_read called\r\n");
    char *buf = (char*)trapframe->x[0];
    int size = trapframe->x[1];
    if (size > 4096) {
        uart_puts("[WARN] sys_uart_read: size is too large\r\n");
        size = 4096;
    }
    if (size <= 0) {
        uart_puts("[WARN] sys_uart_read: size is invalid\r\n");
        return -1;
    }
    if (buf == NULL) {
        uart_puts("[WARN] sys_uart_read: buf is NULL\r\n");
        return -1;
    }

    unsigned int i = uart_getn(buf, size);
    trapframe->x[0] = i;  // return size
}

void sys_uart_write(struct TrapFrame *trapframe) {
    // uart_puts("sys_uart_write called\r\n");
    char *buf = (char*)trapframe->x[0];
    int size = trapframe->x[1];
    if (size > 4096) {
        uart_puts("[WARN] sys_uart_write: size is too large\r\n");
        size = 4096;
    }
    if (size <= 0) {
        uart_puts("[WARN] sys_uart_write: size is invalid\r\n");
        return;
    }
    if (buf == NULL) {
        uart_puts("[WARN] sys_uart_write: buf is NULL\r\n");
        return;
    }

    unsigned int i = uart_putn(buf, size);
    trapframe->x[0] = i;  // return size
}

void sys_exec(struct TrapFrame *trapframe) {
    // uart_puts("sys_exec called\r\n");
    char *name = (char *)trapframe->x[0];
    char **argv = (char **)trapframe->x[1];
    if (name == NULL) {
        uart_puts("[WARN] sys_exec: name is NULL\r\n");
        return -1;
    }
    if (name[0] == '\0') {
        uart_puts("[WARN] sys_exec: name is empty\r\n");
        return -1;
    }
    _exec(name);
}

void sys_fork(struct TrapFrame *trapframe) {
    // uart_puts("sys_fork called\r\n");
    struct ThreadTask *parent_thread = get_current();
    if (parent_thread == NULL) {
        uart_puts("Current task is NULL\r\n");
        return;
    }

    // Fork a new thread
    struct ThreadTask *child_thread = (struct ThreadTask *)alloc(sizeof(struct ThreadTask));
    if (child_thread == NULL) {
        uart_puts("Failed to allocate memory for new task\r\n");
        trapframe->x[0] = -1;
        return;
    }

    child_thread->id = thread_cnt++;
    child_thread->state = TASK_READY;
    child_thread->counter = parent_thread->counter;
    child_thread->priority = parent_thread->priority;
    child_thread->preempt_count = parent_thread->preempt_count;
    child_thread->kernel_stack = alloc(THREAD_STACK_SIZE);
    if (child_thread->kernel_stack == NULL) {
        uart_puts("Failed to allocate memory for new task stack\r\n");
        free(child_thread);
        trapframe->x[0] = -1;
        return;
    }
    child_thread->user_stack = alloc(THREAD_STACK_SIZE);
    if (child_thread->user_stack == NULL) {
        uart_puts("Failed to allocate memory for new task user stack\r\n");
        free(child_thread->kernel_stack);
        free(child_thread);
        trapframe->x[0] = -1;
        return;
    }

    child_thread->pending_sig = parent_thread->pending_sig;
    for (int i = 0; i < SIG_NUM; i++) {
        child_thread->sig_handlers[i] = parent_thread->sig_handlers[i];
    }
    child_thread->next = NULL;

    memcpy(&child_thread->cpu_context, &parent_thread->cpu_context, sizeof(struct cpu_context));
    
    // Copy stack
    memcpy(child_thread->user_stack, parent_thread->user_stack, THREAD_STACK_SIZE);
    memcpy(child_thread->kernel_stack, parent_thread->kernel_stack, THREAD_STACK_SIZE);

    // Set sp
    unsigned long curr_sp;
    asm volatile("mov %0, sp" : "=r"(curr_sp));
    unsigned long curr_sp_offset = curr_sp - (unsigned long)parent_thread->kernel_stack;
    child_thread->cpu_context.sp = (unsigned long)child_thread->kernel_stack + curr_sp_offset;
    child_thread->cpu_context.fp = parent_thread->cpu_context.fp;

    struct TrapFrame *child_frame = (struct TrapFrame *)(child_thread->kernel_stack + ((void*)trapframe - parent_thread->kernel_stack));
    memcpy(child_frame, trapframe, sizeof(struct TrapFrame));
    child_frame->x[0] = 0;
    child_frame->sp_el0 = (unsigned long)(child_thread->user_stack + ((void*)trapframe->sp_el0 - parent_thread->user_stack));  // Set the stack pointer to the new task's stack
    child_thread->cpu_context.lr = &&SYSCALL_FORK_END;

    add_thread_task(&ready_queue, child_thread);

    trapframe->x[0] = child_thread->id;  // return child_thread->id

SYSCALL_FORK_END:
    asm volatile("nop");
    return;
}

void sys_exit(struct TrapFrame *trapframe) {
    // uart_puts("sys_exit called\r\n");
    _exit();
}

void sys_mbox_call(struct TrapFrame *trapframe) {
    // uart_puts("sys_mbox_call called\r\n");
    unsigned char channel = (unsigned char)trapframe->x[0];
    unsigned int *mbox = (unsigned int *)trapframe->x[1];
    if (mbox == NULL) {
        uart_puts("[WARN] sys_mbox_call: mbox is NULL\r\n");
        return -1;
    }
    if (channel > 16) {
        uart_puts("[WARN] sys_mbox_call: channel is invalid\r\n");
        return -1;
    }
    
    int ret = mailbox_call(mbox, channel);
    if (ret == 0) {
        uart_puts("[WARN] sys_mbox_call: mailbox call failed\r\n");
        return -1;
    }

    trapframe->x[0] = mbox[1];  // return mbox[1]
}

void sys_kill(struct TrapFrame *trapframe) {
    // uart_puts("sys_kill called\r\n");
    int pid = (int)trapframe->x[0];
    int ret = _kill(pid);
    if (ret == -1) {
        uart_puts("[WARN] sys_kill: kill failed\r\n");
        return;
    }
}

void sys_signal(struct TrapFrame *trapframe) {
    // uart_puts("sys_signal called\r\n");
    int sig = (int)trapframe->x[0];
    sighandler_t handler = (sighandler_t)trapframe->x[1];
    struct ThreadTask *curr = get_current();
    if (curr == NULL) {
        uart_puts("[WARN] sys_signal: current task is NULL\r\n");
        return;
    }
    
    if (sig < 0 || sig >= SIG_NUM) {
        uart_puts("[WARN] sys_signal: invalid signal number\r\n");
        return;
    }

    uart_puts("[INFO] sys_signal: setting signal handler, new handler @");
    uart_hex((unsigned long)handler);
    uart_puts("\r\n");
    
    sighandler_t old_handler = curr->sig_handlers[sig];
    curr->sig_handlers[sig] = handler;
    
    trapframe->x[0] = (unsigned long)old_handler;  // return old_handler
}

void sys_sigkill(struct TrapFrame *trapframe) {
    // uart_puts("sys_sigkill called\r\n");
    int pid = (int)trapframe->x[0];
    int sig = (int)trapframe->x[1];

    struct ThreadTask *task = get_thread_task_by_id(pid);
    if (task == NULL) {
        uart_puts("[WARN] sys_sigkill: task not found\r\n");
        return;
    }

    if (sig < 0 || sig >= SIG_NUM) {
        uart_puts("[WARN] sys_sigkill: invalid signal number\r\n");
        return;
    }
    
    task->pending_sig |= (1 << sig);  // Set the pending signal
}

void sys_sigreturn(struct TrapFrame *trapframe) {
    // uart_puts("sys_sigreturn called\r\n");
    struct ThreadTask *curr = get_current();
    if (curr == NULL) {
        uart_puts("Current task is NULL\r\n");
        return;
    }

    // Free the handler stack
    free(curr->cpu_context.fp - THREAD_STACK_SIZE);

    // Restore the trapframe
    memcpy(trapframe, &curr->sig_frame, sizeof(struct TrapFrame));
    return;
}

/* Wrapper function for syscall */
int get_pid() {
    int ret;
    asm volatile(
        "mov x8, 0  \n"
        "svc 0      \n"
        "mov %0, x0  \n"
        : "=r"(ret)
    );
    return ret;
}

int uart_read(char buf[], int size) {
    int ret;
    asm volatile(
        "mov x8, 1  \n"
        "mov x0, %0  \n"
        "mov x1, %1  \n"
        "svc 0      \n"
        "mov %0, x0  \n"
        : "=r"(ret)
        : "r"(buf), "r"(size)
    );
    return ret;
}

int uart_write(const char buf[], int size) {
    int ret;
    asm volatile(
        "mov x8, 2  \n"
        "mov x0, %0  \n"
        "mov x1, %1  \n"
        "svc 0      \n"
        "mov %0, x0  \n"
        : "=r"(ret)
        : "r"(buf), "r"(size)
    );
    return ret;
}

int exec(const char* name, char *const argv[]) {
    int ret;
    asm volatile(
        "mov x8, 3  \n"
        "mov x0, %0  \n"
        "mov x1, %1  \n"
        "svc 0      \n"
        "mov %0, x0  \n"
        : "=r"(ret)
        : "r"(name), "r"(argv)
    );
    return ret;
}

int fork() {
    int ret;
    asm volatile(
        "mov x8, 4  \n"
        "svc 0      \n"
        "mov %0, x0  \n"
        : "=r"(ret)
    );
    return ret;
}

void exit() {
    asm volatile(
        "mov x8, 5  \n"
        "svc 0      \n"
    );
}

int mbox_call(unsigned char ch, unsigned int *mbox) {
    int ret;
    asm volatile(
        "mov x8, 6  \n"
        "mov x0, %0  \n"
        "mov x1, %1  \n"
        "svc 0      \n"
        "mov %0, x0  \n"
        : "=r"(ret)
        : "r"(ch), "r"(mbox)
    );
    return ret;
}

void kill(int pid) {
    asm volatile(
        "mov x8, 7  \n"
        "mov x0, %0  \n"
        "svc 0      \n"
        : 
        : "r"(pid)
    );
}

sighandler_t signal(int sig, sighandler_t handler) {
    sighandler_t old_handler;
    asm volatile(
        "mov x8, 8  \n"
        "mov x0, %0  \n"
        "mov x1, %1  \n"
        "svc 0      \n"
        "mov %0, x0  \n"
        : "=r"(old_handler)
        : "r"(sig), "r"(handler)
    );
    return old_handler;
}

int sigkill(int pid, int sig) {
    int ret;
    asm volatile(
        "mov x8, 9  \n"
        "mov x0, %0 \n"
        "mov x1, %1 \n"
        "svc 0      \n"
        "mov %0, x0 \n"
        : "=r"(ret)
        : "r"(pid), "r"(sig)
    );
    return ret;
}

void sigreturn() {
    asm volatile(
        "mov x8, 10 \n"
        "svc 0      \n"
    );
}