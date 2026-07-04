#if 0
/* All previous examples disabled per request. */
#if 0
/* Previous UART resistance-print code kept disabled per request. */
#include <stdint.h>

#define SYSCLK_HZ                   8000000UL
#define UART_BAUD_RATE              9600UL
#define CDS_ADC_MAX_COUNTS          4095UL
#define CDS_FIXED_RESISTOR_OHMS     10000UL

/*
 * Set this to 1 when wired as:
 *   3.3V -- fixed resistor -- A0/PA0 -- CDS -- GND
 *
 * Set this to 0 when wired as:
 *   3.3V -- CDS -- A0/PA0 -- fixed resistor -- GND
 */
#define CDS_CONNECTED_TO_GND        0

#define MMIO32(address)             (*(volatile uint32_t *)(address))

#define RCC_BASE                    0x40021000UL
#define GPIOA_BASE                  0x40010800UL
#define ADC1_BASE                   0x40012400UL
#define USART2_BASE                 0x40004400UL
#define SYSTICK_BASE                0xE000E010UL

#define RCC_CFGR                    MMIO32(RCC_BASE + 0x04UL)
#define RCC_APB2ENR                 MMIO32(RCC_BASE + 0x18UL)
#define RCC_APB1ENR                 MMIO32(RCC_BASE + 0x1CUL)

#define GPIOA_CRL                   MMIO32(GPIOA_BASE + 0x00UL)

#define ADC1_SR                     MMIO32(ADC1_BASE + 0x00UL)
#define ADC1_CR1                    MMIO32(ADC1_BASE + 0x04UL)
#define ADC1_CR2                    MMIO32(ADC1_BASE + 0x08UL)
#define ADC1_SMPR2                  MMIO32(ADC1_BASE + 0x10UL)
#define ADC1_SQR1                   MMIO32(ADC1_BASE + 0x2CUL)
#define ADC1_SQR3                   MMIO32(ADC1_BASE + 0x34UL)
#define ADC1_DR                     MMIO32(ADC1_BASE + 0x4CUL)

#define USART2_SR                   MMIO32(USART2_BASE + 0x00UL)
#define USART2_DR                   MMIO32(USART2_BASE + 0x04UL)
#define USART2_BRR                  MMIO32(USART2_BASE + 0x08UL)
#define USART2_CR1                  MMIO32(USART2_BASE + 0x0CUL)

#define SYSTICK_CTRL                MMIO32(SYSTICK_BASE + 0x00UL)
#define SYSTICK_LOAD                MMIO32(SYSTICK_BASE + 0x04UL)
#define SYSTICK_VAL                 MMIO32(SYSTICK_BASE + 0x08UL)

#define RCC_APB2ENR_AFIOEN          (1UL << 0)
#define RCC_APB2ENR_IOPAEN          (1UL << 2)
#define RCC_APB2ENR_ADC1EN          (1UL << 9)
#define RCC_APB1ENR_USART2EN        (1UL << 17)

#define RCC_CFGR_ADCPRE_MASK        (3UL << 14)
#define RCC_CFGR_ADCPRE_DIV6        (2UL << 14)

#define ADC_SR_EOC                  (1UL << 1)
#define ADC_CR2_ADON                (1UL << 0)
#define ADC_CR2_CAL                 (1UL << 2)
#define ADC_CR2_RSTCAL              (1UL << 3)
#define ADC_CR2_EXTSEL_SWSTART      (7UL << 17)
#define ADC_CR2_EXTTRIG             (1UL << 20)
#define ADC_CR2_SWSTART             (1UL << 22)

#define USART_SR_TXE                (1UL << 7)
#define USART_CR1_TE                (1UL << 3)
#define USART_CR1_UE                (1UL << 13)

#define SYSTICK_CTRL_ENABLE         (1UL << 0)
#define SYSTICK_CTRL_CLKSOURCE      (1UL << 2)
#define SYSTICK_CTRL_COUNTFLAG      (1UL << 16)

volatile uint16_t g_cds_adc_raw;
volatile uint32_t g_cds_resistance_ohms;

void SystemInit(void)
{
    /* Keep the reset clock setup: HSI 8 MHz system clock. */
}

static void delay_ms(uint32_t ms)
{
    while (ms-- != 0UL) {
        SYSTICK_LOAD = (SYSCLK_HZ / 1000UL) - 1UL;
        SYSTICK_VAL = 0UL;
        SYSTICK_CTRL = SYSTICK_CTRL_CLKSOURCE | SYSTICK_CTRL_ENABLE;

        while ((SYSTICK_CTRL & SYSTICK_CTRL_COUNTFLAG) == 0UL) {
        }

        SYSTICK_CTRL = 0UL;
    }
}

static void uart2_init_9600(void)
{
    RCC_APB2ENR |= RCC_APB2ENR_AFIOEN | RCC_APB2ENR_IOPAEN;
    RCC_APB1ENR |= RCC_APB1ENR_USART2EN;

    /* PA2 = USART2_TX, alternate function push-pull, max speed 2 MHz. */
    GPIOA_CRL = (GPIOA_CRL & ~(0xFUL << 8)) | (0xAUL << 8);

    USART2_BRR = (SYSCLK_HZ + (UART_BAUD_RATE / 2UL)) / UART_BAUD_RATE;
    USART2_CR1 = USART_CR1_TE | USART_CR1_UE;
}

