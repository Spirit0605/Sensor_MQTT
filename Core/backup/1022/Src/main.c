/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "i2c.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "oled.h"
#include "dht22.h"
#include "delay.h"
#include "LLCC68_example_send.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
#define SAMPLE_INTERVAL_MS 2000U // 采样周期 3 秒

volatile uint8_t Usart2_Receive_FLAG = 0;

volatile uint16_t adc_buf[2] = {0};
air_t air = {0};   // 接收二氧化碳变量
float HUMI = 0.0f; // 接收湿度
float TEMP = 0.0f; // 接收温度

//volatile bool HOT_FLAG = false;

uint16_t light_val = 0;
float soil_val = 0.0;

uint32_t last_sample_tick = 0;
uint8_t oled_status = 1; // 0 = OK，其他 = 失败

#define RX_DMA_SIZE 6
uint8_t usart2_rx_dma[RX_DMA_SIZE];  // DMA搬运的目标缓冲区
uint8_t usart2_rx_data[RX_DMA_SIZE]; // 完整数据帧保存数组
uint16_t usart2_rx_len = 0;

volatile uint8_t cnt = 0;            // 数组下标计数器
uint8_t rxbuffer[BUFFER_SIZE] = {0}; // 接收缓存区
uint8_t rxdata[BUFFER_SIZE] = {0};   // 数据保存数组
uint8_t txbuffer[BUFFER_SIZE] = {0}; // 发送缓存区
uint8_t txdata[BUFFER_SIZE] = {0};   // 发送

char sendData[64] = {0};
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
bool JW01_GetVal(air_t *air)
{
  // 1.校验帧头
  if (usart2_rx_data[0] != 0x2c)
  {
    printf("JW01帧头地址校验错误\n");
    return false;
  }

  // 2.校验和
  uint8_t checksum = 0;
  for (size_t i = 0; i < 5; i++)
  {
    checksum += usart2_rx_data[i];
  }
  if (usart2_rx_data[5] != checksum)
  {
    printf("JW01校验和失败\n");
    return false;
  }

  // 3.获取数据
  air->co2 = (uint16_t)((usart2_rx_data[1] << 8) | usart2_rx_data[2]); 
  air->max = (uint16_t)((usart2_rx_data[3] << 8) | usart2_rx_data[4]);

  return true;
}

// 读取所有的数据
static void Read_All_Sensors(void)
{
  // 1. 光照 + 土壤：ADC DMA 已在后台刷新 adc_buf
  uint16_t temp = 0;

  light_val = adc_buf[0];
  soil_val = (1 - (float)(adc_buf[1] / 4095.0)) * 100;

  // 2. CO2：串口收到新帧才解析，失败保留原值
  if (Usart2_Receive_FLAG)
  {
    (void)JW01_GetVal(&air);
    Usart2_Receive_FLAG = 0;
    usart2_rx_len = 0;
  }

  // 3. 温湿度：DHT22 失败保留原值，不重试、不刷屏
  (void)DHT22_GetVal(&TEMP, &HUMI);
}

// 串口发送加OLED显示
static void Output_All_Sensors(void)
{
  char line[24];

  /* 串口1 输出一行，方便上位机解析 */
  printf("Light=%u,Soil=%.1f,CO2=%u,TEMP=%.1f,HUMI=%.1f\r\n",
         (unsigned int)light_val,
         soil_val,
         (unsigned int)air.co2,
         TEMP,
         HUMI);

  /* OLED 刷新 */
  if (oled_status == 0)
  {
    snprintf(line, sizeof(line), "Light: %6u", (unsigned int)light_val);
    OLED_ShowString(0, 0, line);
    
    snprintf(line, sizeof(line), "Co2  : %6u", (unsigned int)air.co2);
    OLED_ShowString(0, 10, line);

    snprintf(line, sizeof(line), "Soil : %6.1f", soil_val);
    OLED_ShowString(0, 20, line);
    
    snprintf(line, sizeof(line), "Temp : %6.1f", TEMP);
    OLED_ShowString(0, 30, line);

    snprintf(line, sizeof(line), "Humi : %6.1f", HUMI);
    OLED_ShowString(0, 40, line);

    (void)OLED_Refresh();
  }
}

