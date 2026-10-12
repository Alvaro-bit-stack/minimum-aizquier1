#include <stdarg.h>
#include <stdint.h>

#include "minemu/user_abi.h"

/*
 * Defined in ioctl_trap.S. By the ARM calling convention the four arguments
 * arrive in r0-r3, and the return value is whatever is in r0 on return,
 * which is the kernel's result.
 */
int ioctl_helper(uint32_t fd, uint32_t op, uint32_t a, uint32_t b);

int ioctl(int fd, int op, ...)
{
    va_list args;
    va_start(args, op); /* cursor starts just after `op` */
    uint32_t a = va_arg(args, uint32_t);
    uint32_t b = va_arg(args, uint32_t);
    va_end(args); /* done with the cursor */

    return ioctl_helper((uint32_t)fd, (uint32_t)op, a, b);
}