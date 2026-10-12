#ifndef MINEMU_LOADER_H
#define MINEMU_LOADER_H

#include <stdbool.h>
#include <stdint.h>

#include "minemu/boot.h"

#define USER_STACK_TOP UINT32_C(0x00800000)

/* Map System ROM, find module `name`, and build its address space. Returns entry VA, or 0. */
uint32_t user_load(const struct minemu_boot_info *boot_info, const char *name);

/* Drop to USR mode at entry with the user stack active (user.S). */
void minemu_enter_user(uint32_t entry_vaddr, uint32_t stack_top) __attribute__((noreturn));

/* True if every page of [vaddr, vaddr + length) is mapped for the user with all of pte_flags. */
bool user_range_ok(uint32_t vaddr, uint32_t length, uint32_t pte_flags);

/* Copy between kernel memory and a range already checked with user_range_ok. */
void user_copy_in(void *destination, uint32_t source_vaddr, uint32_t length);
void user_copy_out(uint32_t destination_vaddr, const void *source, uint32_t length);

#endif
