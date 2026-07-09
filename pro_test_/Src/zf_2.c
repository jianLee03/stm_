#include <stdint.h>

#define RCC_CFGR    (*(volatile uint32_t *)0x40021004u)
#define RCC_APB2ENR (*(volatile uint32_t *)0x40021018u)
#define RCC_APB1ENR (*(volatile uint32_t *)0x4002101Cu)

#define GPIOA_CRL (*(volatile uint32_t *)0x40010800u)
#define GPIOC_CRL (*(volatile uint32_t *)0x40011000u)
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

#define RCC_CFGR_ADCPRE_MASK   (0x3u << 14)
#define RCC_APB2ENR_AFIOEN     (1u << 0)
#define RCC_APB2ENR_IOPAEN     (1u << 2)
#define RCC_APB2ENR_IOPCEN     (1u << 4)
#define RCC_APB2ENR_ADC1EN     (1u << 9)
#define RCC_APB1ENR_TIM2EN     (1u << 0)
#define RCC_APB1ENR_USART2EN   (1u << 17)

#define GPIO_CONFIG_MASK       0xFu
#define GPIO_CONFIG_ANALOG     0x0u
#define GPIO_CONFIG_INPUT_PULL 0x8u
#define GPIO_CONFIG_AF_PP_2MHZ 0xAu

#define ADC_SR_EOC                 (1u << 1)
#define ADC_CR2_ADON               (1u << 0)
#define ADC_CR2_CAL                (1u << 2)
#define ADC_CR2_RSTCAL             (1u << 3)
#define ADC_CR2_EXTSEL_SWSTART     (7u << 17)
#define ADC_CR2_EXTTRIG            (1u << 20)
#define ADC_CR2_SWSTART            (1u << 22)
#define ADC_SAMPLE_239_5_CYCLES    7u

#define USART_SR_TXE       (1u << 7)
#define USART_CR1_TE       (1u << 3)
#define USART_CR1_UE       (1u << 13)
#define USART2_BRR_9600    0x0341u

#define TIM_CR1_CEN        (1u << 0)
#define TIM_DIER_UIE       (1u << 0)
#define TIM_SR_UIF         (1u << 0)
#define TIM_EGR_UG         (1u << 0)
#define TIM2_IRQn          28u
#define TIM2_PSC_1MHZ      7u
#define TIM2_ARR_1MS       999u

#define POT_ADC_CHANNEL    0u
#define CDS_ADC_CHANNEL    1u
#define USART2_TX_PIN      2u
#define SWITCH_POT_PIN     6u
#define SWITCH_CDS_PIN     8u
#define SWITCH_ACTIVE_LOW  1u

#define SWITCH_POT_MASK    (1u << 0)
#define SWITCH_CDS_MASK    (1u << 1)

#define ADC_MAX_VALUE      4095u
#define POT_MAX_OHM        5000u

#define CDS_FIXED_RESISTOR_OHM        10000u
#define CDS_RESISTANCE_AT_10_LUX_OHM  10000u
#define CDS_REFERENCE_LUX             10u
#define CDS_ADC_HIGHER_WHEN_BRIGHT    0u

#define SWITCH_DEBOUNCE_MS    20u
#define SWITCH_FILTER_MAX_MS  50u
#define PRINT_PERIOD_MS       500u
#define ADC_TIMEOUT_LOOPS     100000u

typedef struct {
    uint8_t filter_ms;
    uint8_t pressed;
} switch_filter_t;

static volatile uint32_t g_ms_ticks;
static volatile uint8_t g_switch_state;
static volatile uint8_t g_print_requested;

void SystemInit(void)
{
}

static void gpio_config_pin(volatile uint32_t *control_register, uint32_t pin, uint32_t config)
{
    uint32_t shift = pin * 4u;

    *control_register &= ~(GPIO_CONFIG_MASK << shift);
    *control_register |= config << shift;
}

