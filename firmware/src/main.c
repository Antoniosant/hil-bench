#include <stdbool.h>
#include <stddef.h>

#include "board.h"
#include "protocol.h"

static void uart_puts(const char *s)
{
    while (*s != '\0') {
        board_uart_putc(*s++);
    }
}

int main(void)
{
    char line[PROTOCOL_MAX_LINE + 1];
    char response[96];
    size_t len = 0;
    bool overflow = false;

    board_init();
    uart_puts("READY hil-bench-fw " FW_VERSION "\n");

    for (;;) {
        char c;

        if (!board_uart_getc(&c) || c == '\r') {
            continue;
        }

        if (c == '\n') {
            if (overflow) {
                uart_puts("ERR LINE_TOO_LONG\n");
            } else {
                line[len] = '\0';
                protocol_handle_line(line, response, sizeof response);
                uart_puts(response);
                uart_puts("\n");
            }
            len = 0;
            overflow = false;
        } else if (len < PROTOCOL_MAX_LINE) {
            line[len++] = c;
        } else {
            /* Keep consuming until the newline, then report the error once. */
            overflow = true;
        }
    }
}
