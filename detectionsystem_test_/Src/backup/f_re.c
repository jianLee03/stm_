#include <stdint.h>

#define RCC_CFGR       (*(volatile uint32_t *)0x40021004u)
#define AFIO_MAPR      (*(volatile uint32_t *)0x40010004u)
#define RCC_APB2ENR    (*(volatile uint32_t *)0x40021018u)
#define RCC_APB1ENR    (*(volatile uint32_t *)0x4002101Cu)

#define GPIOA_CRL      (*(volatile uint32_t *)0x40010800u)
#define GPIOA_BSRR     (*(volatile uint32_t *)0x40010810u)
#define GPIOA_BRR      (*(volatile uint32_t *)0x40010814u)
#define GPIOB_CRL      (*(volatile uint32_t *)0x40010C00u)
#define GPIOB_CRH      (*(volatile uint32_t *)0x40010C04u)
#define GPIOB_IDR      (*(volatile uint32_t *)0x40010C08u)
#define GPIOB_BSRR     (*(volatile uint32_t *)0x40010C10u)
#define GPIOB_BRR      (*(volatile uint32_t *)0x40010C14u)
#define GPIOC_CRL      (*(volatile uint32_t *)0x40011000u)

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

#define TIM3_CR1       (*(volatile uint32_t *)0x40000400u)
#define TIM3_EGR       (*(volatile uint32_t *)0x40000414u)
#define TIM3_CCMR1     (*(volatile uint32_t *)0x40000418u)
#define TIM3_CCER      (*(volatile uint32_t *)0x40000420u)
#define TIM3_CNT       (*(volatile uint32_t *)0x40000424u)
#define TIM3_PSC       (*(volatile uint32_t *)0x40000428u)
#define TIM3_ARR       (*(volatile uint32_t *)0x4000042Cu)
#define TIM3_CCR2      (*(volatile uint32_t *)0x40000438u)

#define NVIC_ISER0     (*(volatile uint32_t *)0xE000E100u)

#define RCC_APB2ENR_AFIOEN   (1u << 0)
#define RCC_APB2ENR_IOPAEN   (1u << 2)
#define RCC_APB2ENR_IOPBEN   (1u << 3)
#define RCC_APB2ENR_IOPCEN   (1u << 4)
#define RCC_APB2ENR_ADC1EN   (1u << 9)
#define RCC_APB1ENR_TIM2EN   (1u << 0)
#define RCC_APB1ENR_TIM3EN   (1u << 1)
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
#define TIM_CCMR1_OC2PE      (1u << 11)
#define TIM_CCMR1_OC2M_PWM1  (6u << 12)
#define TIM_CCER_CC2E        (1u << 4)

#define AFIO_MAPR_TIM3_REMAP_MASK  (3u << 10)
#define AFIO_MAPR_TIM3_REMAP_FULL  (3u << 10)

#define NVIC_IRQ_TIM2        28u

#define GPIO_CRL_PIN_MASK(pin) (0xFu << ((pin) * 4u))
#define GPIO_CRH_PIN_MASK(pin) (0xFu << (((pin) - 8u) * 4u))
#define GPIO_CRH_PIN_SHIFT(pin) (((pin) - 8u) * 4u)
#define GPIO_MODE_OUTPUT_PP_2MHZ 0x2u
#define GPIO_MODE_OUTPUT_OD_2MHZ 0x6u
#define GPIO_MODE_INPUT_PULL     0x8u

#define ADC_CHANNEL_R        0u
#define ADC_CHANNEL_L        1u
#define ADC_MAX_VALUE        4095u
#define ADC_MAX_SAMPLE_TIME  7u
#define FILTER_SAMPLE_COUNT  10u
#define RISK_PERCENT_MAX     100u
#define RISK_LIGHT_WEIGHT    35u
#define RISK_RESIST_WEIGHT   65u
#define RISK_WEIGHT_TOTAL    (RISK_LIGHT_WEIGHT + RISK_RESIST_WEIGHT)
#define SAMPLE_PERIOD_MS     1u
#define LED_UPDATE_PERIOD_MS 100u
#define SERIAL_OUTPUT_PERIOD_MS 500u

#define SAFE_LED_D9_PIN          7u
#define WARNING_LED_D10_PIN      6u
#define WARNING_LED_D10_MASK     (1u << WARNING_LED_D10_PIN)
#define SAFE_LED_BRIGHTNESS_MAX  10u
#define SAFE_LED_BRIGHTNESS_SAFE 5u

