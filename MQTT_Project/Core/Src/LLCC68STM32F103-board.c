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
#include "tim.h"

extern TIM_HandleTypeDef htim3;
extern volatile uint32_t g_dio1_cnt = 0;

typedef struct __softTimer_s
{
	uint8_t isRun;	  // 定时器运行标志：0 表示停止，1 表示运行
	uint32_t delayMs; // 定时器延时时长，单位 ms
	uint32_t startMs; // 定时器启动时的计数值
} SoftTimer_t;

static DioIrqHandler *dio1IrqCallback = NULL; // 保存 DIO1 的回调函数
static uint32_t timer_count = 0;
static SoftTimer_t txTimerHandle, rxTimerHandle;

/* 初始化 LLCC68 使用的 GPIO；BUSY 引脚由 CubeMX 配置为输入模式。
 */
void LLCC68IoInit(void)
{
	//	uint8_t u8_ret=255;
	//	GPIO_InitTypeDef  GPIO_InitStructure;

	// BUSY 引脚配置为输入模式。
	//	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_GPIOB, ENABLE);	    //使能指定 GPIO 端口时钟
	//	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	 //设置 GPIO 速度为 50 MHz
	//	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_IPU;//配置为上拉输入
	//	GPIO_InitStructure.GPIO_Pin = RADIO_DIO4_BUSY_PIN;
	//	GPIO_Init(RADIO_DIO4_BUSY_PORT, &GPIO_InitStructure);	//初始化 BUSY 引脚

	//	//以下引脚当前未使用，按需要配置为浮空输入。
	//	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_IN_FLOATING;//配置为浮空输入
	//
	//	GPIO_InitStructure.GPIO_Pin = RADIO_DIO0_TXEN_PIN;
	//	GPIO_Init(RADIO_DIO0_TXEN_PORT, &GPIO_InitStructure);	//初始化 TXEN 引脚

	//	GPIO_InitStructure.GPIO_Pin = RADIO_DIO2_PIN;
	//	GPIO_Init(RADIO_DIO2_PORT, &GPIO_InitStructure);	//初始化 DIO2 引脚
	//
	//	GPIO_InitStructure.GPIO_Pin = RADIO_DIO3_PIN;
	//	GPIO_Init(RADIO_DIO3_PORT, &GPIO_InitStructure);	//初始化 DIO3 引脚
	//
	//	GPIO_InitStructure.GPIO_Pin = RADIO_DIO5_RXEN_PIN;
	//	GPIO_Init(RADIO_DIO5_RXEN_PORT, &GPIO_InitStructure);	//初始化 RXEN 引脚

	//	u8_ret=Spi1Init();
	//	if(u8_ret){
	//		printf("spi1 init error\r\n");
	//	}else{
	//		printf("LLCC68IoInit init sucess\r\n");
	//	}
}

/* 初始化 DIO1 中断回调。
 * DIO1 配置为外部中断输入；dioIrq 通常指向 RadioOnDioIrq(void *context)。
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

// void LLCC68IoIrqInit( DioIrqHandler dioIrq )

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	g_dio1_cnt++;
	if (GPIO_Pin ==  LORA_DIO1_Pin)
	{
		if (NULL != dio1IrqCallback)
		{
			
			dio1IrqCallback(NULL);
		}
		// EXTI_ClearITPendingBit(EXTI15_10_IRQn);
	}
}

// void EXTI1_IRQHandler(void)
// {
// 	if (HAL_GPIO_ReadPin(RADIO_DIO1_PORT, LORA_DIO1_Pin))
// 	{
// 		printf("DIO1 irq\r\n");
// 		if (NULL != dio1IrqCallback)
// 		{
// 			dio1IrqCallback(NULL);
// 		}
// 		EXTI_ClearITPendingBit(EXTI15_10_IRQn);
// 	}
// }

void LLCC68IoDeInit(void)
{
	// 当前无需执行 GPIO 反初始化。
}

// 初始化 LoRa 复位引脚并执行复位时序。
void LLCC68ResetInit(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	LLCC68DelayMs(10);

	GPIO_InitStruct.Pin = LORA_RESET_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(LORA_RESET_GPIO_Port, &GPIO_InitStruct);

	/* 真正的复位脉冲：拉低 20ms，再拉高 20ms */
    HAL_GPIO_WritePin(LORA_RESET_GPIO_Port, LORA_RESET_Pin, GPIO_PIN_RESET);
    LLCC68DelayMs(20);
    HAL_GPIO_WritePin(LORA_RESET_GPIO_Port, LORA_RESET_Pin, GPIO_PIN_SET);
    LLCC68DelayMs(20);
}

// 等待 BUSY 引脚变为低电平。
void LLCC68WaitOnBusy(void)
{
	uint32_t u32_count = 0;

	while (HAL_GPIO_ReadPin(LORA_BUSY_GPIO_Port, LORA_BUSY_Pin) == 1)
	{
		if (u32_count++ > 1000)
		{
			// printf("wait busy pin timeout\r\n");
			u32_count = 0;
		}
		LLCC68DelayMs(1);
	}
}

// 设置 SPI 片选电平：0 为低电平，非 0 为高电平。
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

