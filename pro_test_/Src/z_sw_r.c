#include <stdint.h>

#define RCC_CFGR    (*(volatile uint32_t *)0x40021004u)
#define RCC_APB2ENR (*(volatile uint32_t *)0x40021018u)
#define RCC_APB1ENR (*(volatile uint32_t *)0x4002101Cu)

#define GPIOA_CRL (*(volatile uint32_t *)0x40010800u)

#define GPIOC_CRL (*(volatile uint32_t *)0x40011000u)
#define GPIOC_IDR (*(volatile uint32_t *)0x40011008u)
#define GPIOC_ODR (*(volatile uint32_t *)0x4001100Cu)

#define ADC1_SR    (*(volatile uint32_t *)0x40012400u)
#define ADC1_CR2   (*(volatile uint32_t *)0x40012408u)
#define ADC1_SMPR2 (*(volatile uint32_t *)0x40012410u)
#define ADC1_SQR1  (*(volatile uint32_t *)0x4001242Cu)
#define ADC1_SQR3  (*(volatile uint32_t *)0x40012434u)
#define ADC1_DR    (*(volatile uint32_t *)0x4001244Cu)

#define USART2_SR  (*(volatile uint32_t *)0x40004400u)
#define USART2_DR  (*(volatile uint32_t *)0x40004404u)
#define USART2_BRR (*(volatile uint32_t *)0x40004408u)
#define USART2_CR1 (*(volatile uint32_t *)0x4000440Cu)

#define TIM2_CR1   (*(volatile uint32_t *)0x40000000u)
#define TIM2_DIER  (*(volatile uint32_t *)0x4000000Cu)
#define TIM2_SR    (*(volatile uint32_t *)0x40000010u)
#define TIM2_EGR   (*(volatile uint32_t *)0x40000014u)
#define TIM2_CNT   (*(volatile uint32_t *)0x40000024u)
#define TIM2_PSC   (*(volatile uint32_t *)0x40000028u)
#define TIM2_ARR   (*(volatile uint32_t *)0x4000002Cu)

#define NVIC_ISER0 (*(volatile uint32_t *)0xE000E100u)

#define RCC_APB2ENR_AFIOEN   (1u << 0)
#define RCC_APB2ENR_IOPAEN   (1u << 2)
#define RCC_APB2ENR_IOPCEN   (1u << 4)
#define RCC_APB2ENR_ADC1EN   (1u << 9)
#define RCC_APB1ENR_TIM2EN   (1u << 0)
#define RCC_APB1ENR_USART2EN (1u << 17)

#define POT_ADC_CHANNEL      0u
#define SWITCH_PIN           6u
#define USART2_TX_PIN        2u
#define SWITCH_ACTIVE_LOW    1u

#define ADC_MAX_VALUE        4095u
#define POT_MAX_OHM          5000u

#define TIM2_IRQn            28u
#define SWITCH_DEBOUNCE_MS   20u
#define ADC_SAMPLE_MS        20u
#define PRINT_PERIOD_MS      500u

static volatile uint32_t g_ms_ticks;
static volatile uint8_t g_adc_sample_pending = 1u;
static volatile uint8_t g_print_pending;
static volatile uint8_t g_switch_pressed;
static volatile uint16_t g_last_adc;
static volatile uint32_t g_last_ohm;

void SystemInit(void)
{
}

static void timer2_1ms_init(void)
{
    RCC_APB1ENR |= RCC_APB1ENR_TIM2EN;

    TIM2_CR1 = 0u;
    TIM2_PSC = 7u;
    TIM2_ARR = 999u;
    TIM2_CNT = 0u;
    TIM2_EGR = 1u;
    TIM2_SR = 0u;
    TIM2_DIER = 1u;
    NVIC_ISER0 = 1u << TIM2_IRQn;
    TIM2_CR1 = 1u;
}

static void timer_wait_ms(uint32_t ms)
{
    uint32_t start = g_ms_ticks;

    while ((g_ms_ticks - start) < ms) {
    }
}

static void gpio_init(void)
{
    RCC_APB2ENR |= RCC_APB2ENR_AFIOEN | RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPCEN;

    GPIOA_CRL &= ~(0xFu << (POT_ADC_CHANNEL * 4u));

    GPIOA_CRL &= ~(0xFu << (USART2_TX_PIN * 4u));
    GPIOA_CRL |=  (0xAu << (USART2_TX_PIN * 4u));

    GPIOC_CRL &= ~(0xFu << (SWITCH_PIN * 4u));
    GPIOC_CRL |=  (0x8u << (SWITCH_PIN * 4u));
    GPIOC_ODR |= 1u << SWITCH_PIN;
}

static uint8_t switch_raw_pressed(void)
{
    uint8_t high = (GPIOC_IDR & (1u << SWITCH_PIN)) != 0u;

#if SWITCH_ACTIVE_LOW
    return !high;
#else
    return high;
#endif
}

