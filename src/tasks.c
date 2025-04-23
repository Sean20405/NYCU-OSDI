#include "tasks.h"

struct Task {
    struct Task* prev;
    struct Task* next;
    task_callback callback;
    int priority;  // Lower number = higher priority
};

#define MAX_PRIORITY 100

static struct Task* task_head = NULL;
static int curr_priority = MAX_PRIORITY + 1;

void add_task(task_callback callback, int priority) {
    struct Task* new_task = (struct Task*)simple_alloc(sizeof(struct Task));

    if (new_task == NULL) {
        uart_puts("Failed to allocate memory for task\r\n");
        return;
    }

    new_task->callback = callback;
    new_task->priority = priority;

    // Add the new task to the list
    // disable_irq_el1();

    if (task_head == NULL) {  // List is empty
        new_task->prev = NULL;
        new_task->next = NULL;
        task_head = new_task;
    }
    else if (task_head->priority >= new_task->priority) {  // Insert at head
        new_task->next = task_head;
        task_head->prev = new_task;
        new_task->prev = NULL;
        task_head = new_task;
    }
    else {
        struct Task* curr = task_head;
        while (curr->next != NULL && curr->next->priority < new_task->priority) {
            curr = curr->next;
        }
        new_task->next = curr->next;
        new_task->prev = curr;
        curr->next = new_task;
        if (curr->next != NULL) {
            curr->next->prev = new_task;
        }
    }

    // enable_irq_el1();
}


void execute_task() {
    struct Task* curr_task = task_head;
    task_head = task_head->next;
    if (task_head) task_head->prev = NULL;
    curr_task->callback();

    // Free the completed task
}


// void execute_task_preempt() {
//     while (task_head) {
//         struct Task* curr_task = task_head;

//         // Check if the current task has a higher priority than the last executed task
//         if (curr_task->priority < curr_priority) {
//             task_head = task_head->next;
//             if (task_head) task_head->prev = NULL;
//             curr_task->callback();

//             // Free the completed task
//             task_pool_used--;
//         }
//         else {
//             curr_priority = curr_task->priority;
//             break;
//         }
//     }
// }