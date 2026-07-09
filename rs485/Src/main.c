/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : RS485 dual-channel TX/RX demo (bare-metal, no HAL/CMSIS)
 *
 * STM32F103, HSI 8MHz (default reset clock, no PLL).
 *
 * RS485 module 1 : USART1 (PA9  = DI/TX, PA10 = RO/RX), DE+RE = PA8, fixed HIGH (TX only)
 * RS485 module 2 : USART3 (PB10 = DI/TX, PB11 = RO/RX), DE+RE = PA1, fixed LOW  (RX only)
 * Debug console  : USART2 (PA2  = TX,    PA3  = RX)     -> Nucleo ST-Link VCP
 *
 * On a Nucleo board, USART2/PA2/PA3 is already hard-wired to the on-board
 * ST-Link virtual COM port (SB13/SB14), so those pins are reserved here as a
 * plain debug console and are NOT shared with either RS485 transceiver.
 *
 * Module 1's transceiver is permanently driver-enabled and only transmits a
 * heartbeat onto the RS485 bus. Module 2's transceiver is permanently
 * receiver-enabled and only listens; whatever it receives on RO/RX is printed
 * out over the USART2 debug console so both TX and RX can be observed from
 * a single serial monitor (the Nucleo VCP).
 ******************************************************************************
 */

#include <stdint.h>
#include <string.h>

/* ------------------------------------------------------------------------ */
/* Minimal register definitions (no CMSIS device header in this project)    */
/* ------------------------------------------------------------------------ */

#define PERIPH_BASE         (0x40000000UL)
#define APB1PERIPH_BASE     (PERIPH_BASE)
#define APB2PERIPH_BASE     (PERIPH_BASE + 0x10000UL)
#define AHBPERIPH_BASE      (PERIPH_BASE + 0x20000UL)

#define RCC_BASE            (AHBPERIPH_BASE + 0x1000UL)
#define GPIOA_BASE          (APB2PERIPH_BASE + 0x0800UL)
#define GPIOB_BASE          (APB2PERIPH_BASE + 0x0C00UL)
#define USART1_BASE         (APB2PERIPH_BASE + 0x3800UL)
#define USART2_BASE         (APB1PERIPH_BASE + 0x4400UL)
#define USART3_BASE         (APB1PERIPH_BASE + 0x4800UL)
#define SYSTICK_BASE        (0xE000E010UL)

typedef struct {
    volatile uint32_t CR;
    volatile uint32_t CFGR;
    volatile uint32_t CIR;
    volatile uint32_t APB2RSTR;
    volatile uint32_t APB1RSTR;
    volatile uint32_t AHBENR;
    volatile uint32_t APB2ENR;
    volatile uint32_t APB1ENR;
} RCC_TypeDef;

typedef struct {
    volatile uint32_t CRL;
    volatile uint32_t CRH;
    volatile uint32_t IDR;
    volatile uint32_t ODR;
    volatile uint32_t BSRR;
    volatile uint32_t BRR;
    volatile uint32_t LCKR;
} GPIO_TypeDef;

typedef struct {
    volatile uint32_t SR;
    volatile uint32_t DR;
    volatile uint32_t BRR;
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t CR3;
    volatile uint32_t GTPR;
} USART_TypeDef;

typedef struct {
    volatile uint32_t CTRL;
    volatile uint32_t LOAD;
    volatile uint32_t VAL;
    volatile uint32_t CALIB;
} SysTick_TypeDef;

#define RCC     ((RCC_TypeDef *)RCC_BASE)
#define GPIOA   ((GPIO_TypeDef *)GPIOA_BASE)
#define GPIOB   ((GPIO_TypeDef *)GPIOB_BASE)
#define USART1  ((USART_TypeDef *)USART1_BASE)
#define USART2  ((USART_TypeDef *)USART2_BASE)
#define USART3  ((USART_TypeDef *)USART3_BASE)
#define SysTick ((SysTick_TypeDef *)SYSTICK_BASE)

