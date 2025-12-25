#include "esp_at.h"
#include "delay.h"
#include "usart2_dma_at.h"
#include "jsondata.h"

//初始化ESP-AT模块所使用的串口USART2
void esp_at_usart_init(void)//static函数等于文件的私有函数
{
    USART2_DMA_Config();//初始化USART2+DMA接收
}
void power_on_step1_location_get(void)
{
    // 1. 检查ESP-AT模块
    if (!esp_at_check_module()) {
        while(1) delay_ms(100);
    }
    // 2. 设置WiFi工作模式为Station (STA)
    esp_at_set_mode(1);
    delay_ms(100);
    // 3. 连接到WiFi网络
    if (!esp_at_connect_wifi("whatcanisay", "123456qq")) {
        while(1) delay_ms(100);
    }
    // 延迟一段时间确保WiFi连接稳定
    delay_ms(1000);
    // // 4. 获取本地IP地址  未来可以将结构体IP_Location_t的内容放入后背区，掉电不丢失，
    //每次上电，先读取本地IP，与结构体比较IP，如果相同，则不进行HTTP请求，节省时间和流量。
    // char local_ip[20] = {0};
    // esp_at_get_local_ip(local_ip, sizeof(local_ip));
    // delay_ms(1000);
    
    // 5. 发送HTTP GET请求获取公网IP和地理位置信息
    if (esp_at_http_get_IP()) {
        delay_ms(500);
    }
}

void power_on_step2_weather_get(void)
{
    // 发送HTTP GET请求获取天气信息
    if (esp_at_http_get_weather()) {
        delay_ms(500);
    }
}


//清空ESP-AT模块接收缓冲区
void esp_at_clear_buffer(void) {
    rx_flag = 0;
    rx_len = 0;
    memset(MAIN_RX_BUF, 0, UART_RX_BUF_SIZE);
}


/**
 * @brief  等待 ESP32 AT 指令的特定应答
 * @param  expected: 期待的关键字 (如 "OK", "READY", "CONNECT",保险一点可以加\r\n)
 * @param  timeout_ms: 最大等待时间 (毫秒)
 * @return true: 成功匹配到关键字; false: 超时未匹配或收到 ERROR
 */
bool esp_at_wait_response(const char* expected, uint16_t timeout_ms) {
    // 1. 获取起始时间 (使用你提供的 cpu_now() 封装的 ms 函数)
    uint64_t start_time = cpu_get_ms();

    while (cpu_get_ms() - start_time < (uint64_t)timeout_ms) {

        // 2. Check if IDLE interrupt indicates new data packet
        if (rx_flag) {
            //printf("Data: %s\r\n", (char*)MAIN_RX_BUF);
            // 3. 在转存的缓冲区中查找期待的字符串
            if (strstr((char*)MAIN_RX_BUF, expected) != NULL) {
                rx_flag = 0;  // 找到匹配后才清除标志
                return true;  // 找到了，立即返回
            }

            // 4. 可选：如果收到 "ERROR"，可以提前结束等待以节省时间
            if (strstr((char*)MAIN_RX_BUF, "ERROR") != NULL) {
                rx_flag = 0;  // 收到错误后才清除标志
                // 注意：有些指令失败是正常的，根据你的逻辑决定是否直接 return false
                return false; 
            }
            
            // 都不匹配，清除标志，继续等待下一包数据
            // 注意：不在这里 memset MAIN_RX_BUF，
            // 因为有些长响应（如扫描 WiFi）是分多次 IDLE 产生的。
            rx_flag = 0;
        }
        
        // 给 CPU 留一点点喘息时间（可选，视具体主频而定）
        // 如果主频很高，可以在这里加个微秒级小延时
    }

    return false; // 到时间了还没匹配到
}

bool send_at_command(char* cmd, char* expected_res, uint32_t timeout) {
    // 1. 发送前先清理现场
    esp_at_clear_buffer();
    
    // 2. 通过串口发送指令
    // 假设你有一个简单的串口发送字符串函数
    while (cmd && *cmd)//发送字符串，遇到字符串结束符'\0'停止
    {
        while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);//等待发送区空
        USART_SendData(USART2, *cmd++);//*的优先级高于++
    }
    while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
    USART_SendData(USART2, '\r');
    while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
    USART_SendData(USART2, '\n');
    
    // 3. 调用等待函数
    return esp_at_wait_response(expected_res, timeout);
}

