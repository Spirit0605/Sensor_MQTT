/*!
 * \file      LLCC68-board.h
 *
 * \brief     Target board LLCC68 driver implementation
 *
 * \copyright Revised BSD License, see section \ref LICENSE.
 *
 * \code
 *                ______                              _
 *               / _____)             _              | |
 *              ( (____  _____ ____ _| |_ _____  ____| |__
 *               \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 *               _____) ) ____| | | || |_| ____( (___| | | |
 *              (______/|_____)_|_|_| \__)_____)\____)_| |_|
 *              (C)2013-2017 Semtech
 *
 * \endcode
 *
 * \author    Miguel Luis ( Semtech )
 *
 * \author    Gregory Cristian ( Semtech )
 */
#ifndef __LLCC68_BOARD_H__
#define __LLCC68_BOARD_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stdbool.h>
#include "LLCC68.h"

// 进入和退出临界区的宏；启用 RTOS 时映射到操作系统接口。
#if HAVE_OS
	#define CRITICAL_SECTION_BEGIN( )	vPortEnterCritical()
	#define CRITICAL_SECTION_END( )	vPortExitCritical()
#else
	#define CRITICAL_SECTION_BEGIN( )
	#define CRITICAL_SECTION_END( )
#endif

/*!
 * \brief Initializes the radio I/Os pins interface
 */
/* 初始化无线模块使用的板级 I/O；GPIO 外设由 CubeMX 初始化。 */
void LLCC68IoInit( void );

/*!
 * \brief Initializes DIO IRQ handlers
 *
 * \param [IN] irqHandlers Array containing the IRQ callback functions
 */
/* 保存 DIO1 中断回调，供 HAL 外部中断回调转发。 */
void LLCC68IoIrqInit( DioIrqHandler dioIrq );

/*!
 * \brief De-initializes the radio I/Os pins interface.
 *
 * \remark Useful when going in MCU low power modes
 */
/* 反初始化无线模块 I/O；低功耗退出前可调用。 */
void LLCC68IoDeInit( void );

/*!
 * \brief Initializes the radio debug pins.
 */
/* 初始化板级调试引脚；当前板级实现可不执行操作。 */
void LLCC68IoDbgInit( void );

/*!
 * \brief HW Reset of the radio
 */
/* 通过板级复位脚对无线芯片执行硬件复位。 */
void LLCC68ResetInit( void );

/*!
 * \brief Blocking loop to wait while the Busy pin in high
 */
/* 阻塞等待芯片 BUSY 引脚拉低，直到芯片可接收命令。 */
void LLCC68WaitOnBusy( void );

/*!
 * \brief Checks if the given RF frequency is supported by the hardware
 *
 * \param [IN] frequency RF frequency to be checked
 * \retval isSupported [true: supported, false: unsupported]
 */
bool LLCC68CheckRfFrequency( uint32_t frequency );

/* 调用板级毫秒延时函数。 */
void LLCC68DelayMs(uint32_t ms);

/* 初始化用于触发无线收发超时回调的板级定时器。 */
void LLCC68TimerInit(void);
/* 设置对应收发定时器的超时时长，单位为毫秒。 */
void LLCC68SetTxTimerValue(uint32_t nMs);
/* 启动或停止 TX 软件超时计时。 */
void LLCC68TxTimerStart(void);
void LLCC68TxTimerStop(void);
void LLCC68SetRxTimerValue(uint32_t nMs);
/* 启动或停止 RX 软件超时计时。 */
void LLCC68RxTimerStart(void);
void LLCC68RxTimerStop(void);

/* 控制无线芯片的 NSS 片选，并通过 SPI 收发一个字节。 */
void LLCC68SetNss(uint8_t lev );
/* 通过 SPI 发送并接收一个字节。 */
uint8_t LLCC68SpiInOut(uint8_t data);

/*!
 * Radio hardware and global parameters
 */
extern LLCC68_t LLCC68;

#ifdef __cplusplus
}
#endif

#endif // __LLCC68_BOARD_H__