static void uart2_write_char(char ch)
{
    while ((USART2_SR & USART_SR_TXE) == 0UL) {
    }

    USART2_DR = (uint32_t)(uint8_t)ch;
}

static void uart2_write_string(const char *text)
{
    while (*text != '\0') {
        uart2_write_char(*text);
        ++text;
    }
}

static void uart2_write_u32(uint32_t value)
{
    char buffer[10];
    uint32_t index = 0UL;

    if (value == 0UL) {
        uart2_write_char('0');
        return;
    }

    while (value != 0UL) {
        buffer[index] = (char)('0' + (value % 10UL));
        value /= 10UL;
        ++index;
    }

    while (index != 0UL) {
        --index;
        uart2_write_char(buffer[index]);
    }
}

static void adc1_init_pa0(void)
{
    RCC_APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_ADC1EN;

    /* ADC clock = PCLK2 / 6. With reset clock this is 8 MHz / 6. */
    RCC_CFGR = (RCC_CFGR & ~RCC_CFGR_ADCPRE_MASK) | RCC_CFGR_ADCPRE_DIV6;

    /* PA0 (Nucleo A0, ADC1_IN0) as analog input: MODE0=00, CNF0=00. */
    GPIOA_CRL &= ~0xFUL;

    ADC1_CR1 = 0UL;
    ADC1_CR2 = ADC_CR2_EXTSEL_SWSTART | ADC_CR2_EXTTRIG;
    ADC1_SMPR2 = 7UL;       /* Channel 0 sample time: 239.5 ADC cycles. */
    ADC1_SQR1 = 0UL;        /* One conversion in the regular sequence. */
    ADC1_SQR3 = 0UL;        /* First conversion: channel 0. */

    ADC1_CR2 |= ADC_CR2_ADON;
    delay_ms(1UL);

    ADC1_CR2 |= ADC_CR2_RSTCAL;
    while ((ADC1_CR2 & ADC_CR2_RSTCAL) != 0UL) {
    }

    ADC1_CR2 |= ADC_CR2_CAL;
    while ((ADC1_CR2 & ADC_CR2_CAL) != 0UL) {
    }
}

static uint16_t adc1_read_pa0(void)
{
    ADC1_CR2 |= ADC_CR2_SWSTART;

    while ((ADC1_SR & ADC_SR_EOC) == 0UL) {
    }

    return (uint16_t)(ADC1_DR & CDS_ADC_MAX_COUNTS);
}

static uint32_t cds_adc_to_ohms(uint16_t adc_counts)
{
#if CDS_CONNECTED_TO_GND
    if (adc_counts == 0U) {
        return 0UL;
    }

    if (adc_counts >= CDS_ADC_MAX_COUNTS) {
        return UINT32_MAX;
    }

    return (uint32_t)(((uint64_t)CDS_FIXED_RESISTOR_OHMS * adc_counts)
                      / (CDS_ADC_MAX_COUNTS - adc_counts));
#else
    if (adc_counts == 0U) {
        return UINT32_MAX;
    }

    if (adc_counts >= CDS_ADC_MAX_COUNTS) {
        return 0UL;
    }

    return (uint32_t)(((uint64_t)CDS_FIXED_RESISTOR_OHMS
                      * (CDS_ADC_MAX_COUNTS - adc_counts)) / adc_counts);
#endif
}

static void print_cds_resistance(uint32_t resistance_ohms)
{
    uart2_write_string("CDS Resistance: ");

    if (resistance_ohms == UINT32_MAX) {
        uart2_write_string("INF");
    } else {
        uart2_write_u32(resistance_ohms);
    }

    uart2_write_string(" ohm\r\n");
}

int main(void)
{
    uart2_init_9600();
    adc1_init_pa0();

    for (;;) {
        g_cds_adc_raw = adc1_read_pa0();
        g_cds_resistance_ohms = cds_adc_to_ohms(g_cds_adc_raw);
        print_cds_resistance(g_cds_resistance_ohms);
        delay_ms(500UL);
    }
}

#endif

/* Active CDS/PC8 LED code. */
#include <stdint.h>

#define SYSCLK_HZ                   8000000UL
#define UART_BAUD_RATE              9600UL
#define UART_PRINT_INTERVAL_MS      167UL
#define CDS_VREF_MV                 3300UL
#define CDS_ADC_MAX_COUNTS          4095UL
#define CDS_FIXED_RESISTOR_OHMS     10000UL
#define CDS_LED_THRESHOLD_OHMS      20000UL
#define RESISTANCE_FIELD_WIDTH      10UL
#define SENSOR_FIELD_WIDTH          4UL
#define LED_ACTIVE_HIGH             1
#define LED_PC8_PIN_MASK            (1UL << 8)
#define LED_STARTUP_BLINK_COUNT     3UL
#define LED_STARTUP_BLINK_DELAY     250000UL

/*
 * Set this to 1 when wired as:
 *   3.3V -- fixed resistor -- A0/PA0 -- CDS -- GND
 *
 * Set this to 0 when wired as:
 *   3.3V -- CDS -- A0/PA0 -- fixed resistor -- GND
 */
#define CDS_CONNECTED_TO_GND        0

#define MMIO32(address)             (*(volatile uint32_t *)(address))

#define RCC_BASE                    0x40021000UL
#define GPIOA_BASE                  0x40010800UL
#define GPIOC_BASE                  0x40011000UL
#define ADC1_BASE                   0x40012400UL
#define USART2_BASE                 0x40004400UL
#define SYSTICK_BASE                0xE000E010UL

