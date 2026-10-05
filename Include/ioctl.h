#ifndef MSYS_SYSCALL_H
#define MSYS_SYSCALL_H

/*
 * Syscall interface shared by the kernel and user programs.
 * Constants only: no kernel types, no functions.
 *
 * Calling convention:
 *   r7 = syscall number
 *   r0 = fd, r1 = op, r2/r3 = op-specific arguments
 *   return value in r0 (negative on failure)
 */

/* Syscall numbers (r7) */
#define MSYS_IOCTL_SYSCALL 1

/* Error returns (r0) */
#define MSYS_IOCTL_ERROR (-1)  /* generic failure          */
#define MSYS_IOCTL_EBADFD (-2) /* fd is not a known device */
#define MSYS_IOCTL_EBADOP (-3) /* op not valid for this fd */
#define MSYS_IOCTL_EFAULT (-4) /* bad user pointer/range   */

/* Device selectors (fd, r0) */
#define MSYS_FD_RNG 0x10003000
#define MSYS_FD_UART0 0x10004000
#define MSYS_FD_UART1 0x10005000
#define MSYS_FD_TRACE 0x1000f000

/* RNG ops (r1) */
#define MSYS_OP_RNG_SET_SEED 0x0 /* r2 = unsigned int seed;     returns 0 */
#define MSYS_OP_RNG_GET_DATA 0x1 /* r2 = unsigned int *data;    returns 0 */

/* UART ops (r1), shared by UART0 and UART1 */
#define MSYS_OP_UART_READ 0x0  /* r2 = char *dst, r3 = int n; returns bytes read    */
#define MSYS_OP_UART_WRITE 0x1 /* r2 = char *src, r3 = int n; returns bytes written */

/* TRACE ops (r1) */
#define MSYS_OP_TRACE_EVENT 0x0 /* r2 = unsigned int value;    returns 0 */

#endif /* MSYS_SYSCALL_H */