#define RCC_APB2ENR_AFIOEN   (1UL << 0)
#define RCC_APB2ENR_IOPAEN   (1UL << 2)
#define RCC_APB2ENR_IOPBEN   (1UL << 3)
#define RCC_APB2ENR_USART1EN (1UL << 14)
#define RCC_APB1ENR_USART2EN (1UL << 17)
#define RCC_APB1ENR_USART3EN (1UL << 18)

#define USART_SR_TC   (1UL << 6)
#define USART_SR_TXE  (1UL << 7)
#define USART_SR_RXNE (1UL << 5)
#define USART_CR1_RE  (1UL << 2)
#define USART_CR1_TE  (1UL << 3)
#define USART_CR1_UE  (1UL << 13)

#define SYSTICK_CTRL_ENABLE    (1UL << 0)
#define SYSTICK_CTRL_CLKSOURCE (1UL << 2)
#define SYSTICK_CTRL_COUNTFLAG (1UL << 16)

#define HSI_CLOCK_HZ 8000000UL /* default reset clock, no PLL configured */

/* ------------------------------------------------------------------------ */
/* RS485 module description                                                 */
/* ------------------------------------------------------------------------ */

typedef struct {
    USART_TypeDef *usart;
    GPIO_TypeDef  *de_re_port;
    uint16_t       de_re_pin; /* pin number (on de_re_port) driving DE+RE */
} RS485_Module;

static const RS485_Module module1 = { USART1, GPIOA, 8 };
static const RS485_Module module2 = { USART3, GPIOA, 1 };

/* ------------------------------------------------------------------------ */
/* SysTick millisecond tick (free-running, non-blocking)                    */
/* ------------------------------------------------------------------------ */

static void systick_start(void)
{
    SysTick->LOAD = (HSI_CLOCK_HZ / 1000UL) - 1UL;
    SysTick->VAL  = 0;
    SysTick->CTRL = SYSTICK_CTRL_CLKSOURCE | SYSTICK_CTRL_ENABLE;
}

/* Returns 1 once per elapsed millisecond; reading CTRL clears COUNTFLAG. */
static int systick_tick_elapsed(void)
{
    return (SysTick->CTRL & SYSTICK_CTRL_COUNTFLAG) != 0;
}

/* ------------------------------------------------------------------------ */
/* GPIO helpers                                                              */
/* ------------------------------------------------------------------------ */

/* CNF/MODE = 2 bits each, 8 pins per 32-bit CRL/CRH register */
static void gpio_set_mode(GPIO_TypeDef *gpio, uint16_t pin, uint32_t mode, uint32_t cnf)
{
    volatile uint32_t *cr = (pin < 8) ? &gpio->CRL : &gpio->CRH;
    uint32_t shift = (pin % 8) * 4;
    uint32_t value = (cnf << 2) | mode;

    *cr &= ~(0xFUL << shift);
    *cr |= (value << shift);
}

static void gpio_write(GPIO_TypeDef *gpio, uint16_t pin, uint8_t state)
{
    if (state) {
        gpio->BSRR = (1UL << pin);
    } else {
        gpio->BSRR = (1UL << (pin + 16));
    }
}

/* ------------------------------------------------------------------------ */
/* Peripheral initialization                                                 */
/* ------------------------------------------------------------------------ */

static void gpio_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN | RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN;

    /* PA9  (USART1_TX, module1) : AF push-pull, 50MHz -> MODE=11, CNF=10 */
    gpio_set_mode(GPIOA, 9, 0x3, 0x2);
    /* PA10 (USART1_RX, module1) : floating input      -> MODE=00, CNF=01 */
    gpio_set_mode(GPIOA, 10, 0x0, 0x1);
    /* PA8  (module1 DE+RE) : output push-pull, 50MHz  -> MODE=11, CNF=00 */
    gpio_set_mode(GPIOA, 8, 0x3, 0x0);

    /* PB10 (USART3_TX, module2) : AF push-pull, 50MHz -> MODE=11, CNF=10 */
    gpio_set_mode(GPIOB, 10, 0x3, 0x2);
    /* PB11 (USART3_RX, module2) : floating input      -> MODE=00, CNF=01 */
    gpio_set_mode(GPIOB, 11, 0x0, 0x1);
    /* PA1  (module2 DE+RE) : output push-pull, 50MHz  -> MODE=11, CNF=00 */
    gpio_set_mode(GPIOA, 1, 0x3, 0x0);

    /* PA2  (USART2_TX, debug console) : AF push-pull, 50MHz -> MODE=11, CNF=10 */
    gpio_set_mode(GPIOA, 2, 0x3, 0x2);
    /* PA3  (USART2_RX, debug console) : floating input      -> MODE=00, CNF=01 */
    gpio_set_mode(GPIOA, 3, 0x0, 0x1);

    /* Fix module 1 permanently in transmit mode, module 2 permanently in receive mode */
    gpio_write(module1.de_re_port, module1.de_re_pin, 1);
    gpio_write(module2.de_re_port, module2.de_re_pin, 0);
}

