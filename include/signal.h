#ifndef SIGNAL_H
#define SIGNAL_H

#define SIG_NUM 32
#define MAX_PENDING_SIG 32
#define SIGKILL 9

typedef void (*sighandler_t)(int);

#include "sched.h"
#include "syscall.h"
#include "exception.h"

void check_pending_signals(struct ThreadTask *task, struct TrapFrame *trapframe);
void handle_signal(struct ThreadTask *task, int sig, struct TrapFrame *trapframe);

// Default signal handlers
void default_sigkill_handler(int sig);
void default_handler(int sig);

#endif /* SIGNAL_H */