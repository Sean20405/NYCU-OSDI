#ifndef TASK_H
#define TASK_H

#include <stddef.h>
#include "alloc.h"
#include "uart.h"
#include "exception.h"

typedef void (*task_callback)(void);

void add_task(task_callback callback, int priority);
void execute_task();
// void execute_task_preempt();

#endif /* TASK_H */