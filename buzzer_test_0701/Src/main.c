/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Blocking and non-blocking LED examples on PC8
 ******************************************************************************
 */

#include <stdint.h>

/*
 * Blocking and non-blocking behavior run at the same time.
 *
 * - main() only sleeps with WFI.
 * - TIM2 interrupt runs independently every 1 second.
 * - TIM2 interrupt controls PC8 LED without blocking.
 * - USART2 prints "hi"/"bye" 1 second after the LED changes state.
 */

#define SYSCLK_HZ          8000000UL
#define APB1_HZ            SYSCLK_HZ
#define UART_BAUD          9600UL
#define LED_ON_TICKS       3UL
#define LED_OFF_TICKS      3UL
#define TIMER_TICK_MS      1000UL
#define UART_DELAY_TICKS   1UL

/* Set to 1 if LED is wired from 3.3V to PC8 and turns on when PC8 is low. */
#define LED_ACTIVE_LOW     0

#define REG32(addr)        (*(volatile uint32_t *)(addr))

#define RCC_BASE           0x40021000UL
#define RCC_CR             REG32(RCC_BASE + 0x00UL)
#define RCC_CFGR           REG32(RCC_BASE + 0x04UL)
#define RCC_APB2ENR        REG32(RCC_BASE + 0x18UL)
#define RCC_APB1ENR        REG32(RCC_BASE + 0x1CUL)

#define GPIOA_BASE         0x40010800UL
#define GPIOA_CRL          REG32(GPIOA_BASE + 0x00UL)

#define GPIOC_BASE         0x40011000UL
#define GPIOC_CRH          REG32(GPIOC_BASE + 0x04UL)
#define GPIOC_BSRR         REG32(GPIOC_BASE + 0x10UL)

#define TIM2_BASE          0x40000000UL
#define TIM2_CR1           REG32(TIM2_BASE + 0x00UL)
#define TIM2_DIER          REG32(TIM2_BASE + 0x0CUL)
#define TIM2_SR            REG32(TIM2_BASE + 0x10UL)
#define TIM2_EGR           REG32(TIM2_BASE + 0x14UL)
#define TIM2_PSC           REG32(TIM2_BASE + 0x28UL)
#define TIM2_ARR           REG32(TIM2_BASE + 0x2CUL)

#define USART2_BASE        0x40004400UL
#define USART2_SR          REG32(USART2_BASE + 0x00UL)
#define USART2_DR          REG32(USART2_BASE + 0x04UL)
#define USART2_BRR         REG32(USART2_BASE + 0x08UL)
#define USART2_CR1         REG32(USART2_BASE + 0x0CUL)

#define SYST_CSR           REG32(0xE000E010UL)
#define SYST_RVR           REG32(0xE000E014UL)
#define SYST_CVR           REG32(0xE000E018UL)

#define NVIC_ISER0         REG32(0xE000E100UL)
#define NVIC_ISER1         REG32(0xE000E104UL)

enum {
    RCC_CR_HSION          = 1U << 0,
    RCC_CR_HSIRDY         = 1U << 1,
    RCC_APB2ENR_AFIOEN    = 1U << 0,
    RCC_APB2ENR_IOPAEN    = 1U << 2,
    RCC_APB2ENR_IOPCEN    = 1U << 4,
    RCC_APB1ENR_TIM2EN    = 1U << 0,
    RCC_APB1ENR_USART2EN  = 1U << 17,
    TIM_CR1_CEN           = 1U << 0,
    TIM_DIER_UIE          = 1U << 0,
    TIM_SR_UIF            = 1U << 0,
    TIM_EGR_UG            = 1U << 0,
    USART_SR_TXE          = 1U << 7,
    USART_CR1_TE          = 1U << 3,
    USART_CR1_TXEIE       = 1U << 7,
    USART_CR1_UE          = 1U << 13,
    SYST_CSR_ENABLE       = 1U << 0,
    SYST_CSR_CLKSOURCE    = 1U << 2,
    SYST_CSR_COUNTFLAG    = 1U << 16,
    TIM2_IRQ_NUMBER       = 28U,
    USART2_IRQ_NUMBER     = 38U,
};

static volatile uint32_t led_is_on;
static volatile uint32_t led_elapsed_ticks;
static volatile uint32_t led_target_ticks;
static volatile uint32_t uart_delay_ticks;
static volatile uint32_t uart_pending_led_on;
static const char *volatile uart_tx_text;

void SystemInit(void)
{
    RCC_CR |= RCC_CR_HSION;
    while ((RCC_CR & RCC_CR_HSIRDY) == 0U) {
    }

    RCC_CFGR = 0x00000000UL;
}

static void led_gpio_init(void)
{
    RCC_APB2ENR |= RCC_APB2ENR_IOPCEN;

    /* PC8 output, push-pull, 2 MHz. */
    GPIOC_CRH = (GPIOC_CRH & ~(0xFUL << 0)) | (0x2UL << 0);
}

