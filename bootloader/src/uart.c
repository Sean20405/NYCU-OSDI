#include "uart.h"

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


void uart_dec(unsigned int d) {
    unsigned int n = d, count = 0, divisor = 1;
    char str[10];
    while (n > 0) {
        n /= 10;
        count++;
        divisor *= 10;
    }
    divisor /= 10;
    n = d;
    for (int i = 0; i < count; i++) {
        str[i] = (n / divisor) + '0';
        n %= divisor;
        divisor /= 10;
    }
    str[count] = '\0';
    uart_puts(str);
}