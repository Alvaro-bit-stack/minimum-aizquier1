#include "minemu/user_abi.h"

void minemu_user_main(void)
{
    static const char message[] = "hello world\n";
    (void)ioctl(MINEMU_FD_UART_OUT, MINEMU_IOCTL_UART_WRITE, message, sizeof(message) - 1U);

    /* No exit syscall yet: returning would jump to a garbage lr. */
    for (;;)
    {
        __asm__ volatile("nop");
    }
}
