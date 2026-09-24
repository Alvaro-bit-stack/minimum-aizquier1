#include <stdarg.h>
#include "minemu/printf.h"
#include "minemu/kprintf.h"

int kprintf(const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    int ret = vprintf_(fmt, va);
    va_end(va);
    return ret;
}