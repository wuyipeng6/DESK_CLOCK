#ifndef USART2_DMA_AT_H
#define USART2_DMA_AT_H
#include "stm32f4xx.h"

#define UART_RX_BUF_SIZE 2048

extern uint8_t UART_RX_BUF[UART_RX_BUF_SIZE]; // DMA 接收原始缓冲区
extern uint8_t MAIN_RX_BUF[UART_RX_BUF_SIZE]; // 解析用的转存缓冲区
extern volatile uint16_t rx_len;          // 接收到的数据长度
extern volatile uint8_t rx_flag;         // 接收完成标志位

void USART2_DMA_Config(void);  // USART2 + DMA 初始化函数，开启了空闲中断和DMA传输完成中断，DMA负责数据接收



#endif /* USART2_DMA_AT_H */