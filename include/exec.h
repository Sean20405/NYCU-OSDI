#ifndef EXEC_H
#define EXEC_H

#define PROGRAM_STACK_SIZE 4096

#include "uart.h"
#include "cpio.h"
#include "alloc.h"

void exec(char* filename);

#endif /* EXEC_H */