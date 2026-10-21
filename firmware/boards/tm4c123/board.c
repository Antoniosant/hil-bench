/*
 * Board support for the TI Tiva C Series TM4C123G LaunchPad (TM4C123GH6PM,
 * ARM Cortex-M4F). Bare-metal register access, no vendor SDK.
 *
 *   LEDs:     PF1 red, PF3 green, PF2 blue
 *   Host UART: UART0 on PA0 (RX) / PA1 (TX), bridged to USB by the on-board
 *              ICDI debugger (appears as /dev/ttyACM0 on Linux)
 *   GPIO loop: PB4 = STIM input (driven by the host), PB5 = RESP output
 *   Clock:    16 MHz precision internal oscillator (reset default, PLL unused)
 */
#include "board.h"

#define REG(addr) (*(volatile uint32_t *)(addr))

/* System control */
#define SYSCTL_RCGCGPIO REG(0x400FE608u)
#define SYSCTL_RCGCUART REG(0x400FE618u)
#define SYSCTL_PRGPIO   REG(0x400FEA08u)
#define SYSCTL_PRUART   REG(0x400FEA18u)

/* GPIO (APB aperture) */
#define GPIOA_BASE 0x40004000u
#define GPIOB_BASE 0x40005000u
#define GPIOF_BASE 0x40025000u
#define GPIO_DATA_MASKED(base, mask) REG((base) + ((mask) << 2))
#define GPIO_DIR(base)   REG((base) + 0x400u)
#define GPIO_IS(base)    REG((base) + 0x404u)
#define GPIO_IBE(base)   REG((base) + 0x408u)
#define GPIO_IM(base)    REG((base) + 0x410u)
#define GPIO_ICR(base)   REG((base) + 0x41Cu)
#define GPIO_AFSEL(base) REG((base) + 0x420u)
#define GPIO_PDR(base)   REG((base) + 0x514u)
#define GPIO_DEN(base)   REG((base) + 0x51Cu)
#define GPIO_AMSEL(base) REG((base) + 0x528u)
#define GPIO_PCTL(base)  REG((base) + 0x52Cu)

/* UART0 */
#define UART0_BASE 0x4000C000u
#define UART0_DR   REG(UART0_BASE + 0x000u)
#define UART0_FR   REG(UART0_BASE + 0x018u)
#define UART0_IBRD REG(UART0_BASE + 0x024u)
#define UART0_FBRD REG(UART0_BASE + 0x028u)
#define UART0_LCRH REG(UART0_BASE + 0x02Cu)
#define UART0_CTL  REG(UART0_BASE + 0x030u)
#define UART0_CC   REG(UART0_BASE + 0xFC8u)
#define UART_FR_RXFE (1u << 4)
#define UART_FR_TXFF (1u << 5)

/* SysTick */
#define STCTRL    REG(0xE000E010u)
#define STRELOAD  REG(0xE000E014u)
#define STCURRENT REG(0xE000E018u)

/* NVIC */
#define NVIC_EN0 REG(0xE000E100u)
#define IRQ_GPIOB 1u

#define SYSCLK_HZ 16000000u

#define PORT_A (1u << 0)
#define PORT_B (1u << 1)
#define PORT_F (1u << 5)
#define LED_MASK 0x0Eu
#define STIM_PIN (1u << 4)
#define RESP_PIN (1u << 5)

static const uint32_t led_pin[LED_COUNT] = {
    [LED_RED] = 1u << 1,
    [LED_GREEN] = 1u << 3,
    [LED_BLUE] = 1u << 2,
};

static volatile uint32_t uptime_ms;
static volatile uint32_t stim_edges;

void SysTick_Handler(void)
{
    uptime_ms++;
}

void GPIOB_Handler(void)
{
    /* Clear first: clearing as the last write can let the still-pending flag
     * re-enter the handler once more after it returns. */
    GPIO_ICR(GPIOB_BASE) = STIM_PIN;
    GPIO_DATA_MASKED(GPIOB_BASE, RESP_PIN) =
        GPIO_DATA_MASKED(GPIOB_BASE, STIM_PIN) != 0u ? RESP_PIN : 0u;
    stim_edges++;
}

