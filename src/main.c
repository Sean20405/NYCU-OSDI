#include "uart.h"
#include "shell.h"

void main() {
    init_uart();
    shell();
}