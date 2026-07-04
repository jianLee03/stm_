#include <stdint.h>

#define PERIPH_BASE        (0x40000000UL)
#define APB1PERIPH_BASE    PERIPH_BASE
#define APB2PERIPH_BASE    (PERIPH_BASE + 0x10000UL)
#define AHBPERIPH_BASE     (PERIPH_BASE + 0x20000UL)

#define GPIOA_BASE         (APB2PERIPH_BASE + 0x0800UL)
#define GPIOB_BASE         (APB2PERIPH_BASE + 0x0C00UL)
#define RCC_BASE           (AHBPERIPH_BASE  + 0x1000UL)
#define USART2_BASE        (APB1PERIPH_BASE + 0x4400UL)
#define USART3_BASE        (APB1PERIPH_BASE + 0x4800UL)
#define SYSTICK_BASE       (0xE000E010UL)

#define SYSCLK_HZ          (8000000UL)
#define PC_BAUDRATE        (9600UL)
#define RS485_BAUDRATE     (57600UL)
#define RS485_PERIOD_MS    (50UL)

#define RCC_APB2ENR_AFIOEN     (1UL << 0)
#define RCC_APB2ENR_IOPAEN     (1UL << 2)
#define RCC_APB2ENR_IOPBEN     (1UL << 3)
#define RCC_APB1ENR_USART2EN   (1UL << 17)
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

#define MOTOR_CONTROLLER_MACHINE_ID   183U
#define USER_MACHINE_ID               184U
#define MOTOR_ID                      1U
#define PID_PNT_VEL_CMD               207U
#define ENABLE                        1U
#define RETURN_PNT_MAIN_DATA          2U

#define BLDC_PACKET_MAX_SIZE          32U
#define BLDC_PACKET_HEADER_SIZE       5U
#define BLDC_PACKET_CHECKSUM_SIZE     1U
#define BLDC_PNT_VEL_PACKET_SIZE      13U
#define BLDC_DUAL_STATUS_DATA_SIZE    18U
#define PC_LINE_MAX_SIZE              32U

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
    uint8_t pid;
    uint8_t data_num;
    int16_t left_rpm;
    uint16_t left_current;
    uint8_t left_state;
    int32_t left_position;
    int16_t right_rpm;
    uint16_t right_current;
    uint8_t right_state;
    int32_t right_position;
} BldcDualStatus;

#define GPIOA              ((GPIO_TypeDef *)GPIOA_BASE)
#define GPIOB              ((GPIO_TypeDef *)GPIOB_BASE)
#define RCC                ((RCC_TypeDef *)RCC_BASE)
#define USART2             ((USART_TypeDef *)USART2_BASE)
#define USART3             ((USART_TypeDef *)USART3_BASE)
#define SYSTICK            ((SysTick_TypeDef *)SYSTICK_BASE)

static volatile uint32_t g_ms_ticks;

static int16_t g_target_left_rpm;
static int16_t g_target_right_rpm;
static BldcDualStatus g_bldc_status;
static uint32_t g_rs485_tx_count;
static uint32_t g_rs485_rx_ok_count;
static uint32_t g_rs485_rx_error_count;

static uint8_t g_rs485_rx_packet[BLDC_PACKET_MAX_SIZE];
static uint8_t g_rs485_rx_index;
static uint8_t g_rs485_rx_expected_len;

static char g_pc_line[PC_LINE_MAX_SIZE];
static uint8_t g_pc_line_index;