#define RCC_CFGR                    MMIO32(RCC_BASE + 0x04UL)
#define RCC_APB2ENR                 MMIO32(RCC_BASE + 0x18UL)
#define RCC_APB1ENR                 MMIO32(RCC_BASE + 0x1CUL)

#define GPIOA_CRL                   MMIO32(GPIOA_BASE + 0x00UL)
#define GPIOC_CRH                   MMIO32(GPIOC_BASE + 0x04UL)
#define GPIOC_BSRR                  MMIO32(GPIOC_BASE + 0x10UL)

#define ADC1_SR                     MMIO32(ADC1_BASE + 0x00UL)
#define ADC1_CR1                    MMIO32(ADC1_BASE + 0x04UL)
#define ADC1_CR2                    MMIO32(ADC1_BASE + 0x08UL)
#define ADC1_SMPR2                  MMIO32(ADC1_BASE + 0x10UL)
#define ADC1_SQR1                   MMIO32(ADC1_BASE + 0x2CUL)
#define ADC1_SQR3                   MMIO32(ADC1_BASE + 0x34UL)
#define ADC1_DR                     MMIO32(ADC1_BASE + 0x4CUL)

#define USART2_SR                   MMIO32(USART2_BASE + 0x00UL)
#define USART2_DR                   MMIO32(USART2_BASE + 0x04UL)
#define USART2_BRR                  MMIO32(USART2_BASE + 0x08UL)
#define USART2_CR1                  MMIO32(USART2_BASE + 0x0CUL)

#define SYSTICK_CTRL                MMIO32(SYSTICK_BASE + 0x00UL)
#define SYSTICK_LOAD                MMIO32(SYSTICK_BASE + 0x04UL)
#define SYSTICK_VAL                 MMIO32(SYSTICK_BASE + 0x08UL)

#define RCC_APB2ENR_AFIOEN          (1UL << 0)
#define RCC_APB2ENR_IOPAEN          (1UL << 2)
#define RCC_APB2ENR_IOPCEN          (1UL << 4)
#define RCC_APB2ENR_ADC1EN          (1UL << 9)
#define RCC_APB1ENR_USART2EN        (1UL << 17)

#define RCC_CFGR_ADCPRE_MASK        (3UL << 14)
#define RCC_CFGR_ADCPRE_DIV6        (2UL << 14)

#define ADC_SR_EOC                  (1UL << 1)

#define ADC_CR2_ADON                (1UL << 0)
#define ADC_CR2_CAL                 (1UL << 2)
#define ADC_CR2_RSTCAL              (1UL << 3)
#define ADC_CR2_EXTSEL_SWSTART      (7UL << 17)
#define ADC_CR2_EXTTRIG             (1UL << 20)
#define ADC_CR2_SWSTART             (1UL << 22)

#define USART_SR_TXE                (1UL << 7)
#define USART_CR1_TE                (1UL << 3)
#define USART_CR1_UE                (1UL << 13)

#define SYSTICK_CTRL_ENABLE         (1UL << 0)
#define SYSTICK_CTRL_CLKSOURCE      (1UL << 2)
#define SYSTICK_CTRL_COUNTFLAG      (1UL << 16)

volatile uint16_t g_cds_adc_raw;
volatile uint32_t g_cds_voltage_mv;
volatile uint32_t g_cds_resistance_ohms;
volatile uint8_t g_cds_is_dark;
volatile uint8_t g_led_is_on;

void SystemInit(void)
{
    /*
     * Keep the reset clock setup: HSI 8 MHz system clock.
     * ADC is configured below with a safe APB2/ADC prescaler.
     */
}

static void delay_cycles(volatile uint32_t cycles)
{
    while (cycles-- != 0UL) {
        __asm volatile ("nop");
    }
}

static void delay_ms(uint32_t ms)
{
    while (ms-- != 0UL) {
        SYSTICK_LOAD = (SYSCLK_HZ / 1000UL) - 1UL;
        SYSTICK_VAL = 0UL;
        SYSTICK_CTRL = SYSTICK_CTRL_CLKSOURCE | SYSTICK_CTRL_ENABLE;

        while ((SYSTICK_CTRL & SYSTICK_CTRL_COUNTFLAG) == 0UL) {
        }

        SYSTICK_CTRL = 0UL;
    }
}

static void uart2_init_9600(void)
{
    RCC_APB2ENR |= RCC_APB2ENR_AFIOEN | RCC_APB2ENR_IOPAEN;
    RCC_APB1ENR |= RCC_APB1ENR_USART2EN;

    /* PA2 = USART2_TX, alternate function push-pull, max speed 2 MHz. */
    GPIOA_CRL = (GPIOA_CRL & ~(0xFUL << 8)) | (0xAUL << 8);

    USART2_BRR = (SYSCLK_HZ + (UART_BAUD_RATE / 2UL)) / UART_BAUD_RATE;
    USART2_CR1 = USART_CR1_TE | USART_CR1_UE;
}

static void uart2_write_char(char ch)
{
    while ((USART2_SR & USART_SR_TXE) == 0UL) {
    }

    USART2_DR = (uint32_t)(uint8_t)ch;
}

static void uart2_write_string(const char *text)
{
    while (*text != '\0') {
        uart2_write_char(*text);
        ++text;
    }
}

