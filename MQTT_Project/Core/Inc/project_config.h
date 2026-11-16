#ifndef __PROJECT_CONFIG_H__
#define __PROJECT_CONFIG_H__

// Semtech LoRa 驱动移植配置。
// 参考：https://github.com/Lora-net/LoRaMac-node/tree/master/src/radio，移植日期：2021-02-03。
#define SOFT_VERSION "LLCC68 driver for stm32f103 V0.0.0"

//--------------------------------------------- LoRa 默认参数 ---------------------------------------------
#define LORA_FRE 470500000               // 收发频率，单位 Hz
#define LORA_TX_OUTPUT_POWER 22          // 发射功率，单位 dBm；SX1262 支持 -9 至 22 dBm
#define LORA_BANDWIDTH 0                 // 带宽索引：[0: 125 kHz，1: 250 kHz，2: 500 kHz]
#define LORA_SPREADING_FACTOR 9          // 扩频因子，支持范围为 7 至 12
#define LORA_CODINGRATE 1                // 编码率索引：[1: 4/5, 2: 4/6, 3: 4/7, 4: 4/8]
#define LORA_PREAMBLE_LENGTH 8           // 前导码长度（符号数）
#define LORA_LLCC68_SYMBOL_TIMEOUT 0     // 符号超时；LLCC68 设置为 0，其他型号按驱动要求设置
#define LORA_FIX_LENGTH_PAYLOAD_ON false // 是否使用固定长度数据包；false 表示可变长度
#define LORA_IQ_INVERSION_ON false       // 是否反转 IQ；需与通信对端保持一致
#define LORA_RX_TIMEOUT_VALUE 5000       // 单次接收超时时间，单位 ms
#define TCXO_CTRL_VOLTAGE 2              // setting TCXO voltage supply source [0: 1.6v, 1: 1.7v, 2: 1.8v, 3: 2.2v, 4: 2.4v, 5: 2.7v, 6: 3.0v, 7: 3.5v]
#define RF_WAKEUP_TIME 100               // setting RF wakeup time,in second
#define LORA_DATA_SEND_OR_RECEIV_MODE 0  // 收发模式：1 为发送，0 为接收

/*!
 * Board MCU pins definitions
 */
// SPI
#define RADIO_NSS_PIN GPIO_Pin_4
#define RADIO_NSS_PORT GPIOA
#define RADIO_MOSI_PIN GPIO_Pin_7
#define RADIO_MOSI_PORT GPIOA
#define RADIO_MISO_PIN GPIO_Pin_6
#define RADIO_MISO_PORT GPIOA
#define RADIO_SCK_PIN GPIO_Pin_5
#define RADIO_SCK_PORT GPIOA
// 复位引脚
#define RADIO_nRESET_PIN GPIO_Pin_14
#define RADIO_nRESET_PORT GPIOB
// DIO1 中断引脚
#define RADIO_DIO1_PIN GPIO_Pin_10
#define RADIO_DIO1_PORT GPIOB
// BUSY 状态引脚
#define RADIO_DIO4_BUSY_PIN GPIO_Pin_0
#define RADIO_DIO4_BUSY_PORT GPIOA

// LoRa LED
#define GREEN_LED_PIN GPIO_Pin_8
#define GREEN_LED_PORT GPIOA

// 以下引脚当前未使用，预留给需要独立控制收发开关的硬件设计。
// TXEN
#define RADIO_DIO0_TXEN_PIN GPIO_Pin_1
#define RADIO_DIO0_TXEN_PORT GPIOB
// DIO2
#define RADIO_DIO2_PIN GPIO_Pin_8
#define RADIO_DIO2_PORT GPIOB
// DIO3
#define RADIO_DIO3_PIN GPIO_Pin_9
#define RADIO_DIO3_PORT GPIOB
// RXEN
#define RADIO_DIO5_RXEN_PIN GPIO_Pin_1
#define RADIO_DIO5_RXEN_PORT GPIOA

#endif // __PROJECT_CONFIG_H__
