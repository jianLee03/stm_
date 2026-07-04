#include <stdint.h>

#define PERIPH_BASE        (0x40000000UL)
#define APB1PERIPH_BASE    PERIPH_BASE
#define APB2PERIPH_BASE    (PERIPH_BASE + 0x10000UL)
#define AHBPERIPH_BASE     (PERIPH_BASE + 0x20000UL)

#define GPIOA_BASE         (APB2PERIPH_BASE + 0x0800UL)
#define GPIOB_BASE         (APB2PERIPH_BASE + 0x0C00UL)
#define RCC_BASE           (AHBPERIPH_BASE  + 0x1000UL)
#define USART3_BASE        (APB1PERIPH_BASE + 0x4800UL)
#define SYSTICK_BASE       (0xE000E010UL)

#define SYSCLK_HZ          (8000000UL)
#define RS485_BAUDRATE     (19200UL)
#define RS485_PERIOD_MS    (50UL)

#define RCC_APB2ENR_IOPAEN     (1UL << 2)
#define RCC_APB2ENR_IOPBEN     (1UL << 3)
#define RCC_APB1ENR_USART3EN   (1UL << 18)

#define USART_SR_TXE       (1UL << 7)
#define USART_SR_TC        (1UL << 6)
#define USART_SR_RXNE      (1UL << 5)
#define USART_CR1_UE       (1UL << 13)
#define USART_CR1_TE       (1UL << 3)
#define USART_CR1_RE       (1UL << 2)

#define SYSTICK_CTRL_ENABLE    (1UL << 0)
#define SYSTICK_CTRL_TICKINT   (1UL << 1)
#define SYSTICK_CTRL_CLKSOURCE (1UL << 2)

#define RS485_DIR_PIN      6U      /* D10 on Nucleo-F103RB: PB6 */
#define RS485_DIR_HIGH     (1UL << RS485_DIR_PIN)

#define LED_PIN            5U      /* Onboard LED (LD2) on Nucleo-F103RB: PA5 */
#define LED_BIT            (1UL << LED_PIN)

#define COMM_TIMEOUT_MS        200UL   /* No valid reply within this window = comm failure. */
#define LED_BLINK_PERIOD_MS    200UL

#define MOTOR_CONTROLLER_MACHINE_ID   183U
#define USER_MACHINE_ID               184U
#define MOTOR_ID                      1U
#define PID_REQ_PID_DATA              4U
#define PID_COMMAND                   10U
#define CMD_ALARM_RESET               8U
#define PID_VEL_CMD                   130U
#define PID_MAIN_DATA                 193U
#define ENABLE                        1U

#define TARGET_RPM                    500

#define BLDC_PACKET_MAX_SIZE          36U
#define BLDC_PACKET_HEADER_SIZE       5U
#define BLDC_PACKET_CHECKSUM_SIZE     1U
#define BLDC_BYTE_DATA_PACKET_SIZE    7U
#define BLDC_VEL_PACKET_SIZE          8U
#define BLDC_STATUS_DATA_SIZE         17U

typedef struct {
    volatile uint32_t CRL;
    volatile uint32_t CRH;
    volatile uint32_t IDR;
    volatile uint32_t ODR;
    volatile uint32_t BSRR;
    volatile uint32_t BRR;
    volatile uint32_t LCKR;
} GPIO_TypeDef;

typedef struct {
    volatile uint32_t CR;
    volatile uint32_t CFGR;
    volatile uint32_t CIR;
    volatile uint32_t APB2RSTR;
    volatile uint32_t APB1RSTR;
    volatile uint32_t AHBENR;
    volatile uint32_t APB2ENR;
    volatile uint32_t APB1ENR;
    volatile uint32_t BDCR;
    volatile uint32_t CSR;
} RCC_TypeDef;

typedef struct {
    volatile uint32_t SR;
    volatile uint32_t DR;
    volatile uint32_t BRR;
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t CR3;
    volatile uint32_t GTPR;
} USART_TypeDef;

typedef struct {
    volatile uint32_t CTRL;
    volatile uint32_t LOAD;
    volatile uint32_t VAL;
    volatile uint32_t CALIB;
} SysTick_TypeDef;

typedef struct {
    uint8_t valid;
    int16_t rpm;
    uint16_t current;
    uint8_t state;
    int32_t position;
} BldcStatus;

