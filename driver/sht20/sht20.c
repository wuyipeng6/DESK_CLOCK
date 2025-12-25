#include "sht20.h"
#include <stdio.h>

// 声明外部延时函数
extern void delay_ms(uint32_t ms);

/**
  * @brief  初始化 I2C1 (PB6:SCL, PB7:SDA)
  */
void SHT20_Init(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    I2C_InitTypeDef I2C_InitStructure;

    // 1. 开启时钟
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);

    // 2. 硬件复位 I2C1 防止之前调试导致的忙死锁
    RCC_APB1PeriphResetCmd(RCC_APB1Periph_I2C1, ENABLE);
    RCC_APB1PeriphResetCmd(RCC_APB1Periph_I2C1, DISABLE);

    // 3. 配置引脚复用
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource6, GPIO_AF_I2C1);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource7, GPIO_AF_I2C1);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_OD; // 必须开漏
    GPIO_InitStructure.GPIO_Speed = GPIO_High_Speed;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;   // 内部上拉
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    // 4. I2C 规格配置
    I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
    I2C_InitStructure.I2C_OwnAddress1 = 0x00;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_InitStructure.I2C_ClockSpeed = 100000; // 100kHz

    I2C_Init(I2C1, &I2C_InitStructure);
    I2C_Cmd(I2C1, ENABLE);
}

/**
  * @brief  等待标志位并带超时处理
  */
static uint8_t WaitEvent(I2C_TypeDef* I2Cx, uint32_t I2C_EVENT, char* msg) {
    uint32_t timeout = 500000; 
    while(!I2C_CheckEvent(I2Cx, I2C_EVENT)) {
        if((timeout--) == 0) {
            printf("SHT20 Error: %s !\r\n", msg);
            I2C_GenerateSTOP(I2Cx, ENABLE);
            return 1; // 失败
        }
    }
    return 0; // 成功
}

/**
  * @brief  软复位
  */
void SHT20_SoftReset(void) {
    I2C_GenerateSTART(I2C1, ENABLE);
    if(WaitEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT, "Start")) return;

    I2C_Send7bitAddress(I2C1, SHT20_ADDR_W, I2C_Direction_Transmitter);
    if(WaitEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED, "AddrW")) return;

    I2C_SendData(I2C1, CMD_SOFT_RESET);
    if(WaitEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED, "ResetCmd")) return;

    I2C_GenerateSTOP(I2C1, ENABLE);
    delay_ms(15); // 复位需要时间
}

/**
  * @brief  底层测量函数 (Holding Master)
  */
static float SHT20_Measure(uint8_t cmd) {
    uint8_t data[3]; // [0]MSB, [1]LSB, [2]CRC
    uint32_t timeout = 2000000;

    // --- 写入测量命令 ---
    I2C_GenerateSTART(I2C1, ENABLE);
    if(WaitEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT, "Start")) return -1;

    I2C_Send7bitAddress(I2C1, SHT20_ADDR_W, I2C_Direction_Transmitter);
    if(WaitEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED, "AddrW")) return -1;

    I2C_SendData(I2C1, cmd);
    if(WaitEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED, "MeasureCmd")) return -1;

    // --- 开始读取 ---
    I2C_GenerateSTART(I2C1, ENABLE);
    if(WaitEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT, "Restart")) return -1;

    I2C_Send7bitAddress(I2C1, SHT20_ADDR_R, I2C_Direction_Receiver);
    
    // Holding Master 核心：等待传感器释放 SCL 并发出 ADDR 标志位
    while(!I2C_GetFlagStatus(I2C1, I2C_FLAG_ADDR)) {
        if((timeout--) == 0) {
            printf("SHT20 Error: Holding Master Wait Timeout!\r\n");
            return -1;
        }
    }
    
    // --- 接收 3 字节标准序列 (STM32 SPL) ---
    (void)I2C1->SR2; // 读取 SR2 清除 ADDR 位

    // 读第一个字节 MSB
    while(!I2C_GetFlagStatus(I2C1, I2C_FLAG_RXNE));
    data[0] = I2C_ReceiveData(I2C1);

    // 读第二个字节 LSB
    while(!I2C_GetFlagStatus(I2C1, I2C_FLAG_RXNE));
    data[1] = I2C_ReceiveData(I2C1);

    // 准备读最后一个字节 CRC 前：禁用 ACK + 产生 STOP
    I2C_AcknowledgeConfig(I2C1, DISABLE);
    I2C_GenerateSTOP(I2C1, ENABLE);

    // 读最后一个字节 CRC
    while(!I2C_GetFlagStatus(I2C1, I2C_FLAG_RXNE));
    data[2] = I2C_ReceiveData(I2C1);

    I2C_AcknowledgeConfig(I2C1, ENABLE); // 恢复 ACK 供下次测量使用

    // 计算物理值
    uint16_t raw = ((uint16_t)data[0] << 8) | data[1];
    raw &= 0xFFFC; // 屏蔽状态位

    if (cmd == CMD_T_MEASURE_HOLD)
        return -46.85f + 175.72f * ((float)raw / 65536.0f);
    else
        return -6.0f + 125.0f * ((float)raw / 65536.0f);
}

float HOLD_SHT20_GetTemperature(void) { return SHT20_Measure(CMD_T_MEASURE_HOLD); }
float HOLD_SHT20_GetHumidity(void) { return SHT20_Measure(CMD_RH_MEASURE_HOLD); }




