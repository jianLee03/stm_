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

#define TIM3_CR1   (*(volatile uint32_t *)0x40000400u)
#define TIM3_EGR   (*(volatile uint32_t *)0x40000414u)
#define TIM3_CCMR1 (*(volatile uint32_t *)0x40000418u)
#define TIM3_CCER  (*(volatile uint32_t *)0x40000420u)
#define TIM3_PSC   (*(volatile uint32_t *)0x40000428u)
#define TIM3_ARR   (*(volatile uint32_t *)0x4000042Cu)
#define TIM3_CCR2  (*(volatile uint32_t *)0x40000438u)

#define NVIC_ISER0 (*(volatile uint32_t *)0xE000E100u)
#define NVIC_ISER1 (*(volatile uint32_t *)0xE000E104u)

#define RCC_APB2ENR_AFIOEN   (1u << 0)
#define RCC_APB2ENR_IOPAEN   (1u << 2)
#define RCC_APB2ENR_IOPCEN   (1u << 4)
#define RCC_APB2ENR_ADC1EN   (1u << 9)
#define RCC_APB1ENR_TIM2EN   (1u << 0)
#define RCC_APB1ENR_TIM3EN   (1u << 1)
#define RCC_APB1ENR_USART2EN (1u << 17)

#define POT_ADC_CHANNEL      0u
#define CDS_ADC_CHANNEL      1u
#define SWITCH_POT_PIN       6u
#define SWITCH_CDS_PIN       8u
#define BUZZER_PIN           7u
#define USART2_TX_PIN        2u
#define SWITCH_ACTIVE_LOW    1u

#define SWITCH_POT_MASK      (1u << 0)
#define SWITCH_CDS_MASK      (1u << 1)

#define ADC_MAX_VALUE        4095u
#define POT_MAX_OHM          5000u
#define POT_EMERGENCY_OHM    2000u

#define CDS_FIXED_RESISTOR_OHM          10000u
#define CDS_RESISTANCE_AT_10_LUX_OHM    10000u
#define CDS_REFERENCE_LUX               10u
#define CDS_ADC_HIGHER_WHEN_BRIGHT      0u
#define CDS_EMERGENCY_LUX               100u

#define TIM2_IRQn            28u
#define USART2_IRQn          38u
#define TIMER_TICK_HZ        1000000u
#define SWITCH_DEBOUNCE_MS   20u
#define SWITCH_FILTER_MAX_MS 50u
#define PRINT_PERIOD_MS      500u
#define FAST_PRINT_PERIOD_MS (PRINT_PERIOD_MS / 2u)
#define NOTE_ON_MS           160u
#define NOTE_GAP_MS          40u
#define FAST_NOTE_ON_MS      (NOTE_ON_MS / 2u)
#define FAST_NOTE_GAP_MS     (NOTE_GAP_MS / 2u)
#define ADC_TIMEOUT_LOOPS    100000u

#define ADC_VALUE_WIDTH      4u
#define OHM_VALUE_WIDTH      5u

#define BUZZER_OFF_MODE      0u
#define BUZZER_SINGLE_MODE   1u
#define BUZZER_DOUBLE_MODE   2u

#define USART_SR_TXE         (1u << 7)
#define USART_CR1_TXEIE      (1u << 7)
#define USART_CR1_TE         (1u << 3)
#define USART_CR1_UE         (1u << 13)
#define UART_TX_BUFFER_SIZE  512u
#define UART_TX_BUFFER_MASK  (UART_TX_BUFFER_SIZE - 1u)

static volatile uint32_t g_ms_ticks;
static volatile uint8_t g_switch_state;
static volatile uint8_t g_buzzer_mode;
static volatile uint8_t g_sensor_update_pending;
static volatile uint8_t g_print_pending;
static volatile uint16_t g_uart_tx_head;
static volatile uint16_t g_uart_tx_tail;
static volatile char g_uart_tx_buffer[UART_TX_BUFFER_SIZE];

static uint16_t g_last_pot_adc;
static uint16_t g_last_cds_adc;
static uint32_t g_last_pot_ohm;
static uint32_t g_last_cds_lux;
static uint8_t g_last_pot_emergency;
static uint8_t g_last_cds_emergency;

