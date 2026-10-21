#include "protocol.h"

#include <stdbool.h>
#include <stdint.h>

#include "board.h"

#define MAX_TOKENS 4

static bool str_eq(const char *a, const char *b)
{
    while (*a != '\0' && *a == *b) {
        a++;
        b++;
    }
    return *a == *b;
}

/* Append a string to out, truncating if needed; keeps out NUL-terminated. */
static void out_str(char *out, size_t out_len, size_t *pos, const char *s)
{
    while (*s != '\0' && *pos + 1 < out_len) {
        out[(*pos)++] = *s++;
    }
    out[*pos] = '\0';
}

static void out_u32(char *out, size_t out_len, size_t *pos, uint32_t v)
{
    char digits[10];
    int n = 0;

    do {
        digits[n++] = (char)('0' + (v % 10u));
        v /= 10u;
    } while (v != 0u);

    while (n > 0 && *pos + 1 < out_len) {
        out[(*pos)++] = digits[--n];
    }
    out[*pos] = '\0';
}

/* Split s in place on spaces. Returns the token count, or -1 if there are
 * more than max tokens. */
static int tokenize(char *s, char *tok[], int max)
{
    int n = 0;

    while (*s != '\0') {
        while (*s == ' ') {
            *s++ = '\0';
        }
        if (*s == '\0') {
            break;
        }
        if (n == max) {
            return -1;
        }
        tok[n++] = s;
        while (*s != '\0' && *s != ' ') {
            s++;
        }
    }
    return n;
}

static bool parse_led(const char *name, led_t *led)
{
    if (str_eq(name, "RED")) {
        *led = LED_RED;
    } else if (str_eq(name, "GREEN")) {
        *led = LED_GREEN;
    } else if (str_eq(name, "BLUE")) {
        *led = LED_BLUE;
    } else {
        return false;
    }
    return true;
}

void protocol_handle_line(const char *line, char *out, size_t out_len)
{
    char buf[PROTOCOL_MAX_LINE + 1];
    char *tok[MAX_TOKENS];
    size_t pos = 0;
    size_t i = 0;
    int n;

    if (out_len == 0) {
        return;
    }
    out[0] = '\0';

    while (line[i] != '\0' && i < PROTOCOL_MAX_LINE) {
        buf[i] = line[i];
        i++;
    }
    if (line[i] != '\0') {
        out_str(out, out_len, &pos, "ERR LINE_TOO_LONG");
        return;
    }
    buf[i] = '\0';

    n = tokenize(buf, tok, MAX_TOKENS);
    if (n == 0) {
        out_str(out, out_len, &pos, "ERR EMPTY");
        return;
    }
    if (n < 0) {
        out_str(out, out_len, &pos, "ERR BAD_ARGS");
        return;
    }

    if (str_eq(tok[0], "PING")) {
        out_str(out, out_len, &pos, n == 1 ? "PONG" : "ERR BAD_ARGS");
    } else if (str_eq(tok[0], "VERSION")) {
        out_str(out, out_len, &pos, n == 1 ? "VERSION hil-bench-fw " FW_VERSION : "ERR BAD_ARGS");
    } else if (str_eq(tok[0], "LED")) {
        led_t led;
        if (n != 3 || !parse_led(tok[1], &led)) {
            out_str(out, out_len, &pos, "ERR BAD_ARGS");
        } else if (str_eq(tok[2], "ON")) {
            board_led_set(led, true);
            out_str(out, out_len, &pos, "OK");
        } else if (str_eq(tok[2], "OFF")) {
            board_led_set(led, false);
            out_str(out, out_len, &pos, "OK");
        } else {
            out_str(out, out_len, &pos, "ERR BAD_ARGS");
        }
    } else if (str_eq(tok[0], "LEDS")) {
        if (n != 1) {
            out_str(out, out_len, &pos, "ERR BAD_ARGS");
            return;
        }
        out_str(out, out_len, &pos, "LEDS RED=");
        out_str(out, out_len, &pos, board_led_get(LED_RED) ? "1" : "0");
        out_str(out, out_len, &pos, " GREEN=");
        out_str(out, out_len, &pos, board_led_get(LED_GREEN) ? "1" : "0");
        out_str(out, out_len, &pos, " BLUE=");
        out_str(out, out_len, &pos, board_led_get(LED_BLUE) ? "1" : "0");
    } else if (str_eq(tok[0], "UPTIME")) {
        if (n != 1) {
            out_str(out, out_len, &pos, "ERR BAD_ARGS");
            return;
        }
        out_str(out, out_len, &pos, "UPTIME ");
        out_u32(out, out_len, &pos, board_uptime_ms());
    } else if (str_eq(tok[0], "GPIO")) {
        if (n != 1) {
            out_str(out, out_len, &pos, "ERR BAD_ARGS");
            return;
        }
        out_str(out, out_len, &pos, "GPIO STIM=");
        out_str(out, out_len, &pos, board_stim_get() ? "1" : "0");
        out_str(out, out_len, &pos, " RESP=");
        out_str(out, out_len, &pos, board_resp_get() ? "1" : "0");
        out_str(out, out_len, &pos, " EDGES=");
        out_u32(out, out_len, &pos, board_stim_edges());
    } else {
        out_str(out, out_len, &pos, "ERR UNKNOWN_COMMAND");
    }
}
