#include "minemu/platform.h"
#include "minemu/uart.h"
#define UART_RX_BUF_SIZE 64
static volatile char rx_buf[UART_RX_BUF_SIZE];
static volatile size_t rx_head;
static volatile size_t rx_tail;

void uart_init(void)
{
    MINEMU_UART0->control = MINEMU_UART_CONTROL_RX_IRQ_ENABLE;
}

void uart_putc(char c)
{
    // Implement the function to send a character via UART
    while (!(MINEMU_UART0->status & MINEMU_UART_STATUS_TX_READY))
    {
    }
    MINEMU_UART0->tx_data = (uint32_t)(unsigned char)c;
}
void uart_puts(const char *s)
{
    int i = 0;
    while (s[i] != '\0')
    {
        uart_putc(s[i]);
        i++;
    }
}
void putchar_(char c)
{
    uart_putc(c);
}

void uart_irq_handler(void)
{
    while (MINEMU_UART0->status & MINEMU_UART_STATUS_RX_READY)
    {
        uint32_t byte = MINEMU_UART0->rx_data;
        size_t next = (rx_head + 1) % UART_RX_BUF_SIZE;
        if (next != rx_tail)
        {
            rx_buf[rx_head] = byte;
            rx_head = next;
        }
    }
}

int uart_getc(void)
{
    if (rx_head == rx_tail)
    {
        return -1;
    }
    char read_byte = rx_buf[rx_tail];
    rx_tail = (rx_tail + 1) % UART_RX_BUF_SIZE;
    return read_byte;
}