//用于No hold master 模式
void SHT20_Trigger_measurement(uint8_t cmd)
{
    I2C_GenerateSTART (I2C1,ENABLE);
    if(WaitEvent(I2C1,I2C_EVENT_MASTER_MODE_SELECT,"START timeout")) return ;
    I2C_Send7bitAddress(I2C1,SHT20_ADDR_W,I2C_Direction_Transmitter);
    if(WaitEvent (I2C1,I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED,"ADDRESS send timeout")) return ;
    I2C_SendData (I2C1,cmd);
    if(WaitEvent (I2C1,I2C_EVENT_MASTER_BYTE_TRANSMITTED,"BYTE send timeout")) return ;
     I2C_GenerateSTOP(I2C1, ENABLE);
}

//发送读地址，成功接收ACK则返回1，失败返回0
uint8_t STH20_if_R_ACK(void)
{
    I2C_GenerateSTART (I2C1,ENABLE);
    uint32_t i =500000;
    while(!I2C_CheckEvent(I2C1,I2C_EVENT_MASTER_MODE_SELECT))
    {
        i--;
        if(i == 0)
        {
            I2C_GenerateSTOP(I2C1, ENABLE);
            return 0;
        }
    }
            
//    if(!I2C_CheckEvent(I2C1,I2C_EVENT_MASTER_MODE_SELECT)) 
//    {
//        I2C_GenerateSTOP(I2C1, ENABLE);
//        return 0;
//    }
    //if(WaitEvent(I2C1,I2C_EVENT_MASTER_MODE_SELECT,"RESTART timeout")) return 0;
    I2C_Send7bitAddress(I2C1,SHT20_ADDR_R,I2C_Direction_Receiver);
    uint32_t timeout = 10000;
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED)) 
    {
        if(I2C_GetFlagStatus(I2C1, I2C_FLAG_AF) || (timeout-- == 0)) 
        {
            I2C_ClearFlag(I2C1, I2C_FLAG_AF); // 清除应答失败标志
            I2C_GenerateSTOP(I2C1, ENABLE);   // 必须发停止位释放总线
            return 0; // 还没测完或硬件错误
        }
    }
    return 1;
}


//适用于NO HOLD模式下trigger后 数值的获取；
//测量完成，数据发送成功返回0，失败返回1；
void SHT20_GetResult(uint8_t cmd, float *result)
{
    while(!STH20_if_R_ACK())
    {
        delay_ms (20);
    }
//    I2C_GenerateSTART (I2C1,ENABLE);
//    if(WaitEvent(I2C1,I2C_EVENT_MASTER_MODE_SELECT,"RESTART timeout")) return ;
//    I2C_Send7bitAddress(I2C1,SHT20_ADDR_R,I2C_Direction_Receiver);
//    for(int i =0;i<200;i++);//给硬件传输数据的时间
//    //I2C_CheckEvent 成功返回1，失败返回0
//    if(!I2C_CheckEvent(I2C1,I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED)) //在这里面会自动清零ADDR
//    {
//        //复位AF位（应答失败位）
//        I2C_ClearFlag(I2C1, I2C_FLAG_AF);
//        I2C_GenerateSTOP(I2C1, ENABLE);
//        return ;
//    }
    //上面函数给的硬件移位时间太少了，应该这么写。CHECK应该是循环调用的，不能是单次if判断，否则每次都会失败，因为硬件处理时间太短了
//    uint32_t timeout = 10000;
//    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED)) 
//    {
//        if(I2C_GetFlagStatus(I2C1, I2C_FLAG_AF) || (timeout-- == 0)) 
//        {
//            I2C_ClearFlag(I2C1, I2C_FLAG_AF); // 清除应答失败标志
//            I2C_GenerateSTOP(I2C1, ENABLE);   // 必须发停止位释放总线
//            return ; // 还没测完或硬件错误
//        }
//    }
    //下面是收到ACK后的处理
    uint8_t data[3]; // [0]MSB, [1]LSB, [2]CRC
    
        // 读第一个字节 MSB
    while(!I2C_GetFlagStatus(I2C1, I2C_FLAG_RXNE));
    data[0] = I2C_ReceiveData(I2C1);

    // 读第二个字节 LSB
    while(!I2C_GetFlagStatus(I2C1, I2C_FLAG_RXNE));
    data[1] = I2C_ReceiveData(I2C1);

    // 准备读最后一个字节 CRC 前：禁用 ACK + 产生 STOP
    I2C_AcknowledgeConfig(I2C1, DISABLE);
    I2C_GenerateSTOP(I2C1, ENABLE);

    // 读最后一个字节 CRC
    while(!I2C_GetFlagStatus(I2C1, I2C_FLAG_RXNE));
    data[2] = I2C_ReceiveData(I2C1);

    I2C_AcknowledgeConfig(I2C1, ENABLE); // 恢复 ACK 供下次测量使用
    
        // 计算物理值
    uint16_t raw = ((uint16_t)data[0] << 8) | data[1];
    raw &= 0xFFFC; // 屏蔽状态位

    if (cmd == CMD_T_MEASURE_NO_HOLD)
        *result= -46.85f + 175.72f * ((float)raw / 65536.0f);
    else
        *result= -6.0f + 125.0f * ((float)raw / 65536.0f);
}





