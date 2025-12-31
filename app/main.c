#include "stm32f4xx.h"
#include "delay.h"
#include "usart.h"
#include "lcd/lcd.h"
#include "lcd/lcd_init.h"
#include "IIC.h"
#include "sht20/sht20.h"
#include "oled/oled.h"
#include "esp32_AT/esp_at.h"
#include "rtc/rtc.h"
#include "lcd/clock_UI.h"

int main(void)
{
        // 1. 硬件基础初始化
        NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); // 设置中断优先级分组
        cpu_tick_init();                                // 初始化系统滴答定时器
        uart_init(115200);                              // 串口初始化
        RTC_Config();                                   // RTC初始化
        LCD_Init();                                     // LCD初始

        IIC_Init();      //IIC外设初始化
        SHT20_SoftReset();// SHT20传感器复位
        LCD_Fill(0, 0, LCD_W, LCD_H, WHITE);
        LCD_ShowPicture(45,28,150,164,Power_On);

        esp32_at_init(); //初始化esp32模块，初始化时间。相当于开机时间

        LCD_Fill(0, 0, LCD_W, LCD_H, WHITE);
        ClockUI_ShowDate(25, 88);
        ClockUI_ShowWeek(158, 88);

        // // 显示数码管数字示例
        LCD_ShowPicture(8,127,100,106,gImage_inner);

        LCD_ShowPicture(130, 127, 100, 106, UI_hefei);

        while (1)
        {
                ClockUI_TimeRefresh(35, 40, 25, 88, 158, 88);
                ClockUI_ShowTempHumi(33, 156, 33, 185);
        
                delay_ms(200);
        }
}

