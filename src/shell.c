#include "shell.h"

void cmd_help_msg() {
    uart_puts("help\t: print this help menu\r\n");
    uart_puts("hello\t: print Hello World!\r\n");
    uart_puts("mailbox\t: print hardware's information\r\n");
    uart_puts("cat\t: print the content of a file\r\n");
    uart_puts("ls\t: list all files in the archive\r\n");
    uart_puts("memAlloc\t: allocate memory\r\n");
    uart_puts("reboot\t: reboot the system\r\n");
    return;
}

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

int parse_cmd(char *str, struct Command *cmd) {
    /**
     * Parse the command string and store the command name and arguments in the struct Command
     * 
     * @param str: the command string
     * @param cmd: the struct Command to store the command name and arguments
     * @return
     *     0: success
     *     1: str is NULL
     *     2: too many arguments
     */
    if (str == NULL) return 1;

    cmd->name = strtok(str, ' ');
    cmd->argc = 0;

    char *arg = strtok(NULL, ' ');
    while (arg != NULL) {
        if (cmd->argc >= MAX_ARGS) {
            return 2;
        }
        cmd->args[cmd->argc] = arg;
        cmd->argc++;
        arg = strtok(NULL, ' ');
    }
    return 0;
}

void shell() {
    char raw_cmd[MAX_CMD_LENGTH];
    struct Command cmd;

    while(1) {
        uart_puts("# ");
        uart_gets(raw_cmd);

        raw_cmd[MAX_CMD_LENGTH - 1] = '\0';  // Ensure the command string is null-terminated
        int ret = parse_cmd(raw_cmd, &cmd);
        if (ret == 1) {
            uart_puts("Command is NULL\r\n");
            continue;
        }
        else if (ret == 2) {
            uart_puts("Too many arguments\r\n");
            continue;
        }

        char *cmd_name = cmd.name;
        if (strcmp(cmd_name, "help") == 0) {
            cmd_help_msg();
        }
        else if (strcmp(cmd_name, "hello") == 0) {
            uart_puts("Hello World!\r\n");
        }
        else if (strcmp(cmd_name, "mailbox") == 0) {
            cmd_mbox();
        }
        else if (strcmp(cmd_name, "cat") == 0) {
            char *filename = cmd.args[0];
            cpio_cat(filename);
        }
        else if (strcmp(cmd_name, "ls") == 0) {
            cpio_list();
        }
        else if (strcmp(cmd_name, "memAlloc") == 0) {
            char num_mem[6];
            uart_puts("Allocate memory: ");
            uart_gets(num_mem);
            void *ptr = simple_alloc((unsigned int)atoi(num_mem));
            if (ptr == NULL) {
                uart_puts("Memory allocation failed\r\n");
            }
            else {
                uart_puts("Memory allocated at: ");
                uart_hex((unsigned int)ptr);
                uart_puts("\r\n");
            }
        }
        else if (strcmp(cmd_name, "reboot") == 0) {
            uart_puts("Rebooting...\r\n");
            reset(100);
            return;  // To avoid print the shell prompt after reboot
        }
        else {
            uart_puts("Command not found: ");
            uart_puts(cmd_name);
            uart_puts("\r\n");
        }
    }
}