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

#define POT_MAX_OHM 5000u

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
    RCC_APB2ENR |= (1u << 0) | (1u << 2);
    RCC_APB1ENR |= (1u << 17);

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
    RCC_APB2ENR |= (1u << 2) | (1u << 9);

    GPIOA_CRL &= ~0xFu;

    ADC1_SMPR2 = 0x7u;
    ADC1_SQR1 = 0u;
    ADC1_SQR3 = 0u;
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

static uint16_t adc1_read_ch0(void)
{
    ADC1_CR2 |= 1u << 22;
    while ((ADC1_SR & (1u << 1)) == 0u) {
    }
    return (uint16_t)ADC1_DR;
}

int main(void)
{
    uart2_init();
    adc1_init();

    while (1) {
        uint32_t adc = adc1_read_ch0();
        uint32_t ohm = (adc * POT_MAX_OHM) / 4095u;

        uart2_puts("ADC=");
        uart2_put_u32(adc);
        uart2_puts(" R=");
        uart2_put_u32(ohm);
        uart2_puts(" ohm\r\n");

        delay_ms(500);
    }
}