#define GPIOA              ((GPIO_TypeDef *)GPIOA_BASE)
#define GPIOB              ((GPIO_TypeDef *)GPIOB_BASE)
#define RCC                ((RCC_TypeDef *)RCC_BASE)
#define USART3             ((USART_TypeDef *)USART3_BASE)
#define SYSTICK            ((SysTick_TypeDef *)SYSTICK_BASE)

static volatile uint32_t g_ms_ticks;

static BldcStatus g_bldc_status;
static uint8_t g_single_status_request_next;

static uint8_t g_rs485_rx_packet[BLDC_PACKET_MAX_SIZE];
static uint8_t g_rs485_rx_index;
static uint8_t g_rs485_rx_expected_len;

static uint8_t g_comm_ever_ok;
static uint32_t g_last_comm_ok_ms;
static uint8_t g_led_blink_state;
static uint32_t g_last_led_toggle_ms;

static void clock_init(void);
static void gpio_init(void);
static void systick_init_1ms(void);
static uint32_t millis(void);
static void usart3_init(void);
static int uart_read_char_nonblocking(USART_TypeDef *uart, char *ch);
static void rs485_dir_tx(void);
static void rs485_dir_rx(void);
static void rs485_write_packet(const uint8_t *data, uint8_t length);
static uint8_t bldc_checksum(const uint8_t *data, uint8_t length);
static int bldc_checksum_is_valid(const uint8_t *data, uint8_t length);
static void bldc_send_byte_data(uint8_t pid, uint8_t data);
static void bldc_send_alarm_reset(void);
static void bldc_send_velocity_cmd(int16_t rpm);
static void bldc_send_periodic_command(void);
static void bldc_poll_rx(void);
static void bldc_rx_reset(void);
static void bldc_rx_process_byte(uint8_t data);
static void bldc_handle_packet(const uint8_t *packet, uint8_t length);
static int16_t get_i16_le(const uint8_t *data);
static uint16_t get_u16_le(const uint8_t *data);
static int32_t get_i32_le(const uint8_t *data);
static void put_i16_le(uint8_t *data, int16_t value);
static void tiny_delay(void);
static void led_on(void);
static void led_off(void);
static void led_toggle(void);

void SysTick_Handler(void)
{
    g_ms_ticks++;
}

int main(void)
{
    uint32_t now;
    uint32_t last_tx_ms;
    uint8_t comm_ok;

    clock_init();
    gpio_init();
    usart3_init();
    systick_init_1ms();
    rs485_dir_rx();
    led_off();

    bldc_send_alarm_reset();

    last_tx_ms = millis();
    g_last_led_toggle_ms = last_tx_ms;

    for (;;) {
        bldc_poll_rx();

        now = millis();

        if ((uint32_t)(now - last_tx_ms) >= RS485_PERIOD_MS) {
            last_tx_ms = now;
            bldc_send_periodic_command();
        }

        comm_ok = (uint8_t)(g_comm_ever_ok &&
                             ((uint32_t)(now - g_last_comm_ok_ms) <= COMM_TIMEOUT_MS));

        if (comm_ok != 0U) {
            led_on();
        } else if ((uint32_t)(now - g_last_led_toggle_ms) >= LED_BLINK_PERIOD_MS) {
            g_last_led_toggle_ms = now;
            led_toggle();
        }
    }
}

static void clock_init(void)
{
    RCC->CR |= 1UL;                      /* HSION */
    while ((RCC->CR & (1UL << 1)) == 0U) {
    }

    RCC->CFGR &= ~3UL;                   /* SYSCLK = HSI */
    while (((RCC->CFGR >> 2) & 3UL) != 0U) {
    }
}

static void gpio_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN;

    /* PA5 onboard LED: general purpose push-pull output, 2 MHz. */
    GPIOA->CRL &= ~(0xFUL << 20);
    GPIOA->CRL |=  (0x2UL << 20);

    /* PB6/D10 RS485 RE+DE direction: low = receive, high = transmit. */
    GPIOB->CRL &= ~(0xFUL << 24);
    GPIOB->CRL |=  (0x3UL << 24);

    /* PB10 USART3_TX for RS485 DI: alternate function push-pull, 50 MHz. */
    GPIOB->CRH &= ~(0xFUL << 8);
    GPIOB->CRH |=  (0xBUL << 8);

    /* PB11 USART3_RX for RS485 RO: input floating. */
    GPIOB->CRH &= ~(0xFUL << 12);
    GPIOB->CRH |=  (0x4UL << 12);
}

