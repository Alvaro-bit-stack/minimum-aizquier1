#include "minemu/user_abi.h"

    .syntax unified
    .cpu cortex-a9
    .arm

    .global ioctl_helper
    .type ioctl_helper, %function
ioctl_helper:
    push {r7, lr}                   @ r7 is callee-saved: keep the caller's value
    mov r7, #MINEMU_SYSCALL_IOCTL   @ syscall number
    svc #0                          @ trap; r0-r3 already hold fd, op, a, b
    pop {r7, pc}                    @ restore r7, return with result in r0
    .size ioctl_helper, . - ioctl_helper