static void uart2_write_u32(uint32_t value)
{
    char buffer[10];
    uint32_t index = 0UL;

    if (value == 0UL) {
        uart2_write_char('0');
        return;
    }

    while (value != 0UL) {
        buffer[index] = (char)('0' + (value % 10UL));
        value /= 10UL;
        ++index;
    }

    while (index != 0UL) {
        --index;
        uart2_write_char(buffer[index]);
    }
}

static uint32_t decimal_digit_count(uint32_t value)
{
    uint32_t digits = 1UL;

    while (value >= 10UL) {
        value /= 10UL;
        ++digits;
    }

    return digits;
}

static void uart2_write_spaces(uint32_t count)
{
    while (count-- != 0UL) {
        uart2_write_char(' ');
    }
}

static void uart2_write_u32_right_aligned(uint32_t value, uint32_t width)
{
    uint32_t digits = decimal_digit_count(value);

    if (digits < width) {
        uart2_write_spaces(width - digits);
    }

    uart2_write_u32(value);
}

static void uart2_write_string_right_aligned(const char *text, uint32_t width)
{
    const char *cursor = text;
    uint32_t length = 0UL;

    while (*cursor != '\0') {
        ++length;
        ++cursor;
    }

    if (length < width) {
        uart2_write_spaces(width - length);
    }

    uart2_write_string(text);
}

static void led_pc8_write(uint8_t on)
{
#if LED_ACTIVE_HIGH
    GPIOC_BSRR = on ? LED_PC8_PIN_MASK : (LED_PC8_PIN_MASK << 16);
#else
    GPIOC_BSRR = on ? (LED_PC8_PIN_MASK << 16) : LED_PC8_PIN_MASK;
#endif
}

static void led_pc8_init(void)
{
    RCC_APB2ENR |= RCC_APB2ENR_IOPCEN;

    /* PC8 as general purpose push-pull output, max speed 2 MHz. */
    GPIOC_CRH = (GPIOC_CRH & ~0xFUL) | 0x2UL;
    led_pc8_write(0U);
}

static void led_pc8_blink_startup(void)
{
    for (uint32_t i = 0UL; i < LED_STARTUP_BLINK_COUNT; ++i) {
        led_pc8_write(1U);
        delay_cycles(LED_STARTUP_BLINK_DELAY);
        led_pc8_write(0U);
        delay_cycles(LED_STARTUP_BLINK_DELAY);
    }
}

static void adc1_init_pa0(void)
{
    RCC_APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_ADC1EN;

    /* ADC clock = PCLK2 / 6. With reset clock this is 8 MHz / 6. */
    RCC_CFGR = (RCC_CFGR & ~RCC_CFGR_ADCPRE_MASK) | RCC_CFGR_ADCPRE_DIV6;

    /* PA0 (Nucleo A0, ADC1_IN0) as analog input: MODE0=00, CNF0=00. */
    GPIOA_CRL &= ~0xFUL;

    ADC1_CR1 = 0UL;
    ADC1_CR2 = ADC_CR2_EXTSEL_SWSTART | ADC_CR2_EXTTRIG;
    ADC1_SMPR2 = 7UL;       /* Channel 0 sample time: 239.5 ADC cycles. */
    ADC1_SQR1 = 0UL;        /* One conversion in the regular sequence. */
    ADC1_SQR3 = 0UL;        /* First conversion: channel 0. */

    ADC1_CR2 |= ADC_CR2_ADON;
    delay_cycles(8000UL);

    ADC1_CR2 |= ADC_CR2_RSTCAL;
    while ((ADC1_CR2 & ADC_CR2_RSTCAL) != 0UL) {
    }

    ADC1_CR2 |= ADC_CR2_CAL;
    while ((ADC1_CR2 & ADC_CR2_CAL) != 0UL) {
    }
}

static uint16_t adc1_read_pa0(void)
{
    ADC1_CR2 |= ADC_CR2_SWSTART;

    while ((ADC1_SR & ADC_SR_EOC) == 0UL) {
    }

    return (uint16_t)(ADC1_DR & CDS_ADC_MAX_COUNTS);
}

static uint32_t cds_adc_to_millivolts(uint16_t adc_counts)
{
    return (((uint32_t)adc_counts * CDS_VREF_MV) + (CDS_ADC_MAX_COUNTS / 2UL))
           / CDS_ADC_MAX_COUNTS;
}

static uint32_t cds_adc_to_ohms(uint16_t adc_counts)
{
#if CDS_CONNECTED_TO_GND
    if (adc_counts == 0U) {
        return 0UL;
    }

    if (adc_counts >= CDS_ADC_MAX_COUNTS) {
        return UINT32_MAX;
    }

    return (uint32_t)(((uint64_t)CDS_FIXED_RESISTOR_OHMS * adc_counts)
                      / (CDS_ADC_MAX_COUNTS - adc_counts));
#else
    if (adc_counts == 0U) {
        return UINT32_MAX;
    }

    if (adc_counts >= CDS_ADC_MAX_COUNTS) {
        return 0UL;
    }

    return (uint32_t)(((uint64_t)CDS_FIXED_RESISTOR_OHMS
                      * (CDS_ADC_MAX_COUNTS - adc_counts)) / adc_counts);
#endif
}

static uint8_t cds_should_turn_led_on(uint32_t cds_resistance_ohms)
{
    return (cds_resistance_ohms <= CDS_LED_THRESHOLD_OHMS) ? 1U : 0U;
}

