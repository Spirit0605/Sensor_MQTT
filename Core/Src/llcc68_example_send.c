#include "LLCC68_example_send.h"
#include "radio.h"
#include "project_config.h"
#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

/* [修改] Radio.Init 只做一次，发送时不再重复初始化 */
static RadioEvents_t LLCC68RadioEvents;
static uint8_t lora_inited = 0;
static volatile uint8_t tx_done = 0;
static volatile uint8_t tx_timeout = 0;

/* [修改] ISR/任务共享变量 */
extern volatile bool IrqFired;
extern uint16_t LLCC68GetIrqStatus(void);

static void LLCC68OnTxDone( void );
static void LLCC68OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr );
static void LLCC68OnTxTimeout( void );
static void LLCC68OnRxTimeout( void );
static void LLCC68OnRxError( void );

/* [修改] 只初始化一次，不再每次发送都 Radio.Init */
void LoRa_Init(void)
{
	if (lora_inited)
	{
		return;
	}

	LLCC68RadioEvents.TxDone = LLCC68OnTxDone;
	LLCC68RadioEvents.RxDone = LLCC68OnRxDone;
	LLCC68RadioEvents.TxTimeout = LLCC68OnTxTimeout;
	LLCC68RadioEvents.RxTimeout = LLCC68OnRxTimeout;
	LLCC68RadioEvents.RxError = LLCC68OnRxError;

	Radio.Init( &LLCC68RadioEvents );
	Radio.SetChannel(LORA_FRE);
	Radio.SetTxConfig( MODEM_LORA, LORA_TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                     LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                     LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                     true, 0, 0, LORA_IQ_INVERSION_ON, 3000 );

	lora_inited = 1;
}

/* [修改] 发送一包并等待 TX Done，超时返回 false */
bool LoRa_Send(const char *data, uint32_t timeout_ms)
{
	TickType_t start;

	LoRa_Init();

	tx_done = 0;
	tx_timeout = 0;

	Radio.Send( (uint8_t *)data, (uint8_t)strlen(data) );

	start = xTaskGetTickCount();

	while (!tx_done && !tx_timeout)
	{
		/* [修改] DIO1 万一没触发时的兜底；DIO1 正常可以删掉 */
		if (LLCC68GetIrqStatus() != 0U)
		{
			IrqFired = true;
		}

		Radio.IrqProcess();

		if ((xTaskGetTickCount() - start) >= pdMS_TO_TICKS(timeout_ms))
		{
			break;
		}

		vTaskDelay(pdMS_TO_TICKS(5));
	}

	if (tx_timeout)
	{
		/* [修改] 超时后回到 standby；在任务上下文做 SPI */
		Radio.Standby();
	}

	return tx_done != 0;
}

/* [修改] 原接口保留，内部走 LoRa_Send */
void ExampleLLCC68SendDemo(const char * sendData)
{
	(void)LoRa_Send(sendData, 1000);
}

/* [修改] TX Done：只置标志，不在回调里重发或 delay */
static void LLCC68OnTxDone( void )
{
	Radio.Standby();
	tx_done = 1;
}

/* [修改] TX 超时：只置标志，中断上下文不做 SPI */
static void LLCC68OnTxTimeout( void )
{
	tx_timeout = 1;
}

static void LLCC68OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr )
{
	(void)payload;
	(void)size;
	(void)rssi;
	(void)snr;
}

static void LLCC68OnRxTimeout( void )
{
}

static void LLCC68OnRxError( void )
{
}
