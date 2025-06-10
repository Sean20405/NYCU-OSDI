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

void sys_open(struct TrapFrame *trapframe) {
    const char *pathname = (const char *)trapframe->x[0];
    int flags = (int)trapframe->x[1];
    if (pathname == NULL || pathname[0] == '\0') {
        uart_puts("[WARN] sys_open: pathname is NULL or empty\r\n");
        trapframe->x[0] = -1;  // return -1
        return;
    }

    uart_puts("[INFO] sys_open(");
    uart_puts(pathname);
    uart_puts(")\r\n");

    struct ThreadTask *curr = get_current();
    for (int i=0; i<THREAD_MAX_FD; ++i) {
        if (!curr->fd_table[i]) {
            uart_puts("[INFO] sys_open: found an empty slot ");
            uart_puts(itoa(i));
            uart_puts(" in fd_table\r\n");

            // Look up the vnode
            int ret = vfs_open(pathname, flags, &curr->fd_table[i]);
            if (ret != 0) {
                uart_puts("[WARN] sys_open: vfs_open failed\r\n");
                trapframe->x[0] = -ret;
                return;
            }

            uart_puts("[INFO] sys_open: opened file successfully, file->vnode address @");
            uart_hex((unsigned long)curr->fd_table[i]->vnode);
            uart_puts(" (task ");
            uart_puts(itoa(curr->id));
            uart_puts(")\r\n");

            trapframe->x[0] = i;
            return;
        }
    }
}

void sys_close(struct TrapFrame *trapframe) {
    int fd = (int)trapframe->x[0];
    if (fd < 0 || fd >= THREAD_MAX_FD) {
        uart_puts("[WARN] sys_close: invalid file descriptor\r\n");
        trapframe->x[0] = -1;  // return -1
        return;
    }

    uart_puts("[INFO] sys_close(");
    uart_puts(itoa(fd));
    uart_puts(")\r\n");

    struct ThreadTask *curr = get_current();
    if (curr == NULL) {
        uart_puts("[WARN] sys_close: current task is NULL\r\n");
        return;
    }

    // Close the file
    int ret = vfs_close(curr->fd_table[fd]);
    curr->fd_table[fd] = NULL;  // Clear the file descriptor
    trapframe->x[0] = ret;
}

void sys_write(struct TrapFrame *trapframe) {
    // uart_puts("sys_write called\r\n");
    int fd = (int)trapframe->x[0];
    const void *buf = (const void *)trapframe->x[1];
    unsigned long count = (unsigned int)trapframe->x[2];
    if (fd < 0 || fd >= THREAD_MAX_FD) {
        uart_puts("[WARN] sys_write: invalid file descriptor\r\n");
        trapframe->x[0] = -1;  // return -1
        return;
    }
    if (buf == NULL || count == 0) {
        uart_puts("[WARN] sys_write: buffer is NULL or count is zero\r\n");
        trapframe->x[0] = -1;  // return -1
        return;
    }

    uart_puts("[INFO] sys_write(");
    uart_puts(itoa(fd));
    uart_puts(", buf, ");
    uart_puts(itoa(count));
    uart_puts(")\r\n");

    uart_puts("[INFO] sys_write: buf content: ");
    uart_puts((char *)buf);
    uart_puts("\r\n");

    struct ThreadTask *curr = get_current();
    if (curr == NULL) {
        uart_puts("[WARN] sys_write: current task is NULL\r\n");
        trapframe->x[0] = -1;  // return -1
        return;
    }

    struct file *file = curr->fd_table[fd];
    uart_puts("[INFO] sys_write: file address @");
    uart_hex((unsigned long)file->vnode);
    uart_puts(" (task ");
    uart_puts(itoa(curr->id));
    uart_puts(")\r\n");

    if (file->vnode == NULL || file->f_ops == NULL || file->f_ops->write == NULL) {
        uart_puts("[WARN] sys_write: file not open or write operation not supported\r\n");
        trapframe->x[0] = -1;  // return -1
        return;
    }
    int ret = file->f_ops->write(file, buf, count);
    if (ret < 0) {
        uart_puts("[WARN] sys_write: write operation failed\r\n");
        trapframe->x[0] = ret;
        return;
    }

    trapframe->x[0] = ret;
}

