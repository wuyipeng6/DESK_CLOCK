#include "clock_UI.h"
#include "rtc/rtc.h"
#include "esp32_AT/jsondata.h"
#include "esp32_AT/esp_at.h"
#include "sht20/sht20.h"
#include <string.h>

/******************************************************************************
      函数说明：显示2位数码管数字(0-99)
      入口数据：x,y 起始坐标
                num 要显示的数字(0-99)
      返回值：  无
      说明：    数码管数字图片尺寸：数字1为8x35像素，其他数字为23x35像素
                两位数字之间间隔2像素
******************************************************************************/
void ClockUI_ShowDigital2(u16 x, u16 y, u8 num)
{
    u8 tens, ones;              // 十位和个位
    const u8 *digit_pic[10];    // 数字图片数组指针
    u16 tens_width, ones_width; // 十位和个位的宽度
    u16 x_offset;               // 个位的X坐标偏移

    // 限制范围在0-99
    if (num > 99)
        num = 99;

    // 分离十位和个位
    tens = num / 10;
    ones = num % 10;

    // 初始化数字图片指针数组
    digit_pic[0] = digital_0;
    digit_pic[1] = digital_1;
    digit_pic[2] = digital_2;
    digit_pic[3] = digital_3;
    digit_pic[4] = digital_4;
    digit_pic[5] = digital_5;
    digit_pic[6] = digital_6;
    digit_pic[7] = digital_7;
    digit_pic[8] = digital_8;
    digit_pic[9] = digital_9;

    // 获取十位数字的宽度
    tens_width = (tens == 1) ? 8 : 23;

    // 显示十位(跳过前8字节的头信息)
    LCD_ShowPicture(x, y, tens_width, 35, digit_pic[tens] + 8);

    // 计算个位的X坐标偏移 = 十位宽度 + 间隔2像素
    x_offset = tens_width + 2;

    // 获取个位数字的宽度
    ones_width = (ones == 1) ? 8 : 23;

    // 显示个位
    LCD_ShowPicture(x + x_offset, y, ones_width, 35, digit_pic[ones] + 8);
}

/******************************************************************************
      函数说明：显示温度数字
      入口数据：x,y 起始坐标
                temp 要显示的温度(-99~99)
                digits 显示位数(1或2)
      返回值：  无
      说明：    温度小字图片尺寸（高度均为24）：
                减号10像素宽，数字1为9像素宽，数字7为11像素宽，其他数字为13像素宽
                负号与数字之间、两位数字之间间隔1像素
******************************************************************************/
void ClockUI_ShowTemp2(u16 x, u16 y, s8 temp, u8 digits)
{
    u8 is_negative = 0;     // 是否为负数
    u8 abs_temp;            // 温度的绝对值
    u8 tens, ones;          // 十位和个位
    const u8 *temp_pic[10]; // 温度数字图片数组指针
    u16 temp_width[10];     // 每个数字的宽度
    u16 x_offset = 0;       // X坐标偏移

    // 限制位数在1-2之间
    if (digits < 1)
        digits = 1;
    if (digits > 2)
        digits = 2;

    // 限制范围在-99~99
    if (temp > 99)
        temp = 99;
    if (temp < -99)
        temp = -99;

    // 判断是否为负数
    if (temp < 0)
    {
        is_negative = 1;
        abs_temp = -temp;
    }
    else
    {
        abs_temp = temp;
    }

    // 分离十位和个位
    tens = abs_temp / 10;
    ones = abs_temp % 10;

    // 初始化温度数字图片指针数组和宽度数组
    temp_pic[0] = temp_0;
    temp_width[0] = 13;
    temp_pic[1] = temp_1;
    temp_width[1] = 9;
    temp_pic[2] = temp_2;
    temp_width[2] = 13;
    temp_pic[3] = temp_3;
    temp_width[3] = 13;
    temp_pic[4] = temp_4;
    temp_width[4] = 13;
    temp_pic[5] = temp_5;
    temp_width[5] = 13;
    temp_pic[6] = temp_6;
    temp_width[6] = 13;
    temp_pic[7] = temp_7;
    temp_width[7] = 11;
    temp_pic[8] = temp_8;
    temp_width[8] = 13;
    temp_pic[9] = temp_9;
    temp_width[9] = 13;

    // 如果是负数，先显示减号
    if (is_negative)
    {
        LCD_ShowPicture(x, y, 10, 24, temp_negtive + 8);
        x_offset = 10 + 1; // 减号宽度10 + 间隔1
    }

    // 如果是2位数，或者是1位数但值>=10，显示十位
    if (digits == 2 || abs_temp >= 10)
    {
        LCD_ShowPicture(x + x_offset, y, temp_width[tens], 24, temp_pic[tens] + 8);
        x_offset += temp_width[tens] + 1; // 十位宽度 + 间隔1
    }

    // 显示个位
    LCD_ShowPicture(x + x_offset, y, temp_width[ones], 24, temp_pic[ones] + 8);
}