#define BUZZER_D11_PIN              7u
#define BUZZER_D11_MASK             (1u << BUZZER_D11_PIN)
#define BUZZER_WARNING_HZ           4000u
#define TIM2_COUNTER_CLOCK_DIVIDER  8u
#define TIM2_TICK_PERIOD_US         (1000000u / (BUZZER_WARNING_HZ * 2u))
#define SAMPLE_PERIOD_TICKS         ((SAMPLE_PERIOD_MS * 1000u) / TIM2_TICK_PERIOD_US)
#define LED_UPDATE_PERIOD_TICKS     ((LED_UPDATE_PERIOD_MS + SAMPLE_PERIOD_MS - 1u) / SAMPLE_PERIOD_MS)
#define SERIAL_OUTPUT_PERIOD_TICKS  ((SERIAL_OUTPUT_PERIOD_MS + SAMPLE_PERIOD_MS - 1u) / SAMPLE_PERIOD_MS)

#define OLED_I2C_ADDRESS_PRIMARY   0x3Cu
#define OLED_I2C_ADDRESS_ALTERNATE 0x3Du
#define OLED_WIDTH                 128u
#define OLED_HEIGHT                32u
#define OLED_PAGE_COUNT            (OLED_HEIGHT / 8u)
#define OLED_BUFFER_SIZE           (OLED_WIDTH * OLED_PAGE_COUNT)
#define OLED_SCL_PIN               5u
#define OLED_SDA_PIN               9u
#define OLED_SCL_MASK              (1u << OLED_SCL_PIN)
#define OLED_SDA_MASK              (1u << OLED_SDA_PIN)
#define OLED_FONT_WIDTH            5u
#define OLED_FONT_HEIGHT           7u

typedef struct {
    uint16_t samples[FILTER_SAMPLE_COUNT];
    uint32_t sum;
    uint32_t index;
    uint32_t count;
} MovingAverageFilter;

static volatile uint32_t tim2_period_elapsed = 0u;
static volatile uint32_t tim2_sample_tick_count = 0u;
static volatile uint32_t buzzer_enabled = 0u;
static volatile uint32_t buzzer_output_high = 0u;
static uint8_t oled_address = OLED_I2C_ADDRESS_PRIMARY;
static const char *oled_detect_text = "NOACK";
static uint8_t oled_buffer[OLED_BUFFER_SIZE];

static const uint8_t oled_font_space[OLED_FONT_WIDTH] = {
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u
};
static const uint8_t oled_font_a[OLED_FONT_WIDTH] = {
    0x7Eu, 0x11u, 0x11u, 0x11u, 0x7Eu
};
static const uint8_t oled_font_d[OLED_FONT_WIDTH] = {
    0x7Fu, 0x41u, 0x41u, 0x22u, 0x1Cu
};
static const uint8_t oled_font_e[OLED_FONT_WIDTH] = {
    0x7Fu, 0x49u, 0x49u, 0x49u, 0x41u
};
static const uint8_t oled_font_f[OLED_FONT_WIDTH] = {
    0x7Fu, 0x09u, 0x09u, 0x09u, 0x01u
};
static const uint8_t oled_font_g[OLED_FONT_WIDTH] = {
    0x3Eu, 0x41u, 0x49u, 0x49u, 0x7Au
};
static const uint8_t oled_font_i[OLED_FONT_WIDTH] = {
    0x00u, 0x41u, 0x7Fu, 0x41u, 0x00u
};
static const uint8_t oled_font_n[OLED_FONT_WIDTH] = {
    0x7Fu, 0x02u, 0x0Cu, 0x10u, 0x7Fu
};
static const uint8_t oled_font_r[OLED_FONT_WIDTH] = {
    0x7Fu, 0x09u, 0x19u, 0x29u, 0x46u
};
static const uint8_t oled_font_s[OLED_FONT_WIDTH] = {
    0x46u, 0x49u, 0x49u, 0x49u, 0x31u
};
static const uint8_t oled_font_w[OLED_FONT_WIDTH] = {
    0x7Fu, 0x20u, 0x18u, 0x20u, 0x7Fu
};

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

static void oled_i2c_delay(void)
{
    for (volatile uint32_t i = 0u; i < 100u; ++i) {
        __asm volatile ("nop");
    }
}

static void oled_scl_high(void)
{
    GPIOA_BSRR = OLED_SCL_MASK;
    oled_i2c_delay();
}

static void oled_scl_low(void)
{
    GPIOA_BRR = OLED_SCL_MASK;
    oled_i2c_delay();
}