void sys_read(struct TrapFrame *trapframe) {
    int fd = (int)trapframe->x[0];
    void *buf = (void *)trapframe->x[1];
    unsigned long count = (unsigned long)trapframe->x[2];
    if (fd < 0 || fd >= THREAD_MAX_FD) {
        uart_puts("[WARN] sys_read: invalid file descriptor\r\n");
        trapframe->x[0] = -1;  // return -1
        return;
    }
    uart_puts("[INFO] sys_read(");
    uart_puts(itoa(fd));
    uart_puts(", buf, ");
    uart_puts(itoa(count));
    uart_puts(")\r\n");

    struct ThreadTask *curr = get_current();
    if (curr == NULL) {
        uart_puts("[WARN] sys_read: current task is NULL\r\n");
        trapframe->x[0] = -1;  // return -1
        return;
    }
    struct file *file = curr->fd_table[fd];
    if (file->vnode == NULL || file->f_ops == NULL || file->f_ops->read == NULL) {
        uart_puts("[WARN] sys_read: file not open or read operation not supported\r\n");
        trapframe->x[0] = -1;  // return -1
        return;
    }
    int ret = file->f_ops->read(file, buf, count);
    if (ret < 0) {
        uart_puts("[WARN] sys_read: read operation failed\r\n");
        trapframe->x[0] = ret;
        return;
    }
    uart_puts("[INFO] sys_read: read content: ");
    uart_puts((char *)buf);
    uart_puts("\r\n");
    trapframe->x[0] = ret;
}

void sys_mkdir(struct TrapFrame *trapframe) {
    const char *pathname = (const char *)trapframe->x[0];
    unsigned mode = (unsigned)trapframe->x[1];  // Not used
    if (pathname == NULL || pathname[0] == '\0') {
        uart_puts("[WARN] sys_mkdir: pathname is NULL or empty\r\n");
        trapframe->x[0] = -1;  // return -1
        return;
    }

    uart_puts("[INFO] sys_mkdir(");
    uart_puts(pathname);
    uart_puts(")\r\n");

    int ret = vfs_mkdir(pathname);
    if (ret < 0) {
        uart_puts("[WARN] sys_mkdir: vfs_mkdir failed\r\n");
        trapframe->x[0] = ret;
    }
    else {
        trapframe->x[0] = 0;  // return 0 for success
    }
}

void sys_mount(struct TrapFrame *trapframe) {
    // uart_puts("sys_mount called\r\n");
    const char *src = (const char *)trapframe->x[0];  // Not used
    const char *target = (const char *)trapframe->x[1];
    const char *filesystem = (const char *)trapframe->x[2];
    unsigned long flags = (unsigned long)trapframe->x[3];  // Not used
    const void *data = (const void *)trapframe->x[4];  // Not used
    if (target == NULL || target[0] == '\0') {
        uart_puts("[WARN] sys_mount: target is NULL or empty\r\n");
        trapframe->x[0] = -1;  // return -1
        return;
    }
    if (filesystem == NULL || filesystem[0] == '\0') {
        uart_puts("[WARN] sys_mount: filesystem is NULL or empty\r\n");
        trapframe->x[0] = -1;  // return -1
        return;
    }

    uart_puts("[INFO] sys_mount(");
    uart_puts(target);
    uart_puts(", ");
    uart_puts(filesystem);
    uart_puts(")\r\n");

    int ret = vfs_mount(target, filesystem);
    if (ret < 0) {
        uart_puts("[WARN] sys_mount: vfs_mount failed\r\n");
        trapframe->x[0] = ret;  // return error code
    }
    else {
        trapframe->x[0] = 0;  // return 0 for success
    }
}

