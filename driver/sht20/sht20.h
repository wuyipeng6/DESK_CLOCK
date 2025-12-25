#ifndef __SHT20_H
#define __SHT20_H

#include "stm32f4xx.h"

// SHT20 地址及命令定义
#define SHT20_ADDR_W        0x80
#define SHT20_ADDR_R        0x81

#define CMD_T_MEASURE_HOLD  0xE3  // 温度测量(主机锁定)
#define CMD_RH_MEASURE_HOLD 0xE5  // 湿度测量(主机锁定)
#define CMD_T_MEASURE_NO_HOLD  0XF3  //温度测量（NO hold)
#define CMD_RH_MEASURE_NO_HOLD 0XF5  //湿度测量（NO hold）
#define CMD_SOFT_RESET      0xFE  // 软复位命令

// 函数声明
void SHT20_Init(void);   //驱动步骤1  与IIC_Init() 函数相同，调用一个就可以。
void SHT20_SoftReset(void);  //驱动步骤2
//应用在hold master 模式下
float HOLD_SHT20_GetTemperature(void);   //使用HOLD MASTER模式获取温度数据
float HOLD_SHT20_GetHumidity(void);   //直接调用可得数据 hold下驱动步骤3  

//应用再no hold master 模式下
void SHT20_Trigger_measurement(uint8_t cmd);  //驱动步骤3 测温度等85ms，测湿度等29ms后，直接调用会在函数中延时，等待测量完成，不会报错。
void SHT20_GetResult(uint8_t cmd, float *result);//驱动步骤4
#endif