/**
 * @brief 发送HTTP GET请求IP地址查询 并
 * @return true: 成功; false: 失败
 * @note:获取IP地址，返回的数据是连续的，不需要分缓冲区存储。
 */
bool esp_at_http_get_IP(void) {
    char cmd[256];
    
    // 使用AT+HTTPCLIENT命令发送HTTP GET请求
    char host[] = "\"http://ip-api.com/json/?lang=zh-CN\"";
    uint16_t timeout = 15000;

    sprintf(cmd, "AT+HTTPCLIENT=2,1,%s,,,1",host);
    
    if (!send_at_command(cmd, "OK", timeout)) {
        printf("[ERROR] HTTP GET请求失败\r\n");
        return false;
    }
    
    if (!Parse_IP_JSON_To_Global((char*)MAIN_RX_BUF)) {
        printf("[ERROR] IP的JSON数据解析失败\r\n");
        return false;
    }
    return true;
}

/**
 * @brief 发送HTTP GET请求查询天气数据，同时把数据解析存入结构体
 * @return true: 成功; false: 失败
 * @note 依赖 g_ip_info.city 作为城市名参数
 */
bool esp_at_http_get_weather(void) {
    if (!g_ip_info.is_valid || g_ip_info.city[0] == '\0') {
        printf("[ERROR] 城市信息无效，无法请求天气\r\n");
        return false;
    }

    // URL编码城市名称
    char city_encoded[96];  // 预留足够空间，URL编码后长度会增加
    str_to_urlencode(city_encoded, g_ip_info.city);

    char url[256];
    snprintf(url, sizeof(url),
             "\"http://apis.juhe.cn/simpleWeather/query?city=%s&key=%s\"",
             city_encoded, JUHE_WEATHER_KEY);

    char cmd[320];
    uint16_t timeout = 15000;
    snprintf(cmd, sizeof(cmd), "AT+HTTPCLIENT=2,1,%s,,,1", url);

    if (!send_at_command(cmd, "OK", timeout)) {
        printf("[ERROR] 天气HTTP GET请求失败\r\n");
        return false;
    }
    // 解析天气JSON数据并存入全局变量
    if (!Parse_Weather_JSON_To_Global((char*)MAIN_RX_BUF)) {
        printf("[ERROR] 天气JSON数据解析失败\r\n");
        return false;
    }

    // 响应已保存在 MAIN_RX_BUF，按需自行解析或打印
    return true;
}

/**
 * @brief 配置SNTP并获取网络时间，解析到 g_sys_time 前提也是必须联网
 * @return true: 成功; false: 失败
 */
bool esp_at_get_sntp_time(void) {
    // 1. 配置SNTP: 使能，时区8，中国常用NTP服务器
    if (!send_at_command("AT+CIPSNTPCFG=1,8,\"cn.ntp.org.cn\",\"ntp.sjtu.edu.cn\"", "+TIME_UPDATED", 5000)) {
        printf("[ERROR] SNTP配置失败\r\n");
        return false;
    }

    delay_ms(200);

    // 2. 查询时间
    if (!send_at_command("AT+CIPSNTPTIME?", "OK", 5000)) {
        printf("[ERROR] SNTP时间查询失败\r\n");
        return false;
    }

    // 3. 解析 +CIPSNTPTIME 中的时分秒
    if (!Parse_SNTP_TIME_To_Global((char*)MAIN_RX_BUF)) {
        printf("[ERROR] SNTP时间解析失败\r\n");
        return false;
    }
    return true;
}

/**
 * @brief 检查ESP-AT模块是否正常工作 (带重试机制)
 * @return true: 模块正常; false: 模块异常
 * @note: 如果第一次收不到OK，会重新发送，总共尝试3次
 */
bool esp_at_check_module(void) {
    uint8_t retry_count = 3;
    
    for (uint8_t i = 0; i < retry_count; i++) {
        if (send_at_command("AT", "OK", 1000)) {
            return true;  // 成功，直接返回
        }
        // 第一次和第二次失败时，等待后重试
        if (i < retry_count - 1) {
            printf("[WARNING] ESP-AT模块第%d次检查失败,准备重试...\r\n", i + 1);
            delay_ms(500);  // 等待500ms后重试
        }
    }
    
    printf("[ERROR] ESP-AT模块在3次尝试后仍无响应\r\n");
    return false;
}