static void led_write(uint32_t on)
{
    uint32_t pin_high = on;

#if LED_ACTIVE_LOW
    pin_high = !on;
#endif

    if (pin_high != 0UL) {
        GPIOC_BSRR = 1UL << 8;
    } else {
        GPIOC_BSRR = 1UL << (8 + 16);
    }

    led_is_on = (on != 0UL);
}

static void nvic_enable_irq(uint32_t irq_number)
{
    if (irq_number < 32UL) {
        NVIC_ISER0 = 1UL << irq_number;
    } else {
        NVIC_ISER1 = 1UL << (irq_number - 32UL);
    }
}

static void uart2_init(void)
{
    RCC_APB2ENR |= RCC_APB2ENR_AFIOEN | RCC_APB2ENR_IOPAEN;
    RCC_APB1ENR |= RCC_APB1ENR_USART2EN;

    /* PA2 = USART2_TX alternate-function push-pull, PA3 = input floating. */
    GPIOA_CRL = (GPIOA_CRL & ~((0xFUL << 8) | (0xFUL << 12)))
              | (0xBUL << 8)
              | (0x4UL << 12);

    USART2_BRR = (APB1_HZ + (UART_BAUD / 2UL)) / UART_BAUD;
    USART2_CR1 = USART_CR1_TE | USART_CR1_UE;

    nvic_enable_irq(USART2_IRQ_NUMBER);
}

static void uart2_write_async(const char *text)
{
    uart_tx_text = text;
    USART2_CR1 |= USART_CR1_TXEIE;
}

static void schedule_led_uart_message(uint32_t led_on)
{
    uart_pending_led_on = (led_on != 0UL);
    uart_delay_ticks = UART_DELAY_TICKS;
}

static void led_state_start(uint32_t on)
{
    led_write(on);
    led_elapsed_ticks = 0UL;
    led_target_ticks = (on != 0UL) ? LED_ON_TICKS : LED_OFF_TICKS;
    schedule_led_uart_message(on);
}

static void tim2_1s_interrupt_init(void)
{
    RCC_APB1ENR |= RCC_APB1ENR_TIM2EN;

    TIM2_CR1 = 0UL;
    TIM2_PSC = (SYSCLK_HZ / 1000UL) - 1UL;
    TIM2_ARR = TIMER_TICK_MS - 1UL;
    TIM2_EGR = TIM_EGR_UG;
    TIM2_SR = 0UL;
    TIM2_DIER = TIM_DIER_UIE;

    nvic_enable_irq(TIM2_IRQ_NUMBER);
    TIM2_CR1 = TIM_CR1_CEN;
}

void TIM2_IRQHandler(void)
{
    if ((TIM2_SR & TIM_SR_UIF) != 0UL) {
        TIM2_SR &= ~TIM_SR_UIF;

        led_elapsed_ticks++;

        if (uart_delay_ticks > 0UL) {
            uart_delay_ticks--;

            if (uart_delay_ticks == 0UL) {
                if (uart_pending_led_on != 0UL) {
                    uart2_write_async("hi\r\n");
                } else {
                    uart2_write_async("bye\r\n");
                }
            }
        }

        if (led_elapsed_ticks >= led_target_ticks) {
            led_state_start(led_is_on == 0UL);
        }
    }
}

void USART2_IRQHandler(void)
{
    if (((USART2_CR1 & USART_CR1_TXEIE) != 0UL) &&
        ((USART2_SR & USART_SR_TXE) != 0UL)) {
        const char ch = *uart_tx_text;

        if (ch != '\0') {
            USART2_DR = (uint32_t)ch;
            uart_tx_text++;
        } else {
            USART2_CR1 &= ~USART_CR1_TXEIE;
        }
    }
}

int main(void)
{
    led_gpio_init();
    uart2_init();

    led_state_start(1UL);
    tim2_1s_interrupt_init();

    for (;;) {
        __asm volatile ("wfi");
    }
}

#if 0
/*
 * Previous active LED PWM fade code before this request.
 */

#include <stdint.h>

