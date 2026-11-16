#include "esp12e.h"

char resp[512];

/* 清接收缓冲：旧的 u2_rxdata + 新的环形缓冲。
   注意：不要 memset u2_rxbuf，它是 HAL 正在使用的接收缓冲。 */
void USART2_ClearBuffer(void)
{
    memset(u2_rxdata, 0, BUF_SIZE);
    memset(u2_rxbuf, 0, BUF_SIZE);
    u2_rxlen = 0;
    UART_RingClear(); /* AT 用的环形缓冲也一起清 */
}

/*
 * 发送 AT 命令并等待响应（事件驱动）
 * cmd        : 要发送的命令（带 \r\n）
 * dest       : 期望在响应里出现的字符串，如 "OK"、"WIFI GOT IP"、">"
 * timeout_ms : 超时时间（毫秒）
 * 返回 true 表示超时前找到了 dest
 */
bool USART2_SendCmdToWIFI(const char *cmd, const char *dest, uint32_t timeout_ms)
{
    // char resp[512];
    uint16_t len = 0;
    TickType_t start = xTaskGetTickCount();
    TickType_t timeout_ticks = pdMS_TO_TICKS(timeout_ms);

    USART2_ClearBuffer();

    if (HAL_UART_Transmit(&huart2, (uint8_t *)cmd, (uint16_t)strlen(cmd), 100) != HAL_OK)
    {
        return false;
    }

    resp[0] = '\0';

    for (;;)
    {
        TickType_t elapsed = xTaskGetTickCount() - start;

        if (elapsed >= timeout_ticks)
        {
            return false;
        }

        /* 等串口中断叫醒；没有新数据就一直睡到剩余超时 */
        (void)ulTaskNotifyTake(pdTRUE, timeout_ticks - elapsed);

        /* 把环形缓冲里已到的字节追加到响应缓冲 */
        while (len < (sizeof(resp) - 1U))
        {
            uint16_t n = UART_RingRead((uint8_t *)&resp[len],
                                       (uint16_t)(sizeof(resp) - 1U - len));
            if (n == 0)
            {
                break;
            }
            len = (uint16_t)(len + n);
            resp[len] = '\0';
        }

        /* 失败提前退出，不用等满超时 */
        if (strstr(resp, "FAIL") != NULL || strstr(resp, "ERROR") != NULL)
        {
            return false;
        }

        if (strstr(resp, dest) != NULL)
        {
            return true;
        }
    }
}

/* 连接 WiFi 并进入 TCP 透传；成功返回 true */
bool wifi_connect(void)
{
    wifi_conf_flag = 0;
    rx_flag = 0;
    USART2_ClearBuffer();

    // /* 0. 退出透传：+++ 之后要有至少 1 秒保护时间 */
    // vTaskDelay(pdMS_TO_TICKS(1000));
    // (void)HAL_UART_Transmit(&huart2, (uint8_t *)"+++", 3, 100);
    // vTaskDelay(pdMS_TO_TICKS(2000));
    // if (!USART2_SendCmdToWIFI("AT\r\n", "OK", 5000))
    // {
    //     printf("透传退出失败\r\n");
    //     // printf("%s\r\n", resp);
    //     return false;
    // }
    // printf("透传退出成功\r\n");
    // vTaskDelay(pdMS_TO_TICKS(500));

    // /* 1. 复位 */
    // if (!USART2_SendCmdToWIFI("AT+RST\r\n", "OK", 5000))
    // {
    //     printf("RST失败\r\n");
    //     return false;
    // }

    /* 先直接试 RST：正常 AT 模式下这一步就成功 */
    if (!USART2_SendCmdToWIFI("AT+RST\r\n", "OK", 5000))
    {
        /* 失败说明可能在透传模式：静默 1s 后 +++ 退出 */
        vTaskDelay(pdMS_TO_TICKS(1000));
        (void)HAL_UART_Transmit(&huart2, (uint8_t *)"+++", 3, 100);
        vTaskDelay(pdMS_TO_TICKS(1000));

        if (!USART2_SendCmdToWIFI("AT\r\n", "OK", 1000))
        {
            printf("ESP 无响应\r\n");
            return false;
        }

        if (!USART2_SendCmdToWIFI("AT+RST\r\n", "OK", 5000))
        {
            printf("RST失败\r\n");
            return false;
        }
    }
    printf("RST成功\r\n");
    vTaskDelay(pdMS_TO_TICKS(500));

    /* 2. 在线检测 */
    if (!USART2_SendCmdToWIFI("AT\r\n", "OK", 1000))
    {
        printf("wifi模块离线\r\n");
        return false;
    }
    printf("wifi模块在线\r\n");

    /* 3. 模式 */
    if (!USART2_SendCmdToWIFI("AT+CWMODE=3\r\n", "OK", 2000))
    {
        printf("wifi模式设置失败\r\n");
        return false;
    }
    printf("wifi模式设置成功\r\n");

    /* 4. 连接热点 */
    memset(u2_txdata, 0, BUF_SIZE);
    sprintf((char *)u2_txdata, "AT+CWJAP=\"%s\",\"%s\"\r\n", WIFI_NAME, WIFI_PASSWORD);
    if (!USART2_SendCmdToWIFI((char *)u2_txdata, "OK", 20000))
    {
        printf("wifi连接设置失败\r\n");
        return false;
    }
    printf("wifi连接设置成功\r\n");

    /* 5. 透传模式 */
    if (!USART2_SendCmdToWIFI("AT+CIPMODE=1\r\n", "OK", 2000))
    {
        printf("wifi透传模式设置失败\r\n");
        return false;
    }
    printf("wifi透传模式设置成功\r\n");

    /* 6. 连接服务器 */
    memset(u2_txdata, 0, BUF_SIZE);
    sprintf((char *)u2_txdata, "AT+CIPSTART=\"TCP\",\"%s\",%d\r\n", SERVER_ADDRESS, SERVER_PORT);
    if (!USART2_SendCmdToWIFI((char *)u2_txdata, "OK", 5000))
    {
        printf("wifi连接服务器失败\r\n");
        return false;
    }
    printf("wifi连接服务器成功\r\n");

    /* 7. 进入透传 */
    if (!USART2_SendCmdToWIFI("AT+CIPSEND\r\n", ">", 5000))
    {
        printf("wifi进入透传模式失败\r\n");
        printf("%s\r\n", resp);
        return false;
    }
    wifi_conf_flag = 1;
    printf("wifi进入透传模式成功\r\n");
    vTaskDelay(pdMS_TO_TICKS(500));

    return true;
}