static void print_cds_status(uint32_t resistance_ohms,
                             uint16_t sensor_value,
                             uint8_t led_on)
{
    uart2_write_string("Resistance: ");

    if (resistance_ohms == UINT32_MAX) {
        uart2_write_string_right_aligned("INF", RESISTANCE_FIELD_WIDTH);
    } else {
        uart2_write_u32_right_aligned(resistance_ohms,
                                      RESISTANCE_FIELD_WIDTH);
    }

    uart2_write_string(" ohm | Sensor: ");
    uart2_write_u32_right_aligned(sensor_value, SENSOR_FIELD_WIDTH);
    uart2_write_string(" | LED: ");
    uart2_write_string((led_on != 0U) ? "ON\r\n" : "OFF\r\n");
}

int main(void)
{
    adc1_init_pa0();
    led_pc8_init();
    uart2_init_9600();
    led_pc8_blink_startup();

    for (;;) {
        g_cds_adc_raw = adc1_read_pa0();
        g_cds_voltage_mv = cds_adc_to_millivolts(g_cds_adc_raw);
        g_cds_resistance_ohms = cds_adc_to_ohms(g_cds_adc_raw);
        g_cds_is_dark = cds_should_turn_led_on(g_cds_resistance_ohms);
        g_led_is_on = g_cds_is_dark;
        led_pc8_write(g_led_is_on);
        print_cds_status(g_cds_resistance_ohms, g_cds_adc_raw, g_led_is_on);

        delay_ms(UART_PRINT_INTERVAL_MS);
    }
}
#endif

/* Previous user-button edge-detect UART code disabled per request. */
#if 0
/* Active user-button edge-detect UART code. */
#include <stdint.h>

#define SYSCLK_HZ                   8000000UL
#define UART_BAUD_RATE              9600UL
#define USER_BUTTON_PIN_MASK        (1UL << 13)
#define DEBOUNCE_DELAY_MS           20UL
#define HOLD_REFRESH_INTERVAL_MS    50UL
#define TIME_INTEGER_FIELD_WIDTH    5UL

#define MMIO32(address)             (*(volatile uint32_t *)(address))

#define RCC_BASE                    0x40021000UL
#define GPIOA_BASE                  0x40010800UL
#define GPIOC_BASE                  0x40011000UL
#define USART2_BASE                 0x40004400UL
#define SYSTICK_BASE                0xE000E010UL

#define RCC_APB2ENR                 MMIO32(RCC_BASE + 0x18UL)
#define RCC_APB1ENR                 MMIO32(RCC_BASE + 0x1CUL)

#define GPIOA_CRL                   MMIO32(GPIOA_BASE + 0x00UL)
#define GPIOC_CRH                   MMIO32(GPIOC_BASE + 0x04UL)
#define GPIOC_IDR                   MMIO32(GPIOC_BASE + 0x08UL)
#define GPIOC_ODR                   MMIO32(GPIOC_BASE + 0x0CUL)

#define USART2_SR                   MMIO32(USART2_BASE + 0x00UL)
#define USART2_DR                   MMIO32(USART2_BASE + 0x04UL)
#define USART2_BRR                  MMIO32(USART2_BASE + 0x08UL)
#define USART2_CR1                  MMIO32(USART2_BASE + 0x0CUL)

#define SYSTICK_CTRL                MMIO32(SYSTICK_BASE + 0x00UL)
#define SYSTICK_LOAD                MMIO32(SYSTICK_BASE + 0x04UL)
#define SYSTICK_VAL                 MMIO32(SYSTICK_BASE + 0x08UL)

#define RCC_APB2ENR_AFIOEN          (1UL << 0)
#define RCC_APB2ENR_IOPAEN          (1UL << 2)
#define RCC_APB2ENR_IOPCEN          (1UL << 4)
#define RCC_APB1ENR_USART2EN        (1UL << 17)

#define USART_SR_TXE                (1UL << 7)
#define USART_CR1_TE                (1UL << 3)
#define USART_CR1_UE                (1UL << 13)

#define SYSTICK_CTRL_ENABLE         (1UL << 0)
#define SYSTICK_CTRL_TICKINT        (1UL << 1)
#define SYSTICK_CTRL_CLKSOURCE      (1UL << 2)

volatile uint8_t g_user_button_pressed;
volatile uint32_t g_user_button_hold_ms;
volatile uint32_t g_millis_ms;

void SystemInit(void)
{
    /* Keep the reset clock setup: HSI 8 MHz system clock. */
}

void SysTick_Handler(void)
{
    ++g_millis_ms;
}

static void systick_init_1ms(void)
{
    SYSTICK_LOAD = (SYSCLK_HZ / 1000UL) - 1UL;
    SYSTICK_VAL = 0UL;
    SYSTICK_CTRL = SYSTICK_CTRL_CLKSOURCE
                   | SYSTICK_CTRL_TICKINT
                   | SYSTICK_CTRL_ENABLE;
}

static uint32_t millis_now(void)
{
    return g_millis_ms;
}

static void delay_ms(uint32_t ms)
{
    uint32_t start_ms = millis_now();

    while ((millis_now() - start_ms) < ms) {
    }
}

static void uart2_init_9600(void)
{
    RCC_APB2ENR |= RCC_APB2ENR_AFIOEN | RCC_APB2ENR_IOPAEN;
    RCC_APB1ENR |= RCC_APB1ENR_USART2EN;

    /* PA2 = USART2_TX, alternate function push-pull, max speed 2 MHz. */
    GPIOA_CRL = (GPIOA_CRL & ~(0xFUL << 8)) | (0xAUL << 8);

    USART2_BRR = (SYSCLK_HZ + (UART_BAUD_RATE / 2UL)) / UART_BAUD_RATE;
    USART2_CR1 = USART_CR1_TE | USART_CR1_UE;
}

