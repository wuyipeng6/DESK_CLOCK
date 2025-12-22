#ifndef __ESP32_AT_H
#define __ESP32_AT_H

#include "stm32f4xx.h"
#include "usart.h"
#include "delay.h"
#include <stdint.h>
#include <stddef.h>

// Optional: define reset pin in project if available
// #define ESP_RST_GPIO_PORT GPIOB
// #define ESP_RST_PIN GPIO_Pin_0

void ESP_Init(uint32_t baud);
int ESP_Reset(void); // returns 0 on success, -1 on unsupported
int ESP_SendCmd(const char *cmd, const char *expect, uint32_t timeout_ms);
int ESP_WaitResponse(const char *expect, char *outbuf, size_t outlen, uint32_t timeout_ms);
int ESP_SendRaw(const uint8_t *data, size_t len);

#endif
