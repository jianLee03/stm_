#include <stdint.h>

#define RCC_APB2ENR (*(volatile uint32_t *)0x40021018u)
#define RCC_APB1ENR (*(volatile uint32_t *)0x4002101Cu)

#define GPIOA_CRL (*(volatile uint32_t *)0x40010800u)

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

#define RCC_APB2ENR_AFIOEN  (1u << 0)
#define RCC_APB2ENR_IOPAEN  (1u << 2)
#define RCC_APB2ENR_ADC1EN  (1u << 9)
#define RCC_APB1ENR_USART2EN (1u << 17)

#define ADC_CHANNEL_CDS 1u
#define ADC_MAX_VALUE   4095u

#define CDS_FIXED_RESISTOR_OHM          10000u
#define CDS_RESISTANCE_AT_10_LUX_OHM    10000u
#define CDS_REFERENCE_LUX               10u
#define CDS_ADC_HIGHER_WHEN_BRIGHT      0u

void SystemInit(void)
{
}

static void delay_ms(uint32_t ms)
{
    while (ms--) {
        for (volatile uint32_t i = 0; i < 1000u; i++) {
        }
    }
}

static void uart2_init(void)
{
    RCC_APB2ENR |= RCC_APB2ENR_AFIOEN | RCC_APB2ENR_IOPAEN;
    RCC_APB1ENR |= RCC_APB1ENR_USART2EN;

    GPIOA_CRL &= ~(0xFu << 8);
    GPIOA_CRL |=  (0xAu << 8);

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
    while (*s) {
        uart2_putc(*s++);
    }
}

static void uart2_put_u32(uint32_t value)
{
    char buf[10];
    uint32_t i = 0;

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
    RCC_APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_ADC1EN;

    GPIOA_CRL &= ~(0xFu << (ADC_CHANNEL_CDS * 4u));

    ADC1_CR2 = 0u;
    ADC1_SMPR2 &= ~(0x7u << (ADC_CHANNEL_CDS * 3u));
    ADC1_SMPR2 |=  (0x7u << (ADC_CHANNEL_CDS * 3u));
    ADC1_SQR1 = 0u;
    ADC1_SQR3 = ADC_CHANNEL_CDS;
    ADC1_CR2 = (7u << 17) | (1u << 20);

    ADC1_CR2 |= 1u << 0;
    delay_ms(1);
    ADC1_CR2 |= 1u << 3;
    while (ADC1_CR2 & (1u << 3)) {
    }
    ADC1_CR2 |= 1u << 2;
    while (ADC1_CR2 & (1u << 2)) {
    }
}

static uint16_t adc1_read_cds(void)
{
    ADC1_CR2 |= 1u << 22;
    while ((ADC1_SR & (1u << 1)) == 0u) {
    }
    return (uint16_t)(ADC1_DR & 0xFFFFu);
}

static uint32_t isqrt_u64(uint64_t value)
{
    uint64_t bit = 1ull << 62;
    uint64_t result = 0;

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

int main(void)
{
    uart2_init();
    adc1_init();

    while (1) {
        uint32_t adc = adc1_read_cds();
        uint32_t lux = adc_to_lux(adc);

        uart2_puts("ADC : ");
        uart2_put_u32(adc);
        uart2_puts("  L: ");
        uart2_put_u32(lux);
        uart2_puts("\r\n");

        delay_ms(500);
    }
}
