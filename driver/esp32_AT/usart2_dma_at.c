#include "usart2_dma_at.h"
#include <string.h>
#include <stdio.h>


uint8_t UART_RX_BUF[UART_RX_BUF_SIZE]; // DMA 接收原始缓冲区
uint8_t MAIN_RX_BUF[UART_RX_BUF_SIZE]; // 解析用的转存缓冲区
volatile uint16_t rx_len = 0;          // 接收到的数据长度
volatile uint8_t rx_flag = 0;         // 接收完成标志位

// USART2 + DMA 初始化函数，开启了空闲中断和DMA传输完成中断，DMA负责数据接收
void USART2_DMA_Config(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    DMA_InitTypeDef DMA_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    // 1. 开启时钟
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);  // GPIOA 时钟
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1, ENABLE);   // DMA1 时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE); // USART2 时钟

    // 2. GPIO 配置 (PA2->TX, PA3->RX)
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_USART2);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource3, GPIO_AF_USART2);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;      // 复用模式
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // 3. USART2 参数配置
    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART2, &USART_InitStructure);

    // 4. DMA1_Stream5_Channel4 配置 (USART2_RX)
    DMA_DeInit(DMA1_Stream5);
    DMA_Cmd(DMA1_Stream5, DISABLE);
    while(DMA_GetCmdStatus(DMA1_Stream5) != DISABLE);
    
    DMA_InitStructure.DMA_Channel = DMA_Channel_4; 
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&(USART2->DR); // 外设地址
    DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)UART_RX_BUF;    // 存储器1地址
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;           // 外设到内存
    DMA_InitStructure.DMA_BufferSize = UART_RX_BUF_SIZE;              // 缓冲区大小
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;  // 外设地址不增
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;           // 内存地址自增
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;                   // 普通模式
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;
    DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
    //下面是无用配置
    DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_1QuarterFull;
    DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
    DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;

    DMA_Init(DMA1_Stream5, &DMA_InitStructure);


    // 5. 开启串口空闲中断、DMA 传输完成中断
    USART_ITConfig(USART2, USART_IT_IDLE, ENABLE); // 开启 IDLE 中断
    DMA_ITConfig(DMA1_Stream5, DMA_IT_TC, ENABLE); // 开启 DMA 传输完成中断
    

    // 6. NVIC 配置 (增加 DMA 中断通道)
    
    // 串口中断优先级
    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    // DMA 中断优先级
    NVIC_InitStructure.NVIC_IRQChannel = DMA1_Stream5_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    // 7.启动
    USART_DMACmd(USART2, USART_DMAReq_Rx, ENABLE);// 启用 USART2 DMA 接收
    DMA_Cmd(DMA1_Stream5, ENABLE);
    USART_Cmd(USART2, ENABLE);
}

//在IDLE中断中处理接收到的数据，如果波特率太高，可能会丢数据，因为还没有处理完数据，新的数据又来了。所以可以开启双缓冲区。
void USART2_IRQHandler(void) {

    
    // 检查是否为 IDLE 中断
    if (USART_GetITStatus(USART2, USART_IT_IDLE) != RESET) {
        
        /* 清除 IDLE 标志位：先读 SR，再读 DR */
        uint8_t temp;
        temp = USART2->SR;
        temp = USART2->DR; 
        (void)temp; // <--- 关键：这行代码告诉编译器，temp 被“用过”了，警告消失

        // 1. 停止 DMA 传输
        DMA_Cmd(DMA1_Stream5, DISABLE);//防止数据拷贝过程中有新数据进来，虽然概率很小
        while(DMA_GetCmdStatus(DMA1_Stream5) != DISABLE); // 等待停止

        // 2. 计算接收到的数据长度
        // 长度 = 总大小 - 剩余传输计数值
        rx_len = UART_RX_BUF_SIZE - DMA_GetCurrDataCounter(DMA1_Stream5);

        // 3. 数据处理 (拷贝到主缓冲区)
        if (rx_len > 0) {
            memcpy(MAIN_RX_BUF, UART_RX_BUF, rx_len);
            MAIN_RX_BUF[rx_len] = '\0'; // 字符串结束符
            //一旦在这里printf了，我的主程序接收就收到阻塞，最后wait就很容易超时。一定不要printf。
            //printf("rxlen:%d\r\n",rx_len);
            if(rx_flag == 1)
            {
                printf("\r\n[WARNING]: Previous data not processed yet!\r\n");
            }
            rx_flag = 1;               // 置标志位
        }

        // 4. 重置 DMA 计数器并重新开启
        DMA_SetCurrDataCounter(DMA1_Stream5, UART_RX_BUF_SIZE);//重启DMA可以重置写指针
        DMA1->HIFCR = (uint32_t)0XF40;;//清除DMA2_stream5的数据流标志位。
        //防止前面DMA_Cmd(DMA1_Stream5, DISABLE);（EN=0）触发TCIF5中断标志位没有清除，导致误触发DMA中断·
        //前提还是DMA的中断不能抢断我的USART中断。
        DMA_Cmd(DMA1_Stream5, ENABLE);
    }
}



void DMA1_Stream5_IRQHandler(void) {
    if (DMA_GetITStatus(DMA1_Stream5, DMA_IT_TCIF5) != RESET) {
        DMA_ClearITPendingBit(DMA1_Stream5, DMA_IT_TCIF5);

        // 循环模式下，缓冲区满时会自动从头开始覆盖
        // 这里只做警告提示：说明数据处理速度太慢，IDLE中断未及时取走数据
        printf("\r\n DMA full\r\n"); 
    }
}

//如果CPU处理缓冲区的速度太慢，导致下一帧数据传输完毕，但是上一帧还没有来得及处理，那么就会出现数据丢失
