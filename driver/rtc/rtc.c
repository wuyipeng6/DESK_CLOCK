#include "rtc.h"
#include "esp32_AT/jsondata.h"
#define RTC_BKP_MARKER RTC_BKP_DR0
#define RTC_INIT_FLAG  0x32F1U

/* 启用 LSE 时钟并等待就绪，用作 RTC 时钟源 */
static void rtc_enable_lse(void) {
    RCC_LSEConfig(RCC_LSE_ON);
    while (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET) {
    }
}

/* RTC 初始化：首次上电配置 LSE、分频和初始时间，写入备份标记；之后上电只同步即可 */
void RTC_Config(void) {
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
    PWR_BackupAccessCmd(ENABLE);

    if (RTC_ReadBackupRegister(RTC_BKP_MARKER) != RTC_INIT_FLAG) {// 未初始化
        rtc_enable_lse();
        RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
        RCC_RTCCLKCmd(ENABLE);
        RTC_WaitForSynchro();

        RTC_InitTypeDef rtc;
        RTC_StructInit(&rtc);
        rtc.RTC_HourFormat = RTC_HourFormat_24;
        rtc.RTC_AsynchPrediv = 127;
        rtc.RTC_SynchPrediv  = 255; /* 32.768kHz -> 1Hz */
        RTC_Init(&rtc);

        RTC_TimeTypeDef t = {0};
        t.RTC_Hours = 0;
        t.RTC_Minutes = 0;
        t.RTC_Seconds = 0;
        RTC_SetTime(RTC_Format_BIN, &t);

        RTC_WriteBackupRegister(RTC_BKP_MARKER, RTC_INIT_FLAG);// 写入初始化标记
    } else {
        rtc_enable_lse();
        RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
        RCC_RTCCLKCmd(ENABLE);
        RTC_WaitForSynchro();
    }
}

/* 仅在需要手动校时调用一次，设置当前时分秒 */
void RTC_SetTimeOnce(uint8_t hh, uint8_t mm, uint8_t ss) {
    RTC_TimeTypeDef t;
    t.RTC_Hours = hh;
    t.RTC_Minutes = mm;
    t.RTC_Seconds = ss;
    RTC_SetTime(RTC_Format_BIN, &t);
}





/* 读取当前时分秒，注意需先读日期以解锁影子寄存器 */
void RTC_GetHMS(uint8_t *hh, uint8_t *mm, uint8_t *ss) {
    RTC_TimeTypeDef t;
    RTC_DateTypeDef d; /* read date to unlock shadow registers */
    RTC_GetTime(RTC_Format_BIN, &t);
    RTC_GetDate(RTC_Format_BIN, &d);
    if (hh) {
        *hh = t.RTC_Hours;
    }
    if (mm) {
        *mm = t.RTC_Minutes;
    }
    if (ss) {
        *ss = t.RTC_Seconds;
    }
}
