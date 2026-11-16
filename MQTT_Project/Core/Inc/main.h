/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "FreeRTOS.h"
#include "task.h"
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
#define BUF_SIZE 256
extern uint8_t u2_rxbuf[BUF_SIZE];  // 用于记录单段数据的接收缓冲区
extern uint8_t u2_rxdata[BUF_SIZE]; // 用于记录完整数据的接收缓冲区

extern uint8_t u2_txdata[BUF_SIZE]; // 用于记录完整数据的发送缓冲区

extern volatile uint16_t u2_rxlen;      // 用于记录接收完整数据的长度
extern volatile uint16_t u2_txlen;      // 用于记录发送完整数据的长度
extern volatile uint8_t rx_flag;        // 接收标志	用于标记是否收到服务器的消息
extern volatile uint8_t wifi_conf_flag; // 配置标志  进入透传之后会生成

//新增接收数据缓存+标志，供main函数读取
extern volatile uint8_t lora_rx_flag ;
extern uint8_t  lora_rx_buf[255];
extern uint16_t lora_rx_len;
extern int16_t  lora_rx_rssi;
extern int8_t   lora_rx_snr;


extern volatile uint32_t g_dio1_cnt;
/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define LORA_CSS_Pin GPIO_PIN_4
#define LORA_CSS_GPIO_Port GPIOA
#define LORA_BUSY_Pin GPIO_PIN_0
#define LORA_BUSY_GPIO_Port GPIOB
#define LORA_RESET_Pin GPIO_PIN_1
#define LORA_RESET_GPIO_Port GPIOB
#define LORA_DIO1_Pin GPIO_PIN_10
#define LORA_DIO1_GPIO_Port GPIOB
#define LORA_DIO1_EXTI_IRQn EXTI15_10_IRQn
#define LED2_Pin GPIO_PIN_15
#define LED2_GPIO_Port GPIOB
#define LED1_Pin GPIO_PIN_8
#define LED1_GPIO_Port GPIOA

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
