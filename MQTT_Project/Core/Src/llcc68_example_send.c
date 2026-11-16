#include "LLCC68_example_send.h"
#include "radio.h"
#include "project_config.h"
#include "stdio.h"
#include "stm32f1xx_it.h"
#include "delay.h"
#include "string.h"

/*!
 * Radio events function pointer
 * 回调函数在中断处理流程中使用，因此定义为静态全局变量，避免局部变量生命周期结束后失效。
 */
static RadioEvents_t LLCC68RadioEvents;

const char *sendData = "1111111111111111111111111111111111111111";

static void LLCC68OnTxDone( void );
static void LLCC68OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr );
static void LLCC68OnTxTimeout( void );
static void LLCC68OnRxTimeout( void );
static void LLCC68OnRxError( void );

static void LLCC68SendReconfig(void){
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
					 true, 0, 0, LORA_IQ_INVERSION_ON, 3000 ); // LoRa 发射配置：功率、带宽、扩频因子、编码率、前导码、固定长度、CRC、跳频、IQ 反转和超时时间
	printf("LLCC68SendReconfig\r\n");
}

// 初始化 LoRa 发射示例并启动发送。
void ExampleLLCC68SendDemo(void){
	uint8_t OCP_Value = 0;
//	uint16_t count = 0;
	printf("start %s() example\r\n",__func__);
	
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
					 true, 0, 0, LORA_IQ_INVERSION_ON, 3000 ); // LoRa 发射配置：功率、带宽、扩频因子、编码率、前导码、固定长度、CRC、跳频、IQ 反转和超时时间

	OCP_Value = Radio.Read(REG_OCP);
	printf("[%s()-%d]read OCP register value:0x%04X\r\n",__func__,__LINE__,OCP_Value);
	
	printf("all setting\r\n");
	printf("freq: %d\r\n Tx power: %d\r\n band width: %d\r\n FS: %d\r\n CODINGRATE: %d\r\n PREAMBLE_LENGTH: %d\r\n",LORA_FRE,LORA_TX_OUTPUT_POWER,LORA_BANDWIDTH,LORA_SPREADING_FACTOR,LORA_CODINGRATE,LORA_PREAMBLE_LENGTH);
	
	Radio.Send((uint8_t *)sendData,(strlen(sendData)+1));
	while(1){
		Radio.IrqProcess(); // Process Radio IRQ
		delay_ms(1);
	}
}

static void LLCC68OnTxDone( void )
{		
	printf("TXDone\r\n");
	Radio.Standby();
	Radio.Send((uint8_t *)sendData,strlen(sendData));
	//GPIO_SetBits(GREEN_LED_PORT,GREEN_LED_PIN);
	//HAL_GPIO_WritePin(GREEN_LED_PORT, GREEN_LED_PIN, GPIO_PIN_SET);
	delay_ms(100);
	//GPIO_ResetBits(GREEN_LED_PORT,GREEN_LED_PIN);
	//HAL_GPIO_WritePin(GREEN_LED_PORT, GREEN_LED_PIN, GPIO_PIN_RESET);
}

static void LLCC68OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr )
{
	Radio.Standby();
	printf("RxDone\r\nsize:%d\r\nrssi:%d\r\nsnr:%d\r\n",size,rssi,snr);
}

static void LLCC68OnTxTimeout( void )
{
	printf("TxTimeout\r\n");
	LLCC68SendReconfig();
	Radio.Send((uint8_t *)sendData,(strlen(sendData)+1));
}

static void LLCC68OnRxTimeout( void )
{
	Radio.Standby();
	printf("RxTimeout retry recive\r\n");
	Radio.Rx( LORA_RX_TIMEOUT_VALUE ); 
}

static void LLCC68OnRxError( void )
{
	Radio.Standby();
	printf("RxError retry recive\r\n");
	Radio.Rx(LORA_RX_TIMEOUT_VALUE); 
}
