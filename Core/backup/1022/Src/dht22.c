#include "dht22.h"
#include <stdio.h>
#include <string.h>
#include "gpio.h"
#include "delay.h"
#include "main.h"

// 缓冲区清理函数
void CleanBuffer(void)
{
  memset(rxbuffer, 0, sizeof(rxbuffer));
  memset(rxdata, 0, sizeof(rxdata));
}

// 设置DHT22引脚模式
void DHT22_SetPinMode(uint32_t mode)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  GPIO_InitStruct.Pin = DHT22_Pin;
  GPIO_InitStruct.Mode = mode;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}

// AM2303上电，启动模块
void DHT22_Start(void)
{
  // 1. 配置引脚为输出模式
  DHT22_SetPinMode(GPIO_MODE_OUTPUT_PP);

  // 2.输出低电平
  HAL_GPIO_WritePin(DHT22_GPIO_Port, DHT22_Pin, GPIO_PIN_RESET);

  // 3.等待1ms
  delay_ms(1);

  // 4.配置引脚为输入模式
  DHT22_SetPinMode(GPIO_MODE_INPUT);
}

// DHT22应答信号
bool DHT22_WaitAck(void)
{
  int cnt = 0; // 定期器

  // 1.等待引脚电平被拉低  人为的增加超时机制  假设超时时间设置为100us
  while (HAL_GPIO_ReadPin(DHT22_GPIO_Port, DHT22_Pin) == 1 && cnt < 100)
  {
    cnt++;
    delay_us(1); // 1us循环一次
  }

  if (cnt >= 100)
  {
    printf("应答信号接收超时\n");
    return false;
  }

  cnt = 0;
  // 2.等待引脚电平被拉低  人为的增加超时机制  假设超时时间设置为100us
  while (HAL_GPIO_ReadPin(DHT22_GPIO_Port, DHT22_Pin) == 0 && cnt < 100)
  {
    cnt++;
    delay_us(1); // 1us循环一次
  }

  if (cnt >= 100)
  {
    printf("接收信号接收超时\n");
    return false;
  }

  return true;
}

// 处理传感器发回数据
// DHT22读取bit  0000000_0 -- 0x00   0000000_1 -- 0x01
uint8_t DHT22_ReadBit(void)
{
  uint8_t cnt = 0;

  // 等待电平拉低
  while (HAL_GPIO_ReadPin(DHT22_GPIO_Port, DHT22_Pin) == 1 && cnt < 200)
  {
    cnt++;
    delay_us(1);
  }

  if (cnt >= 200)
  {
    printf("电平拉低超时\n");
    //return false;
  }

  cnt = 0;
  // 等待电平拉高
  while (HAL_GPIO_ReadPin(DHT22_GPIO_Port, DHT22_Pin) == 0 && cnt < 200)
  {
    cnt++;
    delay_us(1);
  }

  if (cnt >= 200)
  {
    printf("电平拉高超时\n");
    //return false;
  }

  // 3.延时50us
  delay_us(45);

  // 4.读取引脚电平
  return (HAL_GPIO_ReadPin(DHT22_GPIO_Port, DHT22_Pin) == 1) ? 0x01 : 0x00;
}

// DHT22读取bit  0000000_0 -- 0x00   0000000_1 -- 0x01
// uint8_t DHT22_ReadBit(void)
// {
//   uint16_t timeout = 0;

//   /* 1. 等待引脚被 DHT22 拉低（新 bit 开始的下降沿） */
//   while (HAL_GPIO_ReadPin(DHT22_GPIO_Port, DHT22_Pin) == 1)
//   {
//     if (++timeout > 200) return 0;
//     delay_us(1);
//   }

//   /* 2. 等待 50us 低电平结束，引脚被释放并拉高 */
//   timeout = 0;
//   while (HAL_GPIO_ReadPin(DHT22_GPIO_Port, DHT22_Pin) == 0)
//   {
//     if (++timeout > 200) return 0;
//     delay_us(1);
//   }

//   /* 3. 测量高电平宽度：这段不能调用 delay_us，否则 TIM3 会被清零 */
//   uint32_t t0 = __HAL_TIM_GET_COUNTER(&htim3);

//   while (HAL_GPIO_ReadPin(DHT22_GPIO_Port, DHT22_Pin) == 1)
//   {
//     if ((__HAL_TIM_GET_COUNTER(&htim3) - t0) > 200) break;
//   }

//   uint32_t width = __HAL_TIM_GET_COUNTER(&htim3) - t0;

//   /* 4. 0 的高电平约 26~28us，1 约 70us，取 50us 为界 */
//   return (width > 50) ? 0x01 : 0x00;
// }

// DHT22读取字节
uint8_t DHT22_ReadByte(void)
{
  uint8_t buf = 0;
  int i = 0;

  // 采用MSB先出
  for (i = 0; i < 8; i++)
  {
    buf <<= 1;
    buf |= (uint8_t)DHT22_ReadBit();
  }

  return buf;
}

// DHT22获取数据
bool DHT22_GetVal(float *TEMP, float *HUMI)
{
  uint8_t data[5] = {0};

  // 1.发送信号
  DHT22_Start();

  // 2.等待响应
  if (!DHT22_WaitAck())
  {
    printf("DHT22应答响应失败!");
    return false;
  }
  // 3.接收40bit数据
  uint8_t i = 0;
  for (i = 0; i < 5; i++)
  {
    data[i] = DHT22_ReadByte();
  }
  //4.数据校验
  uint8_t checksum = (uint8_t)(data[0] + data[1] + data[2] + data[3]);
  if (checksum != data[4])
  {
    printf("DHT22接收数据校验失败，请重试\n");
    return false;
  }

  // 5.处理数据
  uint16_t humi = (data[0] << 8) | data[1];
  uint16_t temp = (data[2] << 8) | data[3];

  // 使用单精度浮点型，双精度浮点型会占用更多资源
  *TEMP = temp / 10.0f;
  *HUMI = humi / 10.0f;

  void CleanBuffer(void);

  return true;
}
