#include <stdint.h>

#include "minemu/loader.h"
#include "minemu/mmu.h"
#include "minemu/platform.h"
#include "minemu/syscall.h"
#include "minemu/trace.h"
#include "minemu/trap.h"
#include "minemu/uart.h"
#include "minemu/user_abi.h"

#define CPSR_MODE_MASK UINT32_C(0x1f)
#define CPSR_MODE_USR UINT32_C(0x10)
#define CPSR_THUMB UINT32_C(0x20)
#define A32_SVC_ZERO UINT32_C(0xef000000)

static volatile struct minemu_uart_regs *uart_regs(uint32_t fd)
{
    return fd == MINEMU_FD_UART0 ? MINEMU_UART0 : MINEMU_UART1;
}

/* One RX byte, or -1 if none waiting. UART0 input arrives via the IRQ ring buffer. */
static int uart_try_read(uint32_t fd)
{
    if (fd == MINEMU_FD_UART0)
    {
        return uart_getc();
    }
    if ((MINEMU_UART1->status & MINEMU_UART_STATUS_RX_READY) == 0)
    {
        return -1;
    }
    return (int)(MINEMU_UART1->rx_data & 0xffU);
}

static int32_t sys_uart_read(uint32_t fd, uint32_t dst, int32_t n)
{
    if (n < 0 || !user_range_ok(dst, (uint32_t)n, MINEMU_PTE_WRITABLE))
    {
        return MINEMU_ERR_FAULT; /* checked before any RX byte is consumed */
    }
    int32_t count = 0;
    while (count < n)
    {
        int byte = uart_try_read(fd);
        if (byte < 0)
        {
            break; /* return what was available; don't block */
        }
        char c = (char)byte;
        user_copy_out(dst + (uint32_t)count, &c, 1);
        ++count;
    }
    return count;
}

static int32_t sys_uart_write(uint32_t fd, uint32_t src, int32_t n)
{
    if (n < 0 || !user_range_ok(src, (uint32_t)n, MINEMU_PTE_READABLE))
    {
        return MINEMU_ERR_FAULT;
    }
    volatile struct minemu_uart_regs *uart = uart_regs(fd);
    for (int32_t index = 0; index < n; ++index)
    {
        char c;
        user_copy_in(&c, src + (uint32_t)index, 1);
        while ((uart->status & MINEMU_UART_STATUS_TX_READY) == 0)
        {
        }
        uart->tx_data = (uint32_t)(unsigned char)c;
    }
    return n;
}

/* Reads an RNG register into *data. Pointer checked first so the RNG never advances on a bad call. */
static int32_t sys_rng_read(uint32_t data, volatile uint32_t *reg)
{
    if (!user_range_ok(data, sizeof(uint32_t), MINEMU_PTE_WRITABLE))
    {
        return MINEMU_ERR_FAULT;
    }
    uint32_t value = *reg;
    user_copy_out(data, &value, sizeof(value));
    return 0;
}

/* fd and op are checked here; anything unknown is rejected before any device is touched. */
static int32_t sys_ioctl(uint32_t fd, uint32_t op, uint32_t a, uint32_t b)
{
    switch (fd)
    {
    case MINEMU_FD_RNG:
        switch (op)
        {
        case MINEMU_IOCTL_RNG_SEED:
            MINEMU_RNG->seed = a;
            return 0;
        case MINEMU_IOCTL_RNG_NEXT:
            return sys_rng_read(a, &MINEMU_RNG->data);
        case MINEMU_IOCTL_RNG_STATE:
            return sys_rng_read(a, &MINEMU_RNG->state);
        default:
            return MINEMU_ERR_BAD_OP;
        }

    case MINEMU_FD_UART0:
    case MINEMU_FD_UART1:
        switch (op)
        {
        case MINEMU_IOCTL_UART_READ:
            return sys_uart_read(fd, a, (int32_t)b);
        case MINEMU_IOCTL_UART_WRITE:
            return sys_uart_write(fd, a, (int32_t)b);
        default:
            return MINEMU_ERR_BAD_OP;
        }

    case MINEMU_FD_TRACE:
        switch (op)
        {
        case MINEMU_IOCTL_TRACE_EVENT:
            minemu_trace_event(a);
            return 0;
        default:
            return MINEMU_ERR_BAD_OP;
        }

    default:
        return MINEMU_ERR_BAD_FD;
    }
}

struct minemu_trap_frame *minemu_svc_dispatch(struct minemu_trap_frame *frame)
{
    /* Only accept svc #0 executed by a user program in A32 state. */
    if (frame->exception_id != MINEMU_EXCEPTION_SVC ||
        (frame->spsr & CPSR_MODE_MASK) != CPSR_MODE_USR ||
        (frame->spsr & CPSR_THUMB) != 0)
    {
        frame->r[0] = (uint32_t)MINEMU_ERR_FAILED;
        return frame;
    }
    /* fault_pc is the svc the CPU just executed, so it's a mapped user code page. */
    if (*(const uint32_t *)(uintptr_t)frame->fault_pc != A32_SVC_ZERO)
    {
        frame->r[0] = (uint32_t)MINEMU_ERR_FAILED;
        return frame;
    }

    switch (frame->r[7])
    {
    case MINEMU_SYSCALL_IOCTL:
        frame->r[0] = (uint32_t)sys_ioctl(frame->r[0], frame->r[1], frame->r[2], frame->r[3]);
        break;
    default:
        frame->r[0] = (uint32_t)MINEMU_ERR_FAILED; /* unknown syscall number */
        break;
    }
    return frame;
}
