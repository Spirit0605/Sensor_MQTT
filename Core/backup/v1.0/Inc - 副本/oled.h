#ifndef __OLED_H
#define __OLED_H

#include "stm32f1xx_hal.h"
#include <stdint.h>

#define OLED_WIDTH   128
#define OLED_HEIGHT  64

HAL_StatusTypeDef OLED_Init(I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef OLED_Refresh(void);

void OLED_Clear(void);
void OLED_DrawPoint(uint8_t x, uint8_t y, uint8_t color);
void OLED_ShowChar(uint8_t x, uint8_t y, char ch);
void OLED_ShowString(uint8_t x, uint8_t y, const char *str);

#endif