static void timer2_start_1ms_interrupt(void)
{
    RCC_APB1ENR |= RCC_APB1ENR_TIM2EN;

    TIM2_CR1 = 0u;
    TIM2_PSC = TIM2_PSC_1MHZ;
    TIM2_ARR = TIM2_ARR_1MS;
    TIM2_CNT = 0u;
    TIM2_EGR = TIM_EGR_UG;
    TIM2_SR = 0u;
    TIM2_DIER = TIM_DIER_UIE;
    NVIC_ISER0 = 1u << TIM2_IRQn;
    TIM2_CR1 = TIM_CR1_CEN;
}

static void timer_delay_ms(uint32_t delay_ms)
{
    uint32_t start_ms = g_ms_ticks;

    while ((g_ms_ticks - start_ms) < delay_ms) {
    }
}

static void gpio_init(void)
{
    RCC_APB2ENR |= RCC_APB2ENR_AFIOEN | RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPCEN;

    gpio_config_pin(&GPIOA_CRL, POT_ADC_CHANNEL, GPIO_CONFIG_ANALOG);
    gpio_config_pin(&GPIOA_CRL, CDS_ADC_CHANNEL, GPIO_CONFIG_ANALOG);
    gpio_config_pin(&GPIOA_CRL, USART2_TX_PIN, GPIO_CONFIG_AF_PP_2MHZ);

    gpio_config_pin(&GPIOC_CRL, SWITCH_POT_PIN, GPIO_CONFIG_INPUT_PULL);
    gpio_config_pin(&GPIOC_CRH, SWITCH_CDS_PIN - 8u, GPIO_CONFIG_INPUT_PULL);
    GPIOC_ODR |= (1u << SWITCH_POT_PIN) | (1u << SWITCH_CDS_PIN);
}

static uint8_t switch_raw_pressed(uint32_t pin)
{
    uint8_t high = (GPIOC_IDR & (1u << pin)) != 0u;

#if SWITCH_ACTIVE_LOW
    return !high;
#else
    return high;
#endif
}

static void uart2_init(void)
{
    RCC_APB1ENR |= RCC_APB1ENR_USART2EN;

    USART2_BRR = USART2_BRR_9600;
    USART2_CR1 = USART_CR1_UE | USART_CR1_TE;
}

