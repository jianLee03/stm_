#include <stdint.h>

#define RCC_CFGR       (*(volatile uint32_t *)0x40021004u)
#define RCC_APB2ENR    (*(volatile uint32_t *)0x40021018u)
#define RCC_APB1ENR    (*(volatile uint32_t *)0x4002101Cu)

#define GPIOA_CRL      (*(volatile uint32_t *)0x40010800u)

#define ADC1_SR        (*(volatile uint32_t *)0x40012400u)
#define ADC1_CR1       (*(volatile uint32_t *)0x40012404u)
#define ADC1_CR2       (*(volatile uint32_t *)0x40012408u)
#define ADC1_SMPR2     (*(volatile uint32_t *)0x40012410u)
#define ADC1_SQR1      (*(volatile uint32_t *)0x4001242Cu)
#define ADC1_SQR3      (*(volatile uint32_t *)0x40012434u)
#define ADC1_DR        (*(volatile uint32_t *)0x4001244Cu)

#define USART2_SR      (*(volatile uint32_t *)0x40004400u)
#define USART2_DR      (*(volatile uint32_t *)0x40004404u)
#define USART2_BRR     (*(volatile uint32_t *)0x40004408u)
#define USART2_CR1     (*(volatile uint32_t *)0x4000440Cu)

#define RCC_APB2ENR_IOPAEN   (1u << 2)
#define RCC_APB2ENR_ADC1EN   (1u << 9)
#define RCC_APB1ENR_USART2EN (1u << 17)

#define ADC_SR_EOC           (1u << 1)
#define ADC_CR2_ADON         (1u << 0)
#define ADC_CR2_CAL          (1u << 2)
#define ADC_CR2_RSTCAL       (1u << 3)
#define ADC_CR2_EXTTRIG      (1u << 20)
#define ADC_CR2_SWSTART      (1u << 22)
#define ADC_CR2_EXTSEL_SW    (7u << 17)

#define USART_SR_TXE         (1u << 7)
#define USART_CR1_TE         (1u << 3)
#define USART_CR1_UE         (1u << 13)

void SystemInit(void)
{
    /* Use the reset clock setup: HSI 8 MHz. */
}

static void delay_ms(uint32_t ms)
{
    while (ms-- > 0u) {
        for (volatile uint32_t i = 0u; i < 1000u; ++i) {
            __asm volatile ("nop");
        }
    }
}

static void usart2_init(void)
{
    RCC_APB2ENR |= RCC_APB2ENR_IOPAEN;
    RCC_APB1ENR |= RCC_APB1ENR_USART2EN;

    /* PA2 = USART2_TX, alternate-function push-pull output, 2 MHz. */
    GPIOA_CRL = (GPIOA_CRL & ~(0xFu << 8)) | (0xAu << 8);

    /* PCLK1 is 8 MHz with the reset clock setup. 8 MHz / 9600 baud = 0x0341. */
    USART2_BRR = 0x0341u;
    USART2_CR1 = USART_CR1_UE | USART_CR1_TE;
}

static void usart2_write_char(char ch)
{
    while ((USART2_SR & USART_SR_TXE) == 0u) {
    }

    USART2_DR = (uint32_t)ch;
}

static void usart2_write_string(const char *text)
{
    while (*text != '\0') {
        usart2_write_char(*text++);
    }
}

static void usart2_write_u32(uint32_t value)
{
    char digits[10];
    uint32_t count = 0u;

    if (value == 0u) {
        usart2_write_char('0');
        return;
    }

    while (value > 0u) {
        digits[count++] = (char)('0' + (value % 10u));
        value /= 10u;
    }

    while (count > 0u) {
        usart2_write_char(digits[--count]);
    }
}

static void adc1_init(void)
{
    RCC_APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_ADC1EN;

    /* ADC clock = PCLK2 / 2. PA0(A0) = analog input. */
    RCC_CFGR &= ~(3u << 14);
    GPIOA_CRL &= ~(0xFu << 0);

    ADC1_CR1 = 0u;
    ADC1_SMPR2 = (ADC1_SMPR2 & ~(7u << 0)) | (7u << 0);
    ADC1_SQR1 = 0u;
    ADC1_SQR3 = 0u;

    ADC1_CR2 = ADC_CR2_ADON;
    delay_ms(1u);

    ADC1_CR2 |= ADC_CR2_RSTCAL;
    while ((ADC1_CR2 & ADC_CR2_RSTCAL) != 0u) {
    }

    ADC1_CR2 |= ADC_CR2_CAL;
    while ((ADC1_CR2 & ADC_CR2_CAL) != 0u) {
    }

    ADC1_CR2 = ADC_CR2_ADON | ADC_CR2_EXTTRIG | ADC_CR2_EXTSEL_SW;
}

static uint16_t adc1_read_a0(void)
{
    ADC1_CR2 |= ADC_CR2_SWSTART;
    while ((ADC1_SR & ADC_SR_EOC) == 0u) {
    }

    return (uint16_t)(ADC1_DR & 0x0FFFu);
}

int main(void)
{
    usart2_init();
    adc1_init();

    usart2_write_string("STM32F103 A0 ADC ready\r\n");

    for (;;) {
        uint32_t adc_raw = adc1_read_a0();
        uint32_t millivolts = (adc_raw * 3300u) / 4095u;

        usart2_write_string("ADC: ");
        usart2_write_u32(adc_raw);
        usart2_write_string(", mV: ");
        usart2_write_u32(millivolts);
        usart2_write_string("\r\n");

        delay_ms(500u);
    }
}