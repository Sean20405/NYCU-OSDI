#include "uart.h"

#define BUFFER_SIZE 4096

char* rx_buffer;   // Ring array
char* tx_buffer;   
unsigned long rx_buffer_head = 0;       // The front of the buffer
unsigned long rx_buffer_tail = 0;        // The index of the next character to be read
unsigned long tx_buffer_head = 0;
unsigned long tx_buffer_tail = 0;


void delay(unsigned int cycles) {
    volatile unsigned int i;
    for (i = 0; i < cycles; i++) {
        asm volatile("nop");
    }
}


void init_uart() {
    register unsigned int selector;

    /* Initialize UART */
    *AUXENB |= 1;
    *AUX_MU_CNTL_REG = 0;  // Disable transmitter and receiver
    *AUX_MU_IER_REG = 0;   // Disable interrupt
    *AUX_MU_LCR_REG = 3;   // Set the data size to 8 bit
    *AUX_MU_MCR_REG = 0;   // Don’t need auto flow control.
    *AUX_MU_BAUD = 270;    // Set baud rate to 115200
    *AUX_MU_IIR_REG = 6;

    /* Setting GPIO pins */
    // Set GPIO14 and GPIO15 to alternative function 5
    selector = *GPFSEL1;
    selector &= (~(7 << 12) | ~(7 << 15));  // Clear GPIO14 and GPIO15
    selector |= ((2 << 12) | (2 << 15));    // Set alternative function 5
    *GPFSEL1 = selector;

    // Disable pull-up/down for GPIO14 and GPIO15
    *GPPUD = 0;
    delay(150);
    *GPPUDCLK0 = (1 << 14) | (1 << 15);
    delay(150);
    *GPPUDCLK0 = 0;

    *AUX_MU_CNTL_REG = 3;  // Enable transmitter and receiver

    rx_buffer = (char*)alloc(BUFFER_SIZE);
    tx_buffer = (char*)alloc(BUFFER_SIZE);
}


// Flush both the receive buffer and the transmit buffer
void uart_flush() {
    // Flust the receive buffer: read all unread data until the buffer is empty
    while (*AUX_MU_LSR_REG & 0x01) {
        (void)(*AUX_MU_IO_REG);
    }
    
    // Flush the transmit buffer: wait until the buffer is empty
    while (!(*AUX_MU_LSR_REG & 0x20)) {
        asm volatile("nop");
    }
}


// Only flush the receive buffer
void uart_flush_rx() {
    while (*AUX_MU_LSR_REG & 0x01) {
        (void)(*AUX_MU_IO_REG);
    }
}


// Only flush the transmit buffer
void uart_flush_tx() {
    while (!(*AUX_MU_LSR_REG & 0x20)) {
        asm volatile("nop");
    }
}


char uart_getc() {
    char ch;
    do { asm volatile("nop"); } while (!(*AUX_MU_LSR_REG & 0x1));
    ch = (char)(*AUX_MU_IO_REG);
    return ch;
}


char *uart_gets(char *buffer) {
    char *ptr = buffer;
    char ch;
    while ((ch = uart_getc()) != '\r') {
        uart_putc(ch);
        *ptr = ch;
        ptr++;
    }
    if (ch == '\r') uart_puts("\r\n");
    *ptr = '\0';
    return buffer;
}


int *uart_getn(char *buffer, unsigned int n) {
    char *ptr = buffer;
    char ch;
    unsigned int i;
    for (i = 0; i < n; i++) {
        ch = uart_getc();
        *ptr = ch;
        ptr++;
    }
    *ptr = '\0';
    return i;
}


void uart_putc(char ch) {
    do { asm volatile("nop"); } while (!(*AUX_MU_LSR_REG & 0x20));
    *AUX_MU_IO_REG = (unsigned int)ch;
}


void uart_puts(char *str) {
    while (*str != '\0') {
        if (*str == '\n') {
            uart_putc('\r');
            uart_putc('\n');
        }
        else uart_putc(*str);
        str++;
    }
}


int uart_putn(char *str, unsigned int n) {
    unsigned int i;
    for (i = 0; i < n; i++) {
        uart_putc(str[i]);
    }
    return i;
}


void uart_hex(unsigned int d) {
    unsigned int n;
    int c;
    uart_puts("0x");
    for (c = 28; c >= 0; c -= 4) {
        n = (d >> c) & 0xF;
        if (n <= 9) uart_putc(n + '0');
        else uart_putc(n - 10 + 'a');
    }
}


void uart_int(int d) {
    char str[12];
    int i = 0;
    if (d < 0) {
        uart_putc('-');
        d = -d;
    }
    do {
        str[i++] = d % 10 + '0';
        d /= 10;
    } while (d > 0);
    while (--i >= 0) {
        uart_putc(str[i]);
    }
}

/* IRQ related */
void uart_enable_irq() {
    *AUX_MU_IER_REG |= 0x03;        // Enable RX and TX interrupts
    *ENABLE_IRQS_1 |= (1 << 29);    // Enable the interrupt line for the UART
}

void uart_enable_rx_irq() {
    *AUX_MU_IER_REG |= 0x01;        // Only enable RX interrupt
    *ENABLE_IRQS_1 |= (1 << 29);
}

void uart_enable_tx_irq() {
    *AUX_MU_IER_REG |= 0x02;        // Only enable TX interrupt
    *ENABLE_IRQS_1 |= (1 << 29);
}


