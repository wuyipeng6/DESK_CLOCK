#include <stdint.h>
#include <string.h>
#include "stm32f4xx.h"
#include "delay.h"

#define TICKS_PER_MS    (SystemCoreClock / 1000)
#define TICKS_PER_US    (SystemCoreClock / 1000000)

static volatile uint64_t cpu_tick_count;

void cpu_tick_init(void)
{
    SysTick->LOAD = TICKS_PER_MS;
    SysTick->VAL = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
}

uint64_t cpu_now(void)
{
    uint64_t now, last_count;
    do {
        last_count = cpu_tick_count;
        now = cpu_tick_count + SysTick->LOAD - SysTick->VAL;
    } while (last_count != cpu_tick_count);
    return now;
}

uint64_t cpu_get_us(void)
{
    return cpu_now() / TICKS_PER_US;
}

uint64_t cpu_get_ms(void)
{
    return cpu_now() / TICKS_PER_MS;
}

void delay_us(uint32_t us)
{
    uint64_t now = cpu_now();
    while (cpu_now() - now < (uint64_t)us * TICKS_PER_US);
}

void delay_ms(uint32_t ms)
{
    uint64_t now = cpu_now();
    while (cpu_now() - now < (uint64_t)ms * TICKS_PER_MS);
}

void SysTick_Handler(void)
{
    cpu_tick_count += TICKS_PER_MS;
}
