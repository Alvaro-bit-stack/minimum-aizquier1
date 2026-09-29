#ifndef MINEMU_IRQ_TABLE_H
#define MINEMU_IRQ_TABLE_H

#include <stdint.h>

typedef void (*minemu_irq_handler_t)(void);

void minemu_irq_register(int32_t id, minemu_irq_handler_t fn);

#endif