static uint8_t Merge_All_Sensors(void)
{
  int length = snprintf(sendData, sizeof(sendData),
                        "%u_%.1f_%u_%.1f_%.1f_",
                        (unsigned int)light_val,
                        (float)soil_val,
                        (unsigned int)air.co2,
                        (float)TEMP,
                        (float)HUMI);

  if (length < 0 || (size_t)length >= sizeof(sendData))
  {
    sendData[0] = '\0';
    return 0;
  }

  return (uint8_t)length;
}

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_USART1_UART_Init();
  MX_TIM3_Init();
  MX_TIM4_Init();
  MX_USART2_UART_Init();
  MX_I2C1_Init();
  MX_SPI1_Init();
  /* USER CODE BEGIN 2 */
  HAL_ADCEx_Calibration_Start(&hadc1);               // 1. 开启ADC前进行校准（提高精度）
  HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_buf, 2); // 2. 启动ADC DMA，长度为2
  HAL_TIM_Base_Start(&htim3);                        // 3. 启动计数器
  //HAL_TIM_Base_Start_IT(&htim4);                     // 4. 启用定时器中断

  HAL_UARTEx_ReceiveToIdle_IT(&huart2, usart2_rx_dma, RX_DMA_SIZE); // 5. 启用串口空闲接收中断
  // OLED初始化
  oled_status = OLED_Init(&hi2c1);
  if (oled_status == 0)
  {
    OLED_Clear();
    OLED_ShowString(0, 0, "SYSTEM INIT");
    if (OLED_Refresh() != HAL_OK)
    {
      Error_Handler();
    }
  }
  else
  {
    Error_Handler();
  }

  Read_All_Sensors();
  Output_All_Sensors();
  last_sample_tick = HAL_GetTick();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    uint32_t now = HAL_GetTick();

    if ((now - last_sample_tick) >= SAMPLE_INTERVAL_MS)
    {
      last_sample_tick = now;
      Read_All_Sensors(); // 同一个地方读四个数据
      // if(TEMP >= 27.0)
      // {
      //   HOT_FLAG = true;
      //   HAL_GPIO_WritePin(Relay_GPIO_Port, Relay_Pin, GPIO_PIN_SET);
      // }
      // else
      // {
      //   HOT_FLAG = false;
      //   HAL_GPIO_WritePin(Relay_GPIO_Port, Relay_Pin, GPIO_PIN_RESET);
      // }
      Output_All_Sensors(); // 串口发送 + OLED 显示
      if (Merge_All_Sensors() > 0)
      {
        ExampleLLCC68SendDemo(sendData);
      }
      // Clear_All_Buffers();  // 清缓冲区，等下一轮
      memset(usart2_rx_data, 0, sizeof(usart2_rx_data));
      memset(sendData, 0, sizeof(sendData));
    }

    //通过LORA天线向网关发送数据
    
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
// 通用异步收发接收器 扩展 接收 事件 回调函数
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
  if (huart->Instance == USART2)
  {
    if (RX_DMA_SIZE == Size && Usart2_Receive_FLAG == 0)
    {
      memcpy(usart2_rx_data, usart2_rx_dma, Size);
      usart2_rx_len = Size;
      memset(usart2_rx_dma, 0, Size);
      Usart2_Receive_FLAG = 1;
    }

    HAL_UARTEx_ReceiveToIdle_IT(&huart2, usart2_rx_dma, RX_DMA_SIZE); // 重新启用串口空闲接收中断
  }
}

// adc采样数据转换后中断
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
  if (hadc->Instance == ADC1)
  {
    /* ADC DMA buffer contains the first two sensor readings. */
  }
}

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