#define SYSCLK_HZ          8000000UL
#define TIM3_COUNTER_HZ    1000000UL
#define LED_PWM_HZ         1000UL
#define LED_PWM_PERIOD     (TIM3_COUNTER_HZ / LED_PWM_HZ)
#define LED_MAX_LEVEL      10UL
#define LED_SUBSTEPS       20UL
#define LED_FADE_STEPS     (LED_MAX_LEVEL * LED_SUBSTEPS)
#define LED_FADE_STEP_MS   10UL
#define LED_HOLD_MS        300UL
#define LED_ACTIVE_LOW     0
#define REG32(addr)        (*(volatile uint32_t *)(addr))
#define RCC_BASE           0x40021000UL
#define RCC_CR             REG32(RCC_BASE + 0x00UL)
#define RCC_CFGR           REG32(RCC_BASE + 0x04UL)
#define RCC_APB2ENR        REG32(RCC_BASE + 0x18UL)
#define RCC_APB1ENR        REG32(RCC_BASE + 0x1CUL)
#define AFIO_BASE          0x40010000UL
#define AFIO_MAPR          REG32(AFIO_BASE + 0x04UL)
#define GPIOC_BASE         0x40011000UL
#define GPIOC_CRH          REG32(GPIOC_BASE + 0x04UL)
#define TIM3_BASE          0x40000400UL
#define TIM3_CR1           REG32(TIM3_BASE + 0x00UL)
#define TIM3_EGR           REG32(TIM3_BASE + 0x14UL)
#define TIM3_CCMR2         REG32(TIM3_BASE + 0x1CUL)
#define TIM3_CCER          REG32(TIM3_BASE + 0x20UL)
#define TIM3_PSC           REG32(TIM3_BASE + 0x28UL)
#define TIM3_ARR           REG32(TIM3_BASE + 0x2CUL)
#define TIM3_CCR3          REG32(TIM3_BASE + 0x3CUL)
#define SYST_CSR           REG32(0xE000E010UL)
#define SYST_RVR           REG32(0xE000E014UL)
#define SYST_CVR           REG32(0xE000E018UL)

enum {
    RCC_CR_HSION       = 1U << 0,
    RCC_CR_HSIRDY      = 1U << 1,
    RCC_APB2ENR_AFIOEN = 1U << 0,
    RCC_APB2ENR_IOPCEN = 1U << 4,
    RCC_APB1ENR_TIM3EN = 1U << 1,
    TIM_CR1_CEN        = 1U << 0,
    TIM_CR1_ARPE       = 1U << 7,
    TIM_EGR_UG         = 1U << 0,
    TIM_CCER_CC3E      = 1U << 8,
    SYST_CSR_ENABLE    = 1U << 0,
    SYST_CSR_CLKSOURCE = 1U << 2,
    SYST_CSR_COUNTFLAG = 1U << 16,
};

void SystemInit(void)
{
    RCC_CR |= RCC_CR_HSION;
    while ((RCC_CR & RCC_CR_HSIRDY) == 0U) {
    }

    RCC_CFGR = 0x00000000UL;
}

static void delay_ms(uint32_t ms)
{
    SYST_RVR = (SYSCLK_HZ / 1000UL) - 1UL;
    SYST_CVR = 0UL;
    SYST_CSR = SYST_CSR_CLKSOURCE | SYST_CSR_ENABLE;

    while (ms-- > 0UL) {
        while ((SYST_CSR & SYST_CSR_COUNTFLAG) == 0U) {
        }
    }

    SYST_CSR = 0UL;
}

static void led_pwm_gpio_init(void)
{
    RCC_APB2ENR |= RCC_APB2ENR_AFIOEN | RCC_APB2ENR_IOPCEN;
    AFIO_MAPR = (AFIO_MAPR & ~(3UL << 10)) | (3UL << 10);
    GPIOC_CRH = (GPIOC_CRH & ~(0xFUL << 0)) | (0xBUL << 0);
}

static void led_pwm_init(void)
{
    RCC_APB1ENR |= RCC_APB1ENR_TIM3EN;
    TIM3_CR1 = 0UL;
    TIM3_PSC = (SYSCLK_HZ / TIM3_COUNTER_HZ) - 1UL;
    TIM3_ARR = LED_PWM_PERIOD - 1UL;
    TIM3_CCR3 = 0UL;
    TIM3_CCMR2 = (TIM3_CCMR2 & ~0x00FFUL) | (6UL << 4) | (1UL << 3);
    TIM3_CCER = (TIM3_CCER & ~(3UL << 8)) | TIM_CCER_CC3E;
    TIM3_EGR = TIM_EGR_UG;
    TIM3_CR1 = TIM_CR1_ARPE | TIM_CR1_CEN;
}

static void led_set_brightness_scaled(uint32_t value, uint32_t max_value)
{
    if (max_value == 0UL) {
        return;
    }

    if (value > max_value) {
        value = max_value;
    }

#if LED_ACTIVE_LOW
    value = max_value - value;
#endif

    TIM3_CCR3 = (LED_PWM_PERIOD * value) / max_value;
}

int main(void)
{
    led_pwm_gpio_init();
    led_pwm_init();

    for (;;) {
        for (uint32_t step = 0UL; step <= LED_FADE_STEPS; ++step) {
            led_set_brightness_scaled(step, LED_FADE_STEPS);
            delay_ms(LED_FADE_STEP_MS);
        }

        delay_ms(LED_HOLD_MS);

        for (uint32_t step = LED_FADE_STEPS; step > 0UL; --step) {
            led_set_brightness_scaled(step - 1UL, LED_FADE_STEPS);
            delay_ms(LED_FADE_STEP_MS);
        }

        delay_ms(LED_HOLD_MS);
    }
}
#endif
