/*!
 * \file      sx1262mbxcas-board.c
 *
 * \brief     Target board SX1262MBXCAS shield driver implementation
 *
 * \copyright Revised BSD License, see section \ref LICENSE.
 *
 * \code
 *                ______                              _
 *               / _____)             _              | |
 *              ( (____  _____ ____ _| |_ _____  ____| |__
 *               \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 *               _____) ) ____| | | || |_| ____( (___| | | |
 *              (______/|_____)_|_|_| \__)_____)\____)_| |_|
 *              (C)2013-2017 Semtech
 *
 * \endcode
 *
 * \author    Miguel Luis ( Semtech )
 *
 * \author    Gregory Cristian ( Semtech )
 */
#include <stdlib.h>
#include "project_config.h"
#include "delay.h"
#include "radio.h"
#include "LLCC68-board.h"
#include "spi.h"
#include "stdio.h"
#include "gpio.h"

extern TIM_HandleTypeDef htim4;

typedef struct __softTimer_s
{
	uint8_t isRun;	  // 0停止，1运行
	uint32_t delayMs; // 定时器需要多长时间后唤醒
	uint32_t startMs; // 定时器开始时间
} SoftTimer_t;

/* 保存 Radio 层传入的 DIO1 处理回调，由 HAL 外部中断回调转发。 */
/* [修改] 保持原写法：DioIrqHandler 是函数类型，* 表示函数指针 */
static DioIrqHandler *dio1IrqCallback = NULL;
/* 软件定时器共用的计数基准；当前计数递增语句尚未启用。 */
static uint32_t timer_count = 0;
/* TX 和 RX 分别保存启用状态、超时时长及计时起点。 */
static SoftTimer_t txTimerHandle, rxTimerHandle;

/* 初始化LLCC68需要用到的GPIO初始化，将 BUSY 引脚设置为输入模式
 */
void LLCC68IoInit(void)
{
	//	uint8_t u8_ret=255;
	//	GPIO_InitTypeDef  GPIO_InitStructure;

	// 初始化BUSY为上拉输入模式
	//	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_GPIOB, ENABLE);	    //使能指定端口时钟
	//	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	 //IO口速度为50MHz，这里不用传参，直接写死用最大速度50MHZ
	//	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_IPU;//上拉输入
	//	GPIO_InitStructure.GPIO_Pin = RADIO_DIO4_BUSY_PIN;
	//	GPIO_Init(RADIO_DIO4_BUSY_PORT, &GPIO_InitStructure);	//初始化GPIO

	//	//下面这几个引脚没用用到，设置为浮空输入模式
	//	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_IN_FLOATING;//浮空输入
	//
	//	GPIO_InitStructure.GPIO_Pin = RADIO_DIO0_TXEN_PIN;
	//	GPIO_Init(RADIO_DIO0_TXEN_PORT, &GPIO_InitStructure);	//初始化GPIO

	//	GPIO_InitStructure.GPIO_Pin = RADIO_DIO2_PIN;
	//	GPIO_Init(RADIO_DIO2_PORT, &GPIO_InitStructure);	//初始化GPIO
	//
	//	GPIO_InitStructure.GPIO_Pin = RADIO_DIO3_PIN;
	//	GPIO_Init(RADIO_DIO3_PORT, &GPIO_InitStructure);	//初始化GPIO
	//
	//	GPIO_InitStructure.GPIO_Pin = RADIO_DIO5_RXEN_PIN;
	//	GPIO_Init(RADIO_DIO5_RXEN_PORT, &GPIO_InitStructure);	//初始化GPIO

	//	u8_ret=Spi1Init();
	//	if(u8_ret){
	//		printf("spi1 init error\r\n");
	//	}else{
	//		printf("LLCC68IoInit init sucess\r\n");
	//	}
}

/* 初始化 DIO1
 * 将DIO1设置为外部中断(上升沿触发)，并且回调函数为 dioIrq 函数原型 void RadioOnDioIrq( void* context )
 */
void LLCC68IoIrqInit(DioIrqHandler dioIrq)
{
	//	GPIO_InitTypeDef  GPIO_InitStructure;
	//	EXTI_InitTypeDef EXTI_InitStructure;
	// 	NVIC_InitTypeDef NVIC_InitStructure;

	dio1IrqCallback = dioIrq;

	//	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	//	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	//	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_IPD;
	//	GPIO_InitStructure.GPIO_Pin = RADIO_DIO1_PIN;
	//	GPIO_Init(RADIO_DIO1_PORT, &GPIO_InitStructure);
	//
	//  RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO,ENABLE);

	//  GPIO_EXTILineConfig(GPIO_PortSourceGPIOB,GPIO_PinSource1);
	//	EXTI_InitStructure.EXTI_Line=EXTI_Line1;
	//	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	//	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
	//	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	//	EXTI_Init(&EXTI_InitStructure);

	//	//Priority config
	//	NVIC_InitStructure.NVIC_IRQChannel = EXTI1_IRQn;
	//	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x02;
	//	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x03;
	//	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	//	NVIC_Init(&NVIC_InitStructure);
	//	EXTI_ClearITPendingBit(EXTI_Line11);
}

