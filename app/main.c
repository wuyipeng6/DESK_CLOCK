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
    // 1. 硬件基础初始??
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); // 设置中断优先级分??
    cpu_tick_init();  // 初始化系统滴答定时器
    uart_init(115200);    // 串口初始??

    // IIC_Init();      //IIC外设初始??
    // OLED_Init ();
    // SHT20_SoftReset();
    // esp32_at_demo(); //初始化esp32模块，连接wifi


	u8 i,j;
	float t=0;
	LCD_Init();//LCD初始??
	LCD_Fill(0,0,LCD_W,LCD_H,WHITE);

	while(1) 
	{
        //LCD_ShowPicture(0,0,240,240,gImage_1);
		LCD_ShowChinese(0,0,"中景园电子",RED,WHITE,32,0);
		LCD_ShowString(0,40,"LCD_W:",RED,WHITE,16,0);
		LCD_ShowIntNum(48,40,LCD_W,3,RED,WHITE,16);
		LCD_ShowString(80,40,"LCD_H:",RED,WHITE,16,0);
		LCD_ShowIntNum(128,40,LCD_H,3,RED,WHITE,16);
		LCD_ShowString(80,40,"LCD_H:",RED,WHITE,16,0);
		LCD_ShowString(0,70,"Increaseing Nun:",RED,WHITE,16,0);
		LCD_ShowFloatNum1(128,70,t,4,RED,WHITE,16);
		t+=0.11f;
		for(j=0;j<3;j++)
		{
			for(i=0;i<6;i++)
			{
				LCD_ShowPicture(40*i,120+j*40,40,40,gImage_1);
			}
		}
	}
}





        //float temp = 0, humi = 0;
        
        // // --- 温度测量 ---
        // SHT20_Trigger_measurement(CMD_T_MEASURE_NO_HOLD);
        // OLED_ShowString (0,0,"TEMP:",OLED_8X16);
        
        // //Delay_ms(100); // 关键：温度测量最长需?? 85ms，这里给 100ms
        // SHT20_GetResult(CMD_T_MEASURE_NO_HOLD, &temp);
        // OLED_ShowFloatNum (40,0,temp,2,2,OLED_8X16);


        // // --- 湿度测量 ---
        // SHT20_Trigger_measurement(CMD_RH_MEASURE_NO_HOLD);
        // OLED_ShowString (0,16,"HUMI:",OLED_8X16);
        // //Delay_ms(40);  // 湿度测量最长需?? 29ms，这里给 40ms
        // SHT20_GetResult(CMD_RH_MEASURE_NO_HOLD, &humi);
        // OLED_ShowFloatNum (40,16,humi,2,2,OLED_8X16);
        // OLED_Update ();