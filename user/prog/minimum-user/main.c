/*
 * msh as a user program (ported from the HW1 kernel msh). All console I/O
 * goes through the ioctl syscall; this code never touches MMIO.
 */
#include <stddef.h>

#include "minemu/user_abi.h"

/*
 * The spec says lines can be up to 20 bytes, but the hw2 tests send longer
 * commands ("echo hello from user mode" is 25 bytes), so lines up to 64
 * bytes are accepted and anything longer is discarded as overlong.
 */
#define LINE_MAX 64

static void write_bytes(const char *s, size_t n)
{
    while (n > 0)
    {
        int written = ioctl(MINEMU_FD_UART_OUT, MINEMU_IOCTL_UART_WRITE, (void *)s, (uint32_t)n);
        if (written <= 0)
        {
            return;
        }
        s += written;
        n -= (size_t)written;
    }
}

static void write_string(const char *s)
{
    size_t n = 0;
    while (s[n] != '\0')
    {
        ++n;
    }
    write_bytes(s, n);
}

/* Finds the next space-delimited word at or after *pos. Returns its length (0 if none). */
static int next_word(const char *line, int len, int *pos, int *start)
{
    while (*pos < len && line[*pos] == ' ')
    {
        ++*pos;
    }
    *start = *pos;
    while (*pos < len && line[*pos] != ' ')
    {
        ++*pos;
    }
    return *pos - *start;
}

static int word_is(const char *word, int n, const char *name)
{
    int i = 0;
    while (i < n && name[i] != '\0' && word[i] == name[i])
    {
        ++i;
    }
    return i == n && name[i] == '\0';
}

static void execute(const char *line, int len)
{
    int pos = 0;
    int start;
    int n = next_word(line, len, &pos, &start);
    if (n == 0)
    {
        return; /* empty or all-space line */
    }

    if (word_is(line + start, n, "echo"))
    {
        /* Arguments joined by single spaces, whatever spacing was typed. */
        int first = 1;
        while ((n = next_word(line, len, &pos, &start)) > 0)
        {
            if (!first)
            {
                write_bytes(" ", 1);
            }
            write_bytes(line + start, (size_t)n);
            first = 0;
        }
        write_bytes("\n", 1);
    }
    else
    {
        write_string("command not found: ");
        write_bytes(line + start, (size_t)n);
        write_bytes("\n", 1);
    }
}

void minemu_user_main(void)
{
    static char line[LINE_MAX];
    int len = 0;
    int too_long = 0;

    write_string("msh> ");
    for (;;)
    {
        char input[32];
        /* uart_read returns whatever is waiting (possibly 0), so this polls. */
        int got = ioctl(MINEMU_FD_UART_IN, MINEMU_IOCTL_UART_READ, (void *)input, (uint32_t)sizeof(input));
        for (int i = 0; i < got; ++i)
        {
            char c = input[i];
            if (c == '\n')
            {
                if (!too_long)
                {
                    execute(line, len);
                }
                len = 0;
                too_long = 0;
                write_string("msh> ");
            }
            else if (c == 0x08 || c == 0x7f)
            {
                if (len > 0)
                {
                    --len;
                }
            }
            else if (len < LINE_MAX)
            {
                line[len++] = c;
            }
            else
            {
                too_long = 1;
            }
        }
    }
}