static void uart2_write_char(char ch)
{
    while ((USART2_SR & USART_SR_TXE) == 0UL) {
    }

    USART2_DR = (uint32_t)(uint8_t)ch;
}

static void uart2_write_string(const char *text)
{
    while (*text != '\0') {
        uart2_write_char(*text);
        ++text;
    }
}

static void uart2_write_u32_zero_padded(uint32_t value, uint32_t width)
{
    uint32_t divisor = 1UL;

    while (width > 1UL) {
        divisor *= 10UL;
        --width;
    }

    while (divisor != 0UL) {
        uart2_write_char((char)('0' + ((value / divisor) % 10UL)));
        divisor /= 10UL;
    }
}

static void uart2_write_elapsed_seconds(uint32_t elapsed_ms)
{
    uint32_t seconds = elapsed_ms / 1000UL;
    uint32_t hundredths = (elapsed_ms % 1000UL) / 10UL;

    uart2_write_u32_zero_padded(seconds, TIME_INTEGER_FIELD_WIDTH);
    uart2_write_char('.');
    uart2_write_u32_zero_padded(hundredths, 2UL);
    uart2_write_string(" s");
}

static void uart2_clear_line(void)
{
    uart2_write_string("\r\033[2K");
}

static void uart2_write_live_lines(uint8_t pressed, uint32_t elapsed_ms)
{
    uart2_clear_line();
    uart2_write_string("Button State : ");
    uart2_write_string((pressed != 0U) ? "PRESSED\r\n" : "RELEASED\r\n");

    uart2_clear_line();
    uart2_write_string("Elapsed Time : ");
    uart2_write_elapsed_seconds(elapsed_ms);
    uart2_write_string("\r\n");
}

static void uart2_refresh_live_display(uint8_t pressed, uint32_t elapsed_ms)
{
    uart2_write_string("\033[2F");
    uart2_write_live_lines(pressed, elapsed_ms);
}

static void uart2_start_live_display(uint8_t pressed, uint32_t elapsed_ms)
{
    uart2_write_string("\r\n\r\n");
    uart2_refresh_live_display(pressed, elapsed_ms);
}

static void uart2_append_record(uint32_t record_index, uint32_t elapsed_ms)
{
    uart2_write_string("\033[2F");

    uart2_clear_line();
    uart2_write_string("Record ");
    uart2_write_u32_zero_padded(record_index, 4UL);
    uart2_write_string("  : ");
    uart2_write_elapsed_seconds(elapsed_ms);
    uart2_write_string("\r\n\r\n");

    uart2_write_live_lines(0U, elapsed_ms);
}

static void user_button_pc13_init(void)
{
    RCC_APB2ENR |= RCC_APB2ENR_IOPCEN;

    /* PC13 = input with pull-up. Nucleo user button is active-low. */
    GPIOC_CRH = (GPIOC_CRH & ~(0xFUL << 20)) | (0x8UL << 20);
    GPIOC_ODR |= USER_BUTTON_PIN_MASK;
}

static uint8_t user_button_is_pressed(void)
{
    return ((GPIOC_IDR & USER_BUTTON_PIN_MASK) == 0UL) ? 1U : 0U;
}

int main(void)
{
    uint8_t previous_pressed;
    uint32_t button_pressed_at_ms = 0UL;
    uint32_t next_hold_refresh_ms = 0UL;
    uint32_t record_count = 0UL;

    uart2_init_9600();
    user_button_pc13_init();
    systick_init_1ms();

    previous_pressed = user_button_is_pressed();
    g_user_button_pressed = previous_pressed;
    if (previous_pressed != 0U) {
        button_pressed_at_ms = millis_now();
        next_hold_refresh_ms = button_pressed_at_ms + HOLD_REFRESH_INTERVAL_MS;
    }

    uart2_write_string("User button edge detect ready\r\n");
    uart2_start_live_display(previous_pressed, g_user_button_hold_ms);

    for (;;) {
        uint32_t now_ms = millis_now();
        uint8_t current_pressed = user_button_is_pressed();

        if (current_pressed != previous_pressed) {
            delay_ms(DEBOUNCE_DELAY_MS);

            now_ms = millis_now();
            current_pressed = user_button_is_pressed();
            if (current_pressed != previous_pressed) {
                previous_pressed = current_pressed;
                g_user_button_pressed = current_pressed;

                if (current_pressed != 0U) {
                    button_pressed_at_ms = now_ms;
                    next_hold_refresh_ms = now_ms + HOLD_REFRESH_INTERVAL_MS;
                    g_user_button_hold_ms = 0UL;
                    uart2_refresh_live_display(1U, g_user_button_hold_ms);
                } else {
                    g_user_button_hold_ms = now_ms - button_pressed_at_ms;
                    ++record_count;
                    uart2_append_record(record_count, g_user_button_hold_ms);
                }
            }
        } else if (previous_pressed != 0U) {
            if (now_ms >= next_hold_refresh_ms) {
                g_user_button_hold_ms = now_ms - button_pressed_at_ms;
                uart2_refresh_live_display(1U, g_user_button_hold_ms);
                next_hold_refresh_ms = now_ms + HOLD_REFRESH_INTERVAL_MS;
            }
        }
    }
}
#endif

