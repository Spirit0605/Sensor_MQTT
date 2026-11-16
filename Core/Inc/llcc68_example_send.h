#ifndef __LLCC68_EXAMPLE_SEND_H__
#define __LLCC68_EXAMPLE_SEND_H__

#include <stdbool.h>
#include <stdint.h>

/* [修改] 新增：只初始化一次 + 带超时的发送接口 */
void LoRa_Init(void);
bool LoRa_Send(const char *data, uint32_t timeout_ms);

/* 保留原接口，内部调用 LoRa_Send */
void ExampleLLCC68SendDemo(const char * sendData);

#endif //end of LLCC68_EXAMPLE_SEND_H__