static void oled_sda_high(void)
{
    GPIOB_BSRR = OLED_SDA_MASK;
    oled_i2c_delay();
}

static void oled_sda_low(void)
{
    GPIOB_BRR = OLED_SDA_MASK;
    oled_i2c_delay();
}

static void oled_i2c_start(void)
{
    oled_sda_high();
    oled_scl_high();
    oled_sda_low();
    oled_scl_low();
}

static void oled_i2c_stop(void)
{
    oled_sda_low();
    oled_scl_high();
    oled_sda_high();
}

static void oled_i2c_recover_bus(void)
{
    oled_sda_high();

    for (uint32_t i = 0u; i < 9u; ++i) {
        oled_scl_low();
        oled_scl_high();
    }

    oled_i2c_stop();
}

static uint32_t oled_i2c_write_byte(uint8_t byte)
{
    uint32_t ack;

    for (uint32_t mask = 0x80u; mask != 0u; mask >>= 1u) {
        if ((byte & mask) != 0u) {
            oled_sda_high();
        } else {
            oled_sda_low();
        }

        oled_scl_high();
        oled_scl_low();
    }

    oled_sda_high();
    oled_scl_high();
    ack = ((GPIOB_IDR & OLED_SDA_MASK) == 0u);
    oled_scl_low();

    return ack;
}

static uint32_t oled_probe_address(uint8_t address)
{
    uint32_t ack;

    oled_i2c_start();
    ack = oled_i2c_write_byte((uint8_t)(address << 1));
    oled_i2c_stop();

    return ack;
}

static void oled_gpio_init(void)
{
    RCC_APB2ENR |= RCC_APB2ENR_AFIOEN | RCC_APB2ENR_IOPAEN |
                   RCC_APB2ENR_IOPBEN;

    GPIOA_BSRR = OLED_SCL_MASK;
    GPIOB_BSRR = OLED_SDA_MASK;
    GPIOA_CRL = (GPIOA_CRL & ~GPIO_CRL_PIN_MASK(OLED_SCL_PIN)) |
                (GPIO_MODE_OUTPUT_OD_2MHZ << (OLED_SCL_PIN * 4u));
    GPIOB_CRH = (GPIOB_CRH & ~GPIO_CRH_PIN_MASK(OLED_SDA_PIN)) |
                (GPIO_MODE_OUTPUT_OD_2MHZ << GPIO_CRH_PIN_SHIFT(OLED_SDA_PIN));
    GPIOA_BSRR = OLED_SCL_MASK;
    GPIOB_BSRR = OLED_SDA_MASK;
}

static void oled_write_commands(const uint8_t *commands, uint32_t count)
{
    oled_i2c_start();
    (void)oled_i2c_write_byte((uint8_t)(oled_address << 1));
    (void)oled_i2c_write_byte(0x00u);

    while (count-- > 0u) {
        (void)oled_i2c_write_byte(*commands++);
    }

    oled_i2c_stop();
}

static void oled_write_command(uint8_t command)
{
    oled_write_commands(&command, 1u);
}

static void oled_write_data(const uint8_t *data, uint32_t count)
{
    oled_i2c_start();
    (void)oled_i2c_write_byte((uint8_t)(oled_address << 1));
    (void)oled_i2c_write_byte(0x40u);

    while (count-- > 0u) {
        (void)oled_i2c_write_byte(*data++);
    }

    oled_i2c_stop();
}

static void oled_clear_buffer(void)
{
    for (uint32_t i = 0u; i < OLED_BUFFER_SIZE; ++i) {
        oled_buffer[i] = 0u;
    }
}

static void oled_flush(void)
{
    uint8_t commands[6];

    commands[0] = 0x21u;
    commands[1] = 0u;
    commands[2] = (uint8_t)(OLED_WIDTH - 1u);
    commands[3] = 0x22u;
    commands[4] = 0u;
    commands[5] = (uint8_t)(OLED_PAGE_COUNT - 1u);
    oled_write_commands(commands, 6u);
    oled_write_data(oled_buffer, OLED_BUFFER_SIZE);
}

static const uint8_t *oled_get_glyph(char ch)
{
    switch (ch) {
    case 'A':
        return oled_font_a;
    case 'D':
        return oled_font_d;
    case 'E':
        return oled_font_e;
    case 'F':
        return oled_font_f;
    case 'G':
        return oled_font_g;
    case 'I':
        return oled_font_i;
    case 'N':
        return oled_font_n;
    case 'R':
        return oled_font_r;
    case 'S':
        return oled_font_s;
    case 'W':
        return oled_font_w;
    default:
        return oled_font_space;
    }
}

