#ifndef UART_H
#define UART_H

#include "gpio.h"
#include "exception.h"
#include "tasks.h"

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
int *uart_getn(char *buffer, unsigned int n);  // Read n chars
void uart_putc(char ch);        // Write a char
void uart_puts(char *str);      // Write a string
int uart_putn(char *str, unsigned int n);  // Write n chars
void uart_hex(unsigned int d);  // Write a hex number
void uart_int(int d);           // Write an integer

/* IRQ related */
void uart_enable_irq();
void uart_enable_rx_irq();
void uart_enable_tx_irq();
void uart_disable_irq();
void uart_disable_rx_irq();
void uart_disable_tx_irq();
void uart_irq_handler();
void uart_irq_rx_handler();
void uart_irq_tx_handler();

/* async I/O */
int uart_async_getc(char *ch);
char* uart_async_gets(char *buffer);
int uart_async_putc(char ch);
int uart_async_puts(char *str);
void test_uart_async();

#endif /* UART_H */