static void clock_init(void);
static void gpio_init(void);
static void systick_init_1ms(void);
static uint32_t millis(void);
static void usart_init(USART_TypeDef *uart, uint32_t baudrate);
static void uart_write_char(USART_TypeDef *uart, char ch);
static void uart_write(USART_TypeDef *uart, const char *text);
static void uart_write_i32(USART_TypeDef *uart, int32_t value);
static void uart_write_u32(USART_TypeDef *uart, uint32_t value);
static int uart_read_char_nonblocking(USART_TypeDef *uart, char *ch);
static void rs485_dir_tx(void);
static void rs485_dir_rx(void);
static void rs485_write_packet(const uint8_t *data, uint8_t length);
static uint8_t bldc_checksum(const uint8_t *data, uint8_t length);
static int bldc_checksum_is_valid(const uint8_t *data, uint8_t length);
static void bldc_send_pnt_velocity_cmd(int16_t left_rpm, int16_t right_rpm);
static void bldc_poll_rx(void);
static void bldc_rx_reset(void);
static void bldc_rx_process_byte(uint8_t data);
static void bldc_handle_packet(const uint8_t *packet, uint8_t length);
static void pc_poll_rx(void);
static void pc_process_line(const char *line);
static void pc_print_help(void);
static void pc_print_status(void);
static void set_both_target_rpm(int16_t rpm);
static int parse_i16(const char *text, int16_t *value);
static int16_t clamp_i32_to_i16(int32_t value);
static int16_t get_i16_le(const uint8_t *data);
static uint16_t get_u16_le(const uint8_t *data);
static int32_t get_i32_le(const uint8_t *data);
static void put_i16_le(uint8_t *data, int16_t value);
static void tiny_delay(void);

void SysTick_Handler(void)
{
    g_ms_ticks++;
}

