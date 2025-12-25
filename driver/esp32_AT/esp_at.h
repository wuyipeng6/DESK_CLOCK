#ifndef _ESP_AT_H_
#define _ESP_AT_H_

#pragma diag_suppress 870//不要再报中文的警告了。
#include "stm32f4xx.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>



#define JUHE_WEATHER_KEY "2c6a34d24eaba45ce8724150a070c175"//聚合数据的天气API密钥

void esp_at_clear_buffer(void);
void esp_at_usart_init(void);
bool send_at_command(char* cmd, char* expected_res, uint32_t timeout);
bool esp_at_wait_response(const char* expected,uint16_t timeout);

/* WiFi 和网络相关函数 */
bool esp_at_check_module(void);
bool esp_at_set_mode(uint8_t mode);
bool esp_at_connect_wifi(const char* ssid, const char* password);
bool esp_at_get_local_ip(char* ip_str, uint16_t len);
bool esp_at_disconnect_wifi(void);

/* HTTP 请求相关函数 */
bool esp_at_http_get_IP(void);
bool esp_at_http_get_weather(void);
bool esp_at_get_sntp_time(void);

/* 辅助函数 */
void esp_at_print_response(void);

/* 上电获取信息函数 */
void power_on_step1_location_get(void); //上电获取连wifi，获取IP及地理位置，初始化结构体IP_Location_t
void power_on_step2_weather_get(void); //获取天气信息，初始化结构体Weather_Forecast_t，
//因为需要用到IP_Location_t结构体中的城市信息，所以需要放在第二步执行。


//模块功能演示
void esp32_at_demo(void);



#endif /* _ESP_AT_H_ */