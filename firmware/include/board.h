/*
 * Board abstraction layer.
 *
 * Everything above this interface (protocol, main loop) is board-independent.
 * Porting to a new board (e.g. an STM32 Nucleo) means adding a new
 * boards/<name>/ directory that implements these functions.
 */
#ifndef BOARD_H
#define BOARD_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    LED_RED = 0,
    LED_GREEN,
    LED_BLUE,
    LED_COUNT
} led_t;

void board_init(void);

void board_led_set(led_t led, bool on);
bool board_led_get(led_t led);

/* Milliseconds since board_init(), driven by SysTick. Wraps after ~49 days. */
uint32_t board_uptime_ms(void);

/* Blocking transmit of one byte on the host UART. */
void board_uart_putc(char c);

/* Non-blocking receive: returns true and stores the byte if one was available. */
bool board_uart_getc(char *c);

/*
 * GPIO in the loop. The host drives the STIM input; a pin-change interrupt
 * copies its level to the RESP output and counts every STIM edge, so the host
 * can measure how fast the firmware reacts and whether it misses edges.
 */
bool board_stim_get(void);
bool board_resp_get(void);
uint32_t board_stim_edges(void);

#endif /* BOARD_H */
