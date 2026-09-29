#include "minemu/msh.h"
#include "minemu/kprintf.h"
#include "minemu/uart.h"
#define line_size 21

static int str_eq(const char *a, const char *b)
{
    while (*a && *b && *a == *b)
    {
        a++;
        b++;
    }
    return *a == *b;
}

static void execute(const char *line, int len)
{
    char command[21];
    int command_size = 0;
    char buffer[21];
    int buffer_size = 0;
    int word_count = 0;
    for (int i = 0; i < len; i++)
    {
        // skip leading spaces
        if (line[i] == ' ')
        {
            if (word_count == 0 && buffer_size == 0)
            {
                if (command_size > 0)
                {
                    word_count++;
                    command[command_size] = '\0';
                }
                continue;
            }
            if (buffer_size == 0)
            {
                continue;
            }
            else
            {
                if (buffer_size < 20)
                {
                    buffer[buffer_size++] = line[i];
                }

                continue;
            }
        }
        // check if to write to the command buffer
        if (word_count == 0)
        {
            if (command_size < 20)
            {
                command[command_size++] = line[i];
            }
        }
        // already seen what the command is everything else is TEXT
        else
        {
            if (buffer_size < 20)
            {
                buffer[buffer_size++] = line[i];
            }
        }
    }
    command[command_size] = '\0';
    buffer[buffer_size] = '\0';
    if (command_size == 0)
    {
        return;
    }
    if (str_eq(command, "echo"))
    {
        kprintf("%s\n", buffer);
    }
    else
    {
        kprintf("command not found: %s\n", command);
    }
    return;
}

void msh_run(void)
{
    static char line[line_size];
    static int line_len;
    static int too_long;
    kprintf("msh> ");
    while (1)
    {
        // collects the bytes one by one
        int c = uart_getc();
        while (c < 0)
        {
            c = uart_getc();
        }
        if (c == '\n')
        {
            if (too_long)
            {
                kprintf("Input too large\n");
            }
            else
            {
                line[line_len] = '\0';
                execute(line, line_len);
            }

            line_len = 0;
            too_long = 0;
            kprintf("msh> ");
        }
        // handles backspace
        else if ((c == 0x08 || c == 0x7f))
        {
            if (line_len > 0)
            {
                line_len--;
            }
        }
        else
        {
            // making sure of length limit
            if (line_len >= line_size - 1)
            {
                too_long = 1;
            }
            else
            {
                line[line_len++] = (char)c;
            }
        }
    }
}