static void uart2_putc(char c)
{
    while ((USART2_SR & USART_SR_TXE) == 0u) {
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
    char digits[10];
    uint32_t count = 0u;

    if (value == 0u) {
        uart2_putc('0');
        return;
    }

    while (value > 0u) {
        digits[count++] = (char)('0' + (value % 10u));
        value /= 10u;
    }

    while (count > 0u) {
        uart2_putc(digits[--count]);
    }
}

static void adc_set_sample_time(uint32_t channel)
{
    uint32_t shift = channel * 3u;

    ADC1_SMPR2 &= ~(0x7u << shift);
    ADC1_SMPR2 |= ADC_SAMPLE_239_5_CYCLES << shift;
}

static void adc1_init(void)
{
    RCC_APB2ENR |= RCC_APB2ENR_ADC1EN;
    RCC_CFGR &= ~RCC_CFGR_ADCPRE_MASK;

    ADC1_CR2 = 0u;
    adc_set_sample_time(POT_ADC_CHANNEL);
    adc_set_sample_time(CDS_ADC_CHANNEL);
    ADC1_SQR1 = 0u;
    ADC1_SQR3 = POT_ADC_CHANNEL;
    ADC1_CR2 = ADC_CR2_EXTSEL_SWSTART | ADC_CR2_EXTTRIG;

    ADC1_CR2 |= ADC_CR2_ADON;
    timer_delay_ms(2u);

    ADC1_CR2 |= ADC_CR2_RSTCAL;
    while ((ADC1_CR2 & ADC_CR2_RSTCAL) != 0u) {
    }

    ADC1_CR2 |= ADC_CR2_CAL;
    while ((ADC1_CR2 & ADC_CR2_CAL) != 0u) {
    }
}

static uint16_t adc1_read_channel(uint32_t channel)
{
    uint32_t timeout = ADC_TIMEOUT_LOOPS;

    ADC1_SQR3 = channel;
    ADC1_SR = 0u;
    ADC1_CR2 |= ADC_CR2_SWSTART;

    while (((ADC1_SR & ADC_SR_EOC) == 0u) && (timeout > 0u)) {
        timeout--;
    }

    if (timeout == 0u) {
        return 0u;
    }

    return (uint16_t)(ADC1_DR & 0xFFFFu);
}

static uint32_t adc_to_ohm(uint32_t adc)
{
    return (adc * POT_MAX_OHM) / ADC_MAX_VALUE;
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

static void print_pot_value(uint16_t adc)
{
    uart2_puts("VR ADC=");
    uart2_put_u32(adc);
    uart2_puts(" R=");
    uart2_put_u32(adc_to_ohm(adc));
    uart2_puts(" ohm");
}

static void print_cds_value(uint16_t adc)
{
    uart2_puts("CDS ADC=");
    uart2_put_u32(adc);
    uart2_puts(" L=");
    uart2_put_u32(adc_to_lux(adc));
    uart2_puts(" lux");
}

static uint8_t switch_filter_update(switch_filter_t *filter, uint8_t raw_pressed)
{
    if (raw_pressed != 0u) {
        if (filter->filter_ms < SWITCH_FILTER_MAX_MS) {
            filter->filter_ms++;
        }
    } else if (filter->filter_ms > 0u) {
        filter->filter_ms--;
    }

    if ((filter->pressed == 0u) && (filter->filter_ms >= SWITCH_DEBOUNCE_MS)) {
        filter->pressed = 1u;
    } else if ((filter->pressed != 0u) && (filter->filter_ms == 0u)) {
        filter->pressed = 0u;
    }

    return filter->pressed;
}

static void switches_update_1ms(void)
{
    static switch_filter_t pot_switch;
    static switch_filter_t cds_switch;
    static uint16_t print_timer_ms;
    uint8_t new_state = 0u;

    if (switch_filter_update(&pot_switch, switch_raw_pressed(SWITCH_POT_PIN)) != 0u) {
        new_state |= SWITCH_POT_MASK;
    }

    if (switch_filter_update(&cds_switch, switch_raw_pressed(SWITCH_CDS_PIN)) != 0u) {
        new_state |= SWITCH_CDS_MASK;
    }

    if (new_state != g_switch_state) {
        g_switch_state = new_state;

        if (new_state != 0u) {
            g_print_requested = 1u;
            print_timer_ms = PRINT_PERIOD_MS;
        } else {
            print_timer_ms = 0u;
        }

        return;
    }

    if (new_state == 0u) {
        return;
    }

    if (print_timer_ms > 0u) {
        print_timer_ms--;
    }

    if (print_timer_ms == 0u) {
        g_print_requested = 1u;
        print_timer_ms = PRINT_PERIOD_MS;
    }
}

static void print_selected_values(uint8_t switch_state)
{
    uint16_t pot_adc = 0u;
    uint16_t cds_adc = 0u;

    if ((switch_state & SWITCH_POT_MASK) != 0u) {
        pot_adc = adc1_read_channel(POT_ADC_CHANNEL);
        print_pot_value(pot_adc);
    }

    if ((switch_state & (SWITCH_POT_MASK | SWITCH_CDS_MASK)) == (SWITCH_POT_MASK | SWITCH_CDS_MASK)) {
        uart2_puts("    ");
    }

    if ((switch_state & SWITCH_CDS_MASK) != 0u) {
        cds_adc = adc1_read_channel(CDS_ADC_CHANNEL);
        print_cds_value(cds_adc);
    }

    uart2_puts("\r\n");
}

void TIM2_IRQHandler(void)
{
    if ((TIM2_SR & TIM_SR_UIF) == 0u) {
        return;
    }

    TIM2_SR &= ~TIM_SR_UIF;
    g_ms_ticks++;
    switches_update_1ms();
}

int main(void)
{
    gpio_init();
    uart2_init();
    timer2_start_1ms_interrupt();
    adc1_init();

    while (1) {
        if (g_print_requested != 0u) {
            uint8_t switch_state;

            g_print_requested = 0u;
            switch_state = g_switch_state;

            if (switch_state != 0u) {
                print_selected_values(switch_state);
            }
        }
    }
}
