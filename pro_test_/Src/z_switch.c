#include <stdint.h>

#define RCC_APB2ENR (*(volatile uint32_t *)0x40021018u)

#define GPIOA_CRL (*(volatile uint32_t *)0x40010800u)
#define GPIOA_BSRR (*(volatile uint32_t *)0x40010810u)
#define GPIOA_BRR  (*(volatile uint32_t *)0x40010814u)

#define GPIOC_CRH (*(volatile uint32_t *)0x40011004u)
#define GPIOC_IDR (*(volatile uint32_t *)0x40011008u)

#define RCC_APB2ENR_IOPAEN (1u << 2)
#define RCC_APB2ENR_IOPCEN (1u << 4)

#define LED_PIN       5u
#define SWITCH_PIN    13u
#define SWITCH_ACTIVE_LOW 1u

void SystemInit(void)
{
}

static void gpio_init(void)
{
    RCC_APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPCEN;

    GPIOA_CRL &= ~(0xFu << (LED_PIN * 4u));
    GPIOA_CRL |=  (0x2u << (LED_PIN * 4u));

    GPIOC_CRH &= ~(0xFu << ((SWITCH_PIN - 8u) * 4u));
    GPIOC_CRH |=  (0x4u << ((SWITCH_PIN - 8u) * 4u));
}

static uint8_t switch_pressed(void)
{
    uint8_t high = (GPIOC_IDR & (1u << SWITCH_PIN)) != 0u;

#if SWITCH_ACTIVE_LOW
    return !high;
#else
    return high;
#endif
}

static void led_write(uint8_t on)
{
    if (on) {
        GPIOA_BSRR = 1u << LED_PIN;
    } else {
        GPIOA_BRR = 1u << LED_PIN;
    }
}

int main(void)
{
    gpio_init();

    while (1) {
        led_write(switch_pressed());
    }
}
