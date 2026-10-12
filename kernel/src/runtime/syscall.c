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

/*
 * fd numbering follows the template's user_abi.h: UART_IN and UART_OUT are the
 * console (UART0), UART_DIAG is UART1.
 */
static int is_uart_fd(uint32_t fd)
{
    return fd == MINEMU_FD_UART_IN || fd == MINEMU_FD_UART_OUT || fd == MINEMU_FD_UART_DIAG;
}

static volatile struct minemu_uart_regs *uart_regs(uint32_t fd)
{
    return fd == MINEMU_FD_UART_DIAG ? MINEMU_UART1 : MINEMU_UART0;
}

static int uart_rx_ready(uint32_t fd)
{
    if (fd != MINEMU_FD_UART_DIAG)
    {
        return uart_has_input(); /* UART0 input is drained into the IRQ ring buffer */
    }
    return (MINEMU_UART1->status & MINEMU_UART_STATUS_RX_READY) != 0;
}

/* One RX byte, or -1 if none waiting. */
static int uart_try_read(uint32_t fd)
{
    if (fd != MINEMU_FD_UART_DIAG)
    {
        return uart_getc();
    }
    if ((MINEMU_UART1->status & MINEMU_UART_STATUS_RX_READY) == 0)
    {
        return -1;
    }
    return (int)(MINEMU_UART1->rx_data & 0xffU);
}

/* At most MINEMU_IOCTL_MAX_TRANSFER bytes per call; callers loop on short counts. */
static uint32_t clamp_transfer(int32_t n)
{
    return n > MINEMU_IOCTL_MAX_TRANSFER ? MINEMU_IOCTL_MAX_TRANSFER : (uint32_t)n;
}

static int32_t sys_uart_read(uint32_t fd, uint32_t dst, int32_t n)
{
    if (n < 0)
    {
        return MINEMU_IOCTL_ERROR;
    }
    uint32_t limit = clamp_transfer(n);
    if (!user_range_ok(dst, limit, MINEMU_PTE_WRITABLE))
    {
        return MINEMU_IOCTL_ERROR; /* checked before any RX byte is consumed */
    }
    uint32_t count = 0;
    while (count < limit)
    {
        int byte = uart_try_read(fd);
        if (byte < 0)
        {
            break; /* return what was available; don't block */
        }
        char c = (char)byte;
        user_copy_out(dst + count, &c, 1);
        ++count;
    }
    return (int32_t)count;
}

static int32_t sys_uart_write(uint32_t fd, uint32_t src, int32_t n)
{
    if (n < 0)
    {
        return MINEMU_IOCTL_ERROR;
    }
    uint32_t limit = clamp_transfer(n);
    if (!user_range_ok(src, limit, MINEMU_PTE_READABLE))
    {
        return MINEMU_IOCTL_ERROR;
    }
    volatile struct minemu_uart_regs *uart = uart_regs(fd);
    for (uint32_t index = 0; index < limit; ++index)
    {
        char c;
        user_copy_in(&c, src + index, 1);
        while ((uart->status & MINEMU_UART_STATUS_TX_READY) == 0)
        {
        }
        uart->tx_data = (uint32_t)(unsigned char)c;
    }
    return (int32_t)limit;
}

static int32_t sys_uart_status(uint32_t fd)
{
    int32_t status = 0;
    if (uart_rx_ready(fd))
    {
        status |= MINEMU_IOCTL_UART_RX_READY;
    }
    if ((uart_regs(fd)->status & MINEMU_UART_STATUS_TX_READY) != 0)
    {
        status |= MINEMU_IOCTL_UART_TX_READY;
    }
    return status;
}

/* Reads an RNG register into *data. Pointer checked first so the RNG never advances on a bad call. */
static int32_t sys_rng_read(uint32_t data, volatile uint32_t *reg)
{
    if (!user_range_ok(data, sizeof(uint32_t), MINEMU_PTE_WRITABLE))
    {
        return MINEMU_IOCTL_ERROR;
    }
    uint32_t value = *reg;
    user_copy_out(data, &value, sizeof(value));
    return 0;
}

/* fd and request are checked here; anything unknown is rejected before any device is touched. */
static int32_t sys_ioctl(uint32_t fd, uint32_t request, uint32_t a, uint32_t b)
{
    if (is_uart_fd(fd))
    {
        switch (request)
        {
        case MINEMU_IOCTL_UART_READ:
            return sys_uart_read(fd, a, (int32_t)b);
        case MINEMU_IOCTL_UART_WRITE:
            return sys_uart_write(fd, a, (int32_t)b);
        case MINEMU_IOCTL_UART_STATUS:
            return sys_uart_status(fd);
        default:
            return MINEMU_IOCTL_ERROR;
        }
    }

    switch (fd)
    {
    case MINEMU_FD_RNG:
        switch (request)
        {
        case MINEMU_IOCTL_RNG_SEED:
            MINEMU_RNG->seed = a;
            return 0;
        case MINEMU_IOCTL_RNG_NEXT:
            return sys_rng_read(a, &MINEMU_RNG->data);
        case MINEMU_IOCTL_RNG_STATE:
            return sys_rng_read(a, &MINEMU_RNG->state);
        default:
            return MINEMU_IOCTL_ERROR;
        }

    case MINEMU_FD_TRACE:
        if (request != MINEMU_IOCTL_TRACE_EVENT)
        {
            return MINEMU_IOCTL_ERROR;
        }
        minemu_trace_event(a);
        return 0;

    default:
        return MINEMU_IOCTL_ERROR; /* unknown fd */
    }
}

struct minemu_trap_frame *minemu_svc_dispatch(struct minemu_trap_frame *frame)
{
    /* Only accept svc #0 executed by a user program in A32 state. */
    if (frame->exception_id != MINEMU_EXCEPTION_SVC ||
        (frame->spsr & CPSR_MODE_MASK) != CPSR_MODE_USR ||
        (frame->spsr & CPSR_THUMB) != 0)
    {
        frame->r[0] = (uint32_t)MINEMU_IOCTL_ERROR;
        return frame;
    }
    /* fault_pc is the svc the CPU just executed, so it's a mapped user code page. */
    if (*(const uint32_t *)(uintptr_t)frame->fault_pc != A32_SVC_ZERO)
    {
        frame->r[0] = (uint32_t)MINEMU_IOCTL_ERROR;
        return frame;
    }

    switch (frame->r[7])
    {
    case MINEMU_SYSCALL_IOCTL:
        frame->r[0] = (uint32_t)sys_ioctl(frame->r[0], frame->r[1], frame->r[2], frame->r[3]);
        break;
    default:
        frame->r[0] = (uint32_t)MINEMU_IOCTL_ERROR; /* unknown syscall number */
        break;
    }
    return frame;
}
