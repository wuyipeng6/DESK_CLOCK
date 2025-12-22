#ifndef __USART2_H
#define __USART2_H
#include "usart.h"

// USART2 RX buffer/state (size from usart.h)
extern u8 USART2_RX_BUF[];
extern u16 USART2_RX_STA;

// Initialize USART2 (commonly on PA2=TX, PA3=RX)
void uart2_init(u32 bound);

#endif