static void oled_draw_pixel(uint32_t x, uint32_t y)
{
    if ((x >= OLED_WIDTH) || (y >= OLED_HEIGHT)) {
        return;
    }

    oled_buffer[x + ((y >> 3u) * OLED_WIDTH)] |= (uint8_t)(1u << (y & 0x07u));
}

static uint32_t oled_text_width(const char *text, uint32_t scale)
{
    uint32_t width = 0u;

    while (*text != '\0') {
        width += 6u * scale;
        text++;
    }

    if (width >= scale) {
        width -= scale;
    }

    return width;
}

static void oled_draw_char_scaled(uint32_t x, uint32_t y, char ch, uint32_t scale)
{
    const uint8_t *glyph = oled_get_glyph(ch);

    for (uint32_t col = 0u; col < OLED_FONT_WIDTH; ++col) {
        for (uint32_t row = 0u; row < OLED_FONT_HEIGHT; ++row) {
            if ((glyph[col] & (1u << row)) == 0u) {
                continue;
            }

            for (uint32_t dx = 0u; dx < scale; ++dx) {
                for (uint32_t dy = 0u; dy < scale; ++dy) {
                    oled_draw_pixel(x + (col * scale) + dx,
                                    y + (row * scale) + dy);
                }
            }
        }
    }
}

static void oled_draw_text_centered(const char *text, uint32_t y, uint32_t scale)
{
    uint32_t width = oled_text_width(text, scale);
    uint32_t x = 0u;

    if (width < OLED_WIDTH) {
        x = (OLED_WIDTH - width) / 2u;
    }

    while (*text != '\0') {
        oled_draw_char_scaled(x, y, *text, scale);
        x += 6u * scale;
        text++;
    }
}

static void oled_show_status(const char *status)
{
    oled_clear_buffer();
    oled_draw_text_centered(status, 9u, 2u);
    oled_flush();
}

static void oled_write_ack_text(uint32_t ack)
{
    if (ack != 0u) {
        usart2_write_string("ACK");
    } else {
        usart2_write_string("NOACK");
    }
}

static void oled_init(void)
{
    static const uint8_t init_sequence[] = {
        0xAEu,
        0xD5u, 0x80u,
        0xA8u, 0x1Fu,
        0xD3u, 0x00u,
        0x40u,
        0x8Du, 0x14u,
        0x20u, 0x00u,
        0xA1u,
        0xC8u,
        0xDAu, 0x02u,
        0x81u, 0x8Fu,
        0xD9u, 0xF1u,
        0xDBu, 0x40u,
        0xA4u,
        0xA6u,
        0x2Eu,
        0xAFu
    };
    uint32_t ack_primary;
    uint32_t ack_alternate;

    oled_gpio_init();
    delay_ms(100u);
    oled_i2c_recover_bus();

    ack_primary = oled_probe_address(OLED_I2C_ADDRESS_PRIMARY);
    ack_alternate = oled_probe_address(OLED_I2C_ADDRESS_ALTERNATE);

    usart2_write_string("OLED scan PA5=SCK/SCL PB9=SDA 0x3C=");
    oled_write_ack_text(ack_primary);
    usart2_write_string(" 0x3D=");
    oled_write_ack_text(ack_alternate);
    usart2_write_string(" SDA=");
    usart2_write_u32((GPIOB_IDR & OLED_SDA_MASK) != 0u);
    usart2_write_string("\r\n");

    if (ack_primary != 0u) {
        oled_address = OLED_I2C_ADDRESS_PRIMARY;
        oled_detect_text = "0x3C";
    } else if (ack_alternate != 0u) {
        oled_address = OLED_I2C_ADDRESS_ALTERNATE;
        oled_detect_text = "0x3D";
    } else {
        oled_address = OLED_I2C_ADDRESS_PRIMARY;
        oled_detect_text = "NOACK";
        usart2_write_string("OLED not detected: check VCC/GND and SCK->D13/PA5, SDA->D14/PB9\r\n");
    }

    for (uint32_t i = 0u;
         i < (uint32_t)(sizeof(init_sequence) / sizeof(init_sequence[0]));
         ++i) {
        oled_write_command(init_sequence[i]);
    }
    oled_write_command(0xA5u);
    delay_ms(800u);
    oled_write_command(0xA4u);
    oled_show_status("SAFE");
}

