#ifndef __CLOCK_UI_H__
#define __CLOCK_UI_H__
#pragma once

#include "lcd.h"
#include "pic.h"

void ClockUI_ShowDigital2(u16 x, u16 y, u8 num);          // 显示2位数码管数字(0-99)
void ClockUI_ShowTemp2(u16 x, u16 y, s8 temp, u8 digits); // 显示温度数字,digits为显示位数(1或2)
void ClockUI_ShowTime(u16 x, u16 y);                      // 显示时分秒(自动从RTC获取)
void ClockUI_ShowDate(u16 x, u16 y);                      // 显示年月日(自动从天气数据获取)
void ClockUI_ShowWeek(u16 x, u16 y);                      // 显示星期(自动计算)

// 时间刷新函数(自动在 23:59:59 联网更新数据)
void ClockUI_TimeRefresh(u16 x_time, u16 y_time, u16 x_date, u16 y_date, u16 x_week, u16 y_week);

// 显示温度和湿度
void ClockUI_ShowTempHumi(u16 x_temp, u16 y_temp, u16 x_humi, u16 y_humi);

#endif // __CLOCK_UI_H__