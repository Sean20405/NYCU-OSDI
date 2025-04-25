#ifndef SYSCALL_H
#define SYSCALL_H

#include "exception.h"
#include "uart.h"

#define SYS_GETPID_NUM      0
#define SYS_UART_READ_NUM   1
#define SYS_UART_WRITE_NUM  2
#define SYS_EXEC_NUM        3
#define SYS_FORK_NUM        4
#define SYS_EXIT_NUM        5
#define SYS_MBOX_CALL_NUM   6
#define SYS_KILL_NUM     7

#ifndef __ASSEMBLY__
extern int get_pid();
extern int uart_read(/*char buf[], size_t size*/);
extern int uart_write(/*const char buf[], size_t size*/);
extern int _exec(/*const char* name, char *const argv[]*/);
extern int fork();
extern void exit();
extern int mbox_call(/*unsigned char ch, unsigned int *mbox*/);
extern void kill(/*int pid*/);
#endif  /* __ASSEMBLY__ */

struct TrapFrame;

void sys_getpid(struct TrapFrame *trapframe);
void sys_uart_read(struct TrapFrame *trapframe);
void sys_uart_write(struct TrapFrame *trapframe);
void sys_exec(struct TrapFrame *trapframe);
void sys_fork(struct TrapFrame *trapframe);
void sys_exit(struct TrapFrame *trapframe);
void sys_mbox_call(struct TrapFrame *trapframe);
void sys_kill(struct TrapFrame *trapframe);

#endif /* SYSCALL_H */