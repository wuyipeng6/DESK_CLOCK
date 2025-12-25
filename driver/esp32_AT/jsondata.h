#ifndef JSONDATA_H
#define JSONDATA_H
#include "stm32f4xx.h"

// 定义一个结构体来存储IP地理位置信息(utf-8编码)
//如果用串口输出中文，那么串口助手需要设置为utf-8编码格式
//如果要用GBK编码格式输出中文，需要在拷贝字符串后进行编码转换，比较麻烦，这里就不做了。
//如果需要在OLED等显示屏上显示中文，需要对字库进行索引，那么字库的编码格式也需要是utf-8格式。所以最好文件都用utf-8编码格式。
typedef struct {
    char country[32];    // 国家
    char regionName[32]; // 省份
    char city[32];       // 城市
    char ip[32];         // 公网IP
    int  is_valid;       // 数据是否有效 (1有效, 0无效)
} IP_Location_t;

extern IP_Location_t g_ip_info; 

// 定义一个结构体来存储单日天气信息(utf-8编码)
typedef struct {
    char date[16];        // 日期，如 "2025-12-29"
    char temperature[16]; // 温度范围，如 "1/14℃"
    char weather[32];     // 天气状况，如 "多云"
} Daily_Weather_t;

// 定义一个结构体来存储五天天气预报信息
typedef struct {
    Daily_Weather_t days[5]; // 5天的天气数据
    int is_valid;            // 数据是否有效 (1有效, 0无效)
} Weather_Forecast_t;

extern Weather_Forecast_t g_weather_forecast;

// 时间数据结构与函数
typedef struct {
    uint8_t hour;   // 时
    uint8_t minute; // 分
    uint8_t second; // 秒
    int     is_valid; // 数据是否有效 (1有效, 0无效)
} System_Time_t;

extern System_Time_t g_sys_time;

//json数据处理相关函数
int get_json_value(const char *json, const char *key, char *result, int max_len);//从 JSON 字符串中提取指定键的字符串值

//IP地理位置请求相关函数
int Parse_IP_JSON_To_Global(const char *json);//解析IP的JSON数据并存入结构体
void Print_IP_Info(void);//打印全局变量中存储的 IP 地理位置信息

//天气请求相关函数
void str_to_urlencode(char* dest, const char* src);//URL编码函数
int Parse_Weather_JSON_To_Global(const char *json);//解析天气的JSON数据并存入结构体
void Print_Weather_Forecast(void);//打印全局变量中存储的天气预报信息

//SNTP时间请求相关函数
int Parse_SNTP_TIME_To_Global(const char *raw_buffer);//解析+CIPSNTPTIME返回的数据并存入结构体
void Print_SNTP_Time(void);//打印全局变量中存储的时分秒


#endif /* JSONDATA_H */
