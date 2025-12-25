#include "stm32f4xx.h"
#include "delay.h"
#include "usart.h"
#include "lcd/lcd.h"
#include "lcd/lcd_init.h"
#include "IIC.h"
#include "lcd/pic.h"
#include "sht20/sht20.h"
#include "oled/oled.h"
#include "esp32_AT/esp_at.h"



int main(void) {
    // 1. 硬件基础初始化
    cpu_tick_init();  // 初始化系统滴答定时器
    uart_init(115200);    // 串口初始化

    IIC_Init();      //IIC外设初始化
    OLED_Init ();
    SHT20_SoftReset();

    esp32_at_demo(); //初始化esp32模块，连接wifi


    while (1) {
        float temp = 0, humi = 0;
        
        // --- 温度测量 ---
        SHT20_Trigger_measurement(CMD_T_MEASURE_NO_HOLD);
        OLED_ShowString (0,0,"TEMP:",OLED_8X16);
        
        //Delay_ms(100); // 关键：温度测量最长需要 85ms，这里给 100ms
        SHT20_GetResult(CMD_T_MEASURE_NO_HOLD, &temp);
        OLED_ShowFloatNum (40,0,temp,2,2,OLED_8X16);


        // --- 湿度测量 ---
        SHT20_Trigger_measurement(CMD_RH_MEASURE_NO_HOLD);
        OLED_ShowString (0,16,"HUMI:",OLED_8X16);
        //Delay_ms(40);  // 湿度测量最长需要 29ms，这里给 40ms
        SHT20_GetResult(CMD_RH_MEASURE_NO_HOLD, &humi);
        OLED_ShowFloatNum (40,16,humi,2,2,OLED_8X16);
        OLED_Update ();



        delay_ms(100); 
    }
}





