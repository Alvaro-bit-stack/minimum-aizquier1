#ifndef MINEMU_USER_ABI_H
#define MINEMU_USER_ABI_H

/*
 * Syscall ABI shared by the kernel and user programs (the hw2 test fixtures
 * include this file by name). Constants plus the user-side ioctl prototype.
 *
 *   r7 = syscall number
 *   r0 = fd, r1 = op, r2/r3 = op-specific arguments
 *   result in r0 (negative on failure)
 */

/* Syscall numbers (r7) */
#define MINEMU_SYSCALL_IOCTL 1

/* Error returns (r0) */
#define MINEMU_ERR_FAILED (-1) /* generic failure / bad syscall */
#define MINEMU_ERR_BAD_FD (-2) /* fd is not a known device */
#define MINEMU_ERR_BAD_OP (-3) /* op not valid for this fd */
#define MINEMU_ERR_FAULT  (-4) /* bad user pointer or length */

/* Device selectors (fd, r0): the device's MMIO base address */
#define MINEMU_FD_RNG   0x10003000
#define MINEMU_FD_UART0 0x10004000
#define MINEMU_FD_UART1 0x10005000
#define MINEMU_FD_TRACE 0x1000f000
#define MINEMU_FD_UART_OUT MINEMU_FD_UART0 /* the console */

/* RNG ops (r1) */
#define MINEMU_IOCTL_RNG_SEED  0x0 /* r2 = unsigned seed;  returns 0 (spec: rng_set_seed) */
#define MINEMU_IOCTL_RNG_NEXT  0x1 /* r2 = unsigned *data; returns 0 (spec: rng_get_data) */
#define MINEMU_IOCTL_RNG_STATE 0x2 /* r2 = unsigned *data; current state, no advance; returns 0 */

/* UART ops (r1), shared by UART0 and UART1 */
#define MINEMU_IOCTL_UART_READ  0x0 /* r2 = char *dst, r3 = int n; returns bytes read */
#define MINEMU_IOCTL_UART_WRITE 0x1 /* r2 = char *src, r3 = int n; returns bytes written */

/* TRACE ops (r1) */
#define MINEMU_IOCTL_TRACE_EVENT 0x0 /* r2 = unsigned value; returns 0 */

#ifndef __ASSEMBLER__
/* User-side wrapper (user/lib). The kernel never defines a function named ioctl. */
int ioctl(int fd, int op, ...);
#endif

#endif /* MINEMU_USER_ABI_H */
