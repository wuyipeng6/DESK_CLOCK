#include "jsondata.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>


// 声明全局变量
IP_Location_t g_ip_info = {0};
Weather_Forecast_t g_weather_forecast = {0};
System_Time_t g_sys_time = {0};



/**
 * @brief 从 JSON 字符串中提取指定键的字符串值
 * @param json 原始 JSON 字符串
 * @param key  要查找的键名 (例如 "city")
 * @param result 存放结果的缓冲区
 * @param max_len 缓冲区最大长度
 * @return int 成功返回 0，失败返回 -1
 */
int get_json_value(const char *json, const char *key, char *result, int max_len) {
    char key_pattern[32];
    // 构造查找模式，例如: "city":"
    snprintf(key_pattern, sizeof(key_pattern), "\"%s\":\"", key);
    
    char *p_start = strstr(json, key_pattern);
    if (p_start == NULL) {
        return -1; // 没找到键名
    }
    
    // 指针移动到 " 之后（即值的起始位置）
    p_start += strlen(key_pattern);
    
    // 找到下一个双引号（即值的结束位置）
    char *p_end = strchr(p_start, '\"');
    if (p_end == NULL) {
        return -1; // 格式错误
    }
    
    // 计算值的长度
    int value_len = p_end - p_start;
    
    // 防止缓冲区溢出
    if (value_len >= max_len) {
        value_len = max_len - 1;
    }
    
    // 拷贝结果并添加结束符
    memcpy(result, p_start, value_len);
    result[value_len] = '\0';
    
    return 0;
}


/**
 * @brief 解析IP的原始数据并更新全局变量
 * @param raw_buffer 串口收到的原始字符串 (如 +HTTPCLIENT:...)
 * @return int 解析成功返回 1，失败返回 0
 */
int Parse_IP_JSON_To_Global(const char *raw_buffer) {
    // 1. 数据预处理：定位到 JSON 的起始括号 '{'
    const char *json_body = strchr(raw_buffer, '{');
    if (json_body == NULL) {
        g_ip_info.is_valid = 0;
        return 0; // 没有找到 JSON 数据
    }

    // 2. 清空全局变量
    memset(&g_ip_info, 0, sizeof(IP_Location_t));

    // 3. 依次提取各个字段
    // 注意：我们将 JSON 中的 "query" 提取并存入结构体的 "ip" 字段中
    int res = 0;
    res += get_json_value(json_body, "country",    g_ip_info.country,    32);
    res += get_json_value(json_body, "regionName", g_ip_info.regionName, 32);
    res += get_json_value(json_body, "city",       g_ip_info.city,       32);
    res += get_json_value(json_body, "query",      g_ip_info.ip,         32);

    // 4. 判断提取是否成功
    // 如果返回值为 0，说明至少成功提取了部分关键信息
    if (res <= 0 && strlen(g_ip_info.ip) > 0) {
        g_ip_info.is_valid = 1;
    } else {
        g_ip_info.is_valid = 0;
    }
    return g_ip_info.is_valid;
}


/**
 * @brief 打印全局变量中存储的 IP 地理位置信息
 */
void Print_IP_Info(void) {
    printf("\r\n========== 当前地理位置信息 ==========\n");
    
    if (g_ip_info.is_valid) {
        printf("公网 IP:   %s\n", g_ip_info.ip);
        printf("国家:      %s\n", g_ip_info.country);
        printf("省份:      %s\n", g_ip_info.regionName);
        printf("城市:      %s\n", g_ip_info.city);
    } else {
        printf("地理位置信息无效或未获取到数据\n"); 
    }
    printf("======================================\n\r\n");
}

/**
 * @brief 解析天气JSON数据并更新全局变量
 * @param raw_buffer 串口收到的原始字符串 (如 +HTTPCLIENT:...)
 * @return int 解析成功返回 1，失败返回 0
 */