static const uint8_t g_single_melody[] = {5u, 7u, 5u, 7u};
static const uint8_t g_double_melody[] = {7u, 9u, 7u, 9u};
static const uint16_t g_level_frequency_hz[11] = {
    0u, 262u, 294u, 330u, 349u, 392u, 440u, 494u, 523u, 587u, 659u
};

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
    uint32_t pc6_shift = SWITCH_POT_PIN * 4u;
    uint32_t pc8_shift = (SWITCH_CDS_PIN - 8u) * 4u;

    RCC_APB2ENR |= RCC_APB2ENR_AFIOEN | RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPCEN;

    GPIOA_CRL &= ~(0xFu << (POT_ADC_CHANNEL * 4u));
    GPIOA_CRL &= ~(0xFu << (CDS_ADC_CHANNEL * 4u));

    GPIOA_CRL &= ~(0xFu << (USART2_TX_PIN * 4u));
    GPIOA_CRL |=  (0xAu << (USART2_TX_PIN * 4u));

    GPIOC_CRL &= ~(0xFu << pc6_shift);
    GPIOC_CRL |=  (0x8u << pc6_shift);

    GPIOC_CRH &= ~(0xFu << pc8_shift);
    GPIOC_CRH |=  (0x8u << pc8_shift);

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

    USART2_BRR = 0x0341u;
    USART2_CR1 = USART_CR1_UE | USART_CR1_TE;
    NVIC_ISER1 = 1u << (USART2_IRQn - 32u);
}

static void uart2_putc(char c)
{
    uint16_t head = g_uart_tx_head;
    uint16_t next = (uint16_t)((head + 1u) & UART_TX_BUFFER_MASK);

    while (next == g_uart_tx_tail) {
    }

    g_uart_tx_buffer[head] = c;
    g_uart_tx_head = next;
    USART2_CR1 |= USART_CR1_TXEIE;
}

static void uart2_puts(const char *s)
{
    while (*s != '\0') {
        uart2_putc(*s++);
    }
}

static void uart2_put_spaces(uint8_t count)
{
    while (count > 0u) {
        uart2_putc(' ');
        count--;
    }
}

