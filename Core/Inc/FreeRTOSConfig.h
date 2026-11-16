#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/*===========================================================
 * FreeRTOS V9.0.0 配置文件 (适用于 STM32F1 系列)
 *===========================================================*/

/* 1. 核心调度与时钟配置 */
#define configUSE_PREEMPTION                  1    // 1=使用抢占式调度器；0=协作式调度器
#define configUSE_IDLE_HOOK                   0    // 1=启用空闲任务钩子(通常用于低功耗)，0=禁用
#define configUSE_TICK_HOOK                   0    // 1=启用系统节拍钩子，0=禁用
#define configCPU_CLOCK_HZ                    ( ( unsigned long ) 72000000 ) // 系统内核时钟频率(72MHz)
#define configTICK_RATE_HZ                    ( ( TickType_t ) 1000 )       // 系统节拍频率 1000Hz (即 1 Tick = 1ms)
#define configMAX_PRIORITIES                  ( 5 )  // 最大任务优先级数量(可设置 0 ~ 4 级)
#define configMINIMAL_STACK_SIZE              ( ( unsigned short ) 128 ) // 空闲任务的最小栈大小(单位:字，128字=512字节)
#define configMAX_TASK_NAME_LEN               ( 16 ) // 任务名字符串的最大长度(含结束符 \0)
#define configUSE_16_BIT_TICKS                0      // 0=Tick计数器使用32位；1=使用16位(为了兼容8/16位老架构)
#define configIDLE_SHOULD_YIELD               1      // 1=空闲任务会让出CPU给同等优先级的用户任务；0=不让出
#define configUSE_TRACE_FACILITY              0      // 1=启用可视化跟踪调试功能；0=禁用(节省资源)
#define configUSE_CO_ROUTINES                 0      // 1=启用协程功能；0=禁用(通常不用)
#define configMAX_CO_ROUTINE_PRIORITIES       ( 2 )  // 协程的最大优先级数量

/* 2. 内存分配配置 */
#define configSUPPORT_DYNAMIC_ALLOCATION      1      // 1=允许动态内存分配(如使用 xTaskCreate)
#define configTOTAL_HEAP_SIZE                 ( ( size_t ) ( 12 * 1024 ) ) // [修改] 8KB -> 12KB，容纳 3 个 768 word 任务
#define configUSE_MUTEXES                     1 // [修改] 预留打印互斥量

/* 3. API 函数裁剪 (1=包含该函数，0=不包含以节省空间) */
#define INCLUDE_vTaskPrioritySet              1 // 包含动态修改任务优先级的 API
#define INCLUDE_uxTaskPriorityGet             1 // 包含获取任务优先级的 API
#define INCLUDE_vTaskDelete                   1 // 包含删除任务的 API
#define INCLUDE_vTaskCleanUpResources         0 // 不包含清除任务资源的 API(通常用于静态创建)
#define INCLUDE_vTaskSuspend                  1 // 包含挂起/恢复任务的 API
#define INCLUDE_vTaskDelayUntil               1 // 包含绝对延时(周期性执行)的 API
#define INCLUDE_vTaskDelay                    1 // 包含相对延时(阻塞指定 Tick)的 API
#define INCLUDE_uxTaskGetStackHighWaterMark   0 // 包含获取任务历史最小栈剩余空间的 API(调试用)
#define INCLUDE_xTaskGetSchedulerState        1 // 包含获取当前调度器状态的 API (SysTick 防卡死必开)

/* 4. 中断优先级配置 (Cortex-M3/M4 关键配置) 
 * 注意：数值越小，硬件优先级越高！ */
#define configKERNEL_INTERRUPT_PRIORITY       255 // 内核使用的最高优先级(SysTick/PendSV最低，即 0xFF)
// 允许调用 FreeRTOS API 的最高中断优先级(数值 191 即 0xBF，通常对应 NVIC 的优先级 11)
// 注意：硬件优先级高于 11 的中断(如 0~10)，绝对不允许调用任何 FreeRTOS 的 API！
#define configMAX_SYSCALL_INTERRUPT_PRIORITY  191 
// 对应 ST HAL 库的 NVIC 优先级配置(通常为 15 级，最低)
#define configLIBRARY_KERNEL_INTERRUPT_PRIORITY 15

/* 5. 异常处理向量映射 (重定向) 
 * 将 FreeRTOS 内核的汇编处理函数，重命名对接给启动文件 startup_xxx.s 中的中断向量 */
#define vPortSVCHandler                       SVC_Handler
#define xPortPendSVHandler                    PendSV_Handler

/* 6. 钩子函数与断言宏 (调试相关) */
// 内核发现非法操作(如任务函数中途 return、在中断中调用 vTaskDelay)时触发此宏
// 这里配置为关闭中断并死循环，方便调试器抓停在此处
#define configASSERT( x ) if( ( x ) == 0 ) { taskDISABLE_INTERRUPTS(); for( ;; ); }

#define configCHECK_FOR_STACK_OVERFLOW        2 // [修改] 打开栈溢出检测
#define configUSE_MALLOC_FAILED_HOOK          0 // [修改] 保持关闭，避免缺少钩子函数

#endif /* FREERTOS_CONFIG_H */