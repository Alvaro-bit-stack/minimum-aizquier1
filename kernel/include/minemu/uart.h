void uart_putc(char c);
void uart_puts(const char *s);
void putchar_(char c);
void uart_irq_handler(void);
int uart_getc(void);
void uart_init(void);