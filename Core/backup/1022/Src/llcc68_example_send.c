#include "LLCC68_example_send.h"
#include "radio.h"
#include "project_config.h"
#include "stdio.h"
#include "stm32f1xx_it.h"
#include "delay.h"
#include "string.h"
#include "main.h"

/* Radio.Init 会保存此回调表的地址，因此回调表的生命周期必须长于本函数。 */
static RadioEvents_t LLCC68RadioEvents;

/* 示例发送载荷；无线驱动会在发送过程中读取该缓冲区。 */
// extern char sendData[128];

static void LLCC68OnTxDone( void );
static void LLCC68OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr );
static void LLCC68OnTxTimeout( void );
static void LLCC68OnRxTimeout( void );
static void LLCC68OnRxError( void );

/* 发送超时后重新注册回调并恢复 LoRa 发射配置。 */
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
					 true, 0, 0, LORA_IQ_INVERSION_ON, 3000 );
	printf("LLCC68SendReconfig\r\n");
}

/* 初始化无线模块并启动第一次异步发送。 */
void ExampleLLCC68SendDemo(const char * sendData){
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
					 true, 0, 0, LORA_IQ_INVERSION_ON, 3000 );

	OCP_Value = Radio.Read(REG_OCP);
	printf("[%s()-%d]read OCP register value:0x%04X\r\n",__func__,__LINE__,OCP_Value);
	
	printf("all setting\r\n");
	printf("freq: %d\r\n Tx power: %d\r\n band width: %d\r\n FS: %d\r\n CODINGRATE: %d\r\n PREAMBLE_LENGTH: %d\r\n",LORA_FRE,LORA_TX_OUTPUT_POWER,LORA_BANDWIDTH,LORA_SPREADING_FACTOR,LORA_CODINGRATE,LORA_PREAMBLE_LENGTH);
	
	Radio.Send((uint8_t *)sendData,(strlen(sendData)+1));
	// while(1){
		Radio.IrqProcess(); // 处理一次无线中断事件
		delay_ms(1);
	// }
}

/* 发射完成回调：切回待机并按示例逻辑提交下一包。 */
static void LLCC68OnTxDone( void )
{		
	printf("TXDone\r\n");
	Radio.Standby();
	/* 当前数据包发送完成后立即开始发送下一包。 */
	Radio.Send((uint8_t *)sendData,strlen(sendData));
	//GPIO_SetBits(GREEN_LED_PORT,GREEN_LED_PIN);
	//HAL_GPIO_WritePin(GREEN_LED_PORT, GREEN_LED_PIN, GPIO_PIN_SET);
	/* 此延时运行在无线发送完成回调中。 */
	delay_ms(100);
	//GPIO_ResetBits(GREEN_LED_PORT,GREEN_LED_PIN);
	//HAL_GPIO_WritePin(GREEN_LED_PORT, GREEN_LED_PIN, GPIO_PIN_RESET);
}

/* 接收完成回调：切回待机并输出包长、RSSI 和 SNR。 */
static void LLCC68OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr )
{
	Radio.Standby();
	printf("RxDone\r\nsize:%d\r\nrssi:%d\r\nsnr:%d\r\n",size,rssi,snr);
}

/* 发射超时回调：恢复无线配置并重试发送。 */
static void LLCC68OnTxTimeout( void )
{
	printf("TxTimeout\r\n");
	LLCC68SendReconfig();
	Radio.Send((uint8_t *)sendData,(strlen(sendData)+1));
}

/* 接收超时回调：退出当前接收并重新启动限时接收。 */
static void LLCC68OnRxTimeout( void )
{
	Radio.Standby();
	printf("RxTimeout retry recive\r\n");
	Radio.Rx( LORA_RX_TIMEOUT_VALUE ); 
}

/* 接收错误回调：退出当前接收并重新启动限时接收。 */
static void LLCC68OnRxError( void )
{
	Radio.Standby();
	printf("RxError retry recive\r\n");
	Radio.Rx(LORA_RX_TIMEOUT_VALUE); 
}