static uint8_t decimal_digits_u32(uint32_t value)
{
    uint8_t digits = 1u;

    while (value >= 10u) {
        value /= 10u;
        digits++;
    }

    return digits;
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

static void uart2_put_u32_width(uint32_t value, uint8_t width)
{
    uint8_t digits = decimal_digits_u32(value);

    if (digits < width) {
        uart2_put_spaces((uint8_t)(width - digits));
    }

    uart2_put_u32(value);
}

static void buzzer_init(void)
{
    RCC_APB2ENR |= RCC_APB2ENR_AFIOEN | RCC_APB2ENR_IOPAEN;
    RCC_APB1ENR |= RCC_APB1ENR_TIM3EN;

    GPIOA_CRL &= ~(0xFu << (BUZZER_PIN * 4u));
    GPIOA_CRL |=  (0xAu << (BUZZER_PIN * 4u));

    TIM3_CR1 = 0u;
    TIM3_PSC = 7u;
    TIM3_CCMR1 &= ~(0xFFu << 8);
    TIM3_CCMR1 |= (6u << 12) | (1u << 11);
    TIM3_CCER &= ~(1u << 4);
    TIM3_CR1 |= 1u << 7;
}

static void buzzer_set_frequency(uint32_t frequency_hz)
{
    uint32_t period;

    if (frequency_hz == 0u) {
        TIM3_CCER &= ~(1u << 4);
        TIM3_CR1 &= ~(1u << 0);
        return;
    }

    period = TIMER_TICK_HZ / frequency_hz;
    if (period < 2u) {
        period = 2u;
    }

    TIM3_ARR = period - 1u;
    TIM3_CCR2 = period / 2u;
    TIM3_EGR = 1u;
    TIM3_CCER |= 1u << 4;
    TIM3_CR1 |= 1u << 0;
}

static void buzzer_set_level(uint8_t level)
{
    if (level > 10u) {
        level = 10u;
    }

    buzzer_set_frequency(g_level_frequency_hz[level]);
}

static void adc1_init(void)
{
    RCC_APB2ENR |= RCC_APB2ENR_ADC1EN;
    RCC_CFGR &= ~(0x3u << 14);

    ADC1_CR2 = 0u;
    ADC1_SMPR2 &= ~(0x7u << (POT_ADC_CHANNEL * 3u));
    ADC1_SMPR2 |=  (0x7u << (POT_ADC_CHANNEL * 3u));
    ADC1_SMPR2 &= ~(0x7u << (CDS_ADC_CHANNEL * 3u));
    ADC1_SMPR2 |=  (0x7u << (CDS_ADC_CHANNEL * 3u));
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

static uint16_t adc1_read_channel(uint32_t channel)
{
    uint32_t timeout = ADC_TIMEOUT_LOOPS;

    ADC1_SQR3 = channel;
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

static uint32_t adc_to_ohm(uint32_t adc)
{
    return (adc * POT_MAX_OHM) / ADC_MAX_VALUE;
}

static void print_pot_value(uint16_t adc, uint32_t ohm, uint8_t emergency)
{
    uart2_puts("VR ADC=");
    uart2_put_u32_width(adc, ADC_VALUE_WIDTH);
    uart2_puts("    R=");
    uart2_put_u32_width(ohm, OHM_VALUE_WIDTH);
    uart2_puts(" ohm");

    if (emergency != 0u) {
        uart2_puts("    EMERGENCY");
    }
}

static void print_cds_value(uint16_t adc, uint32_t lux, uint8_t emergency)
{
    uart2_puts("CDS ADC=");
    uart2_put_u32_width(adc, ADC_VALUE_WIDTH);
    uart2_puts("    L=");
    uart2_put_u32(lux);

    if (emergency != 0u) {
        uart2_puts("    EMERGENCY");
    }
}

static void update_sensor_and_buzzer(uint8_t switch_state)
{
    uint8_t emergency_state = 0u;

    g_last_pot_emergency = 0u;
    g_last_cds_emergency = 0u;

    if (switch_state == 0u) {
        g_buzzer_mode = BUZZER_OFF_MODE;
        return;
    }

    if ((switch_state & SWITCH_POT_MASK) != 0u) {
        g_last_pot_adc = adc1_read_channel(POT_ADC_CHANNEL);
        g_last_pot_ohm = adc_to_ohm(g_last_pot_adc);

        if (g_last_pot_ohm <= POT_EMERGENCY_OHM) {
            g_last_pot_emergency = 1u;
            emergency_state |= SWITCH_POT_MASK;
        }
    }

    if ((switch_state & SWITCH_CDS_MASK) != 0u) {
        g_last_cds_adc = adc1_read_channel(CDS_ADC_CHANNEL);
        g_last_cds_lux = adc_to_lux(g_last_cds_adc);

        if (g_last_cds_lux <= CDS_EMERGENCY_LUX) {
            g_last_cds_emergency = 1u;
            emergency_state |= SWITCH_CDS_MASK;
        }
    }

    if (emergency_state == 0u) {
        g_buzzer_mode = BUZZER_OFF_MODE;
    } else if ((switch_state & (SWITCH_POT_MASK | SWITCH_CDS_MASK)) == (SWITCH_POT_MASK | SWITCH_CDS_MASK)) {
        g_buzzer_mode = BUZZER_DOUBLE_MODE;
    } else {
        g_buzzer_mode = BUZZER_SINGLE_MODE;
    }
}

static uint8_t update_switch_filter_1ms(uint8_t raw, uint8_t *filter_count, uint8_t pressed)
{
    if (raw != 0u) {
        if (*filter_count < SWITCH_FILTER_MAX_MS) {
            (*filter_count)++;
        }
    } else if (*filter_count > 0u) {
        (*filter_count)--;
    }

    if ((pressed == 0u) && (*filter_count >= SWITCH_DEBOUNCE_MS)) {
        return 1u;
    }

    if ((pressed != 0u) && (*filter_count == 0u)) {
        return 0u;
    }

    return pressed;
}

static uint16_t switch_print_period_ms(uint8_t switch_state)
{
    if ((switch_state & (SWITCH_POT_MASK | SWITCH_CDS_MASK)) == (SWITCH_POT_MASK | SWITCH_CDS_MASK)) {
        return FAST_PRINT_PERIOD_MS;
    }

    return PRINT_PERIOD_MS;
}

static void update_switches_1ms(void)
{
    static uint8_t pot_filter_count;
    static uint8_t cds_filter_count;
    static uint16_t print_ms;
    uint8_t new_state = 0u;
    uint8_t pot_pressed;
    uint8_t cds_pressed;

    pot_pressed = (g_switch_state & SWITCH_POT_MASK) != 0u;
    cds_pressed = (g_switch_state & SWITCH_CDS_MASK) != 0u;

    pot_pressed = update_switch_filter_1ms(
        switch_raw_pressed(SWITCH_POT_PIN),
        &pot_filter_count,
        pot_pressed
    );
    cds_pressed = update_switch_filter_1ms(
        switch_raw_pressed(SWITCH_CDS_PIN),
        &cds_filter_count,
        cds_pressed
    );

    if (pot_pressed != 0u) {
        new_state |= SWITCH_POT_MASK;
    }
    if (cds_pressed != 0u) {
        new_state |= SWITCH_CDS_MASK;
    }

    if (new_state != g_switch_state) {
        g_switch_state = new_state;
        g_buzzer_mode = BUZZER_OFF_MODE;

        if (new_state != 0u) {
            g_sensor_update_pending = 1u;
            g_print_pending = 1u;
            print_ms = switch_print_period_ms(new_state);
        } else {
            g_sensor_update_pending = 0u;
            g_print_pending = 0u;
            print_ms = 0u;
        }
    }

    if (g_switch_state == 0u) {
        return;
    }

    g_sensor_update_pending = 1u;

    if (print_ms > 0u) {
        print_ms--;
    }

    if (print_ms == 0u) {
        g_print_pending = 1u;
        print_ms = switch_print_period_ms(g_switch_state);
    }
}

static void print_values(uint8_t switch_state)
{
    if ((switch_state & SWITCH_POT_MASK) != 0u) {
        print_pot_value(g_last_pot_adc, g_last_pot_ohm, g_last_pot_emergency);
    }
    if ((switch_state & (SWITCH_POT_MASK | SWITCH_CDS_MASK)) == (SWITCH_POT_MASK | SWITCH_CDS_MASK)) {
        uart2_puts("        ");
    }
    if ((switch_state & SWITCH_CDS_MASK) != 0u) {
        print_cds_value(g_last_cds_adc, g_last_cds_lux, g_last_cds_emergency);
    }

    uart2_puts("\r\n");
}

static void update_buzzer_1ms(void)
{
    static uint8_t last_mode;
    static uint8_t melody_index;
    static uint8_t note_is_on;
    static uint16_t note_ms;
    const uint8_t *melody;
    uint8_t mode = g_buzzer_mode;
    uint16_t note_on_ms;
    uint16_t note_gap_ms;

    if (g_switch_state == 0u) {
        mode = BUZZER_OFF_MODE;
    }

    if (mode == BUZZER_OFF_MODE) {
        if (last_mode != BUZZER_OFF_MODE) {
            buzzer_set_frequency(0u);
        }

        last_mode = BUZZER_OFF_MODE;
        melody_index = 0u;
        note_is_on = 0u;
        note_ms = 0u;
        return;
    }

    melody = (mode == BUZZER_DOUBLE_MODE) ? g_double_melody : g_single_melody;
    note_on_ms = (mode == BUZZER_DOUBLE_MODE) ? FAST_NOTE_ON_MS : NOTE_ON_MS;
    note_gap_ms = (mode == BUZZER_DOUBLE_MODE) ? FAST_NOTE_GAP_MS : NOTE_GAP_MS;

    if (mode != last_mode) {
        last_mode = mode;
        melody_index = 0u;
        note_is_on = 1u;
        note_ms = note_on_ms;
        buzzer_set_level(melody[melody_index]);
        return;
    }

    if (note_ms > 0u) {
        note_ms--;
    }

    if (note_ms != 0u) {
        return;
    }

    if (note_is_on != 0u) {
        buzzer_set_frequency(0u);
        note_is_on = 0u;
        note_ms = note_gap_ms;
        return;
    }

    melody_index++;
    if (melody_index >= sizeof(g_single_melody)) {
        melody_index = 0u;
    }

    buzzer_set_level(melody[melody_index]);
    note_is_on = 1u;
    note_ms = note_on_ms;
}

void TIM2_IRQHandler(void)
{
    if ((TIM2_SR & 1u) == 0u) {
        return;
    }

    TIM2_SR &= ~1u;
    g_ms_ticks++;
    update_switches_1ms();
    update_buzzer_1ms();
}

void USART2_IRQHandler(void)
{
    uint16_t tail;

    if (((USART2_SR & USART_SR_TXE) == 0u) || ((USART2_CR1 & USART_CR1_TXEIE) == 0u)) {
        return;
    }

    tail = g_uart_tx_tail;
    if (tail == g_uart_tx_head) {
        USART2_CR1 &= ~USART_CR1_TXEIE;
        return;
    }

    USART2_DR = (uint32_t)g_uart_tx_buffer[tail];
    g_uart_tx_tail = (uint16_t)((tail + 1u) & UART_TX_BUFFER_MASK);
}

int main(void)
{
    gpio_init();
    uart2_init();
    buzzer_init();
    timer2_1ms_init();
    adc1_init();

    while (1) {
        uint8_t switch_state = g_switch_state;

        if (g_sensor_update_pending != 0u) {
            g_sensor_update_pending = 0u;
            update_sensor_and_buzzer(switch_state);
        }

        if (switch_state == 0u) {
            g_buzzer_mode = BUZZER_OFF_MODE;
            continue;
        }

        if (g_print_pending != 0u) {
            g_print_pending = 0u;
            switch_state = g_switch_state;

            if (switch_state != 0u) {
                print_values(switch_state);
            }
        }
    }
}
