#include "uart.h"
#include "load.h"

void main() {
    init_uart();
    uart_flush();
    load_kernel();
}