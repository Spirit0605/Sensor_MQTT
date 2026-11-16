#ifndef __DHT22_H
#define __DHT22_H

#include "stm32f1xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

#define BUFFER_SIZE 256
extern volatile uint8_t cnt;            // 数组下标计数器
extern uint8_t rxbuffer[BUFFER_SIZE]; // 接收缓存区
extern uint8_t rxdata[BUFFER_SIZE];   // 数据保存数组
extern uint8_t txbuffer[BUFFER_SIZE]; // 发送缓存区
extern uint8_t txdata[BUFFER_SIZE];   // 发送

// volatile uint8_t cnt = 0;            // 数组下标计数器
// extern uint8_t rxbuffer[BUFFER_SIZE] = {0}; // 接收缓存区
// extern uint8_t rxdata[BUFFER_SIZE] = {0};   // 数据保存数组
// extern uint8_t txbuffer[BUFFER_SIZE] = {0}; // 发送缓存区
// extern uint8_t txdata[BUFFER_SIZE] = {0};   // 发送

// 缓冲区清理函数
void CleanBuffer(void);
void DHT22_SetPinMode(uint32_t mode);
void DHT22_Start(void);
bool DHT22_WaitAck(void);
uint8_t DHT22_ReadBit(void);
uint8_t DHT22_ReadByte(void);
bool DHT22_GetVal(float *TEMP, float *HUMI);


#endif