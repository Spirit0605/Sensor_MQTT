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
#include "iwdg.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "mqtt.h"
#include "delay.h"
#include "esp12e.h"
#include "radio.h"
#include "llcc68_example_recive.h"
#include "llcc68-board.h"
#include "project_config.h"

#include "FreeRTOSConfig.h"
#include "queue.h"
#include "iwdg.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* LoRa 上行：终端板发来的传感器数据 */
typedef struct
{
  uint16_t light;
  uint16_t co2;
  float soil;
  float temp;
  float humi;
} SensorData;

/* LoRa 下行：网关发给终端板的命令帧，如 "RELAY:ON" */
typedef struct
{
  char frame[32];
} LoraCmd;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
#define BUF_SIZE 256
#define RADIO_RX_TEXT_SIZE 256U
uint8_t u2_rxbuf[BUF_SIZE] = {0};  // 用于记录单段数据的接收缓冲区
uint8_t u2_rxdata[BUF_SIZE] = {0}; // 用于记录完整数据的接收缓冲区

uint8_t u2_txdata[BUF_SIZE] = {0}; // 用于记录完整数据的发送缓冲区

volatile uint16_t u2_rxlen = 0;      // 用于记录接收完整数据的长度
volatile uint16_t u2_txlen = 0;      // 用于记录发送完整数据的长度
volatile uint8_t rx_flag = 0;        // 接收标志	用于标记是否收到服务器的消息
volatile uint8_t wifi_conf_flag = 0; // 配置标志  进入透传之后会生成

volatile uint8_t num = 0;

uint16_t light;
uint16_t co2;
float soil;
float temp;
float humi;

static QueueHandle_t xUplinkQueue = NULL; /* LoRaTask -> NetTask  */
static QueueHandle_t xLoraTxQueue = NULL; /* NetTask  -> LoRaTask */

static TaskHandle_t xNetTaskHandle = NULL; /* 给中断/其他任务发通知用 */
static TaskHandle_t xLoRaTaskHandle = NULL;

volatile uint8_t g_net_ready = 0;   /* 1 = MQTT 已连接，可以上报 */
extern bool IrqFired;                     /* radio.c 里的全局变量 */


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static void vNetTask(void *pvParameters);
static void vLoRaTask(void *pvParameters);
static void vStatusTask(void *pvParameters);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
int parse_sensor(const char *s)
{
  char tmp[64];
  char *p, *tok;

  /* 复制一份，strtok 会改原串 */
  if (strlen(s) >= sizeof(tmp))
    return -1;
  strcpy(tmp, s);

  tok = strtok(tmp, "_");
  if (!tok)
    return -1;
  light = (uint16_t)atoi(tok);
  tok = strtok(NULL, "_");
  if (!tok)
    return -1;
  soil = atof(tok);
  tok = strtok(NULL, "_");
  if (!tok)
    return -1;
  co2 = (uint16_t)atoi(tok);
  tok = strtok(NULL, "_");
  if (!tok)
    return -1;
  temp = atof(tok);
  tok = strtok(NULL, "_");
  if (!tok)
    return -1;
  humi = atof(tok);

  return 0;
}