int main(void)
{
    uint32_t last_tx_ms;

    clock_init();
    gpio_init();
    usart_init(USART2, PC_BAUDRATE);
    usart_init(USART3, RS485_BAUDRATE);
    systick_init_1ms();
    rs485_dir_rx();

    uart_write(USART2, "\r\nSTM32F103 BLDC RS485 ready\r\n");
    uart_write(USART2, "USART2 PC: 9600 8N1, USART3 RS485: 57600 8N1\r\n");
    pc_print_help();

    last_tx_ms = millis();

    for (;;) {
        bldc_poll_rx();
        pc_poll_rx();

        if ((uint32_t)(millis() - last_tx_ms) >= RS485_PERIOD_MS) {
            last_tx_ms = millis();
            bldc_send_pnt_velocity_cmd(g_target_left_rpm, g_target_right_rpm);
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
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN | RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN;

    /* PA2 USART2_TX for PC monitor: alternate function push-pull, 50 MHz. */
    GPIOA->CRL &= ~(0xFUL << 8);
    GPIOA->CRL |=  (0xBUL << 8);

    /* PA3 USART2_RX for PC monitor: input floating. */
    GPIOA->CRL &= ~(0xFUL << 12);
    GPIOA->CRL |=  (0x4UL << 12);

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

static void usart_init(USART_TypeDef *uart, uint32_t baudrate)
{
    if (uart == USART2) {
        RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
    } else {
        RCC->APB1ENR |= RCC_APB1ENR_USART3EN;
    }

    uart->CR1 = 0U;
    uart->BRR = (SYSCLK_HZ + (baudrate / 2U)) / baudrate;
    uart->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
}

static void uart_write_char(USART_TypeDef *uart, char ch)
{
    while ((uart->SR & USART_SR_TXE) == 0U) {
    }
    uart->DR = (uint32_t)(uint8_t)ch;
}

static void uart_write(USART_TypeDef *uart, const char *text)
{
    while (*text != '\0') {
        uart_write_char(uart, *text++);
    }
}

static void uart_write_i32(USART_TypeDef *uart, int32_t value)
{
    if (value < 0) {
        uart_write_char(uart, '-');
        value = -value;
    }
    uart_write_u32(uart, (uint32_t)value);
}

static void uart_write_u32(USART_TypeDef *uart, uint32_t value)
{
    char buffer[10];
    uint32_t index = 0U;

    if (value == 0U) {
        uart_write_char(uart, '0');
        return;
    }

    while (value > 0U) {
        buffer[index++] = (char)('0' + (value % 10U));
        value /= 10U;
    }

    while (index > 0U) {
        uart_write_char(uart, buffer[--index]);
    }
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

static void bldc_send_pnt_velocity_cmd(int16_t left_rpm, int16_t right_rpm)
{
    uint8_t packet[BLDC_PNT_VEL_PACKET_SIZE];

    packet[0] = MOTOR_CONTROLLER_MACHINE_ID;
    packet[1] = USER_MACHINE_ID;
    packet[2] = MOTOR_ID;
    packet[3] = PID_PNT_VEL_CMD;
    packet[4] = 7U;
    packet[5] = ENABLE;
    put_i16_le(&packet[6], left_rpm);
    packet[8] = ENABLE;
    put_i16_le(&packet[9], right_rpm);
    packet[11] = RETURN_PNT_MAIN_DATA;
    packet[12] = bldc_checksum(packet, 12U);

    rs485_write_packet(packet, BLDC_PNT_VEL_PACKET_SIZE);
    g_rs485_tx_count++;
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
        g_rs485_rx_error_count++;
        return;
    }

    g_rs485_rx_packet[g_rs485_rx_index++] = data;

    if ((g_rs485_rx_index == 2U) &&
        (g_rs485_rx_packet[1] != MOTOR_CONTROLLER_MACHINE_ID)) {
        bldc_rx_reset();
        g_rs485_rx_error_count++;
        return;
    }

    if ((g_rs485_rx_index == 3U) &&
        (g_rs485_rx_packet[2] != MOTOR_ID)) {
        bldc_rx_reset();
        g_rs485_rx_error_count++;
        return;
    }

    if (g_rs485_rx_index == BLDC_PACKET_HEADER_SIZE) {
        data_num = g_rs485_rx_packet[4];
        if (data_num > (BLDC_PACKET_MAX_SIZE -
                        BLDC_PACKET_HEADER_SIZE -
                        BLDC_PACKET_CHECKSUM_SIZE)) {
            bldc_rx_reset();
            g_rs485_rx_error_count++;
            return;
        }

        total_len = (uint8_t)(BLDC_PACKET_HEADER_SIZE +
                              data_num +
                              BLDC_PACKET_CHECKSUM_SIZE);

        if (total_len > BLDC_PACKET_MAX_SIZE) {
            bldc_rx_reset();
            g_rs485_rx_error_count++;
            return;
        }

        g_rs485_rx_expected_len = total_len;
    }

    if ((g_rs485_rx_expected_len != 0U) &&
        (g_rs485_rx_index >= g_rs485_rx_expected_len)) {
        if (bldc_checksum_is_valid(g_rs485_rx_packet, g_rs485_rx_expected_len)) {
            bldc_handle_packet(g_rs485_rx_packet, g_rs485_rx_expected_len);
            g_rs485_rx_ok_count++;
        } else {
            g_rs485_rx_error_count++;
        }

        bldc_rx_reset();
    }
}

static void bldc_handle_packet(const uint8_t *packet, uint8_t length)
{
    (void)length;

    if (packet[4] == BLDC_DUAL_STATUS_DATA_SIZE) {
        g_bldc_status.valid = 1U;
        g_bldc_status.pid = packet[3];
        g_bldc_status.data_num = packet[4];
        g_bldc_status.left_rpm = get_i16_le(&packet[5]);
        g_bldc_status.left_current = get_u16_le(&packet[7]);
        g_bldc_status.left_state = packet[9];
        g_bldc_status.left_position = get_i32_le(&packet[10]);
        g_bldc_status.right_rpm = get_i16_le(&packet[14]);
        g_bldc_status.right_current = get_u16_le(&packet[16]);
        g_bldc_status.right_state = packet[18];
        g_bldc_status.right_position = get_i32_le(&packet[19]);
    }
}

static void pc_poll_rx(void)
{
    char ch;

    while (uart_read_char_nonblocking(USART2, &ch)) {
        if ((ch == '\r') || (ch == '\n')) {
            if (g_pc_line_index > 0U) {
                g_pc_line[g_pc_line_index] = '\0';
                pc_process_line(g_pc_line);
                g_pc_line_index = 0U;
            }
        } else if (g_pc_line_index < (PC_LINE_MAX_SIZE - 1U)) {
            g_pc_line[g_pc_line_index++] = ch;
        } else {
            g_pc_line_index = 0U;
            uart_write(USART2, "ERR line too long\r\n");
        }
    }
}

static void pc_process_line(const char *line)
{
    int16_t value;

    if ((line[0] == '?') && (line[1] == '\0')) {
        pc_print_help();
    } else if ((line[0] == 's') && (line[1] == '\0')) {
        pc_print_status();
    } else if ((line[0] == 'S') && (line[1] == '\0')) {
        pc_print_status();
    } else if ((line[0] == '+') && (line[1] == '\0')) {
        set_both_target_rpm(clamp_i32_to_i16((int32_t)g_target_left_rpm + 100));
        pc_print_status();
    } else if ((line[0] == '-') && (line[1] == '\0')) {
        set_both_target_rpm(clamp_i32_to_i16((int32_t)g_target_left_rpm - 100));
        pc_print_status();
    } else if ((line[0] == '0') && (line[1] == '\0')) {
        set_both_target_rpm(0);
        pc_print_status();
    } else if (((line[0] == 'v') || (line[0] == 'V')) && parse_i16(&line[1], &value)) {
        set_both_target_rpm(value);
        pc_print_status();
    } else if (((line[0] == 'l') || (line[0] == 'L')) && parse_i16(&line[1], &value)) {
        g_target_left_rpm = value;
        pc_print_status();
    } else if (((line[0] == 'r') || (line[0] == 'R')) && parse_i16(&line[1], &value)) {
        g_target_right_rpm = value;
        pc_print_status();
    } else if (parse_i16(line, &value)) {
        set_both_target_rpm(value);
        pc_print_status();
    } else {
        uart_write(USART2, "ERR unknown command\r\n");
        pc_print_help();
    }
}

static void pc_print_help(void)
{
    uart_write(USART2, "Commands: v RPM, l RPM, r RPM, +, -, 0, s, ?\r\n");
}

static void pc_print_status(void)
{
    uart_write(USART2, "TARGET L=");
    uart_write_i32(USART2, g_target_left_rpm);
    uart_write(USART2, " R=");
    uart_write_i32(USART2, g_target_right_rpm);
    uart_write(USART2, " TX=");
    uart_write_u32(USART2, g_rs485_tx_count);
    uart_write(USART2, " RX_OK=");
    uart_write_u32(USART2, g_rs485_rx_ok_count);
    uart_write(USART2, " RX_ERR=");
    uart_write_u32(USART2, g_rs485_rx_error_count);

    if (g_bldc_status.valid != 0U) {
        uart_write(USART2, " | PID=");
        uart_write_u32(USART2, g_bldc_status.pid);
        uart_write(USART2, " LRPM=");
        uart_write_i32(USART2, g_bldc_status.left_rpm);
        uart_write(USART2, " RRPM=");
        uart_write_i32(USART2, g_bldc_status.right_rpm);
        uart_write(USART2, " LCUR=");
        uart_write_u32(USART2, g_bldc_status.left_current);
        uart_write(USART2, " RCUR=");
        uart_write_u32(USART2, g_bldc_status.right_current);
    }

    uart_write(USART2, "\r\n");
}

static void set_both_target_rpm(int16_t rpm)
{
    g_target_left_rpm = rpm;
    g_target_right_rpm = rpm;
}

static int parse_i16(const char *text, int16_t *value)
{
    int sign = 1;
    int32_t result = 0;
    uint8_t has_digit = 0U;

    while ((*text == ' ') || (*text == '\t')) {
        text++;
    }

    if (*text == '-') {
        sign = -1;
        text++;
    } else if (*text == '+') {
        text++;
    }

    while ((*text >= '0') && (*text <= '9')) {
        has_digit = 1U;
        result = (result * 10) + (*text - '0');
        if (result > 32768) {
            return 0;
        }
        text++;
    }

    while ((*text == ' ') || (*text == '\t')) {
        text++;
    }

    if ((*text != '\0') || (has_digit == 0U)) {
        return 0;
    }

    result *= sign;
    if ((result < -32768) || (result > 32767)) {
        return 0;
    }

    *value = (int16_t)result;
    return 1;
}

static int16_t clamp_i32_to_i16(int32_t value)
{
    if (value > 32767) {
        return 32767;
    }

    if (value < -32768) {
        return -32768;
    }

    return (int16_t)value;
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