static void safe_led_init(void)
{
    RCC_APB2ENR |= RCC_APB2ENR_AFIOEN | RCC_APB2ENR_IOPBEN |
                   RCC_APB2ENR_IOPCEN;
    RCC_APB1ENR |= RCC_APB1ENR_TIM3EN;

    GPIOB_BSRR = WARNING_LED_D10_MASK;
    GPIOB_CRL = (GPIOB_CRL & ~GPIO_CRL_PIN_MASK(WARNING_LED_D10_PIN)) |
                (GPIO_MODE_OUTPUT_PP_2MHZ << (WARNING_LED_D10_PIN * 4u));
    GPIOB_BSRR = WARNING_LED_D10_MASK;

    AFIO_MAPR = (AFIO_MAPR & ~AFIO_MAPR_TIM3_REMAP_MASK) |
                AFIO_MAPR_TIM3_REMAP_FULL;

    GPIOC_CRL = (GPIOC_CRL & ~GPIO_CRL_PIN_MASK(SAFE_LED_D9_PIN)) |
                (0xAu << (SAFE_LED_D9_PIN * 4u));

    TIM3_CR1 = 0u;
    TIM3_PSC = 800u - 1u;
    TIM3_ARR = SAFE_LED_BRIGHTNESS_MAX - 1u;
    TIM3_CCR2 = SAFE_LED_BRIGHTNESS_MAX;
    TIM3_CNT = 0u;
    TIM3_CCMR1 = (TIM3_CCMR1 & ~(7u << 12)) |
                 TIM_CCMR1_OC2M_PWM1 | TIM_CCMR1_OC2PE;
    TIM3_CCER |= TIM_CCER_CC2E;
    TIM3_EGR = TIM_EGR_UG;
    TIM3_CR1 = TIM_CR1_CEN;
}

static void warning_led_d10_set(uint32_t on)
{
    if (on != 0u) {
        GPIOB_BRR = WARNING_LED_D10_MASK;
    } else {
        GPIOB_BSRR = WARNING_LED_D10_MASK;
    }
}

static void safe_led_set_brightness(uint32_t brightness)
{
    if (brightness > SAFE_LED_BRIGHTNESS_MAX) {
        brightness = SAFE_LED_BRIGHTNESS_MAX;
    }

    TIM3_CCR2 = SAFE_LED_BRIGHTNESS_MAX - brightness;
}

static void safe_led_update(const char *status)
{
    static uint32_t warning_blink_phase;

    if (status[0] == 'S') {
        warning_blink_phase = 0u;
        warning_led_d10_set(0u);
        safe_led_set_brightness(SAFE_LED_BRIGHTNESS_SAFE);
    } else if (status[0] == 'W') {
        if (warning_blink_phase == 0u) {
            warning_led_d10_set(1u);
            safe_led_set_brightness(0u);
            warning_blink_phase = 1u;
        } else {
            warning_led_d10_set(0u);
            safe_led_set_brightness(SAFE_LED_BRIGHTNESS_MAX);
            warning_blink_phase = 0u;
        }
    } else {
        warning_blink_phase = 0u;
        warning_led_d10_set(1u);
        safe_led_set_brightness(SAFE_LED_BRIGHTNESS_MAX);
    }
}

static void buzzer_d11_init(void)
{
    RCC_APB2ENR |= RCC_APB2ENR_IOPAEN;

    GPIOA_BRR = BUZZER_D11_MASK;
    GPIOA_CRL = (GPIOA_CRL & ~GPIO_CRL_PIN_MASK(BUZZER_D11_PIN)) |
                (GPIO_MODE_OUTPUT_PP_2MHZ << (BUZZER_D11_PIN * 4u));
    GPIOA_BRR = BUZZER_D11_MASK;
}

static void buzzer_set(uint32_t on)
{
    if (on != 0u) {
        buzzer_enabled = 1u;
        return;
    }

    buzzer_enabled = 0u;
    buzzer_output_high = 0u;
    GPIOA_BRR = BUZZER_D11_MASK;
}

static void buzzer_update(const char *status)
{
    buzzer_set(status[0] == 'D');
}

static void buzzer_timer_tick_from_isr(void)
{
    if (buzzer_enabled != 0u) {
        if (buzzer_output_high == 0u) {
            GPIOA_BSRR = BUZZER_D11_MASK;
            buzzer_output_high = 1u;
        } else {
            GPIOA_BRR = BUZZER_D11_MASK;
            buzzer_output_high = 0u;
        }
    } else if (buzzer_output_high != 0u) {
        GPIOA_BRR = BUZZER_D11_MASK;
        buzzer_output_high = 0u;
    }
}

