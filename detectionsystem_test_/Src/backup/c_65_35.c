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

#define TIM2_CR1       (*(volatile uint32_t *)0x40000000u)
#define TIM2_DIER      (*(volatile uint32_t *)0x4000000Cu)
#define TIM2_SR        (*(volatile uint32_t *)0x40000010u)
#define TIM2_EGR       (*(volatile uint32_t *)0x40000014u)
#define TIM2_CNT       (*(volatile uint32_t *)0x40000024u)
#define TIM2_PSC       (*(volatile uint32_t *)0x40000028u)
#define TIM2_ARR       (*(volatile uint32_t *)0x4000002Cu)

#define NVIC_ISER0     (*(volatile uint32_t *)0xE000E100u)

#define RCC_APB2ENR_IOPAEN   (1u << 2)
#define RCC_APB2ENR_ADC1EN   (1u << 9)
#define RCC_APB1ENR_TIM2EN   (1u << 0)
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

#define TIM_CR1_CEN          (1u << 0)
#define TIM_DIER_UIE         (1u << 0)
#define TIM_SR_UIF           (1u << 0)
#define TIM_EGR_UG           (1u << 0)

#define NVIC_IRQ_TIM2        28u

#define GPIO_CRL_PIN_MASK(pin) (0xFu << ((pin) * 4u))

#define ADC_CHANNEL_R        0u
#define ADC_CHANNEL_L        1u
#define ADC_MAX_VALUE        4095u
#define ADC_MAX_SAMPLE_TIME  7u
#define FILTER_SAMPLE_COUNT  10u
#define RISK_PERCENT_MAX     100u
#define RISK_LIGHT_WEIGHT    35u
#define RISK_RESIST_WEIGHT   65u
#define RISK_WEIGHT_TOTAL    (RISK_LIGHT_WEIGHT + RISK_RESIST_WEIGHT)
#define SAMPLE_PERIOD_MS     500u

typedef struct {
    uint16_t samples[FILTER_SAMPLE_COUNT];
    uint32_t sum;
    uint32_t index;
    uint32_t count;
} MovingAverageFilter;

static volatile uint32_t tim2_period_elapsed = 0u;

void SystemInit(void)
{
    /* Keep the reset clock setup: HSI 8 MHz. */
}

static void delay_ms(uint32_t ms)
{
    while (ms-- > 0u) {
        for (volatile uint32_t i = 0u; i < 1000u; ++i) {
            __asm volatile ("nop");
        }
    }
}

static uint32_t moving_average_update(MovingAverageFilter *filter, uint16_t sample)
{
    if (filter->count < FILTER_SAMPLE_COUNT) {
        filter->samples[filter->index] = sample;
        filter->sum += sample;
        filter->count++;
    } else {
        filter->sum -= filter->samples[filter->index];
        filter->samples[filter->index] = sample;
        filter->sum += sample;
    }

    filter->index++;
    if (filter->index >= FILTER_SAMPLE_COUNT) {
        filter->index = 0u;
    }

    return filter->sum / filter->count;
}

static uint32_t calculate_risk_percent(uint32_t light_level,
                                       uint32_t resistance_level)
{
    uint32_t risk_percent;
    const uint32_t weighted_raw = (light_level * RISK_LIGHT_WEIGHT) +
                                  (resistance_level * RISK_RESIST_WEIGHT);

    risk_percent = ((weighted_raw * RISK_PERCENT_MAX) +
                    ((ADC_MAX_VALUE * RISK_WEIGHT_TOTAL) / 2u)) /
                   (ADC_MAX_VALUE * RISK_WEIGHT_TOTAL);
    if (risk_percent > RISK_PERCENT_MAX) {
        risk_percent = RISK_PERCENT_MAX;
    }

    return risk_percent;
}

static void usart2_init(void)
{
    RCC_APB2ENR |= RCC_APB2ENR_IOPAEN;
    RCC_APB1ENR |= RCC_APB1ENR_USART2EN;

    /* PA2 = USART2_TX, alternate-function push-pull output, 2 MHz. */
    GPIOA_CRL = (GPIOA_CRL & ~GPIO_CRL_PIN_MASK(2u)) | (0xAu << 8);

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

static void tim2_init(void)
{
    RCC_APB1ENR |= RCC_APB1ENR_TIM2EN;

    TIM2_CR1 = 0u;
    TIM2_PSC = 8000u - 1u;
    TIM2_ARR = SAMPLE_PERIOD_MS - 1u;
    TIM2_CNT = 0u;
    TIM2_EGR = TIM_EGR_UG;
    TIM2_SR = 0u;
    TIM2_DIER = TIM_DIER_UIE;

    NVIC_ISER0 = (1u << NVIC_IRQ_TIM2);
    TIM2_CR1 = TIM_CR1_CEN;
}

static void adc1_set_channel_sample_time(uint32_t channel)
{
    const uint32_t shift = channel * 3u;

    ADC1_SMPR2 = (ADC1_SMPR2 & ~(7u << shift)) |
                 (ADC_MAX_SAMPLE_TIME << shift);
}

static void adc1_init(void)
{
    RCC_APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_ADC1EN;

    /* ADC clock = PCLK2 / 2. PA1(A1)=L(CDS), PA0(A0)=R(resistance). */
    RCC_CFGR &= ~(3u << 14);
    GPIOA_CRL &= ~(GPIO_CRL_PIN_MASK(0u) | GPIO_CRL_PIN_MASK(1u));

    ADC1_CR1 = 0u;
    adc1_set_channel_sample_time(ADC_CHANNEL_L);
    adc1_set_channel_sample_time(ADC_CHANNEL_R);
    ADC1_SQR1 = 0u;
    ADC1_SQR3 = ADC_CHANNEL_L;

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

static uint16_t adc1_read_channel(uint32_t channel)
{
    ADC1_SQR3 = channel;
    ADC1_CR2 |= ADC_CR2_SWSTART;
    while ((ADC1_SR & ADC_SR_EOC) == 0u) {
    }

    return (uint16_t)(ADC1_DR & 0x0FFFu);
}

static void serial_write_risk(uint32_t light_level,
                              uint32_t resistance_level,
                              uint32_t risk_level)
{
    usart2_write_string("L=");
    usart2_write_u32(light_level);
    usart2_write_string(" R=");
    usart2_write_u32(resistance_level);
    usart2_write_string(" C=");
    usart2_write_u32(risk_level);
    usart2_write_string("\r\n");
}

void TIM2_IRQHandler(void)
{
    if ((TIM2_SR & TIM_SR_UIF) != 0u) {
        TIM2_SR &= ~TIM_SR_UIF;
        tim2_period_elapsed = 1u;
    }
}

int main(void)
{
    MovingAverageFilter light_filter = {0u};
    MovingAverageFilter resistance_filter = {0u};

    usart2_init();
    adc1_init();
    tim2_init();

    usart2_write_string("STM32F103 risk monitor ready\r\n");

    for (;;) {
        uint16_t light_raw;
        uint16_t resistance_raw;
        uint32_t light_level;
        uint32_t resistance_level;
        uint32_t risk_level;

        if (tim2_period_elapsed == 0u) {
            continue;
        }
        tim2_period_elapsed = 0u;

        light_raw = adc1_read_channel(ADC_CHANNEL_L);
        resistance_raw = adc1_read_channel(ADC_CHANNEL_R);

        light_level = moving_average_update(&light_filter, light_raw);
        resistance_level = moving_average_update(&resistance_filter, resistance_raw);
        risk_level = calculate_risk_percent(light_level, resistance_level);

        serial_write_risk(light_level, resistance_level, risk_level);
    }
}