// static DioIrqHandler dio1IrqCallback = NULL;
// void LLCC68IoIrqInit( DioIrqHandler dioIrq )
/* HAL 外部中断回调：确认 DIO1 为高电平后转发无线事件。 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	/* [修改] 用 EXTI 传入的引脚号判断，不再读电平；并去掉中断里的 printf */
	if (GPIO_Pin == LORA_DIO1_Pin)
	{
		if (NULL != dio1IrqCallback)
		{
			dio1IrqCallback(NULL);
		}
	}
}

// void EXTI1_IRQHandler(void){
//	if(HAL_GPIO_ReadPin(RADIO_DIO1_PORT,RADIO_DIO1_PIN)){
//		printf("DIO1 irq\r\n");
//		if(NULL!=dio1IrqCallback){
//			dio1IrqCallback(NULL);
//		}
//		EXTI_ClearITPendingBit(EXTI_Line1);
//	}
// }

/* 反初始化无线模块 I/O；当前板级实现没有额外释放操作。 */
void LLCC68IoDeInit(void)
{
	// GPIO去初始化代码
}

// 配置Lora复位引脚
/* 将复位脚短暂配置为输出再释放为输入，完成硬件复位。 */
void LLCC68ResetInit(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	GPIO_InitStruct.Pin = LORA_RESET_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(LORA_RESET_GPIO_Port, &GPIO_InitStruct);

	/* [修改] 使用 HAL 的 GPIO_PIN_RESET/SET，并显式产生复位低脉冲 */
	HAL_GPIO_WritePin(LORA_RESET_GPIO_Port, LORA_RESET_Pin, GPIO_PIN_RESET);
	LLCC68DelayMs(20);
	HAL_GPIO_WritePin(LORA_RESET_GPIO_Port, LORA_RESET_Pin, GPIO_PIN_SET);
	LLCC68DelayMs(20);
}

// 读取Busy引脚电平状态
/* 阻塞等待 BUSY 拉低；高电平期间芯片尚未准备好接收命令。 */
void LLCC68WaitOnBusy(void)
{
	uint32_t u32_count = 0;

	while (HAL_GPIO_ReadPin(LORA_BUSY_GPIO_Port, LORA_BUSY_Pin) == 1)
	{
		if (u32_count++ > 1000)
		{
			printf("wait busy pin timeout\r\n");
			/* [修改] 原代码只清零继续等，这里 break，避免 BUSY 一直高时永久卡死 */
			break;
		}
		LLCC68DelayMs(1);
	}
}

// 设置SPI的NSS引脚电平，0低电平，非零高电平
/* 控制 SPI 片选：0 拉低选中芯片，非零拉高结束事务。 */
void LLCC68SetNss(uint8_t lev)
{
	if (lev)
	{
		HAL_GPIO_WritePin(LORA_CSS_GPIO_Port, LORA_CSS_Pin, GPIO_PIN_SET); // 输出高电平
	}
	else
	{
		HAL_GPIO_WritePin(LORA_CSS_GPIO_Port, LORA_CSS_Pin, GPIO_PIN_RESET); // 输出低电平
	}
}

// spi传输一个字节的数据
/* 同步发送一个 SPI 字节，并返回该字节传输期间收到的数据。 */
uint8_t LLCC68SpiInOut(uint8_t data)
{
	uint8_t recvdata = 0;
	HAL_SPI_TransmitReceive(&hspi1, &data, &recvdata, 1, 10);
	return recvdata;
}

// 检查频率是否符合要求，如果不需要判断则可以直接返回true
/* 检查频率是否受板级硬件支持；当前实现对所有频率均返回支持。 */
bool LLCC68CheckRfFrequency(uint32_t frequency)
{
	// Implement check. Currently all frequencies are supported
	return true;
}

// 毫秒延时
/* 提供驱动所需的毫秒级阻塞延时。 */
void LLCC68DelayMs(uint32_t ms)
{
	delay_ms(ms);
}

// tx/rx定时器操作