void mqtt_ping(void)
{
    uint8_t ping[2] = {0xC0, 0x00};
    HAL_UART_Transmit(&huart2, ping, 2, 100);
}

// #include "esp12e.h"

// void wifi_connect(void)
// {
//     // 0.发送退出透传的数据包 "+++"
//     HAL_UART_Transmit(&huart2, (uint8_t *)"+++", strlen("+++"), 100);
//     HAL_Delay(1000);

//     if (USART2_SendCmdToWIFI("AT+RST\r\n", "OK"))
//         printf("\nRST成功\r\n");
//     else
//         printf("\nRST失败\r\n");
//     // printf(u2_rxdata);

//     // 1.发送测试指令 "AT\r\n"
//     if (USART2_SendCmdToWIFI("AT\r\n", "OK"))
//         printf("wifi模块在线\r\n");
//     else
//         printf("wifi模块离线\r\n");
//     // printf("%s", u2_rxdata);

//     // 2.设置WIFI模式
//     if (USART2_SendCmdToWIFI("AT+CWMODE=3\r\n", "OK"))
//         printf("wifi模式设置成功\r\n");
//     else
//         printf("wifi模式设置失败\r\n");
//     // printf("%s", u2_rxdata);

//     // 3.连接手机热点
//     memset(u2_txdata, 0, BUF_SIZE);
//     sprintf((char *)u2_txdata, "AT+CWJAP=\"%s\",\"%s\"\r\n", WIFI_NAME, WIFI_PASSWORD);
//     // if (USART2_SendCmdToWIFI((char *)u2_txdata, "OK"))
//     if (USART2_SendCmdToWIFI((char *)u2_txdata, "WIFI GOT IP"))
//         printf("wifi连接设置成功\r\n");
//     else
//         printf("wifi连接设置失败\r\n");
//     //printf("%s", u2_rxdata);

//     // 4.设置透传模式
//     if (USART2_SendCmdToWIFI("AT+CIPMODE=1\r\n", "OK"))
//         printf("wifi透传模式设置成功\r\n");
//     else
//         printf("wifi透传模式设置失败\r\n");
//     // printf("%s", u2_rxdata);

//     // 5.连接服务器和端口
//     memset(u2_txdata, 0, BUF_SIZE);
//     sprintf((char *)u2_txdata, "AT+CIPSTART=\"TCP\",\"%s\",%d\r\n", SERVER_ADDRESS, SERVER_PORT);
//     if (USART2_SendCmdToWIFI((char *)u2_txdata, "OK"))
//         printf("wifi连接服务器成功\r\n");
//     else
//         printf("wifi连接服务器失败\r\n");
//     // printf("%s", u2_rxdata);

//     // 6.进入透传模式，下面发的都会无条件传输
//     if (USART2_SendCmdToWIFI("AT+CIPSEND\r\n", ">"))
//     {
//         wifi_conf_flag = 1;
//         printf("wifi进入透传模式成功\r\n");
//         delay_ms(500);
//     }
//     else
//         printf("wifi进入透传模式失败\r\n");
//     // printf("%s", u2_rxdata);

// #pragma region TCP订阅指令
//     // 7.发送TCP订阅指令
//     // uint8_t subscribe_cmd[100] = {0};
//     // sprintf((char *)subscribe_cmd, "cmd=1&uid=%s&topic=LED002\r\n", CLIENT_KEY);
//     // if (USART2_SendCmdToWIFI(subscribe_cmd, "cmd=1&res=1"))
//     //   USART1_Print("\n订阅主题成功\r\n");
//     // else
//     //   USART1_Print("\n订阅主题失败\r\n");
//     // USART1_Print(u2_rxdata);
// #pragma endregion TCP订阅指令

// }

// // 清空缓冲区
// void USART2_ClearBuffer(void)
// {
//     memset(u2_rxbuf, 0, BUF_SIZE);
//     memset(u2_rxdata, 0, BUF_SIZE);

//     u2_rxlen = 0;
// }

// // UART2发送AT指令
// bool USART2_SendCmdToWIFI(char *cmd, char *dest)
// {
//     int cnt = 1000; // 计数器

//     // 清空缓冲区
//     USART2_ClearBuffer();

//     // 发送AT指令
//     HAL_UART_Transmit(&huart2, (uint8_t *)cmd, strlen((char *)cmd), 100);

//     // 等待响应内容
//     while (cnt--)
//     {
//         HAL_Delay(10);

//         if (strstr((char *)u2_rxbuf, dest))
//         {
//             // 说明找到ok
//             return true;
//         }
//     }

//     return false;
// }