static void systick_init_1ms(void)
{
    SYSTICK->LOAD = (SYSCLK_HZ / 1000UL) - 1UL;
    SYSTICK->VAL = 0U;
    SYSTICK->CTRL = SYSTICK_CTRL_CLKSOURCE | SYSTICK_CTRL_TICKINT | SYSTICK_CTRL_ENABLE;
}

static uint32_t millis(void)
{
    return g_ms_ticks;
}

static void usart3_init(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_USART3EN;

    USART3->CR1 = 0U;
    USART3->BRR = (SYSCLK_HZ + (RS485_BAUDRATE / 2U)) / RS485_BAUDRATE;
    USART3->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
}

static int uart_read_char_nonblocking(USART_TypeDef *uart, char *ch)
{
    if ((uart->SR & USART_SR_RXNE) == 0U) {
        return 0;
    }

    *ch = (char)(uart->DR & 0xFFU);
    return 1;
}

static void rs485_dir_tx(void)
{
    GPIOB->BSRR = RS485_DIR_HIGH;
}

static void rs485_dir_rx(void)
{
    GPIOB->BRR = RS485_DIR_HIGH;
}

static void rs485_write_packet(const uint8_t *data, uint8_t length)
{
    uint8_t i;

    rs485_dir_tx();
    tiny_delay();

    for (i = 0U; i < length; i++) {
        while ((USART3->SR & USART_SR_TXE) == 0U) {
        }
        USART3->DR = data[i];
    }

    while ((USART3->SR & USART_SR_TC) == 0U) {
    }

    tiny_delay();
    rs485_dir_rx();
}

static uint8_t bldc_checksum(const uint8_t *data, uint8_t length)
{
    uint8_t sum = 0U;
    uint8_t i;

    for (i = 0U; i < length; i++) {
        sum = (uint8_t)(sum + data[i]);
    }

    return (uint8_t)(0U - sum);
}

static int bldc_checksum_is_valid(const uint8_t *data, uint8_t length)
{
    uint8_t sum = 0U;
    uint8_t i;

    for (i = 0U; i < length; i++) {
        sum = (uint8_t)(sum + data[i]);
    }

    return (sum == 0U);
}

static void bldc_send_byte_data(uint8_t pid, uint8_t data)
{
    uint8_t packet[BLDC_BYTE_DATA_PACKET_SIZE];

    packet[0] = MOTOR_CONTROLLER_MACHINE_ID;
    packet[1] = USER_MACHINE_ID;
    packet[2] = MOTOR_ID;
    packet[3] = pid;
    packet[4] = 1U;
    packet[5] = data;
    packet[6] = bldc_checksum(packet, 6U);

    rs485_write_packet(packet, BLDC_BYTE_DATA_PACKET_SIZE);
}

static void bldc_send_alarm_reset(void)
{
    bldc_send_byte_data(PID_COMMAND, CMD_ALARM_RESET);
}

static void bldc_send_velocity_cmd(int16_t rpm)
{
    uint8_t packet[BLDC_VEL_PACKET_SIZE];

    packet[0] = MOTOR_CONTROLLER_MACHINE_ID;
    packet[1] = USER_MACHINE_ID;
    packet[2] = MOTOR_ID;
    packet[3] = PID_VEL_CMD;
    packet[4] = 2U;
    put_i16_le(&packet[5], rpm);
    packet[7] = bldc_checksum(packet, 7U);

    rs485_write_packet(packet, BLDC_VEL_PACKET_SIZE);
}

static void bldc_send_periodic_command(void)
{
    if (g_single_status_request_next != 0U) {
        bldc_send_byte_data(PID_REQ_PID_DATA, PID_MAIN_DATA);
        g_single_status_request_next = 0U;
    } else {
        bldc_send_velocity_cmd((int16_t)TARGET_RPM);
        g_single_status_request_next = 1U;
    }
}

static void bldc_poll_rx(void)
{
    char ch;

    while (uart_read_char_nonblocking(USART3, &ch)) {
        bldc_rx_process_byte((uint8_t)ch);
    }
}

