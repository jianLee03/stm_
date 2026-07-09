#include <stdint.h>

#define RCC_APB2ENR (*(volatile uint32_t *)0x40021018u)
#define RCC_APB1ENR (*(volatile uint32_t *)0x4002101Cu)

#define GPIOA_CRL (*(volatile uint32_t *)0x40010800u)

#define USART2_SR  (*(volatile uint32_t *)0x40004400u)
#define USART2_DR  (*(volatile uint32_t *)0x40004404u)
#define USART2_BRR (*(volatile uint32_t *)0x40004408u)
#define USART2_CR1 (*(volatile uint32_t *)0x4000440Cu)

#define TIM3_CR1   (*(volatile uint32_t *)0x40000400u)
#define TIM3_EGR   (*(volatile uint32_t *)0x40000414u)
#define TIM3_CCMR1 (*(volatile uint32_t *)0x40000418u)
#define TIM3_CCER  (*(volatile uint32_t *)0x40000420u)
#define TIM3_PSC   (*(volatile uint32_t *)0x40000428u)
#define TIM3_ARR   (*(volatile uint32_t *)0x4000042Cu)
#define TIM3_CCR2  (*(volatile uint32_t *)0x40000438u)

#define TIM2_CR1   (*(volatile uint32_t *)0x40000000u)
#define TIM2_DIER  (*(volatile uint32_t *)0x4000000Cu)
#define TIM2_SR    (*(volatile uint32_t *)0x40000010u)
#define TIM2_EGR   (*(volatile uint32_t *)0x40000014u)
#define TIM2_CNT   (*(volatile uint32_t *)0x40000024u)
#define TIM2_PSC   (*(volatile uint32_t *)0x40000028u)
#define TIM2_ARR   (*(volatile uint32_t *)0x4000002Cu)

#define NVIC_ISER0 (*(volatile uint32_t *)0xE000E100u)

#define RCC_APB2ENR_AFIOEN  (1u << 0)
#define RCC_APB2ENR_IOPAEN  (1u << 2)
#define RCC_APB1ENR_USART2EN (1u << 17)
#define RCC_APB1ENR_TIM2EN  (1u << 0)
#define RCC_APB1ENR_TIM3EN  (1u << 1)

#define BUZZER_PIN          7u
#define TIMER_TICK_HZ       1000000u
#define NOTE_ON_MS          160u
#define NOTE_GAP_MS         40u
#define EMERGENCY_PRINT_MS  200u
#define TIM2_IRQn           28u

static const uint8_t melody[] = {5u, 7u, 5u, 7u};
static const uint16_t level_frequency_hz[11] = {
    0u, 262u, 294u, 330u, 349u, 392u, 440u, 494u, 523u, 587u, 659u
};

static volatile uint32_t note_remaining_ms = NOTE_ON_MS;
static volatile uint32_t melody_index = 0u;
static volatile uint32_t note_is_on = 1u;
static volatile uint32_t emergency_print_remaining_ms = EMERGENCY_PRINT_MS;
static volatile uint32_t emergency_print_pending = 1u;

void SystemInit(void)
{
}

static void uart2_init(void)
{
    RCC_APB2ENR |= RCC_APB2ENR_AFIOEN | RCC_APB2ENR_IOPAEN;
    RCC_APB1ENR |= RCC_APB1ENR_USART2EN;

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

static void buzzer_init(void)
{
    RCC_APB2ENR |= RCC_APB2ENR_AFIOEN | RCC_APB2ENR_IOPAEN;
    RCC_APB1ENR |= RCC_APB1ENR_TIM3EN;

    GPIOA_CRL &= ~(0xFu << (BUZZER_PIN * 4u));
    GPIOA_CRL |=  (0xAu << (BUZZER_PIN * 4u));

    TIM3_PSC = 7u;
    TIM3_CCMR1 &= ~(0xFFu << 8);
    TIM3_CCMR1 |= (6u << 12) | (1u << 11);
    TIM3_CCER |= (1u << 4);
    TIM3_CR1 |= (1u << 7);
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
    TIM3_CCER |= (1u << 4);
    TIM3_CR1 |= (1u << 0);
}

static void buzzer_set_level(uint32_t level)
{
    if (level > 10u) {
        level = 10u;
    }

    buzzer_set_frequency(level_frequency_hz[level]);
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

void TIM2_IRQHandler(void)
{
    if ((TIM2_SR & 1u) == 0u) {
        return;
    }

    TIM2_SR &= ~1u;

    if (note_remaining_ms > 0u) {
        note_remaining_ms--;
    }

    if (note_remaining_ms != 0u) {
        goto update_serial_timer;
    }

    if (note_is_on) {
        buzzer_set_frequency(0u);
        note_is_on = 0u;
        note_remaining_ms = NOTE_GAP_MS;
    } else {
        melody_index++;
        if (melody_index >= sizeof(melody)) {
            melody_index = 0u;
        }

        buzzer_set_level(melody[melody_index]);
        note_is_on = 1u;
        note_remaining_ms = NOTE_ON_MS;
    }

update_serial_timer:
    if (emergency_print_remaining_ms > 0u) {
        emergency_print_remaining_ms--;
    }

    if (emergency_print_remaining_ms == 0u) {
        emergency_print_remaining_ms = EMERGENCY_PRINT_MS;
        emergency_print_pending = 1u;
    }
}

int main(void)
{
    uart2_init();
    buzzer_init();
    buzzer_set_level(melody[0]);
    timer2_1ms_init();

    while (1) {
        if (emergency_print_pending != 0u) {
            emergency_print_pending = 0u;
            uart2_puts("EMERGENCY!!!\r\n");
        }
    }
}