/* 网络任务：以后负责 USART2 / AT / MQTT */
static void vNetTask(void *pvParameters)
{
  SensorData d;
  char pub[16];

  TickType_t last_print = xTaskGetTickCount();

  (void)pvParameters;

  if (HAL_UARTEx_ReceiveToIdle_IT(&huart2, u2_rxbuf, BUF_SIZE) != HAL_OK)
  {
    Error_Handler();
  }

  /* 1. WiFi + TCP 透传 */
  for (;;)
  {
    while (wifi_connect() == false)
    {
      printf("WiFi 连接失败，2 秒后重试\r\n");
      g_net_ready = 0;
      vTaskDelay(pdMS_TO_TICKS(2000));
    }
    

    if (MQTT_CONNECT((char *)CLIENT_KEY))
    {
      printf("MQTT CONNECT 成功\r\n");
      break;
    }

    printf("MQTT CONNECT 失败，2 秒后重连\r\n");
    g_net_ready = 0;
    vTaskDelay(pdMS_TO_TICKS(2000));
  }

  /* 2. 订阅 5 个传感器 + 1 个继电器 */
  MQTT_SUBSCRIBE(CLIENT_LIGHT_TOPIC, 0);
  MQTT_SUBSCRIBE(CLIENT_SOIL_TOPIC, 0);
  MQTT_SUBSCRIBE(CLIENT_CO2_TOPIC, 0);
  MQTT_SUBSCRIBE(CLIENT_TEMP_TOPIC, 0);
  MQTT_SUBSCRIBE(CLIENT_HUMI_TOPIC, 0);
  MQTT_SUBSCRIBE(CLIENT_RELAY_TOPIC, 0);

  printf("MQTT 初始化完成\r\n");
  g_net_ready = 1;
  TickType_t hreat = xTaskGetTickCount();
  /* 3. 主循环：处理下行 */
  for (;;)
  { 
    if(xTaskGetTickCount() - hreat > pdMS_TO_TICKS(60000))
    {
      mqtt_ping();
      hreat = xTaskGetTickCount();
    }


    (void)ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(500));

    /* ===== 上行：LoRaTask 投递的传感器数据 ===== */
    while (xQueueReceive(xUplinkQueue, &d, 0) == pdPASS)
    {
      snprintf(pub, sizeof(pub), "%u", (unsigned)d.light);
      MQTT_Publish(CLIENT_LIGHT_TOPIC, pub);

      snprintf(pub, sizeof(pub), "%.1f", d.soil);
      MQTT_Publish(CLIENT_SOIL_TOPIC, pub);

      snprintf(pub, sizeof(pub), "%u", (unsigned)d.co2);
      MQTT_Publish(CLIENT_CO2_TOPIC, pub);

      snprintf(pub, sizeof(pub), "%.1f", d.temp);
      MQTT_Publish(CLIENT_TEMP_TOPIC, pub);

      snprintf(pub, sizeof(pub), "%.1f", d.humi);
      MQTT_Publish(CLIENT_HUMI_TOPIC, pub);
    }

    if (rx_flag)
    {
      rx_flag = 0;

      if (u2_rxlen > 4 && u2_rxdata[0] == 0x30)
      {
        uint8_t rem = u2_rxdata[1]; // 本帧剩余长度
        uint16_t topic_len = (u2_rxdata[2] << 8) | u2_rxdata[3];
        uint16_t payload_index = 2 + 2 + topic_len;
        uint16_t payload_len = rem - 2 - topic_len;

        if (payload_index < u2_rxlen)
        {
          uint8_t payload_buf[BUF_SIZE] = {0};

          if (payload_len >= BUF_SIZE)
            payload_len = BUF_SIZE - 1;
          memcpy(payload_buf, &u2_rxdata[payload_index], payload_len);
          payload_buf[payload_len] = '\0';

          printf("云平台:[%s]\r\n", payload_buf);

          if ((topic_len == strlen(CLIENT_LIGHT_TOPIC)) && (memcmp(&u2_rxdata[4], CLIENT_LIGHT_TOPIC, topic_len) == 0))
          {
            if (strstr((char *)payload_buf, "on"))
              HAL_GPIO_WritePin(GPIOA, LED1_Pin, GPIO_PIN_RESET);
            else if (strstr((char *)payload_buf, "off"))
              HAL_GPIO_WritePin(GPIOA, LED1_Pin, GPIO_PIN_SET);
          }
          else if ((topic_len == strlen(CLIENT_RELAY_TOPIC)) && (memcmp(&u2_rxdata[4], CLIENT_RELAY_TOPIC, topic_len) == 0))
          {
            // 发送继电器下发的开关命令
          }
        }
      }

      USART2_ClearBuffer();
    }

    if ((xTaskGetTickCount() - last_print) >= pdMS_TO_TICKS(1000))
    {
      last_print = xTaskGetTickCount();
      printf("NET alive, heap=%u\r\n", (unsigned)xPortGetFreeHeapSize());
    }
  }
}