static uint32_t tim2_take_sample_tick(void)
{
    uint32_t tick_available = 0u;

    __asm volatile ("cpsid i" ::: "memory");
    if (tim2_period_elapsed != 0u) {
        tim2_period_elapsed--;
        tick_available = 1u;
    }
    __asm volatile ("cpsie i" ::: "memory");

    return tick_available;
}

static void tim2_init(void)
{
    RCC_APB1ENR |= RCC_APB1ENR_TIM2EN;

    tim2_period_elapsed = 0u;
    tim2_sample_tick_count = 0u;
    TIM2_CR1 = 0u;
    TIM2_PSC = TIM2_COUNTER_CLOCK_DIVIDER - 1u;
    TIM2_ARR = TIM2_TICK_PERIOD_US - 1u;
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

static const char *risk_status_text(uint32_t risk_level)
{
    if (risk_level <= 30u) {
        return "SAFE";
    }

    if (risk_level < 70u) {
        return "WARNING";
    }

    return "DANGER";
}

static void serial_write_risk(uint32_t light_level,
                              uint32_t resistance_level,
                              uint32_t risk_level,
                              const char *risk_text)
{
    usart2_write_string("L=");
    usart2_write_u32(light_level);
    usart2_write_string(" R=");
    usart2_write_u32(resistance_level);
    usart2_write_string(" C=");
    usart2_write_u32(risk_level);
    usart2_write_string(" STATUS=");
    usart2_write_string(risk_text);
    usart2_write_string(" OLED=");
    usart2_write_string(oled_detect_text);
    usart2_write_string("\r\n");
}

void TIM2_IRQHandler(void)
{
    if ((TIM2_SR & TIM_SR_UIF) != 0u) {
        TIM2_SR &= ~TIM_SR_UIF;
        buzzer_timer_tick_from_isr();

        tim2_sample_tick_count++;
        if (tim2_sample_tick_count >= SAMPLE_PERIOD_TICKS) {
            tim2_sample_tick_count = 0u;
            if (tim2_period_elapsed < 0xFFFFFFFFu) {
                tim2_period_elapsed++;
            }
        }
    }
}

int main(void)
{
    MovingAverageFilter light_filter = {0u};
    MovingAverageFilter resistance_filter = {0u};
    uint32_t led_update_tick_count = 0u;
    uint32_t serial_output_tick_count = 0u;
    char last_status_initial = '\0';

    usart2_init();
    adc1_init();
    oled_init();
    safe_led_init();
    buzzer_d11_init();
    safe_led_update("SAFE");
    tim2_init();

    usart2_write_string("STM32F103 risk monitor with OLED ready\r\n");

    for (;;) {
        uint16_t light_raw;
        uint16_t resistance_raw;
        uint32_t light_level;
        uint32_t resistance_level;
        uint32_t risk_level;
        const char *risk_text;
        uint32_t status_changed;

        if (tim2_take_sample_tick() == 0u) {
            continue;
        }

        light_raw = adc1_read_channel(ADC_CHANNEL_L);
        resistance_raw = adc1_read_channel(ADC_CHANNEL_R);

        light_level = moving_average_update(&light_filter, light_raw);
        resistance_level = moving_average_update(&resistance_filter, resistance_raw);
        risk_level = calculate_risk_percent(light_level, resistance_level);
        risk_text = risk_status_text(risk_level);
        status_changed = (risk_text[0] != last_status_initial);

        buzzer_update(risk_text);

        if (led_update_tick_count < LED_UPDATE_PERIOD_TICKS) {
            led_update_tick_count++;
        }
        if (serial_output_tick_count < SERIAL_OUTPUT_PERIOD_TICKS) {
            serial_output_tick_count++;
        }

        if (status_changed != 0u) {
            safe_led_update(risk_text);
            oled_show_status(risk_text);
            serial_write_risk(light_level, resistance_level, risk_level, risk_text);
            last_status_initial = risk_text[0];
            led_update_tick_count = 0u;
            serial_output_tick_count = 0u;
        } else {
            if (led_update_tick_count >= LED_UPDATE_PERIOD_TICKS) {
                safe_led_update(risk_text);
                led_update_tick_count = 0u;
            }
            if (serial_output_tick_count >= SERIAL_OUTPUT_PERIOD_TICKS) {
                serial_write_risk(light_level, resistance_level, risk_level, risk_text);
                serial_output_tick_count = 0u;
            }
        }
    }
}
