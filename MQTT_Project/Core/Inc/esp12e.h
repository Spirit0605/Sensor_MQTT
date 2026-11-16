#ifndef __ESP12E_H
#define __ESP12E_H

#include "main.h"
#include "usart.h"
#include <stdbool.h>
#include "stdio.h"
#include "mqtt.h"
#include <string.h>

// void USART2_ClearBuffer(void);
// bool USART2_SendCmdToWIFI(char *cmd, char *dest);
// void wifi_connect(void);

void USART2_ClearBuffer(void);
bool USART2_SendCmdToWIFI(const char *cmd, const char *dest, uint32_t timeout_ms);
bool wifi_connect(void);
void mqtt_ping(void);


#endif
