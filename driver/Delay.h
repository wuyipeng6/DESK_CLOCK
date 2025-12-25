#ifndef __DELAY_H__
#define __DELAY_H__

#include <stdint.h>

void cpu_tick_init(void);
uint64_t cpu_now(void);
uint64_t cpu_get_us(void);
uint64_t cpu_get_ms(void);
void delay_us(uint32_t us);
void delay_ms(uint32_t ms);

#endif /* __DELAY_H__ */
