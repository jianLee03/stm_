/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : STM32F103 Nucleo dual UART RS485 transmitter
 ******************************************************************************
 */

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

#define GPIOA              ((GPIO_TypeDef *)GPIOA_BASE)
#define GPIOB              ((GPIO_TypeDef *)GPIOB_BASE)
#define RCC                ((RCC_TypeDef *)RCC_BASE)
#define USART2             ((USART_TypeDef *)USART2_BASE)
#define USART3             ((USART_TypeDef *)USART3_BASE)

#define RS485_DIR_PIN      6U      /* D10 on Nucleo-F103RB: PB6 */
#define RS485_DIR_HIGH     (1UL << RS485_DIR_PIN)

static void clock_init(void);
static void gpio_init(void);
static void usart_init_9600(USART_TypeDef *uart);
static void delay_ms(uint32_t ms);
static void uart_write_char(USART_TypeDef *uart, char ch);
static void uart_write(USART_TypeDef *uart, const char *text);
static int uart_read_char_nonblocking(USART_TypeDef *uart, char *ch);
static void rs485_dir_high(void);
static void comm_send_line(const char *text);
static void uart_print_u32(USART_TypeDef *uart, uint32_t value);

int main(void)
{
    uint32_t ms = 0;
    uint32_t seq = 0;
    char ch;

    clock_init();
    gpio_init();
    usart_init_9600(USART2);
    usart_init_9600(USART3);
    rs485_dir_high();

    uart_write(USART2, "\r\nSTM32F103 dual UART RS485 TX ready\r\n");
    uart_write(USART2, "PC UART : USART2 PA2(TX), PA3(RX), 9600 8N1\r\n");
    uart_write(USART2, "COMM UART: USART3 PB10(TX), PB11(RX), 9600 8N1\r\n");
    uart_write(USART2, "RS485 DIR: D10/PB6 fixed HIGH\r\n");

    for (;;) {
        if (ms == 0U) {
            uart_write(USART2, "COMM TX seq=");
            uart_print_u32(USART2, seq);
            uart_write(USART2, " payload=PING\r\n");

            uart_write(USART3, "SEQ=");
            uart_print_u32(USART3, seq);
            uart_write(USART3, ";");
            comm_send_line("PING");

            seq++;
        }

        while (uart_read_char_nonblocking(USART3, &ch)) {
            uart_write(USART2, "COMM RX:");
            uart_write_char(USART2, ch);
            uart_write(USART2, "\r\n");
        }

        while (uart_read_char_nonblocking(USART2, &ch)) {
            uart_write(USART3, "PC:");
            uart_write_char(USART3, ch);
            uart_write(USART3, "\r\n");
            uart_write(USART2, "PC CMD forwarded to COMM\r\n");
        }

        delay_ms(1U);
        ms++;
        if (ms >= 1000U) {
            ms = 0U;
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

    /* PB6/D10 RS485 RE+DE direction: push-pull output, fixed HIGH. */
    GPIOB->CRL &= ~(0xFUL << 24);
    GPIOB->CRL |=  (0x3UL << 24);

    /* PB10 USART3_TX for RS485 DI: alternate function push-pull, 50 MHz. */
    GPIOB->CRH &= ~(0xFUL << 8);
    GPIOB->CRH |=  (0xBUL << 8);

    /* PB11 USART3_RX for RS485 RO / confirmation reply: input floating. */
    GPIOB->CRH &= ~(0xFUL << 12);
    GPIOB->CRH |=  (0x4UL << 12);
}

static void usart_init_9600(USART_TypeDef *uart)
{
    if (uart == USART2) {
        RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
    } else {
        RCC->APB1ENR |= RCC_APB1ENR_USART3EN;
    }

    uart->CR1 = 0U;
    uart->BRR = 0x0341U;                 /* 8 MHz PCLK1 / 9600 baud */
    uart->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
}

static void delay_ms(uint32_t ms)
{
    while (ms-- > 0U) {
        for (volatile uint32_t i = 0; i < 8000U; i++) {
        }
    }
}

static void uart_write_char(USART_TypeDef *uart, char ch)
{
    while ((uart->SR & USART_SR_TXE) == 0U) {
    }
    uart->DR = (uint32_t)ch;
}

static void uart_write(USART_TypeDef *uart, const char *text)
{
    while (*text != '\0') {
        uart_write_char(uart, *text++);
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

static void rs485_dir_high(void)
{
    GPIOB->BSRR = RS485_DIR_HIGH;
}

static void comm_send_line(const char *text)
{
    rs485_dir_high();
    uart_write(USART3, text);
    uart_write(USART3, "\r\n");
    while ((USART3->SR & USART_SR_TC) == 0U) {
    }
    rs485_dir_high();
}

static void uart_print_u32(USART_TypeDef *uart, uint32_t value)
{
    char buffer[10];
    uint32_t index = 0;

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
