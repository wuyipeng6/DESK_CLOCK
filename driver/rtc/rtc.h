#ifndef __RTC_H
#define __RTC_H

#include "stm32f4xx.h"

void RTC_Config(void);
void RTC_SetTimeOnce(uint8_t hh, uint8_t mm, uint8_t ss);
void RTC_GetHMS(uint8_t *hh, uint8_t *mm, uint8_t *ss);

#endif
