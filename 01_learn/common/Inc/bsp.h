/**
 ******************************************************************************
 * @file    bsp.h
 * @brief   板级支持包头文件 / Board Support Package Header
 * @note    适用于 NUCLEO-F401RE 和 STM32F4DISCOVERY
 ******************************************************************************
 */

#ifndef __BSP_H
#define __BSP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>

/* ======================== 开发板选择 / Board Selection ======================== */
/* 取消注释以选择目标开发板 */
#define BOARD_NUCLEO_F401RE
/* #define BOARD_STM32F4DISCOVERY */
/* #define BOARD_NUCLEO_F103RB */

/* ======================== NUCLEO-F401RE 定义 ======================== */
#ifdef BOARD_NUCLEO_F401RE

/* LED 定义 */
#define LED2_PIN                GPIO_PIN_5
#define LED2_GPIO_PORT          GPIOA
#define LED2_GPIO_CLK_ENABLE()  __HAL_RCC_GPIOA_CLK_ENABLE()

/* 按键定义 (低电平有效) */
#define BTN1_PIN                GPIO_PIN_13
#define BTN1_GPIO_PORT          GPIOC
#define BTN1_GPIO_CLK_ENABLE()  __HAL_RCC_GPIOC_CLK_ENABLE()
#define BTN1_EXTI_IRQn          EXTI15_10_IRQn
#define BTN1_PRESSED            GPIO_PIN_RESET  /* 低电平有效 */

/* 调试串口 (通过 ST-Link 虚拟串口) */
#define DEBUG_UART              USART2
#define DEBUG_UART_TX_PIN       GPIO_PIN_2
#define DEBUG_UART_RX_PIN       GPIO_PIN_3
#define DEBUG_UART_GPIO_PORT    GPIOA
#define DEBUG_UART_CLK_ENABLE() __HAL_RCC_USART2_CLK_ENABLE()
#define DEBUG_UART_GPIO_CLK_ENABLE() __HAL_RCC_GPIOA_CLK_ENABLE()
#define DEBUG_UART_AF           GPIO_AF7_USART2
#define DEBUG_UART_BAUDRATE     115200

#endif /* BOARD_NUCLEO_F401RE */

/* ======================== STM32F4DISCOVERY 定义 ======================== */
#ifdef BOARD_STM32F4DISCOVERY

/* LED 定义 */
#define LED3_PIN                GPIO_PIN_13  /* 橙色 */
#define LED4_PIN                GPIO_PIN_12  /* 绿色 */
#define LED5_PIN                GPIO_PIN_14  /* 红色 */
#define LED6_PIN                GPIO_PIN_15  /* 蓝色 */
#define LED_GPIO_PORT           GPIOD
#define LED_GPIO_CLK_ENABLE()   __HAL_RCC_GPIOD_CLK_ENABLE()

/* 按键定义 (高电平有效) */
#define BTN1_PIN                GPIO_PIN_0
#define BTN1_GPIO_PORT          GPIOA
#define BTN1_GPIO_CLK_ENABLE()  __HAL_RCC_GPIOA_CLK_ENABLE()
#define BTN1_PRESSED            GPIO_PIN_SET  /* 高电平有效 */

#endif /* BOARD_STM32F4DISCOVERY */

/* ======================== 通用宏定义 ======================== */

/* 延时宏 */
#define DELAY_MS(x)             HAL_Delay(x)

/* 位操作宏 */
#define SET_BIT_VAL(REG, BIT)   ((REG) |= (BIT))
#define CLR_BIT_VAL(REG, BIT)   ((REG) &= ~(BIT))
#define TOG_BIT_VAL(REG, BIT)   ((REG) ^= (BIT))
#define GET_BIT_VAL(REG, BIT)   ((REG) & (BIT))

/* 数组大小 */
#define ARRAY_SIZE(x)           (sizeof(x) / sizeof((x)[0]))

/* 最大最小值 */
#define MAX(a, b)               ((a) > (b) ? (a) : (b))
#define MIN(a, b)               ((a) < (b) ? (a) : (b))

/* 限幅 */
#define CLAMP(val, min, max)    (MAX(MIN(val, max), min))

/* 映射 (类似 Arduino map) */
#define MAP(x, in_min, in_max, out_min, out_max) \
    ((x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min)

/* ======================== 函数声明 ======================== */

/* 系统初始化 */
void BSP_Init(void);
void SystemClock_Config(void);

/* LED 操作 */
void BSP_LED_Init(void);
void BSP_LED_On(void);
void BSP_LED_Off(void);
void BSP_LED_Toggle(void);

/* 按键操作 */
void BSP_Button_Init(void);
uint8_t BSP_Button_Read(void);

/* 调试串口 */
void BSP_UART_Init(void);
void BSP_UART_Print(const char *str);
void BSP_UART_Printf(const char *fmt, ...);

/* 错误处理 */
void Error_Handler(void);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_H */