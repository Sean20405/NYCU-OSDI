#ifndef UART_H
#define UART_H

#include "gpio.h"

// Define the address of the registers
#define AUXENB          ((volatile unsigned int*)(MMIO_BASE + 0x00215004))
#define AUX_MU_IO_REG   ((volatile unsigned int*)(MMIO_BASE + 0x00215040))
#define AUX_MU_IER_REG  ((volatile unsigned int*)(MMIO_BASE + 0x00215044))
#define AUX_MU_IIR_REG  ((volatile unsigned int*)(MMIO_BASE + 0x00215048))
#define AUX_MU_LCR_REG  ((volatile unsigned int*)(MMIO_BASE + 0x0021504c))
#define AUX_MU_MCR_REG  ((volatile unsigned int*)(MMIO_BASE + 0x00215050))
#define AUX_MU_LSR_REG  ((volatile unsigned int*)(MMIO_BASE + 0x00215054))
#define AUX_MU_CNTL_REG ((volatile unsigned int*)(MMIO_BASE + 0x00215060))
#define AUX_MU_BAUD     ((volatile unsigned int*)(MMIO_BASE + 0x00215068))

void delay(unsigned int cycles);
void init_uart();
void uart_flush();
void uart_flush_rx();
void uart_flush_tx();
char uart_getc();               // Read a char
char *uart_gets(char *buffer);  // Read a string
void uart_putc(char ch);        // Write a char
void uart_puts(char *str);      // Write a string
void uart_putn(char *str, unsigned int n);  // Write n chars
void uart_hex(unsigned int d);  // Write a hex number
void uart_int(int d);           // Write an integer

#endif /* UART_H */