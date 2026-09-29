#include "minemu/trap.h"
#include "minemu/irq_table.h"
#include "minemu/platform.h"

static minemu_irq_handler_t handlers[MINEMU_IRQ_COUNT];

void minemu_irq_register(int32_t id, minemu_irq_handler_t fn)
{
    if (id >= 0 && id < MINEMU_IRQ_COUNT)
    {
        handlers[id] = fn;
    }
}

struct minemu_trap_frame *minemu_irq_dispatch(struct minemu_trap_frame *frame)
{
    int32_t interrupt_id = frame->exception_id;

    if (interrupt_id >= 0 && interrupt_id < MINEMU_IRQ_COUNT && handlers[interrupt_id] != NULL)
    {
        handlers[interrupt_id]();
    }

    MINEMU_INTERRUPT->eoi = (uint32_t)interrupt_id; // writing the source id back to the eoi ends the interrupt

    return frame;
}
