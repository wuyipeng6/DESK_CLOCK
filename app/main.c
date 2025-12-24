#include "stm32f4xx.h"
#include "delay.h"
#include "usart.h"
#include "lcd.h"
#include "lcd_init.h"
#include "IIC.h"
#include "pic.h"
#include "sht20.h"
#include "OLED.h"
#include "esp32_at.h"



int main(void) {
    // 1. 硬件基础初始化
    uart_init(115200);    // 串口初始化
    ESP_Init(115200);     // ESP32(AT) 驱动初始化（封装了串口依赖）
    // 简单AT自检
        if (ESP_SendCmd("AT+RESTORE", "OK", 1000) == 0) {
            printf("ESP32 AT RESTORE OK\r\n");
        } else {
            printf("ESP32 AT no response\r\n");
        }
    Delay_ms(2000);//RESTORE后需要等待一段时间让模块重启
    // 尝试连接 WiFi（填写你自己的 SSID/PASSWORD）
    const char *ssid = "whatcanisay";
    const char *pwd  = "123456qq";
        printf("Connecting to SSID: %s\r\n", ssid);
    if (ESP_ConnectAP(ssid, pwd, 20000) == 0) {
        // 获取本机 IP 作为检查
        printf("ESP connected to AP\r\n");
        ESP_SendCmd("AT+CIFSR", NULL, 10000);//这里总是超时，等待排查。
    } else {
            printf("ESP connect AP failed\r\n");
    }
    // IIC_Init();      //IIC外设初始化
    // OLED_Init ();
    // SHT20_SoftReset();


    while (1) {
        // float temp = 0, humi = 0;
        
        // // --- 温度测量 ---
        // SHT20_Trigger_measurement(CMD_T_MEASURE_NO_HOLD);
        // OLED_ShowString (0,0,"TEMP:",OLED_8X16);
        
        // //Delay_ms(100); // 关键：温度测量最长需要 85ms，这里给 100ms
        // SHT20_GetResult(CMD_T_MEASURE_NO_HOLD, &temp);
        // OLED_ShowFloatNum (40,0,temp,2,2,OLED_8X16);


        // // --- 湿度测量 ---
        // SHT20_Trigger_measurement(CMD_RH_MEASURE_NO_HOLD);
        // OLED_ShowString (0,16,"HUMI:",OLED_8X16);
        // //Delay_ms(40);  // 湿度测量最长需要 29ms，这里给 40ms
        // SHT20_GetResult(CMD_RH_MEASURE_NO_HOLD, &humi);
        // OLED_ShowFloatNum (40,16,humi,2,2,OLED_8X16);
        // OLED_Update ();



        Delay_ms(100); 
    }
}





