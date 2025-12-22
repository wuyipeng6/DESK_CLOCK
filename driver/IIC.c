#include "stm32f4xx.h"
#include "IIC.h"
#include "usart.h"


// I2C初始化
void IIC_Init(void)
{
    uint32_t i, j;
    	/*在初始化前，加入适量延时，待OLED供电稳定*/
	for (i = 0; i < 1000; i ++)
	{
		for (j = 0; j < 1000; j ++);
	}
    
    GPIO_InitTypeDef GPIO_InitStructure;
    I2C_InitTypeDef I2C_InitStructure;
    
    // 使能I2C和GPIO时钟
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
    
    GPIO_PinAFConfig(GPIOB,GPIO_PinSource6,GPIO_AF_I2C1);    //SCL引脚复用
    GPIO_PinAFConfig(GPIOB,GPIO_PinSource7,GPIO_AF_I2C1);    //SDA引脚复用
    
    // 配置I2C引脚: SCL-PB6, SDA-PB7
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF; 
    GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    
    // I2C配置
    I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
    I2C_InitStructure.I2C_OwnAddress1 = 0x00;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_InitStructure.I2C_ClockSpeed = 400000; // 400kHz
    
    I2C_Init(I2C1, &I2C_InitStructure);
    I2C_Cmd(I2C1, ENABLE);
}

// 产生I2C起始信号
void I2C_Start(void)
{
    int time =100000;
    I2C_GenerateSTART(I2C1, ENABLE);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT))
    {
        time --;
        if(time == 0)
        {
            printf("timeout\r\n");
        }
    }
}

// 产生I2C停止信号
void I2C_Stop(void)
{
    I2C_GenerateSTOP(I2C1, ENABLE);
}

// 发送一个字节
void I2C_SendByte(uint8_t data)
{
    int time = 100000;
    I2C_SendData(I2C1, data);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
    {
        time --;
        if(time ==0 )
        {
            if(data == 0xFE)
            {
                printf("addr 收到，但是复位命令没有成功\r\n");
            }
            printf("sendByte timeout\r\n");
        }
    }
}

// 读取一个字节
uint8_t I2C_ReadByte(void)
{
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED));
    return I2C_ReceiveData(I2C1);
}

// 等待应答
uint8_t I2C_WaitAck(void)
{
    int time = 100000;
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
    {
        time --;
        if(time == 0)
        {
            printf("waitAck timeout\r\n");
        }
    }
    return I2C_GetFlagStatus(I2C1, I2C_FLAG_AF) == RESET;
}

// 发送应答
void I2C_Ack(void)
{
    I2C_AcknowledgeConfig(I2C1, ENABLE);
}

// 发送非应答
void I2C_NAck(void)
{
    I2C_AcknowledgeConfig(I2C1, DISABLE);
}
