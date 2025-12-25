#ifndef __USART_H
#define __USART_H
#include "stdio.h"	
#include "stm32f4xx_conf.h"
#include "stm32f4xx.h"
	
#define USART_REC_LEN  			200  	// 串口接收缓冲区最大长度(字节数)
#define EN_USART1_RX 			1		// 使能(1)/禁止(0) 串口1接收功能
	  	
extern u8  USART_RX_BUF[USART_REC_LEN]; // 串口接收缓冲区，最多USART_REC_LEN个字节
extern u16 USART_RX_STA;         		// 串口接收状态标志
											// bit15:   接收完成标志
											// bit14:   接收到0x0d标志
											// bit13~0: 接收到的有效字节数

/**
 * @brief  初始化USART1
 * @param  bound: 波特率
 * @retval None
 */
void uart_init(u32 bound);

#endif


