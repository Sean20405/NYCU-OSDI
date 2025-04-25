// #include "syscall.h"

.global get_pid
get_pid:
    mov x8, 0
    svc #0
    ret

.global uart_read
uart_read:
    mov x8, 1
    svc #0
    ret

.global uart_write
uart_write:
    mov x8, 2
    svc #0
    ret

.global _exec
_exec:
    mov x8, 3
    svc #0
    ret

.global fork
fork:
    mov x8, 4
    svc #0
    ret

.global exit
exit:
    mov x8, 5
    svc #0
    ret

.global mbox_call
mbox_call:
    mov x8, 6
    svc #0
    ret

.global kill
kill:
    mov x8, 7
    svc #0
    ret