/******************************************************************************
      函数说明：显示时分秒时间
      入口数据：x,y 起始坐标
      返回值：  无
      说明：    使用数码管数字显示时:分:秒格式，自动从RTC获取时间
                每组数字之间用冒号(两个圆点)分隔
                只刷新有变化的时间位
******************************************************************************/
void ClockUI_ShowTime(u16 x, u16 y)
{
    static u8 last_hour = 0xFF;   // 上次的小时（初始化为无效值）
    static u8 last_minute = 0xFF; // 上次的分钟
    static u8 last_second = 0xFF; // 上次的秒

    u16 x_offset = 0;
    u16 colon_x, colon_y;    // 冒号位置
    const u16 color = BLACK; // 冒号颜色
    u8 hour, minute, second; // 从RTC读取的时间

    // 从RTC获取当前时间
    RTC_GetHMS(&hour, &minute, &second);

    // 只在小时变化时清空并刷新小时
    if (hour != last_hour)
    {
        LCD_Fill(x + x_offset, y, x + x_offset + 47, y + 34, WHITE); // 清空小时区域
        ClockUI_ShowDigital2(x + x_offset, y, hour);
        last_hour = hour;
    }
    x_offset += 48 + 3; // 两位数字最大宽度(23+2+23) + 间隔3像素

    // 画第一个冒号（两个圆点）
    colon_x = x + x_offset;
    colon_y = y + 7;                                                   // 数字高度35，上面的点距顶部7像素
    LCD_Fill(colon_x, colon_y, colon_x + 6, colon_y + 6, color);       // 上面的点，7x7像素
    LCD_Fill(colon_x, colon_y + 14, colon_x + 6, colon_y + 20, color); // 下面的点，7x7像素，间隔14像素
    x_offset += 7 + 3;                                                 // 冒号宽度7 + 间隔3像素

    // 只在分钟变化时清空并刷新分钟
    if (minute != last_minute)
    {
        LCD_Fill(x + x_offset, y, x + x_offset + 47, y + 34, WHITE); // 清空分钟区域
        ClockUI_ShowDigital2(x + x_offset, y, minute);
        last_minute = minute;
    }
    x_offset += 48 + 3;

    // 画第二个冒号
    colon_x = x + x_offset;
    colon_y = y + 7;
    LCD_Fill(colon_x, colon_y, colon_x + 6, colon_y + 6, color);
    LCD_Fill(colon_x, colon_y + 14, colon_x + 6, colon_y + 20, color);
    x_offset += 7 + 3;

    // 只在秒变化时清空并刷新秒
    if (second != last_second)
    {
        LCD_Fill(x + x_offset, y, x + x_offset + 47, y + 34, WHITE); // 清空秒区域
        ClockUI_ShowDigital2(x + x_offset, y, second);
        last_second = second;
    }
}