static void uart2_init(void)
{
    RCC_APB1ENR |= RCC_APB1ENR_USART2EN;

    USART2_BRR = 0x0341u;
    USART2_CR1 = (1u << 13) | (1u << 3);
}

static void uart2_putc(char c)
{
    while ((USART2_SR & (1u << 7)) == 0u) {
    }

    USART2_DR = (uint32_t)c;
}

static void uart2_puts(const char *s)
{
    while (*s != '\0') {
        uart2_putc(*s++);
    }
}

static void uart2_put_u32(uint32_t value)
{
    char buf[10];
    uint32_t i = 0u;

    if (value == 0u) {
        uart2_putc('0');
        return;
    }

    while (value > 0u) {
        buf[i++] = (char)('0' + (value % 10u));
        value /= 10u;
    }

    while (i > 0u) {
        uart2_putc(buf[--i]);
    }
}

static void adc1_init(void)
{
    RCC_APB2ENR |= RCC_APB2ENR_ADC1EN;
    RCC_CFGR &= ~(0x3u << 14);

    ADC1_CR2 = 0u;
    ADC1_SMPR2 &= ~(0x7u << (POT_ADC_CHANNEL * 3u));
    ADC1_SMPR2 |=  (0x7u << (POT_ADC_CHANNEL * 3u));
    ADC1_SQR1 = 0u;
    ADC1_SQR3 = POT_ADC_CHANNEL;
    ADC1_CR2 = (7u << 17) | (1u << 20);

    ADC1_CR2 |= 1u << 0;
    timer_wait_ms(2u);

    ADC1_CR2 |= 1u << 3;
    while ((ADC1_CR2 & (1u << 3)) != 0u) {
    }

    ADC1_CR2 |= 1u << 2;
    while ((ADC1_CR2 & (1u << 2)) != 0u) {
    }
}

static uint16_t adc1_read_ch0(void)
{
    ADC1_SR = 0u;
    ADC1_CR2 |= 1u << 22;

    while ((ADC1_SR & (1u << 1)) == 0u) {
    }

    return (uint16_t)(ADC1_DR & 0xFFFFu);
}

static void update_adc_value(uint16_t adc)
{
    g_last_adc = adc;
    g_last_ohm = ((uint32_t)adc * POT_MAX_OHM) / ADC_MAX_VALUE;
}

static void print_pot_value(void)
{
    uart2_puts("ADC=");
    uart2_put_u32(g_last_adc);
    uart2_puts(" R=");
    uart2_put_u32(g_last_ohm);
    uart2_puts(" ohm\r\n");
}

static void update_switch_1ms(void)
{
    static uint8_t last_raw;
    static uint8_t stable_state;
    static uint8_t debounce_ms;
    static uint16_t print_ms;
    uint8_t raw = switch_raw_pressed();

    if (raw == last_raw) {
        if (debounce_ms < SWITCH_DEBOUNCE_MS) {
            debounce_ms++;
        }
    } else {
        last_raw = raw;
        debounce_ms = 0u;
    }

    if ((debounce_ms >= SWITCH_DEBOUNCE_MS) && (stable_state != raw)) {
        stable_state = raw;
        g_switch_pressed = stable_state;

        if (stable_state != 0u) {
            g_print_pending = 1u;
            print_ms = PRINT_PERIOD_MS;
        } else {
            print_ms = 0u;
        }
    }

    if (g_switch_pressed == 0u) {
        return;
    }

    if (print_ms > 0u) {
        print_ms--;
    }

    if (print_ms == 0u) {
        g_print_pending = 1u;
        print_ms = PRINT_PERIOD_MS;
    }
}

void TIM2_IRQHandler(void)
{
    static uint16_t adc_sample_ms = ADC_SAMPLE_MS;

    if ((TIM2_SR & 1u) == 0u) {
        return;
    }

    TIM2_SR &= ~1u;
    g_ms_ticks++;
    update_switch_1ms();

    if (adc_sample_ms > 0u) {
        adc_sample_ms--;
    }

    if (adc_sample_ms == 0u) {
        g_adc_sample_pending = 1u;
        adc_sample_ms = ADC_SAMPLE_MS;
    }
}

int main(void)
{
    gpio_init();
    uart2_init();
    timer2_1ms_init();
    adc1_init();
    update_adc_value(adc1_read_ch0());

    while (1) {
        if (g_adc_sample_pending != 0u) {
            g_adc_sample_pending = 0u;
            update_adc_value(adc1_read_ch0());
        }

        if (g_print_pending != 0u) {
            g_print_pending = 0u;
            if (g_switch_pressed != 0u) {
                update_adc_value(adc1_read_ch0());
                print_pot_value();
            }
        }
    }
}