void uart_disable_irq() {
    *AUX_MU_IER_REG &= ~0x03;       // Disable RX and TX interrupts
    *ENABLE_IRQS_1 &= ~(1 << 29);
}

void uart_disable_rx_irq() {
    *AUX_MU_IER_REG &= ~0x01;       // Only disable RX interrupt
}

void uart_disable_tx_irq() {
    *AUX_MU_IER_REG &= ~0x02;       // Only disable TX interrupt
}


void uart_irq_handler() {
    if (*AUX_MU_IIR_REG & 0x04) {  // receive interrupt
        add_task(uart_irq_rx_handler, 0);
        execute_task();
    }
    if (*AUX_MU_IIR_REG & 0x02) {  // transmit interrupt
        add_task(uart_irq_tx_handler, 0);
        execute_task();
    }
}

/**
 * uart_irq_rx_handler - UART RX interrupt handler
 * 
 * This function will be triggered by the RX interrupt of the UART
 * when the RX FIFO is not empty. It will read one character from
 * the RX FIFO and store it in the RX buffer (`rx_buffer`).
 */
void uart_irq_rx_handler() {

    // Check if the buffer is full
    if ((rx_buffer_head + 1) % BUFFER_SIZE == rx_buffer_tail) {
        uart_disable_rx_irq();
    }
    else {
        rx_buffer[rx_buffer_head] = (char)(*AUX_MU_IO_REG);
        rx_buffer_head = (rx_buffer_head + 1) % BUFFER_SIZE;
    }
}


/**
 * uart_irq_tx_handler - UART TX interrupt handler
 * 
 * This function will be triggered by the TX interrupt of the UART
 * when the TX FIFO is empty. It will send one character from the
 * TX buffer (`tx_buffer`) to the TX FIFO.
 */
void uart_irq_tx_handler() {
    // Check if the buffer is empty
    if (tx_buffer_head == tx_buffer_tail) {
        uart_disable_tx_irq();
    }
    else {
        *AUX_MU_IO_REG = tx_buffer[tx_buffer_tail];
        tx_buffer_tail = (tx_buffer_tail + 1) % BUFFER_SIZE;
    }
}


/**
 * uart_async_getc - Asynchronous UART get character
 * 
 * @return: 1 if a character was received, 0 if no character was available
 */
int uart_async_getc(char *ch) {
    // Check if the buffer is empty
    if (rx_buffer_head == rx_buffer_tail) {
        *AUX_MU_IER_REG |= 0x01;  // Enable RX interrupt
        return 0;
    }

    *ch = rx_buffer[rx_buffer_tail];
    rx_buffer_tail = (rx_buffer_tail + 1) % BUFFER_SIZE;
    return 1;
}


/**
 * uart_async_putc - Asynchronous UART put character
 * 
 * @return: 1 if a character was added to the buffer, 0 if the buffer is full
 */
int uart_async_putc(char ch) {
    // Check if the buffer is full
    if ((tx_buffer_head + 1) % BUFFER_SIZE == tx_buffer_tail) {
        uart_enable_tx_irq();  // Buffer is full, enable TX interrupt
        return 0;
    }

    if (ch == '\r') {
        tx_buffer[tx_buffer_head] = '\r';
        tx_buffer_head = (tx_buffer_head + 1) % BUFFER_SIZE;
        tx_buffer[tx_buffer_head] = '\n';
        tx_buffer_head = (tx_buffer_head + 1) % BUFFER_SIZE;
    }
    else{
        tx_buffer[tx_buffer_head] = ch;
        tx_buffer_head = (tx_buffer_head + 1) % BUFFER_SIZE;
    }
    uart_enable_tx_irq();  // Have data to send, enable TX interrupt
    return 1;
}


char* uart_async_gets(char *buffer) {
    char *ptr = buffer;
    char *ch;
    while (uart_async_getc(ch) && *ch != '\r') {
        // uart_putc(ch);
        *ptr = *ch;
        ptr++;
    }
    // if (ch == '\r') uart_puts("\r\n");
    *ptr = '\0';
    return buffer;
}


int uart_async_puts(char *str) {
    while (*str != '\0') {
        int ret;
        if (*str == '\n') {
            ret = uart_async_putc('\r');
            if (!ret) return 0;
            ret = uart_async_putc('\n');
            if (!ret) return 0;
        }
        else {
            ret = uart_async_putc(*str);
            if (!ret) return 0;
        }
        str++;
    }
    return 1;  // Data added to buffer
}

void test_uart_async() {
    uart_puts("Testing async UART...\r\n=========================\r\n");

    uart_enable_rx_irq();
    int ret;
    ret = uart_async_puts("Async UART test started...\r");
    if (ret == 0) {
        uart_disable_irq();
        uart_puts("Async UART test failed...\r\n");
        return;
    }
    ret = uart_async_puts("Press 'q' to exit...\r");
    if (ret == 0) {
        uart_disable_irq();
        uart_puts("Async UART test failed...\r\n");
        return;
    }
    while (1) {
        char ch;
        ret = uart_async_getc(&ch);
        if (ret == 0) {
            continue;  // No data available
        }
        if (ch == 'q') {
            uart_disable_irq();
            uart_puts("Exiting async UART test...\r\n=========================\r\n");
            break;
        }
        ret = uart_async_putc(ch);
        if (ret == 0) {
            uart_disable_irq();
            uart_puts("Async UART test failed...\r\n");
            break;
        }
        // uart_async_puts("\r\n");
    }
}