// 定时器3中断服务程序
// tx定时结束需要回调 RadioOnTxTimeoutIrq
// rx定时结束需要回调 RadioOnRxTimeoutIrq
// void TIM3_IRQHandler(void)   //TIM3中断
/* TIM 周期中断回调：检查 TX/RX 软件超时并转发对应事件。
 * 当前 timer_count 的递增代码被注释掉，因此超时计时不会正常推进。
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	uint32_t diffMs = 0;

	//	if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET){
	//		timer_count++;
	//		TIM_ClearITPendingBit(TIM3, TIM_IT_Update  );  //清除TIMx更新中断标志

	// printf("timer_count=%d\r\n",timer_count);

	// 处理tx定时器
	if (txTimerHandle.isRun)
	{
		if (timer_count >= txTimerHandle.startMs)
		{
			diffMs = timer_count - txTimerHandle.startMs;
		}
		else
		{ // 溢出了
			diffMs = 0xffffffff - txTimerHandle.startMs + timer_count;
		}
		if (diffMs > txTimerHandle.delayMs)
		{
			// 计时结束
			// printf("----------------- tx timer irq ---------------\r\n");
			LLCC68TxTimerStop();
			txTimerHandle.startMs = timer_count;
			RadioOnTxTimeoutIrq(NULL); // tx定时结束需要回调 RadioOnTxTimeoutIrq
		}
	}

	// 处理rx定时器
	if (rxTimerHandle.isRun)
	{
		if (timer_count >= rxTimerHandle.startMs)
		{
			diffMs = timer_count - rxTimerHandle.startMs;
		}
		else
		{ // 溢出了
			diffMs = 0xffffffff - rxTimerHandle.startMs + timer_count;
		}
		if (diffMs > rxTimerHandle.delayMs)
		{
			// 计时结束
			// printf("----------------- rx timer irq ---------------\r\n");
			LLCC68RxTimerStop();
			rxTimerHandle.startMs = timer_count;
			RadioOnRxTimeoutIrq(NULL); // rx定时结束需要回调 RadioOnRxTimeoutIrq
		}
	}
	//}
}

// 初始化定时器(这里用TIM4定时器模拟出了两个ms定时器)
// tx定时结束需要回调 RadioOnTxTimeoutIrq
// rx定时结束需要回调 RadioOnRxTimeoutIrq
/* 清除 TX/RX 软件定时器状态，并启动 TIM4 中断。 */
void LLCC68TimerInit(void)
{
	//	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	//	NVIC_InitTypeDef NVIC_InitStructure;

	//	//使能TIM3
	//	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE); //时钟使能
	//
	//	//TIM_ITConfig(TIM3,TIM_IT_Update,ENABLE ); //使能指定的TIM3中断,允许更新中断

	//	//中断优先级NVIC设置
	//	NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;  //TIM3中断
	//	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;  //先占优先级0级
	//	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;  //从优先级3级
	//	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; //IRQ通道被使能
	//	NVIC_Init(&NVIC_InitStructure);  //初始化NVIC寄存器
	//
	//	//定时器TIM3初始化
	//	TIM_TimeBaseStructure.TIM_Period = 10; //设置在下一个更新事件装入活动的自动重装载寄存器周期的值
	//	TIM_TimeBaseStructure.TIM_Prescaler =SystemCoreClock/10000; //设置用来作为TIMx时钟频率除数的预分频值(1s10000次，也就是1次0.1ms)
	//	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; //设置时钟分割:TDTS = Tck_tim
	//	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM向上计数模式
	//	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure); //根据指定的参数初始化TIMx的时间基数单位

	// 关闭软件定时器
	txTimerHandle.isRun = 0;
	rxTimerHandle.isRun = 0;

	HAL_TIM_Base_Start_IT(&htim3);

	//	TIM_Cmd(TIM3, ENABLE);  //使能TIMx
	//	TIM_ClearITPendingBit(TIM3, TIM_IT_Update  );  //清除TIMx更新中断标志
	//	TIM_ITConfig(TIM3,TIM_IT_Update,ENABLE ); //使能指定的TIM2中断,允许更新中断
}

/* 设置 TX 超时时长，单位为毫秒。 */
void LLCC68SetTxTimerValue(uint32_t nMs)
{
	// printf("[%s()-%d]set timer out %d ms\r\n", __func__, __LINE__, nMs);
	txTimerHandle.delayMs = nMs;
}

/* 记录当前计数作为 TX 起点，并启动 TX 软件定时器。 */
void LLCC68TxTimerStart(void)
{
	// printf("[%s()-%d]start timer\r\n", __func__, __LINE__);
	txTimerHandle.startMs = timer_count;
	txTimerHandle.isRun = 1;
}

/* 停止 TX 软件定时器。 */
void LLCC68TxTimerStop(void)
{
	// printf("[%s()-%d]stop timer\r\n", __func__, __LINE__);
	txTimerHandle.isRun = 0;
}

/* 设置 RX 超时时长，单位为毫秒。 */
void LLCC68SetRxTimerValue(uint32_t nMs)
{
	// printf("[%s()-%d]set timer out %d ms\r\n", __func__, __LINE__, nMs);
	rxTimerHandle.delayMs = nMs;
}

/* 记录当前计数作为 RX 起点，并启动 RX 软件定时器。 */
void LLCC68RxTimerStart(void)
{
	// printf("[%s()-%d]start timer\r\n", __func__, __LINE__);
	rxTimerHandle.startMs = timer_count;
	rxTimerHandle.isRun = 1;
}

/* 停止 RX 软件定时器。 */
void LLCC68RxTimerStop(void)
{
	// printf("[%s()-%d]stop timer\r\n", __func__, __LINE__);
	rxTimerHandle.isRun = 0;
}
