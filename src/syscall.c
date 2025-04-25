void sys_getpid(struct TrapFrame *trapframe) {
    uart_puts("sys_getpid called\r\n");
}

void sys_uart_read(struct TrapFrame *trapframe) {
    uart_puts("sys_uart_read called\r\n");
}

void sys_uart_write(struct TrapFrame *trapframe) {
    uart_puts("sys_uart_write called\r\n");
}

void sys_exec(struct TrapFrame *trapframe) {
    uart_puts("sys_exec called\r\n");
}

void sys_fork(struct TrapFrame *trapframe) {
    uart_puts("sys_fork called\r\n");
}

void sys_exit(struct TrapFrame *trapframe) {
    uart_puts("sys_exit called\r\n");
}

void sys_mbox_call(struct TrapFrame *trapframe) {
    uart_puts("sys_mbox_call called\r\n");
}

void sys_kill(struct TrapFrame *trapframe) {
    uart_puts("sys_kill called\r\n");
}