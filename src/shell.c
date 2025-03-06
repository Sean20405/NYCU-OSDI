#include "shell.h"

void cmd_mbox() {
    uart_puts("Mailbox info:\r\n");

    volatile unsigned int  __attribute__((aligned(16))) mbox[8];

    /* Board revision */
    mbox[0] = 7 * 4;
    mbox[1] = REQUEST_CODE;
    mbox[2] = GET_BOARD_REVISION; // Get Board Revision
    mbox[3] = 4;
    mbox[4] = TAG_REQUEST_CODE;
    mbox[5] = 0;
    mbox[6] = END_TAG;

    unsigned int ret = mailbox_call(mbox, 8);
    if (ret) {
        uart_puts("Board revision: ");
        uart_hex(mbox[5]);
        uart_puts("\r\n");
    }

    /* Get Arm memory */
    mbox[0] = 8 * 4;
    mbox[1] = REQUEST_CODE;
    mbox[2] = GET_ARM_MEMORY;
    mbox[3] = 8;
    mbox[4] = TAG_REQUEST_CODE;
    mbox[5] = 0; // base address
    mbox[6] = 0; // size
    mbox[7] = END_TAG;
    
    ret = mailbox_call(mbox, 8);
    if (ret) {
        uart_puts("ARM memory base address: ");
        uart_hex(mbox[5]);
        uart_puts("\r\n");
        uart_puts("ARM memory size: ");
        uart_hex(mbox[6]);
        uart_puts("\r\n");
    }
}

void shell() {
    char str[32];
    char help_msg[] = "help\t: print this help menu\nhello\t: print Hello World!\nmailbox\t: print hardware's information\n";

    while(1) {
        uart_puts("# ");
        uart_gets(str);
        if (strcmp(str, "help") == 0) {
            uart_puts(help_msg);
        }
        else if (strcmp(str, "hello") == 0) {
            uart_puts("Hello World!\r\n");
        }
        else if (strcmp(str, "mailbox") == 0) {
            cmd_mbox();
        }
        else {
            uart_puts("Command not found: ");
            uart_puts(str);
            uart_puts("\r\n");
        }
    }
}