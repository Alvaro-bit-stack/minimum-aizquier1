#include "minemu/platform.h"
#include "minemu/uart.h"

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
