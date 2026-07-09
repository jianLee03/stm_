#include <stdint.h>

#define RCC_CFGR    (*(volatile uint32_t *)0x40021004u)
#define RCC_APB2ENR (*(volatile uint32_t *)0x40021018u)
#define RCC_APB1ENR (*(volatile uint32_t *)0x4002101Cu)

#define GPIOA_CRL (*(volatile uint32_t *)0x40010800u)
#define GPIOC_CRH (*(volatile uint32_t *)0x40011004u)
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

#define TIM2_CR1  (*(volatile uint32_t *)0x40000000u)
#define TIM2_DIER (*(volatile uint32_t *)0x4000000Cu)
#define TIM2_SR   (*(volatile uint32_t *)0x40000010u)
#define TIM2_EGR  (*(volatile uint32_t *)0x40000014u)
#define TIM2_CNT  (*(volatile uint32_t *)0x40000024u)
#define TIM2_PSC  (*(volatile uint32_t *)0x40000028u)
#define TIM2_ARR  (*(volatile uint32_t *)0x4000002Cu)

#define NVIC_ISER0 (*(volatile uint32_t *)0xE000E100u)

#define RCC_APB2ENR_AFIOEN   (1u << 0)
#define RCC_APB2ENR_IOPAEN   (1u << 2)
#define RCC_APB2ENR_IOPCEN   (1u << 4)
#define RCC_APB2ENR_ADC1EN   (1u << 9)
#define RCC_APB1ENR_TIM2EN   (1u << 0)
#define RCC_APB1ENR_USART2EN (1u << 17)

#define CDS_ADC_CHANNEL      1u
#define SWITCH_PIN           8u
#define USART2_TX_PIN        2u
#define SWITCH_ACTIVE_LOW    1u

#define ADC_MAX_VALUE        4095u
#define CDS_FIXED_RESISTOR_OHM          10000u
#define CDS_RESISTANCE_AT_10_LUX_OHM    10000u
#define CDS_REFERENCE_LUX               10u
#define CDS_ADC_HIGHER_WHEN_BRIGHT      0u

#define TIM2_IRQn            28u
#define SWITCH_DEBOUNCE_MS   20u
#define SWITCH_FILTER_MAX_MS 50u
#define PRINT_PERIOD_MS      500u
#define ADC_TIMEOUT_LOOPS    100000u

static volatile uint32_t g_ms_ticks;
static volatile uint8_t g_switch_pressed;
static volatile uint8_t g_print_pending;

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
    uint32_t pc8_shift = (SWITCH_PIN - 8u) * 4u;

    RCC_APB2ENR |= RCC_APB2ENR_AFIOEN | RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPCEN;

    GPIOA_CRL &= ~(0xFu << (CDS_ADC_CHANNEL * 4u));

    GPIOA_CRL &= ~(0xFu << (USART2_TX_PIN * 4u));
    GPIOA_CRL |=  (0xAu << (USART2_TX_PIN * 4u));

    GPIOC_CRH &= ~(0xFu << pc8_shift);
    GPIOC_CRH |=  (0x8u << pc8_shift);
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
    ADC1_SMPR2 &= ~(0x7u << (CDS_ADC_CHANNEL * 3u));
    ADC1_SMPR2 |=  (0x7u << (CDS_ADC_CHANNEL * 3u));
    ADC1_SQR1 = 0u;
    ADC1_SQR3 = CDS_ADC_CHANNEL;
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

static uint16_t adc1_read_cds(void)
{
    uint32_t timeout = ADC_TIMEOUT_LOOPS;

    ADC1_SR = 0u;
    ADC1_CR2 |= 1u << 22;

    while (((ADC1_SR & (1u << 1)) == 0u) && (timeout > 0u)) {
        timeout--;
    }

    if (timeout == 0u) {
        return 0u;
    }

    return (uint16_t)(ADC1_DR & 0xFFFFu);
}

static uint32_t isqrt_u64(uint64_t value)
{
    uint64_t bit = 1ull << 62;
    uint64_t result = 0u;

    while (bit > value) {
        bit >>= 2;
    }

    while (bit != 0u) {
        if (value >= result + bit) {
            value -= result + bit;
            result = (result >> 1) + bit;
        } else {
            result >>= 1;
        }
        bit >>= 2;
    }

    return (uint32_t)result;
}

static uint32_t adc_to_lux(uint32_t adc)
{
    uint64_t numerator_adc;
    uint64_t denominator_adc;
    uint64_t ratio_q16;
    uint64_t sqrt_ratio_q8;
    uint64_t lux;

    if (adc == 0u) {
        adc = 1u;
    } else if (adc >= ADC_MAX_VALUE) {
        adc = ADC_MAX_VALUE - 1u;
    }

#if CDS_ADC_HIGHER_WHEN_BRIGHT
    numerator_adc = adc;
    denominator_adc = ADC_MAX_VALUE - adc;
#else
    numerator_adc = ADC_MAX_VALUE - adc;
    denominator_adc = adc;
#endif

    ratio_q16 = ((uint64_t)CDS_RESISTANCE_AT_10_LUX_OHM * numerator_adc * (1ull << 16)) /
                ((uint64_t)CDS_FIXED_RESISTOR_OHM * denominator_adc);
    sqrt_ratio_q8 = isqrt_u64(ratio_q16);
    lux = ((uint64_t)CDS_REFERENCE_LUX * ratio_q16 * sqrt_ratio_q8 + (1ull << 23)) >> 24;

    if (lux > UINT32_MAX) {
        return UINT32_MAX;
    }

    return (uint32_t)lux;
}

static void print_cds_value(uint16_t adc)
{
    uart2_puts("ADC : ");
    uart2_put_u32(adc);
    uart2_puts("  L: ");
    uart2_put_u32(adc_to_lux(adc));
    uart2_puts("\r\n");
}

static void update_switch_1ms(void)
{
    static uint8_t filter_count;
    static uint16_t print_ms;
    uint8_t raw = switch_raw_pressed();

    if (raw != 0u) {
        if (filter_count < SWITCH_FILTER_MAX_MS) {
            filter_count++;
        }
    } else if (filter_count > 0u) {
        filter_count--;
    }

    if ((g_switch_pressed == 0u) && (filter_count >= SWITCH_DEBOUNCE_MS)) {
        g_switch_pressed = 1u;
        g_print_pending = 1u;
        print_ms = PRINT_PERIOD_MS;
    } else if ((g_switch_pressed != 0u) && (filter_count == 0u)) {
        g_switch_pressed = 0u;
        print_ms = 0u;
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
    if ((TIM2_SR & 1u) == 0u) {
        return;
    }

    TIM2_SR &= ~1u;
    g_ms_ticks++;
    update_switch_1ms();
}

int main(void)
{
    gpio_init();
    uart2_init();
    timer2_1ms_init();
    adc1_init();

    while (1) {
        if (g_print_pending != 0u) {
            g_print_pending = 0u;

            if (g_switch_pressed != 0u) {
                print_cds_value(adc1_read_cds());
            }
        }
    }
}
