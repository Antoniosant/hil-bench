/*
 * Line-based command protocol spoken over the host UART.
 *
 * Requests are ASCII lines terminated by '\n' ('\r' is ignored).
 * Every request gets exactly one response line.
 *
 *   PING                          -> PONG
 *   VERSION                       -> VERSION hil-bench-fw <x.y.z>
 *   LED <RED|GREEN|BLUE> <ON|OFF> -> OK
 *   LEDS                          -> LEDS RED=<0|1> GREEN=<0|1> BLUE=<0|1>
 *   UPTIME                        -> UPTIME <milliseconds>
 *   GPIO                          -> GPIO STIM=<0|1> RESP=<0|1> EDGES=<count>
 *
 * Errors: ERR EMPTY, ERR UNKNOWN_COMMAND, ERR BAD_ARGS, ERR LINE_TOO_LONG
 */
#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stddef.h>

#define PROTOCOL_MAX_LINE 64

/* Handle one request line (without terminator) and write the response
 * (without terminator) into out, always NUL-terminated. */
void protocol_handle_line(const char *line, char *out, size_t out_len);

#endif /* PROTOCOL_H */