void sys_chdir(struct TrapFrame *trapframe) {
    // uart_puts("sys_chdir called\r\n");
    const char *path = (const char *)trapframe->x[0];
    if (path == NULL || path[0] == '\0') {
        uart_puts("[WARN] sys_chdir: path is NULL or empty\r\n");
        trapframe->x[0] = -1;  // return -1
        return;
    }

    uart_puts("[INFO] sys_chdir(");
    uart_puts(path);
    uart_puts(")\r\n");

    struct ThreadTask *curr = get_current();
    if (curr == NULL) {
        uart_puts("[WARN] sys_chdir: current task is NULL\r\n");
        trapframe->x[0] = -1;  // return -1
        return;
    }

    int ret = vfs_lookup(path, &curr->cwd);
    if (ret < 0) {
        uart_puts("[WARN] sys_chdir: vfs_lookup failed\r\n");
        trapframe->x[0] = ret;  // return error code
    }
    else {
        uart_puts("[INFO] sys_chdir: changed directory to ");
        uart_puts(path);
        uart_puts(" (");
        uart_hex((unsigned long)curr->cwd);
        uart_puts(")\r\n");
        trapframe->x[0] = 0;  // return 0 for success
    }
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

int open(const char *pathname, int flags) {
    int ret;
    asm volatile(
        "mov x8, 11 \n"
        "mov x0, %0 \n"
        "mov x1, %1 \n"
        "svc 0      \n"
        "mov %0, x0 \n"
        : "=r"(ret)
        : "r"(pathname), "r"(flags)
    );
    return ret;
}

int close(int fd) {
    int ret;
    asm volatile(
        "mov x8, 12 \n"
        "mov x0, %0 \n"
        "svc 0      \n"
        "mov %0, x0 \n"
        : "=r"(ret)
        : "r"(fd)
    );
    return ret;
}

long write(int fd, const void *buf, unsigned long count) {
    long ret;
    asm volatile(
        "mov x8, 13 \n"
        "mov x0, %0 \n"
        "mov x1, %1 \n"
        "mov x2, %2 \n"
        "svc 0      \n"
        "mov %0, x0 \n"
        : "=r"(ret)
        : "r"(fd), "r"(buf), "r"(count)
    );
    return ret;
}

long read(int fd, void *buf, unsigned long count) {
    long ret;
    asm volatile(
        "mov x8, 14 \n"
        "mov x0, %0 \n"
        "mov x1, %1 \n"
        "mov x2, %2 \n"
        "svc 0      \n"
        "mov %0, x0 \n"
        : "=r"(ret)
        : "r"(fd), "r"(buf), "r"(count)
    );
    return ret;
}

int mkdir(const char *pathname, unsigned mode) {
    int ret;
    asm volatile(
        "mov x8, 15 \n"
        "mov x0, %0 \n"
        "mov x1, %1 \n"
        "svc 0      \n"
        "mov %0, x0 \n"
        : "=r"(ret)
        : "r"(pathname), "r"(mode)
    );
    return ret;
}

int mount(const char *src, const char *target, const char *filesystem, unsigned long flags, const void *data) {
    int ret;
    asm volatile(
        "mov x8, 16 \n"
        "mov x0, %0 \n"
        "mov x1, %1 \n"
        "mov x2, %2 \n"
        "mov x3, %3 \n"
        "mov x4, %4 \n"
        "svc 0      \n"
        "mov %0, x0 \n"
        : "=r"(ret)
        : "r"(src), "r"(target), "r"(filesystem), "r"(flags), "r"(data)
    );
    return ret;
}

int chdir(const char *path) {
    int ret;
    asm volatile(
        "mov x8, 17 \n"
        "mov x0, %0 \n"
        "svc 0      \n"
        "mov %0, x0 \n"
        : "=r"(ret)
        : "r"(path)
    );
    return ret;
}