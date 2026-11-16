# SENSOR_Project FreeRTOS 修改复盘

> 日期：2026-10-23  
> 工程：`D:\STM32F103C8T6\Smart_Terminal\SENSOR_Project`  
> 范围：传感器采集 + OLED + LoRa 发送从裸机迁移到 FreeRTOS 后的阻塞问题修复  
> 原则：不改原有编码风格，所有改动处加 `[修改]` 注释

---

## 1. 背景与现象

工程从裸机迁移到 FreeRTOS 后，创建了三个任务：

| 任务 | 优先级 | 职责 |
|---|---|---|
| `vReadDataTask` | 4 | ADC、JW01 CO2、DHT22 |
| `vOutputDataTask` | 3 | 串口打印 + OLED |
| `vSendDataTask` | 2 | 组包并通过 LoRa 发送 |

现象：

- 串口没有 `Light=...` 输出；
- OLED 不刷新；
- 网关收不到 LoRa；
- 整体看起来像“程序阻塞/死机”。

---

## 2. 根因分析

### 2.1 三个任务都没有 `vTaskDelay`（最主要原因）

`vReadDataTask` 是最高优先级 4，但循环里没有阻塞点：

```c
for (;;)
{
    light_val = adc_buf[0];
    ...
    (void)DHT22_GetVal(&TEMP, &HUMI);
}
```

`DHT22_GetVal()` 里的 `delay_us()`/`delay_ms()` 是 TIM3 忙等，不是 `vTaskDelay`，不会让出 CPU。  
读任务始终处于就绪态，FreeRTOS 会一直运行它，优先级 3 和 2 的任务**永远得不到 CPU**。

同理：

- `vOutputDataTask` 没有延时，一直 printf + OLED；
- `vSendDataTask` 没有延时，一直 `ExampleLLCC68SendDemo()`。

结果就是：最高优先级任务把 CPU 占死，低优先级任务看起来像被阻塞。

### 2.2 OLED 初始化失败直接 `Error_Handler()`

原代码在 `vReadDataTask` 里：

```c
oled_status = OLED_Init(&hi2c1);
if (oled_status == 0) { ... }
else
{
    Error_Handler();
}
```

`Error_Handler()` 是：

```c
__disable_irq();
while (1) { }
```

只要 OLED 没接、I2C 地址不对、接线松动，程序就关中断死循环，SysTick 都停，整个系统真正“死机”。

### 2.3 DHT22 每圈都读，没有 2 秒间隔

DHT22/AM2302 要求两次采样间隔 ≥2 秒。  
原任务每圈都读，读完立刻再读，传感器返回失败/校验错，然后 `printf` 刷屏，进一步占满 CPU 和串口。

### 2.4 LoRa 发送逻辑的问题

`ExampleLLCC68SendDemo()` 原实现：

```c
Radio.Init(&LLCC68RadioEvents);          // 每次发送都重新初始化
Radio.Send(...);
Radio.IrqProcess();                      // 只处理一次
delay_ms(1);
```

问题：

1. 每包都 `Radio.Init()`，上一包 TX 可能还没完成就被下一次 Init 打断；
2. `Radio.Send()` 是异步的，`Radio.IrqProcess()` 只调一次，TX Done 很可能还没到；
3. `LLCC68OnTxDone()` 里又 `Radio.Send()` 一次，还 `delay_ms(100)`，和任务里的发送重复；
4. `LLCC68WaitOnBusy()` 的“超时”只打印不 `break`，BUSY 一直高就永久卡死；
5. `LLCC68ResetInit()` 只配置输出、没有拉低复位脚，没有真正的复位脉冲；
6. `HAL_GPIO_EXTI_Callback()` 在 EXTI 里 `printf("DIO1 irq")`，而且又读一次引脚电平，可能把中断丢掉。

### 2.5 中断优先级和 TIM4 时间基准

- TIM4 原配置是 500ms 中断，但 `LLCC68SetTxTimerValue(3000)` 按 ms 理解，实际超时变成 1500 秒；
- TIM4、EXTI、USART2 中断优先级都是 0，会抢占 FreeRTOS 内核/SysTick，不适合；
- `printf` 放在 EXTI 里，会长时间阻塞 SysTick。

### 2.6 其他

- `configCHECK_FOR_STACK_OVERFLOW = 0`，栈溢出不会报错；
- `xTaskCreate` 返回值没检查，堆不够时任务可能没建出来；
- 堆 8KB 对 3 个 768 word 任务偏紧。

---

## 3. 修改清单