int Parse_Weather_JSON_To_Global(const char *raw_buffer) {
    // 1. 数据预处理：定位到 JSON 的起始括号 '{'
    const char *json_body = strchr(raw_buffer, '{');
    if (json_body == NULL) {
        g_weather_forecast.is_valid = 0;
        return 0; // 没有找到 JSON 数据
    }

    // 2. 清空全局变量
    memset(&g_weather_forecast, 0, sizeof(Weather_Forecast_t));

    // 3. 查找 "future" 数组的起始位置
    const char *future_start = strstr(json_body, "\"future\":[");
    if (future_start == NULL) {
        g_weather_forecast.is_valid = 0;
        return 0;
    }
    future_start += 10; // 跳过 "future":[

    // 4. 逐个提取5天的天气数据
    const char *pos = future_start;
    int day_count = 0;
    
    while (day_count < 5 && pos != NULL) {
        // 查找当前天气对象的起始 '{'
        pos = strchr(pos, '{');
        if (pos == NULL) break;
        
        // 提取 date
        if (get_json_value(pos, "date", g_weather_forecast.days[day_count].date, 16) != 0) {
            break;
        }
        
        // 提取 temperature
        if (get_json_value(pos, "temperature", g_weather_forecast.days[day_count].temperature, 16) != 0) {
            break;
        }
        
        // 提取 weather
        if (get_json_value(pos, "weather", g_weather_forecast.days[day_count].weather, 32) != 0) {
            break;
        }
        
        day_count++;
        
        // 移动到下一个对象
        pos = strchr(pos + 1, '}');
        if (pos == NULL) break;
        pos++; // 跳过当前的 '}'
    }

    // 5. 判断提取是否成功
    if (day_count >= 5) {
        g_weather_forecast.is_valid = 1;
    } else {
        g_weather_forecast.is_valid = 0;
    }
    
    return g_weather_forecast.is_valid;
}


/**
 * @brief 打印全局变量中存储的天气预报信息
 */
void Print_Weather_Forecast(void) {
    printf("\r\n========== 五天天气预报 ==========\n");
    
    if (g_weather_forecast.is_valid) {
        for (int i = 0; i < 5; i++) {
            printf("日期: %s  温度: %s  天气: %s\n",
                   g_weather_forecast.days[i].date,
                   g_weather_forecast.days[i].temperature,
                   g_weather_forecast.days[i].weather);
        }
    } else {
        printf("天气预报信息无效或未获取到数据\n");
    }
    printf("==================================\n\r\n");
}


// URL 编码函数 用于将字符串转换为 URL 编码格式，从而进行http请求
void str_to_urlencode(char* dest, const char* src) {
    while (*src) {
        if (isalnum(*src)) { // 字母和数字直接拷贝
            *dest++ = *src;
        } else { // 其他字符转为 %XX
            sprintf(dest, "%%%02X", (unsigned char)*src);
            dest += 3;
        }
        src++;
    }
    *dest = '\0';
}

/**
 * @brief 解析 +CIPSNTPTIME 返回的时间字符串，将时分秒写入 g_sys_time
 * @param raw_buffer 串口收到的原始字符串 (包含 "+CIPSNTPTIME:Fri Oct 27 14:30:05 2023")
 * @return int 解析成功返回 1，失败返回 0
 */
int Parse_SNTP_TIME_To_Global(const char *raw_buffer) {
    const char *tag = "+CIPSNTPTIME:";
    const char *p = strstr(raw_buffer, tag);
    if (p == NULL) {
        g_sys_time.is_valid = 0;
        return 0;
    }
    p += strlen(tag); // 指向 "Fri Oct 27 14:30:05 2023" 的起始

    // 使用 sscanf 提取时分秒
    // 格式: Week Month Day HH:MM:SS Year
    char week[8] = {0};
    char month[8] = {0};
    int day = 0, year = 0, hh = 0, mm = 0, ss = 0;
    int n = sscanf(p, "%7s %7s %d %d:%d:%d %d", week, month, &day, &hh, &mm, &ss, &year);
    if (n == 7) {
        g_sys_time.hour = (uint8_t)hh;
        g_sys_time.minute = (uint8_t)mm;
        g_sys_time.second = (uint8_t)ss;
        g_sys_time.is_valid = 1;
        return 1;
    }
    g_sys_time.is_valid = 0;
    return 0;
}

/**
 * @brief 打印 SNTP 获取的时分秒
 */
void Print_SNTP_Time(void) {
    printf("\r\n========== 当前网络时间 =========\n");
    if (g_sys_time.is_valid) {
        printf("时间: %02u:%02u:%02u\n", g_sys_time.hour, g_sys_time.minute, g_sys_time.second);
    } else {
        printf("时间信息无效或未获取到数据\n");
    }
    printf("================================\n\r\n");
}