/* LoRa 任务：收终端板的数据，投给 NetTask */
static void vLoRaTask(void *pvParameters)
{
  SensorData data;

  (void)pvParameters;

  /* 如果之前加了“网络就绪才收”，这两行保留在 g_net_ready 判断之后 */
  while(g_net_ready == 0)
  {
    vTaskDelay(pdMS_TO_TICKS(100));
  }

  LLCC68ReciveInit();
  printf("LoRa init done\r\n");

  for (;;)
  {
    // printf("%d\n", g_dio1_cnt);
    /* 真正的事件源：DIO1 中断把 IrqFired 置 1，
       这里只负责处理 */
    Radio.IrqProcess();

    if (lora_rx_flag)
    {
      lora_rx_flag = 0;

      if (parse_sensor((const char *)lora_rx_buf) == 0)
      {
        data.light = light;
        data.co2   = co2;
        data.soil  = soil;
        data.temp  = temp;
        data.humi  = humi;

        printf("LoRa RX: %s (rssi=%d snr=%d)\r\n",
               lora_rx_buf, lora_rx_rssi, lora_rx_snr);

        if (xQueueSend(xUplinkQueue, &data, 0) == pdPASS)
        {
          xTaskNotifyGive(xNetTaskHandle);
        }
      }
    }

    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

/* 状态任务：心跳灯 + 喂狗 */
static void vStatusTask(void *pvParameters)
{
  (void)pvParameters;

  for (;;)
  {
    // HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
    (void)HAL_IWDG_Refresh(&hiwdg);

    vTaskDelay(pdMS_TO_TICKS(500));
  }
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
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  MX_SPI1_Init();
  MX_TIM4_Init();
  MX_TIM3_Init();
  MX_IWDG_Init();
  /* USER CODE BEGIN 2 */


  /* 延时用 TIM4，保留 */
  HAL_TIM_Base_Start(&htim4);

  /* 1. 先创建 IPC 对象 */
  xUplinkQueue = xQueueCreate(4, sizeof(SensorData));
  xLoraTxQueue = xQueueCreate(4, sizeof(LoraCmd));

  if ((xUplinkQueue == NULL) || (xLoraTxQueue == NULL))
  {
    Error_Handler();
  }

  /* 2. 再创建任务 */
  if (xTaskCreate(vNetTask, "NET", 768, NULL, 4, &xNetTaskHandle) != pdPASS)
    Error_Handler();
  if (xTaskCreate(vLoRaTask, "LORA", 512, NULL, 3, &xLoRaTaskHandle) != pdPASS)
    Error_Handler();
  if (xTaskCreate(vStatusTask, "STAT", 128, NULL, 1, NULL) != pdPASS)
    Error_Handler();

  /* 3. 启动调度器（成功不返回） */
  vTaskStartScheduler();

  /* 只有启动失败才会到这里 */
  Error_Handler();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
// 空闲中断的回调函数  第二个参数Size就是接收的数据长度
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
  BaseType_t woken = pdFALSE;

  if (huart->Instance == USART2)
  {
    /* 1. AT 用的环形缓冲 */
    UART_RingPut(u2_rxbuf, Size);

    /* 2. MQTT 用的累积缓冲（你原来的逻辑） */
    if (u2_rxlen + Size < BUF_SIZE)
    {
      memcpy(&u2_rxdata[u2_rxlen], u2_rxbuf, Size);
      u2_rxlen = (uint16_t)(u2_rxlen + Size);
      u2_rxdata[u2_rxlen] = '\0';
    }

    /* 3. 透传后收到的都是服务器数据 */
    if (wifi_conf_flag)
    {
      rx_flag = 1;
    }

    /* 4. 叫醒 NetTask */
    vTaskNotifyGiveFromISR(xNetTaskHandle, &woken);

    /* 5. 重新挂接收 */
    HAL_UARTEx_ReceiveToIdle_IT(&huart2, u2_rxbuf, BUF_SIZE);

    portYIELD_FROM_ISR(woken);
  }
}
// void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
// {
//   if (huart->Instance == USART2)
//   {
//     // 该标志一旦成立，说明wifi已经进入透传，此时触发回调收到的数据统统来自服务器
//     if (wifi_conf_flag)
//     {
//       rx_flag = 1;
//     }

//     if (u2_rxlen + Size < BUF_SIZE)
//     {
//       // 拷贝本次接收的单段数据到记录完整数据的接收缓冲区中
//       memcpy(&u2_rxdata[u2_rxlen], u2_rxbuf, Size);
//       u2_rxlen += Size;
//       u2_rxdata[u2_rxlen] = '\0';
//     }

//     // 重新启用空闲接收
//     HAL_UARTEx_ReceiveToIdle_IT(&huart2, u2_rxbuf, BUF_SIZE);
//   }
// }
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