| 文件 | 修改 |
|---|---|
| `Core/Src/main.c` | 三个任务加 `vTaskDelay`；OLED 初始化移到 `main()` 且失败不重启；任务创建加返回值检查；增加栈溢出钩子；删除重复 OLED 初始化 |
| `Core/Inc/FreeRTOSConfig.h` | 堆 8KB → 12KB；`configUSE_MUTEXES = 1`；`configCHECK_FOR_STACK_OVERFLOW = 2`；`configUSE_MALLOC_FAILED_HOOK = 0` |
| `Core/Src/LLCC68STM32F103-board.c` | 复位脚低 20ms/高 20ms；BUSY 超时 `break`；DIO1 回调去掉 printf、改用 `GPIO_Pin` 判断；保留 `DioIrqHandler *` |
| `Core/Src/tim.c` | TIM4 从 500ms 改为 1ms；TIM4 中断优先级 0 → 12 |
| `Core/Src/gpio.c` | BUSY / DIO1 改为下拉；EXTI 优先级 0 → 12 |
| `Core/Src/usart.c` | USART2 优先级 0 → 12 |
| `Core/Src/radio.c` | `IrqFired` 加 `volatile` |
| `Core/Inc/llcc68_example_send.h` | 新增 `LoRa_Init()`、`LoRa_Send()` |
| `Core/Src/llcc68_example_send.c` | 只初始化一次；发送后循环处理 IRQ 等 TX Done；TX Done 不再重发；TX 超时不在中断里做 SPI |

没有改：`dht22.c`、`adc.c`、`dma.c`、`i2c.c`、`spi.c`、`stm32f1xx_it.c`。

---

## 4. 关键代码前后对比

### 4.1 三个任务加阻塞点

`main.c`：

```c
/* [修改] DHT22 要求两次采样间隔 ≥2s，同时让出 CPU 给其他任务 */
vTaskDelay(pdMS_TO_TICKS(2000));
```

```c
/* [修改] 1 秒刷新一次，OLED 全屏刷新约 100ms */
vTaskDelay(pdMS_TO_TICKS(1000));
```

```c
/* [修改] 2 秒发一包，和采样周期对齐 */
vTaskDelay(pdMS_TO_TICKS(2000));
```

### 4.2 OLED 初始化移到 `main()`

```c
/* [修改] OLED 初始化移到调度器启动前；失败只打印，不再 Error_Handler */
oled_status = OLED_Init(&hi2c1);
if (oled_status == 0)
{
    OLED_Clear();
    OLED_ShowString(0, 32, "Hello");
    (void)OLED_Refresh();
}
else
{
    printf("OLED init failed, continue without display\r\n");
}
```

`vReadDataTask` 里不再有 `OLED_Init` 和 `Error_Handler()`。

### 4.3 LoRa 只初始化一次，发送后等 TX Done

`llcc68_example_send.c`：

```c
void LoRa_Init(void)
{
    if (lora_inited) return;

    LLCC68RadioEvents.TxDone = LLCC68OnTxDone;
    ...
    Radio.Init(&LLCC68RadioEvents);
    Radio.SetChannel(LORA_FRE);
    Radio.SetTxConfig(...);

    lora_inited = 1;
}

bool LoRa_Send(const char *data, uint32_t timeout_ms)
{
    LoRa_Init();

    tx_done = 0;
    tx_timeout = 0;
    Radio.Send((uint8_t *)data, (uint8_t)strlen(data));

    while (!tx_done && !tx_timeout)
    {
        if (LLCC68GetIrqStatus() != 0U) IrqFired = true;
        Radio.IrqProcess();
        if ((xTaskGetTickCount() - start) >= pdMS_TO_TICKS(timeout_ms)) break;
        vTaskDelay(pdMS_TO_TICKS(5));
    }

    return tx_done != 0;
}
```

回调：

```c
static void LLCC68OnTxDone(void)
{
    Radio.Standby();
    tx_done = 1;          /* 不再重发、不再 delay */
}

static void LLCC68OnTxTimeout(void)
{
    tx_timeout = 1;       /* 中断里不做 SPI */
}
```

### 4.4 复位、BUSY、DIO1

```c
/* [修改] 使用 HAL 的 GPIO_PIN_RESET/SET，并显式产生复位低脉冲 */
HAL_GPIO_WritePin(LORA_RESET_GPIO_Port, LORA_RESET_Pin, GPIO_PIN_RESET);
LLCC68DelayMs(20);
HAL_GPIO_WritePin(LORA_RESET_GPIO_Port, LORA_RESET_Pin, GPIO_PIN_SET);
LLCC68DelayMs(20);
```

```c
/* [修改] 原代码只清零继续等，这里 break，避免 BUSY 一直高时永久卡死 */
if (u32_count++ > 1000)
{
    printf("wait busy pin timeout\r\n");
    break;
}
```