/******************************************************************************
      函数说明：显示日期(年月日)
      入口数据：x,y 起始坐标
      返回值：  无
      说明：    使用温度小字显示年.月.日格式，自动从天气数据获取日期
                年份显示4位，月日显示2位，之间用点分隔
                日期格式: "2025-12-29"
******************************************************************************/
void ClockUI_ShowDate(u16 x, u16 y)
{
    u16 x_offset = 0;
    u8 year_digits[4]; // 年份的四位数字
    const u8 *temp_pic[10];
    u16 temp_width[10];
    u16 dot_size = 3; // 点的大小
    const u16 color = BLACK;
    u16 year = 2025;
    u8 month = 1;
    u8 day = 1;

    // 从天气数据中解析日期 (格式: "2025-12-29")
    if (g_weather_forecast.is_valid && strlen(g_weather_forecast.days[0].date) >= 10)
    {
        // 解析年份 (前4位)
        year = (g_weather_forecast.days[0].date[0] - '0') * 1000 +
               (g_weather_forecast.days[0].date[1] - '0') * 100 +
               (g_weather_forecast.days[0].date[2] - '0') * 10 +
               (g_weather_forecast.days[0].date[3] - '0');

        // 解析月份 (第6-7位，跳过'-')
        month = (g_weather_forecast.days[0].date[5] - '0') * 10 +
                (g_weather_forecast.days[0].date[6] - '0');

        // 解析日期 (第9-10位，跳过第二个'-')
        day = (g_weather_forecast.days[0].date[8] - '0') * 10 +
              (g_weather_forecast.days[0].date[9] - '0');
    }

    // 初始化温度数字图片指针数组和宽度数组
    temp_pic[0] = temp_0;
    temp_width[0] = 13;
    temp_pic[1] = temp_1;
    temp_width[1] = 9;
    temp_pic[2] = temp_2;
    temp_width[2] = 13;
    temp_pic[3] = temp_3;
    temp_width[3] = 13;
    temp_pic[4] = temp_4;
    temp_width[4] = 13;
    temp_pic[5] = temp_5;
    temp_width[5] = 13;
    temp_pic[6] = temp_6;
    temp_width[6] = 13;
    temp_pic[7] = temp_7;
    temp_width[7] = 11;
    temp_pic[8] = temp_8;
    temp_width[8] = 13;
    temp_pic[9] = temp_9;
    temp_width[9] = 13;

    // 分离年份的四位数字
    year_digits[0] = year / 1000;       // 千位
    year_digits[1] = (year / 100) % 10; // 百位
    year_digits[2] = (year / 10) % 10;  // 十位
    year_digits[3] = year % 10;         // 个位

    // 显示年份(4位)
    for (u8 i = 0; i < 4; i++)
    {
        LCD_ShowPicture(x + x_offset, y, temp_width[year_digits[i]], 24, temp_pic[year_digits[i]] + 8);
        x_offset += temp_width[year_digits[i]] + 1;
    }

    // 画第一个点分隔符
    LCD_Fill(x + x_offset, y + 10, x + x_offset + dot_size - 1, y + 10 + dot_size - 1, color);
    x_offset += dot_size + 2;

    // 显示月份(2位)
    ClockUI_ShowTemp2(x + x_offset, y, month, 2);
    x_offset += 13 + 1 + 13 + 2; // 最大宽度 + 间隔 + 点的间隔

    // 画第二个点分隔符
    LCD_Fill(x + x_offset, y + 10, x + x_offset + dot_size - 1, y + 10 + dot_size - 1, color);
    x_offset += dot_size + 2;

    // 显示日期(2位)
    ClockUI_ShowTemp2(x + x_offset, y, day, 2);
}

/******************************************************************************
      函数说明：显示星期
      入口数据：x,y 起始坐标
      返回值：  无
      说明：    显示"星期X"汉字，图片尺寸40x24 + 20x24 = 60x24像素
                使用基姆拉尔森公式自动计算星期几
******************************************************************************/
void ClockUI_ShowWeek(u16 x, u16 y)
{
    const u8 *week_pic[7] = {
        week_1, week_2, week_3, week_4, week_5, week_6, week_7};

    u16 year = 2025;
    u8 month = 1;
    u8 day = 1;
    u8 week;
    int year_calc, month_calc, day_calc, w;

    // 从天气数据中解析日期 (格式: "2025-12-29")
    if (g_weather_forecast.is_valid && strlen(g_weather_forecast.days[0].date) >= 10)
    {
        // 解析年份 (前4位)
        year = (g_weather_forecast.days[0].date[0] - '0') * 1000 +
               (g_weather_forecast.days[0].date[1] - '0') * 100 +
               (g_weather_forecast.days[0].date[2] - '0') * 10 +
               (g_weather_forecast.days[0].date[3] - '0');

        // 解析月份 (第6-7位，跳过'-')
        month = (g_weather_forecast.days[0].date[5] - '0') * 10 +
                (g_weather_forecast.days[0].date[6] - '0');

        // 解析日期 (第9-10位，跳过第二个'-')
        day = (g_weather_forecast.days[0].date[8] - '0') * 10 +
              (g_weather_forecast.days[0].date[9] - '0');
    }

    // 基姆拉尔森公式计算星期几
    // 1月和2月看作上一年的13月和14月
    year_calc = year;
    month_calc = month;
    day_calc = day;

    if (month_calc == 1 || month_calc == 2)
    {
        month_calc += 12;
        year_calc -= 1;
    }

    // 基姆拉尔森公式: W = (d + 2*m + 3*(m+1)/5 + y + y/4 - y/100 + y/400 + 1) % 7
    // 结果: 0=星期一, 1=星期二, ..., 6=星期日
    w = (day_calc + 2 * month_calc + 3 * (month_calc + 1) / 5 + year_calc + year_calc / 4 - year_calc / 100 + year_calc / 400 + 1) % 7;

    // 转换为 1-7 (1=星期一, 7=星期日)
    week = (w == 0) ? 7 : w;

    // 显示"星期"两个字 (40x24)
    LCD_ShowPicture(x, y, 40, 24, week_CH + 8);

    // 显示具体的星期几 (20x24)
    LCD_ShowPicture(x + 40, y, 20, 24, week_pic[week - 1] + 8);
}