static void bldc_rx_reset(void)
{
    g_rs485_rx_index = 0U;
    g_rs485_rx_expected_len = 0U;
}

static void bldc_rx_process_byte(uint8_t data)
{
    uint8_t data_num;
    uint8_t total_len;

    if (g_rs485_rx_index == 0U) {
        if (data != USER_MACHINE_ID) {
            return;
        }
    }

    if (g_rs485_rx_index >= BLDC_PACKET_MAX_SIZE) {
        bldc_rx_reset();
        return;
    }

    g_rs485_rx_packet[g_rs485_rx_index++] = data;

    if ((g_rs485_rx_index == 2U) &&
        (g_rs485_rx_packet[1] != MOTOR_CONTROLLER_MACHINE_ID)) {
        bldc_rx_reset();
        return;
    }

    if ((g_rs485_rx_index == 3U) &&
        (g_rs485_rx_packet[2] != MOTOR_ID)) {
        bldc_rx_reset();
        return;
    }

    if (g_rs485_rx_index == BLDC_PACKET_HEADER_SIZE) {
        data_num = g_rs485_rx_packet[4];
        if (data_num > (BLDC_PACKET_MAX_SIZE -
                        BLDC_PACKET_HEADER_SIZE -
                        BLDC_PACKET_CHECKSUM_SIZE)) {
            bldc_rx_reset();
            return;
        }

        total_len = (uint8_t)(BLDC_PACKET_HEADER_SIZE +
                              data_num +
                              BLDC_PACKET_CHECKSUM_SIZE);

        if (total_len > BLDC_PACKET_MAX_SIZE) {
            bldc_rx_reset();
            return;
        }

        g_rs485_rx_expected_len = total_len;
    }

    if ((g_rs485_rx_expected_len != 0U) &&
        (g_rs485_rx_index >= g_rs485_rx_expected_len)) {
        if (bldc_checksum_is_valid(g_rs485_rx_packet, g_rs485_rx_expected_len)) {
            bldc_handle_packet(g_rs485_rx_packet, g_rs485_rx_expected_len);
            g_comm_ever_ok = 1U;
            g_last_comm_ok_ms = millis();
        }

        bldc_rx_reset();
    }
}

static void bldc_handle_packet(const uint8_t *packet, uint8_t length)
{
    if ((packet[3] == PID_MAIN_DATA) &&
        (packet[4] == BLDC_STATUS_DATA_SIZE) &&
        (length >= (BLDC_PACKET_HEADER_SIZE +
                    BLDC_STATUS_DATA_SIZE +
                    BLDC_PACKET_CHECKSUM_SIZE))) {
        g_bldc_status.valid = 1U;
        g_bldc_status.rpm = get_i16_le(&packet[5]);
        g_bldc_status.current = get_u16_le(&packet[7]);
        g_bldc_status.state = packet[14];
        g_bldc_status.position = get_i32_le(&packet[15]);
    }
}

static int16_t get_i16_le(const uint8_t *data)
{
    return (int16_t)((uint16_t)data[0] | ((uint16_t)data[1] << 8));
}

static uint16_t get_u16_le(const uint8_t *data)
{
    return (uint16_t)((uint16_t)data[0] | ((uint16_t)data[1] << 8));
}

static int32_t get_i32_le(const uint8_t *data)
{
    return (int32_t)((uint32_t)data[0] |
                     ((uint32_t)data[1] << 8) |
                     ((uint32_t)data[2] << 16) |
                     ((uint32_t)data[3] << 24));
}

static void put_i16_le(uint8_t *data, int16_t value)
{
    uint16_t raw = (uint16_t)value;

    data[0] = (uint8_t)(raw & 0xFFU);
    data[1] = (uint8_t)((raw >> 8) & 0xFFU);
}

static void tiny_delay(void)
{
    volatile uint32_t i;

    for (i = 0U; i < 80U; i++) {
    }
}

static void led_on(void)
{
    g_led_blink_state = 1U;
    GPIOA->BSRR = LED_BIT;
}

static void led_off(void)
{
    g_led_blink_state = 0U;
    GPIOA->BRR = LED_BIT;
}

static void led_toggle(void)
{
    if (g_led_blink_state != 0U) {
        led_off();
    } else {
        led_on();
    }
}