```c
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    /* [修改] 用 EXTI 传入的引脚号判断，不再读电平；并去掉中断里的 printf */
    if (GPIO_Pin == LORA_DIO1_Pin)
    {
        if (NULL != dio1IrqCallback)
        {
            dio1IrqCallback(NULL);
        }
    }
}
```

### 4.5 TIM4 改成 1ms

```c
/* [修改] TIM4 用于 LLCC68 软件超时，改成 1ms 计数（原来 500ms） */
htim4.Init.Prescaler = 72-1;
htim4.Init.Period = 1000-1;
```

```c
/* [修改] 中断优先级从 0 改为 12，避免抢占 FreeRTOS 内核 */
HAL_NVIC_SetPriority(TIM4_IRQn, 12, 0);
```

### 4.6 `IrqFired` 加 `volatile`

```c
/* [修改] 中断里写、任务里读，必须 volatile */
volatile bool IrqFired = false;
```

---

## 5. 编译验证

使用 Keil armcc 对修改过的文件逐个编译：

```text
main.c                      0 errors
llcc68_example_send.c       0 errors
LLCC68STM32F103-board.c     0 errors
tim.c                       0 errors
gpio.c                      0 errors
usart.c                     0 errors
radio.c                     0 errors
```

只有少量“文件末尾没有换行”的警告，不影响。

> 注意：`DioIrqHandler` 在 `llcc68.h` 里是**函数类型**：
>
> ```c
> typedef void ( DioIrqHandler )( void* context );
> ```
>
> 所以 `dio1IrqCallback` 必须写成 `DioIrqHandler *`（函数指针）。  
> 之前尝试去掉 `*` 时，armcc 报：
>
> ```text
> function "dio1IrqCallback" may not be initialized
> ```
>
> 现已保留原写法。

---

## 6. 经验教训

1. **RTOS 任务必须有阻塞点。**  
   最高优先级任务如果没有 `vTaskDelay`/信号量/队列等待，会饿死所有低优先级任务。

2. **可选外设失败不要 `Error_Handler()`。**  
   OLED、DHT22 这种外设坏了，系统应该降级继续跑，而不是关中断死循环。

3. **DHT22 采样间隔 ≥2 秒。**  
   `delay_us` 只能用于位时序，不能代替任务级延时。

4. **Radio 初始化只做一次。**  
   每包都 `Radio.Init()` 会打断上一包 TX，也会让 TX Done 永远等不到。

5. **中断里不要 printf、不要做 SPI。**  
   EXTI 里 printf 会阻塞 SysTick；TIM4 里 `Radio.Rx()`/`Radio.Standby()` 会和任务里的 SPI 抢总线。  
   中断只置标志，处理放到任务里。

6. **`DioIrqHandler` 是函数类型，不是函数指针。**  
   定义变量时要写 `DioIrqHandler *`。

7. **编码和换行要保留。**  
   `llcc68_example_send.c`、`LLCC68STM32F103-board.c`、`radio.c` 是 GBK，修改时保持 GBK；否则 Keil 里中文注释会乱码。

8. **一次只改一个变量。**  
   任务延时、OLED 初始化、LoRa 发送逻辑分开改，出问题容易定位。

---

## 7. 后续验证清单

- [ ] EIDE / Keil **Rebuild 整个工程**（不是只编译修改文件）
- [ ] 看链接结果：RAM 是否够；不够就把堆从 12KB 降到 10KB，或任务栈改成 512/512/768
- [ ] 串口每秒输出一行 `Light=...,Soil=...,CO2=...,TEMP=...,HUMI=...`
- [ ] OLED 每秒刷新一次
- [ ] 网关每 2 秒收到一包 LoRa
- [ ] 拔掉 OLED 上电，系统不死机（只打印 `OLED init failed`）
- [ ] DHT22 没接时，不会疯狂刷屏（2 秒一次）
- [ ] 冷启动、热复位（不拔电）各测一次
- [ ] 如果打印 `STACK OVERFLOW: xxx`，把对应任务栈加大
- [ ] 如果 LoRa 不发：查 DIO1、BUSY、复位脚、SPI

---

## 8. 回滚方式

修改前备份在：

```text
D:\DeepSeek\log\SENSOR_Project_Core_Backup_20261023_192410
```

需要回滚时，把备份里的 `Src`、`Inc` 覆盖回：

```text
D:\STM32F103C8T6\Smart_Terminal\SENSOR_Project\Core
```

---

## 9. 本次变更文件

```text
Core/Src/main.c
Core/Inc/FreeRTOSConfig.h
Core/Src/LLCC68STM32F103-board.c
Core/Src/tim.c
Core/Src/gpio.c
Core/Src/usart.c
Core/Src/radio.c
Core/Inc/llcc68_example_send.h
Core/Src/llcc68_example_send.c
```

所有改动处均带 `[修改]` 注释，可用 Keil 的“查找”快速定位。
