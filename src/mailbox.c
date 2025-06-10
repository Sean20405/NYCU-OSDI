#include "mailbox.h"

unsigned int mailbox_call(volatile unsigned int *mbox, unsigned char channel) {
    uart_puts("[mailbox_call] called with channel: ");
    uart_hex(channel);
    uart_puts(" and mbox: ");
    uart_hex((unsigned long)mbox);
    uart_puts("\r\n");
    
    unsigned int msg = ((unsigned int)((unsigned long)mbox) & ~0xF) | (channel & 0xF);
    do { asm volatile("nop"); } while (*MAILBOX_STATUS & MAILBOX_FULL);
    *MAILBOX_WRITE = msg;

    do { asm volatile("nop");  } while (*MAILBOX_STATUS & MAILBOX_EMPTY);
    unsigned int res = *MAILBOX_READ;

    if (msg == res) {
        if (mbox[1] & REQUEST_SUCCEED) {
            return 1;
        }
        else {
            uart_puts("[Error] Mailbox request failed: ");
            uart_hex(mbox[1]);
            uart_puts("\r\n");
            return 0;
        }
    }
    else {
        uart_puts("[Error] Mailbox response mismatch: ");
        uart_hex(res);
        uart_puts("\r\n");
        return 0;
    }
}