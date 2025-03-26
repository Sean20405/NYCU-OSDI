#ifndef SHELL_H
#define SHELL_H

#include "uart.h"
#include "mailbox.h"
#include "string.h"
#include "power.h"
#include "cpio.h"
#include "alloc.h"
#include "utils.h"
#include <stddef.h>

#define MAX_CMD_LENGTH 64
#define MAX_ARGS 8

struct Command {
    char* name;
    char* args[MAX_ARGS];
    int argc;
};

void cmd_mbox();
int parse_cmd(char *str, struct Command *cmd);
void shell();

#endif /* SHELL_H */