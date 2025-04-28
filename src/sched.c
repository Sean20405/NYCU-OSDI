#include "sched.h"

struct ThreadTask *ready_queue = NULL;
struct ThreadTask *wait_queue = NULL;
struct ThreadTask *zombie_queue = NULL;

unsigned int thread_cnt = 0;

void print_queue(struct ThreadTask *queue) {
    struct ThreadTask *current = queue;
    while (current != NULL) {
        uart_puts(itoa(current->id));
        uart_puts("(");
        uart_hex((unsigned long)current);
        uart_puts(")");
        uart_puts(" -> ");
        current = current->next;
    }
    uart_puts("NULL\r\n");
}

void add_thread_task(struct ThreadTask **queue, struct ThreadTask *task) {
    // Add the task to the ready queue
    if (*queue == NULL) {
        *queue = task;
    }
    else {
        struct ThreadTask *current = *queue;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = task;
    }
    task->next = NULL;

    // print_queue(*queue);
}

// Get the first task from the queue
struct ThreadTask* pop_thread_task(struct ThreadTask **queue) {
    if (*queue == NULL) {
        return NULL;
    }
    struct ThreadTask *task = *queue;
    *queue = (*queue)->next;
    return task;
}

void rm_thread_task(struct ThreadTask **queue, struct ThreadTask *task) {
    if (*queue == NULL) {
        return;
    }
    struct ThreadTask *current = *queue;
    struct ThreadTask *prev = NULL;

    while (current != NULL) {
        if (current == task) {
            if (prev == NULL) {
                *queue = current->next;
            }
            else {
                prev->next = current->next;
            }
            break;
        }
        prev = current;
        current = current->next;
    }
}

void sched_init() {
    ready_queue = NULL;
    wait_queue = NULL;
    zombie_queue = NULL;
    thread_cnt = 0;

    // Create a task for "idle"
    struct ThreadTask *idle_task = (struct ThreadTask *)alloc(sizeof(struct ThreadTask));
    if (idle_task == NULL) {
        uart_puts("Failed to allocate memory for idle task!\n");
        return;
    }

    thread_create(idle);
    set_current(idle_task);
}

struct ThreadTask* thread_create(void (*callback)(void)) {
    // Allocate memory for the task
    struct ThreadTask *task = (struct ThreadTask *)alloc(sizeof(struct ThreadTask));
    if (task == NULL) {
        uart_puts("Failed to allocate memory for task!\n");
        return -1;
    }

    // Initialize the task
    task->id = thread_cnt++;
    task->state = TASK_READY;
    task->counter = DEFAULT_PRIORITY;
    task->priority = DEFAULT_PRIORITY;
    task->preempt_count = 1;
    task->kernel_stack = alloc(THREAD_STACK_SIZE);
    if (task->kernel_stack == NULL) {
        uart_puts("Failed to allocate memory for task stack!\n");
        free(task);
        return -1;
    }
    task->user_stack = alloc(THREAD_STACK_SIZE);
    if (task->user_stack == NULL) {
        uart_puts("Failed to allocate memory for task stack!\n");
        free(task->kernel_stack);
        free(task);
        return -1;
    }
    task->next = NULL;

    memset((void*)&task->cpu_context, 0, sizeof(struct cpu_context));
    task->cpu_context.lr = (unsigned long)callback; // Set the entry point of the task
    task->cpu_context.sp = (unsigned long)task->user_stack + THREAD_STACK_SIZE;
    task->cpu_context.fp = task->cpu_context.sp;

    // Add the task to the ready queue
    add_thread_task(&ready_queue, task);

    return task;
}

struct ThreadTask* get_thread_task_by_id(int pid) {
    struct ThreadTask *current = ready_queue;
    while (current != NULL) {
        if (current->id == pid) {
            return current;
        }
        current = current->next;
    }

    current = wait_queue;
    while (current != NULL) {
        if (current->id == pid) {
            return current;
        }
        current = current->next;
    }
    
    return NULL;
}

void _exit() {
    struct ThreadTask *curr = get_current();
    if (curr == NULL) return;

    rm_thread_task(&ready_queue, curr);
    rm_thread_task(&wait_queue, curr);

    curr->state = TASK_EXITED;
    add_thread_task(&zombie_queue, curr);

    schedule();  // Switch to the next task
}

void schedule() {
    // disable_irq_el1();
    // timer_disable_irq();

    struct ThreadTask *prev = get_current();
    if (prev == NULL) {
        ready_queue->state = TASK_RUNNING;
        set_current(ready_queue);
    }
    else {
        struct ThreadTask *next = pop_thread_task(&ready_queue);

        if (next == NULL || next == prev) return;

        // uart_puts("prev: ");
        // uart_hex((unsigned long)prev);
        // uart_puts(" (");
        // uart_puts(itoa(prev->id));
        // uart_puts("), next: ");
        // uart_hex((unsigned long)next);
        // uart_puts(" (");
        // if (next != NULL) {
        //     uart_puts(itoa(next->id));
        // }
        // else {
        //     uart_puts("NULL");
        // }
        // uart_puts(")\r\n");

        if (prev->state == TASK_RUNNING) {
            prev->state = TASK_READY;
            add_thread_task(&ready_queue, prev);
        }
        else if (prev->state == TASK_BLOCKED) {
            add_thread_task(&wait_queue, prev);
        }
        else if (prev->state == TASK_EXITED) {
            add_thread_task(&zombie_queue, prev);
        }
        else if (prev->state == TASK_READY);
        else {
            uart_puts("Invalid thread state!\n");
            return;
        }

        // print_queue(ready_queue);

        // Switch to the next task
        next->state = TASK_RUNNING;
        cpu_switch_to(prev, next);
    }

    // timer_enable_irq();
    // enable_irq_el1();
    return;
}

void kill_zombies() {
    struct ThreadTask *zombie = pop_thread_task(&zombie_queue);
    while (zombie != NULL) {
        free(zombie->kernel_stack);
        free(zombie->user_stack);
        free(zombie);
        zombie = pop_thread_task(&zombie_queue);
    }
}

void idle() {
    while (1) {
        kill_zombies();
        schedule();
    }
}