static void usart_init(USART_TypeDef *usart, uint32_t baud)
{
    uint32_t apb_clock = HSI_CLOCK_HZ; /* no prescalers configured -> APBx = HSI */
    uint32_t usartdiv  = (apb_clock * 25UL) / (4UL * baud); /* fixed-point /100 */
    uint32_t mantissa  = usartdiv / 100UL;
    uint32_t fraction  = ((usartdiv - (mantissa * 100UL)) * 16UL + 50UL) / 100UL;

    usart->BRR = (mantissa << 4) | (fraction & 0xF);
    usart->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
}

static void rs485_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN | RCC_APB1ENR_USART3EN;

    usart_init(USART1, 9600);   /* module 1 (RS485 TX) */
    usart_init(USART2, 115200); /* debug console (VCP) */
    usart_init(USART3, 9600);   /* module 2 (RS485 RX) */
}

/* ------------------------------------------------------------------------ */
/* RS485 transmit / receive                                                  */
/* ------------------------------------------------------------------------ */

/* DE+RE is fixed at init time (module 1 = TX only, module 2 = RX only);
 * this just clocks bytes out over the USART itself. */
static void usart_send(USART_TypeDef *usart, const char *data, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++) {
        while ((usart->SR & USART_SR_TXE) == 0) {
        }
        usart->DR = (uint8_t)data[i];
    }

    while ((usart->SR & USART_SR_TC) == 0) {
    }
}

static int rs485_try_receive(const RS485_Module *mod, uint8_t *byte)
{
    if (mod->usart->SR & USART_SR_RXNE) {
        *byte = (uint8_t)mod->usart->DR;
        return 1;
    }
    return 0;
}

/* ------------------------------------------------------------------------ */
/* Application                                                               */
/* ------------------------------------------------------------------------ */

void SystemInit(void)
{
    /* HSI 8MHz reset default is used as-is; nothing to configure. */
}

/* Send exactly one test byte from module 1 and wait (with a timeout) for
 * module 2 to receive it, so nothing is ever queued up waiting to be read
 * and no overrun can occur. Result is reported on the USART2 debug console. */
static void run_one_byte_test(uint8_t test_byte)
{
    uint8_t rx_byte = 0;
    int received = 0;

    usart_send(module1.usart, (const char *)&test_byte, 1);

    for (uint32_t timeout = 0; timeout < 100000UL; timeout++) {
        if (rs485_try_receive(&module2, &rx_byte)) {
            received = 1;
            break;
        }
    }

    usart_send(USART2, "TX: ", 4);
    usart_send(USART2, (const char *)&test_byte, 1);
    usart_send(USART2, "  RX: ", 6);
    if (received) {
        usart_send(USART2, (const char *)&rx_byte, 1);
    } else {
        usart_send(USART2, "(timeout)", 9);
    }
    usart_send(USART2, "\r\n", 2);
}

int main(void)
{
    gpio_init();
    rs485_init();
    systick_start();

    uint32_t period_timer = 0;
    const uint32_t period_ms = 500;
    uint8_t test_byte = 'A';

    for (;;) {
        if (systick_tick_elapsed()) {
            period_timer += 1;
        }

        if (period_timer >= period_ms) {
            period_timer = 0;

            run_one_byte_test(test_byte);

            test_byte++;
            if (test_byte > 'Z') {
                test_byte = 'A';
            }
        }
    }
}