/* Active A0 potentiometer UART monitor code. */
#include <stdint.h>

#define BIT(bit)                        (1UL << (bit))

#define SYSCLK_HZ                       8000000UL
#define UART_BAUD_RATE                  9600UL
#define ADC_MAX_COUNTS                  4095UL
#define ADC_VREF_MV                     3300UL
#define POT_TOTAL_RESISTANCE_OHMS       5000UL
#define POT_ROTATION_DEGREES            300UL
#define DECIMAL_1_SCALE                 10UL
#define PRINT_INTERVAL_MS               300UL

#define RCC_BASE                        0x40021000UL
#define GPIOA_BASE                      0x40010800UL
#define ADC1_BASE                       0x40012400UL
#define USART2_BASE                     0x40004400UL
#define SYSTICK_BASE                    0xE000E010UL

#define RCC                             ((RccRegisters *)RCC_BASE)
#define GPIOA                           ((GpioRegisters *)GPIOA_BASE)
#define ADC1                            ((AdcRegisters *)ADC1_BASE)
#define USART2                          ((UsartRegisters *)USART2_BASE)
#define SYSTICK                         ((SysTickRegisters *)SYSTICK_BASE)

#define GPIO_CRL_PIN_MASK(pin)          (0xFUL << ((pin) * 4UL))
#define GPIO_CRL_AF_PP_2MHZ(pin)        (0xAUL << ((pin) * 4UL))

#define RCC_APB2ENR_AFIOEN              BIT(0)
#define RCC_APB2ENR_IOPAEN              BIT(2)
#define RCC_APB2ENR_ADC1EN              BIT(9)
#define RCC_APB1ENR_USART2EN            BIT(17)
#define RCC_CFGR_ADCPRE_MASK            (3UL << 14)
#define RCC_CFGR_ADCPRE_DIV6            (2UL << 14)

#define ADC_SR_EOC                      BIT(1)
#define ADC_CR2_ADON                    BIT(0)
#define ADC_CR2_CAL                     BIT(2)
#define ADC_CR2_RSTCAL                  BIT(3)
#define ADC_CR2_EXTSEL_SWSTART          (7UL << 17)
#define ADC_CR2_EXTTRIG                 BIT(20)
#define ADC_CR2_SWSTART                 BIT(22)

#define USART_SR_TXE                    BIT(7)
#define USART_CR1_TE                    BIT(3)
#define USART_CR1_UE                    BIT(13)

#define SYSTICK_CTRL_ENABLE             BIT(0)
#define SYSTICK_CTRL_CLKSOURCE          BIT(2)
#define SYSTICK_CTRL_COUNTFLAG          BIT(16)

typedef struct {
    volatile uint32_t CR;
    volatile uint32_t CFGR;
    volatile uint32_t CIR;
    volatile uint32_t APB2RSTR;
    volatile uint32_t APB1RSTR;
    volatile uint32_t AHBENR;
    volatile uint32_t APB2ENR;
    volatile uint32_t APB1ENR;
} RccRegisters;

typedef struct {
    volatile uint32_t CRL;
    volatile uint32_t CRH;
    volatile uint32_t IDR;
    volatile uint32_t ODR;
    volatile uint32_t BSRR;
    volatile uint32_t BRR;
    volatile uint32_t LCKR;
} GpioRegisters;

typedef struct {
    volatile uint32_t SR;
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t SMPR1;
    volatile uint32_t SMPR2;
    volatile uint32_t JOFR1;
    volatile uint32_t JOFR2;
    volatile uint32_t JOFR3;
    volatile uint32_t JOFR4;
    volatile uint32_t HTR;
    volatile uint32_t LTR;
    volatile uint32_t SQR1;
    volatile uint32_t SQR2;
    volatile uint32_t SQR3;
    volatile uint32_t JSQR;
    volatile uint32_t JDR1;
    volatile uint32_t JDR2;
    volatile uint32_t JDR3;
    volatile uint32_t JDR4;
    volatile uint32_t DR;
} AdcRegisters;

typedef struct {
    volatile uint32_t SR;
    volatile uint32_t DR;
    volatile uint32_t BRR;
    volatile uint32_t CR1;
} UsartRegisters;

typedef struct {
    volatile uint32_t CTRL;
    volatile uint32_t LOAD;
    volatile uint32_t VAL;
    volatile uint32_t CALIB;
} SysTickRegisters;

typedef struct {
    uint16_t adc_counts;
    uint32_t voltage_mv;
    uint32_t resistance_ohms;
    uint32_t turn_percent_x10;
    uint32_t angle_degrees_x10;
} PotentiometerReading;

void SystemInit(void)
{
    /* Keep the reset clock setup: HSI 8 MHz system clock. */
}

static void delay_ms(uint32_t milliseconds)
{
    while (milliseconds-- != 0UL) {
        SYSTICK->LOAD = (SYSCLK_HZ / 1000UL) - 1UL;
        SYSTICK->VAL = 0UL;
        SYSTICK->CTRL = SYSTICK_CTRL_CLKSOURCE | SYSTICK_CTRL_ENABLE;

        while ((SYSTICK->CTRL & SYSTICK_CTRL_COUNTFLAG) == 0UL) {
        }

        SYSTICK->CTRL = 0UL;
    }
}

