#include "signal.h"

void check_pending_signals(struct ThreadTask *task, struct TrapFrame *trapframe) {
    if (task->pending_sig == 0) {
        return;  // No pending signals
    }
    
    for (int sig = 0; sig < SIG_NUM; sig++) {
        if (task->pending_sig & (1 << sig)) {
            task->pending_sig &= ~(1 << sig);  // Clear the signal
            handle_signal(task, sig, trapframe);
        }
    }
}

void sighander_user_wrapper(void (*handler)()) {
    handler();
    sigreturn();
}

void handle_signal(struct ThreadTask *task, int sig, struct TrapFrame *trapframe) {
    uart_puts("[INFO] handle_signal: handling signal ");
    uart_puts(itoa(sig));
    uart_puts(" in pid ");
    uart_puts(itoa(task->id));
    uart_puts("\r\n");
    
    if (sig < 0 || sig >= SIG_NUM) {
        uart_puts("[WARN] handle_signal: invalid signal number\r\n");
        return;
    }
    if (task->sig_handlers[sig] == NULL) {
        uart_puts("[WARN] handle_signal: no handler for signal\r\n");
        return;
    }
    
    if (task->sig_handlers[sig] == default_handler || task->sig_handlers[sig] == default_sigkill_handler) {
        // Default handler, can be run in kernel mode
        uart_puts("[INFO] handle_signal: using default handler\r\n");
        task->sig_handlers[sig](sig);
    }
    else {
        // Custom handler, switch to user mode
        uart_puts("[INFO] handle_signal: using custom handler\r\n");
        memcpy(&task->sig_frame, trapframe, sizeof(struct TrapFrame));

        task->cpu_context.sp = alloc(THREAD_STACK_SIZE) + THREAD_STACK_SIZE;
        task->cpu_context.fp = task->cpu_context.sp;
        task->cpu_context.lr = sighander_user_wrapper;

        asm volatile(
            "mov x5, 0x0\n"
            "msr spsr_el1, x5\n"
            "msr elr_el1, %0\n"
            "msr sp_el0, %1\n"
            "mov x0, %2\n"
            "eret"
            :
            : "r"(task->cpu_context.lr), "r"(task->cpu_context.sp), 
              "r"(task->sig_handlers[sig])
            : "x5"
        );
    }
}

void default_sigkill_handler(int sig) {
    _exit();
}

void default_handler(int sig) {
    uart_puts("[INFO] default_handler called\r\n");
    uart_puts("Signal: ");
    uart_hex(sig);
    uart_puts("\r\n");
}
