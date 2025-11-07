#ifndef SYSCALL_H
#define SYSCALL_H

#include "exception.h"
#include "uart.h"
#include "sched.h"
#include "mailbox.h"
#include "exec.h"
#include "signal.h"
#include "dev_framebuffer.h"

#define SYS_GETPID_NUM      0
#define SYS_UART_READ_NUM   1
#define SYS_UART_WRITE_NUM  2
#define SYS_EXEC_NUM        3
#define SYS_FORK_NUM        4
#define SYS_EXIT_NUM        5
#define SYS_MBOX_CALL_NUM   6
#define SYS_KILL_NUM        7
#define SYS_SIGNAL_NUM      8
#define SYS_SIGKILL_NUM     9
#define SYS_SIGRETURE_NUM   10
#define SYS_OPEN_NUM        11
#define SYS_CLOSE_NUM       12
#define SYS_WRITE_NUM       13
#define SYS_READ_NUM        14
#define SYS_MKDIR_NUM       15
#define SYS_MOUNT_NUM       16
#define SYS_CHDIR_NUM       17
#define SYS_LSEEK64_NUM     18
#define SYS_IOCTL_NUM       19

void sys_getpid(struct TrapFrame *trapframe);
void sys_uart_read(struct TrapFrame *trapframe);
void sys_uart_write(struct TrapFrame *trapframe);
void sys_exec(struct TrapFrame *trapframe);
void sys_fork(struct TrapFrame *trapframe);
void sys_exit(struct TrapFrame *trapframe);
void sys_mbox_call(struct TrapFrame *trapframe);
void sys_kill(struct TrapFrame *trapframe);
void sys_signal(struct TrapFrame *trapframe);
void sys_sigkill(struct TrapFrame *trapframe);
void sys_sigreturn(struct TrapFrame *trapframe);
void sys_open(struct TrapFrame *trapframe);
void sys_close(struct TrapFrame *trapframe);
void sys_write(struct TrapFrame *trapframe);
void sys_read(struct TrapFrame *trapframe);
void sys_mkdir(struct TrapFrame *trapframe);
void sys_mount(struct TrapFrame *trapframe);
void sys_chdir(struct TrapFrame *trapframe);
void sys_lseek64(struct TrapFrame *trapframe);
void sys_ioctl(struct TrapFrame *trapframe);

/* Wrapper function for syscall */
int get_pid();
int uart_read(char buf[], int size);
int uart_write(const char buf[], int size);
int exec(const char* name, char *const argv[]);
int fork();
void exit();
int mbox_call(unsigned char ch, unsigned int *mbox);
void kill(int pid);
sighandler_t signal(int sig, sighandler_t handler);
int sigkill(int pid, int sig);
void sigreturn();
int open(const char *pathname, int flags);
int close(int fd);
long write(int fd, const void *buf, unsigned long count);
long read(int fd, void *buf, unsigned long count);
int mkdir(const char *pathname, unsigned mode);
int mount(const char *src, const char *target, const char *filesystem, unsigned long flags, const void *data);
int chdir(const char *path);
long lseek64(int fd, long offset, int whence);
int ioctl(int fd, unsigned long request, void *argp);

#endif /* SYSCALL_H */