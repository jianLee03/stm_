#include <stdint.h>

#define RCC_APB2ENR (*(volatile uint32_t *)0x40021018UL)
#define GPIOA_CRL   (*(volatile uint32_t *)0x40010800UL)
#define GPIOA_ODR   (*(volatile uint32_t *)0x4001080CUL)

#define SYST_CSR    (*(volatile uint32_t *)0xE000E010UL)
#define SYST_RVR    (*(volatile uint32_t *)0xE000E014UL)
#define SYST_CVR    (*(volatile uint32_t *)0xE000E018UL)

#define LED_PIN_MASK       (1UL << 5)
#define SYSTICK_1S_RELOAD  (8000000UL - 1UL)

void SystemInit(void) {}

static void Delay_Seconds(uint32_t seconds)
{
    while (seconds-- > 0UL) {
        SYST_CVR = 0UL;
        while ((SYST_CSR & (1UL << 16)) == 0UL) {}
    }
}

int main(void)
{
    RCC_APB2ENR |= (1UL << 2);

    GPIOA_CRL &= ~(0xFUL << 20);
    GPIOA_CRL |=  (0x2UL << 20);
    GPIOA_ODR &= ~LED_PIN_MASK;

    SYST_RVR = SYSTICK_1S_RELOAD;
    SYST_CVR = 0UL;
    SYST_CSR = (1UL << 2) | (1UL << 0);

    while (1) {
        GPIOA_ODR |= LED_PIN_MASK;
        Delay_Seconds(3UL);

        GPIOA_ODR &= ~LED_PIN_MASK;
        Delay_Seconds(1UL);
    }
}