/**
 * @brief 设置WiFi工作模式
 * @param mode: 1=Station模式(STA), 2=AP模式, 3=Station+AP模式
 * @return true: 设置成功; false: 设置失败
 */
bool esp_at_set_mode(uint8_t mode) {
    char cmd[32];
    sprintf(cmd, "AT+CWMODE=%d", mode);
    if (!send_at_command(cmd, "OK", 1000)) {
        printf("[ERROR] WiFi模式设置失败 (mode=%d)\r\n", mode);
        return false;
    }
    return true;
}

/**
 * @brief 连接WiFi网络
 * @param ssid: WiFi网络名称 (SSID)
 * @param password: WiFi密码
 * @return true: 连接成功; false: 连接失败
 */
bool esp_at_connect_wifi(const char* ssid, const char* password) {
    char cmd[128];
    // AT+CWJAP="SSID","password"
    sprintf(cmd, "AT+CWJAP=\"%s\",\"%s\"", ssid, password);
    if (!send_at_command(cmd, "OK", 10000)) {
        printf("[ERROR] WiFi连接失败 (SSID=%s)\r\n", ssid);
        return false;
    }
    return true;
}

/**
 * @brief 断开WiFi连接
 * @return true: 断开成功; false: 断开失败
 */
bool esp_at_disconnect_wifi(void) {
    if (!send_at_command("AT+CWQAP", "OK", 2000)) {
        printf("[ERROR] WiFi断开连接失败\r\n");
        return false;
    }
    return true;
}

/**
 * @brief 获取本地IP地址 (使用AT+CIPSTA命令)
 * @param ip_str: 用于存储IP地址的缓冲区
 * @param len: 缓冲区长度
 * @return true: 获取成功; false: 获取失败
 * @note: IP地址格式为 "192.168.x.x"
 */
bool esp_at_get_local_ip(char* ip_str, uint16_t len) {
    if (!send_at_command("AT+CIPSTA?", "OK", 2000)) {
        printf("[ERROR] AT+CIPSTA?命令发送失败\r\n");
        return false;
    }
    
    // 从缓冲区中提取IP地址
    // 响应格式: +CIPSTA:ip:"192.168.x.x"
    char* ip_start = strstr((char*)MAIN_RX_BUF, "ip:\"");
    if (ip_start == NULL) {
        printf("[ERROR] 未找到IP地址信息\r\n");
        return false;
    }
    
    // 移动指针到IP地址开始位置
    ip_start += 4;  // 跳过 "ip:\""
    
    // 查找结束引号
    char* ip_end = strchr(ip_start, '"');
    if (ip_end == NULL) {
        printf("[ERROR] IP地址格式错误\r\n");
        return false;
    }
    
    // 计算IP地址长度
    uint16_t ip_len = ip_end - ip_start;
    if (ip_len >= len) {
        printf("[ERROR] 缓冲区太小\r\n");
        return false;
    }
    
    // 拷贝IP地址到缓冲区
    strncpy(ip_str, ip_start, ip_len);
    ip_str[ip_len] = '\0';
    
    return true;
}

/**
 * @brief 打印接收到的HTTP响应数据
 */
void esp_at_print_response(void) {
    printf("\r\n========== HTTP 响应数据 ==========\r\n");
    printf("%s\r\n", (char*)MAIN_RX_BUF);
    printf("==================================\r\n\r\n");
}

// ESP32 AT 模块功能演示
void esp32_at_demo(void)
{
    esp_at_usart_init();//初始化ESP-AT模块所使用的串口USART2
    power_on_step1_location_get();//上电联网并获取IP及地理位置，初始化结构体IP_Location_t
    Print_IP_Info();  //打印IP及地理位置信息
    power_on_step2_weather_get();//获取天气信息，初始化结构体Weather_Forecast_t，
    Print_Weather_Forecast();//打印天气预报信息
    esp_at_get_sntp_time();//获取SNTP时间，初始化结构体System_Time_t
    Print_SNTP_Time();//打印SNTP时间信息

}