static void leds_init(void)
{
    GPIO_DIR(GPIOF_BASE) |= LED_MASK;
    GPIO_DEN(GPIOF_BASE) |= LED_MASK;
    GPIO_DATA_MASKED(GPIOF_BASE, LED_MASK) = 0u;
}

static void uart0_init(void)
{
    /* PA0/PA1 to their UART0 alternate function (PCTL value 1). */
    GPIO_AFSEL(GPIOA_BASE) |= 0x03u;
    GPIO_PCTL(GPIOA_BASE) = (GPIO_PCTL(GPIOA_BASE) & ~0xFFu) | 0x11u;
    GPIO_DEN(GPIOA_BASE) |= 0x03u;
    GPIO_AMSEL(GPIOA_BASE) &= ~0x03u;

    /* 115200 8N1: divisor = 16 MHz / (16 * 115200) = 8.6806 -> IBRD 8, FBRD round(0.6806 * 64) = 44 */
    UART0_CTL = 0u;
    UART0_CC = 0u;
    UART0_IBRD = 8u;
    UART0_FBRD = 44u;
    UART0_LCRH = (0x3u << 5) | (1u << 4); /* 8-bit words, FIFOs enabled */
    UART0_CTL = (1u << 0) | (1u << 8) | (1u << 9); /* UARTEN, TXE, RXE */
}

static void gpio_loop_init(void)
{
    GPIO_DIR(GPIOB_BASE) = (GPIO_DIR(GPIOB_BASE) & ~STIM_PIN) | RESP_PIN;
    GPIO_PDR(GPIOB_BASE) |= STIM_PIN; /* read 0, not noise, when nothing is wired */
    GPIO_DEN(GPIOB_BASE) |= STIM_PIN | RESP_PIN;
    GPIO_DATA_MASKED(GPIOB_BASE, RESP_PIN) =
        GPIO_DATA_MASKED(GPIOB_BASE, STIM_PIN) != 0u ? RESP_PIN : 0u;

    GPIO_IS(GPIOB_BASE) &= ~STIM_PIN; /* edge-sensitive */
    GPIO_IBE(GPIOB_BASE) |= STIM_PIN; /* both edges */
    GPIO_ICR(GPIOB_BASE) = STIM_PIN;  /* drop anything latched during setup */
    GPIO_IM(GPIOB_BASE) |= STIM_PIN;
    NVIC_EN0 = 1u << IRQ_GPIOB;
}

static void systick_init(void)
{
    STRELOAD = SYSCLK_HZ / 1000u - 1u;
    STCURRENT = 0u;
    STCTRL = (1u << 2) | (1u << 1) | (1u << 0); /* system clock, interrupt, enable */
}

void board_init(void)
{
    SYSCTL_RCGCGPIO |= PORT_A | PORT_B | PORT_F;
    SYSCTL_RCGCUART |= 1u;
    while ((SYSCTL_PRGPIO & (PORT_A | PORT_B | PORT_F)) != (PORT_A | PORT_B | PORT_F)) {
    }
    while ((SYSCTL_PRUART & 1u) == 0u) {
    }

    leds_init();
    uart0_init();
    gpio_loop_init();
    systick_init();
}

void board_led_set(led_t led, bool on)
{
    GPIO_DATA_MASKED(GPIOF_BASE, led_pin[led]) = on ? led_pin[led] : 0u;
}

bool board_led_get(led_t led)
{
    return (GPIO_DATA_MASKED(GPIOF_BASE, led_pin[led]) & led_pin[led]) != 0u;
}

uint32_t board_uptime_ms(void)
{
    return uptime_ms;
}

bool board_stim_get(void)
{
    return GPIO_DATA_MASKED(GPIOB_BASE, STIM_PIN) != 0u;
}

bool board_resp_get(void)
{
    return GPIO_DATA_MASKED(GPIOB_BASE, RESP_PIN) != 0u;
}

uint32_t board_stim_edges(void)
{
    return stim_edges;
}

void board_uart_putc(char c)
{
    while ((UART0_FR & UART_FR_TXFF) != 0u) {
    }
    UART0_DR = (uint32_t)(unsigned char)c;
}

bool board_uart_getc(char *c)
{
    if ((UART0_FR & UART_FR_RXFE) != 0u) {
        return false;
    }
    *c = (char)(UART0_DR & 0xFFu);
    return true;
}