// 通过 SPI 收发一个字节。
uint8_t LLCC68SpiInOut(uint8_t data)
{
	uint8_t recvdata = 0;
	HAL_SPI_TransmitReceive(&hspi1, &data, &recvdata, 1, 10);
	return recvdata;
}

// 检查是否支持指定的射频频率。
bool LLCC68CheckRfFrequency(uint32_t frequency)
{
	// Implement check. Currently all frequencies are supported
	return true;
}

// 毫秒延时。
void LLCC68DelayMs(uint32_t ms)
{
	delay_ms(ms);
}

// TX/RX 超时定时器。

// 定时器中断回调。
// TX 超时时调用 RadioOnTxTimeoutIrq。
// RX 超时时调用 RadioOnRxTimeoutIrq。
// void TIM3_IRQHandler(void)   //TIM3 中断处理函数
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if (htim->Instance != TIM3)
		return;

	timer_count++;

	uint32_t diffMs = 0;

	// 处理 TX 超时。
	if (txTimerHandle.isRun)
	{
		if (timer_count >= txTimerHandle.startMs)
		{
			diffMs = timer_count - txTimerHandle.startMs;
		}
		else
		{ // 计数器发生回绕。
			diffMs = 0xffffffff - txTimerHandle.startMs + timer_count;
		}
		if (diffMs > txTimerHandle.delayMs)
		{
			// 定时器超时。
			LLCC68TxTimerStop();
			txTimerHandle.startMs = timer_count;
			RadioOnTxTimeoutIrq(NULL); // 调用 TX 超时回调
		}
	}

	// 处理 RX 超时。
	if (rxTimerHandle.isRun)
	{
		if (timer_count >= rxTimerHandle.startMs)
		{
			diffMs = timer_count - rxTimerHandle.startMs;
		}
		else
		{ // 计数器发生回绕。
			diffMs = 0xffffffff - rxTimerHandle.startMs + timer_count;
		}
		if (diffMs > rxTimerHandle.delayMs)
		{
			// 定时器超时。
			LLCC68RxTimerStop();
			rxTimerHandle.startMs = timer_count;
			RadioOnRxTimeoutIrq(NULL); // 调用 RX 超时回调
		}
	}
	//}
}

// 初始化基于 TIM4 的毫秒定时器。
// TX/RX 超时时调用对应的 Radio 回调。
void LLCC68TimerInit(void)
{
	//	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	//	NVIC_InitTypeDef NVIC_InitStructure;

	//	//启用 TIM3 时钟。
	//	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE); //使能 TIM3 外设时钟
	//
	//	//TIM_ITConfig(TIM3,TIM_IT_Update,ENABLE ); //使能 TIM3 更新中断

	//	//配置 NVIC 中断优先级。
	//	NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;  //选择 TIM3 中断通道
	// TIM_ClearITPendingBit(TIM3, TIM_IT_Update); // 清除 TIM3 更新中断标志（旧版示例，当前未启用）
	//	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;  //子优先级 3
	//	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; //使能 IRQ 通道
	//	NVIC_Init(&NVIC_InitStructure);  //初始化 NVIC
	//
	//	//初始化 TIM3 定时器。
	//	TIM_TimeBaseStructure.TIM_Period = 10; //设置自动重装载值
	//	TIM_TimeBaseStructure.TIM_Prescaler =SystemCoreClock/10000; //设置预分频系数
	//	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; //设置时钟分频
	//	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //向上计数模式
	//	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure); //初始化 TIM3

	// 初始化时关闭 TX/RX 超时定时器。
	txTimerHandle.isRun = 0;
	rxTimerHandle.isRun = 0;
	timer_count = 0;

	HAL_TIM_Base_Start_IT(&htim3);
	//	TIM_Cmd(TIM3, ENABLE);  //使能 TIM3
	//	TIM_ClearITPendingBit(TIM3, TIM_IT_Update  );  //清除 TIM3 更新中断标志
	//	TIM_ITConfig(TIM3,TIM_IT_Update,ENABLE ); //使能定时器更新中断
}

void LLCC68SetTxTimerValue(uint32_t nMs)
{
	// printf("[%s()-%d]set timer out %d ms\r\n", __func__, __LINE__, nMs);
	txTimerHandle.delayMs = nMs;
}

void LLCC68TxTimerStart(void)
{
	// printf("[%s()-%d]start timer\r\n", __func__, __LINE__);
	txTimerHandle.startMs = timer_count;
	txTimerHandle.isRun = 1;
}

void LLCC68TxTimerStop(void)
{
	// printf("[%s()-%d]stop timer\r\n", __func__, __LINE__);
	txTimerHandle.isRun = 0;
}

void LLCC68SetRxTimerValue(uint32_t nMs)
{
	// printf("[%s()-%d]set timer out %d ms\r\n", __func__, __LINE__, nMs);
	rxTimerHandle.delayMs = nMs;
}

void LLCC68RxTimerStart(void)
{
	// printf("[%s()-%d]start timer\r\n", __func__, __LINE__);
	rxTimerHandle.startMs = timer_count;
	rxTimerHandle.isRun = 1;
}

void LLCC68RxTimerStop(void)
{
	// printf("[%s()-%d]stop timer\r\n", __func__, __LINE__);
	rxTimerHandle.isRun = 0;
}