static void usart2_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN | RCC_APB2ENR_IOPAEN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    /* PA2 = USART2_TX, connected to ST-LINK Virtual COM on Nucleo-F103RB. */
    GPIOA->CRL = (GPIOA->CRL & ~GPIO_CRL_PIN_MASK(2UL))
                 | GPIO_CRL_AF_PP_2MHZ(2UL);

    USART2->BRR = (SYSCLK_HZ + (UART_BAUD_RATE / 2UL)) / UART_BAUD_RATE;
    USART2->CR1 = USART_CR1_TE | USART_CR1_UE;
}

static void usart2_write_char(char value)
{
    while ((USART2->SR & USART_SR_TXE) == 0UL) {
    }

    USART2->DR = (uint32_t)(uint8_t)value;
}

static void usart2_write_string(const char *text)
{
    while (*text != '\0') {
        usart2_write_char(*text);
        ++text;
    }
}

static void usart2_write_u32(uint32_t value)
{
    char digits[10];
    uint32_t count = 0UL;

    if (value == 0UL) {
        usart2_write_char('0');
        return;
    }

    while (value != 0UL) {
        digits[count] = (char)('0' + (value % 10UL));
        value /= 10UL;
        ++count;
    }

    while (count != 0UL) {
        --count;
        usart2_write_char(digits[count]);
    }
}

static void usart2_write_decimal_1(uint32_t value_x10)
{
    usart2_write_u32(value_x10 / DECIMAL_1_SCALE);
    usart2_write_char('.');
    usart2_write_char((char)('0' + (value_x10 % DECIMAL_1_SCALE)));
}

static void adc1_pa0_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_ADC1EN;
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_ADCPRE_MASK) | RCC_CFGR_ADCPRE_DIV6;

    /* PA0 = Arduino A0 = ADC1_IN0. */
    GPIOA->CRL &= ~GPIO_CRL_PIN_MASK(0UL);

    ADC1->CR1 = 0UL;
    ADC1->CR2 = ADC_CR2_EXTSEL_SWSTART | ADC_CR2_EXTTRIG;
    ADC1->SMPR2 = 7UL;      /* Channel 0 sample time: 239.5 ADC cycles. */
    ADC1->SQR1 = 0UL;       /* One regular conversion. */
    ADC1->SQR3 = 0UL;       /* First conversion is channel 0. */

    ADC1->CR2 |= ADC_CR2_ADON;
    delay_ms(1UL);

    ADC1->CR2 |= ADC_CR2_RSTCAL;
    while ((ADC1->CR2 & ADC_CR2_RSTCAL) != 0UL) {
    }

    ADC1->CR2 |= ADC_CR2_CAL;
    while ((ADC1->CR2 & ADC_CR2_CAL) != 0UL) {
    }
}

static uint16_t adc1_pa0_read(void)
{
    ADC1->CR2 |= ADC_CR2_SWSTART;

    while ((ADC1->SR & ADC_SR_EOC) == 0UL) {
    }

    return (uint16_t)(ADC1->DR & ADC_MAX_COUNTS);
}

static uint32_t adc_to_millivolts(uint16_t adc_counts)
{
    return (((uint32_t)adc_counts * ADC_VREF_MV) + (ADC_MAX_COUNTS / 2UL))
           / ADC_MAX_COUNTS;
}

static uint32_t adc_to_pot_ohms(uint16_t adc_counts)
{
    return (((uint32_t)adc_counts * POT_TOTAL_RESISTANCE_OHMS)
            + (ADC_MAX_COUNTS / 2UL)) / ADC_MAX_COUNTS;
}

static uint32_t adc_to_turn_percent_x10(uint16_t adc_counts)
{
    return (((uint32_t)adc_counts * 100UL * DECIMAL_1_SCALE)
            + (ADC_MAX_COUNTS / 2UL)) / ADC_MAX_COUNTS;
}

static uint32_t adc_to_angle_degrees_x10(uint16_t adc_counts)
{
    return (((uint32_t)adc_counts * POT_ROTATION_DEGREES * DECIMAL_1_SCALE)
            + (ADC_MAX_COUNTS / 2UL)) / ADC_MAX_COUNTS;
}

static PotentiometerReading potentiometer_read(void)
{
    PotentiometerReading reading;

    reading.adc_counts = adc1_pa0_read();
    reading.voltage_mv = adc_to_millivolts(reading.adc_counts);
    reading.resistance_ohms = adc_to_pot_ohms(reading.adc_counts);
    reading.turn_percent_x10 = adc_to_turn_percent_x10(reading.adc_counts);
    reading.angle_degrees_x10 = adc_to_angle_degrees_x10(reading.adc_counts);

    return reading;
}

static void potentiometer_print(const PotentiometerReading *reading)
{
    usart2_write_string("Resistance: ");
    usart2_write_u32(reading->resistance_ohms);
    usart2_write_string(" ohm | ADC: ");
    usart2_write_u32(reading->adc_counts);
    usart2_write_string(" | Voltage: ");
    usart2_write_u32(reading->voltage_mv);
    usart2_write_string(" mV | Turn: ");
    usart2_write_decimal_1(reading->turn_percent_x10);
    usart2_write_string("% | Angle: ");
    usart2_write_decimal_1(reading->angle_degrees_x10);
    usart2_write_string("°\r\n");
}

int main(void)
{
    usart2_init();
    adc1_pa0_init();

    usart2_write_string("A0 potentiometer monitor ready, total 5k ohm, angle 300°\r\n");

    for (;;) {
        PotentiometerReading reading = potentiometer_read();

        potentiometer_print(&reading);
        delay_ms(PRINT_INTERVAL_MS);
    }
}