/******************************************************************************
      函数说明：时间刷新函数
      入口数据：x_time, y_time 时间显示位置
                x_date, y_date 日期显示位置
                x_week, y_week 星期显示位置
      返回值：  无
      说明：    刷新时间显示，当检测到 23:59:59 时自动联网更新数据
******************************************************************************/
void ClockUI_TimeRefresh(u16 x_time, u16 y_time, u16 x_date, u16 y_date, u16 x_week, u16 y_week)
{
    static u8 last_refresh_done = 0; // 防止多次刷新标志
    u8 hour, minute, second;

    // 显示当前时间
    ClockUI_ShowTime(x_time, y_time);

    // 获取当前时间用于判断
    RTC_GetHMS(&hour, &minute, &second);

    // 如果是 23:59:59，则联网刷新数据
    if (hour == 23 && minute == 59 && second == 59)
    {
        if (!last_refresh_done) // 防止在同一秒内多次执行
        {
            // 联网获取最新数据
            power_on_step1_location_get(); // 获取IP及地理位置
            power_on_step2_weather_get();  // 获取天气信息
            esp_at_get_sntp_time();        // 获取SNTP时间并设置RTC

            // 刷新日期和星期显示
            ClockUI_ShowDate(x_date, y_date);
            ClockUI_ShowWeek(x_week, y_week);

            last_refresh_done = 1; // 标记已刷新
        }
    }
    else
    {
        // 不是 23:59:59 时，重置标志
        last_refresh_done = 0;
    }
}

/******************************************************************************
      函数说明：显示温度和湿度
      入口数据：x_temp, y_temp 温度显示坐标
                x_humi, y_humi 湿度显示坐标
      返回值：  无
      说明：    通过SHT20传感器获取温度和湿度，转化为整数，在LCD上显示
******************************************************************************/
void ClockUI_ShowTempHumi(u16 x_temp, u16 y_temp, u16 x_humi, u16 y_humi)
{
    float temperature = 0.0f;
    float humidity = 0.0f;
    s8 temp_int = 0; // 温度整数
    s8 humi_int = 0; // 湿度整数

    // 获取温度和湿度
    temperature = HOLD_SHT20_GetTemperature();
    humidity = HOLD_SHT20_GetHumidity();

    // 将浮点数转化为整数 (0.5以上进位)
    temp_int = (s8)(temperature + 0.5f);
    humi_int = (s8)(humidity + 0.5f);

    // 限制范围
    if (temp_int > 99)
        temp_int = 99;
    if (temp_int < -99)
        temp_int = -99;
    if (humi_int > 99)
        humi_int = 99;
    if (humi_int < 0)
        humi_int = 0;

    // 显示温度 (使用温度小字显示, 两位数字)
    ClockUI_ShowTemp2(x_temp, y_temp, temp_int, 2);

    // 显示湿度 (使用温度小字显示, 两位数字)
    ClockUI_ShowTemp2(x_humi, y_humi, humi_int, 2);
}