/**
 ******************************************************************************
 * @file    bsp.c
 * @brief   板级支持包实现 / Board Support Package Implementation
 * @note    HAL 库文件需要从 STM32CubeF4 固件包中获取
 *          The HAL library files must be obtained from STM32CubeF4 firmware package
 ******************************************************************************
 */

#include "bsp.h"
#include <stdarg.h>

/* 私有变量 */
static UART_HandleTypeDef huart_debug;

/**
 * @brief  系统时钟配置 (NUCLEO-F401RE: 84MHz)
 *         HSE(8MHz) -> PLL -> SYSCLK(84MHz)
 */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /* 配置主内部调压器输出电压 */
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

    /* 初始化 RCC 振荡器 */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;  /* NUCLEO 使用 ST-Link MCO */
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 8;
    RCC_OscInitStruct.PLL.PLLN = 336;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;  /* 84MHz */
    RCC_OscInitStruct.PLL.PLLQ = 7;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    /* 初始化 CPU, AHB, APB 总线时钟 */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;   /* HCLK = 84MHz */
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;    /* APB1 = 42MHz */
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;    /* APB2 = 84MHz */

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
 * @brief  BSP 初始化
 */
void BSP_Init(void)
{
    HAL_Init();
    SystemClock_Config();
    BSP_LED_Init();
    BSP_Button_Init();
    BSP_UART_Init();
}

/**
 * @brief  LED 初始化
 */
void BSP_LED_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    LED2_GPIO_CLK_ENABLE();

    GPIO_InitStruct.Pin   = LED2_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED2_GPIO_PORT, &GPIO_InitStruct);

    /* 默认关闭 LED */
    HAL_GPIO_WritePin(LED2_GPIO_PORT, LED2_PIN, GPIO_PIN_RESET);
}

/**
 * @brief  点亮 LED
 */
void BSP_LED_On(void)
{
    HAL_GPIO_WritePin(LED2_GPIO_PORT, LED2_PIN, GPIO_PIN_SET);
}

/**
 * @brief  关闭 LED
 */
void BSP_LED_Off(void)
{
    HAL_GPIO_WritePin(LED2_GPIO_PORT, LED2_PIN, GPIO_PIN_RESET);
}

/**
 * @brief  翻转 LED
 */
void BSP_LED_Toggle(void)
{
    HAL_GPIO_TogglePin(LED2_GPIO_PORT, LED2_PIN);
}

/**
 * @brief  按键初始化
 */
void BSP_Button_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    BTN1_GPIO_CLK_ENABLE();

    GPIO_InitStruct.Pin  = BTN1_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;  /* 板载已有上拉电阻 */
    HAL_GPIO_Init(BTN1_GPIO_PORT, &GPIO_InitStruct);
}

/**
 * @brief  读取按键状态
 * @retval 1: 按键按下, 0: 按键释放
 */
uint8_t BSP_Button_Read(void)
{
    /* NUCLEO B1: 低电平有效 */
    return (HAL_GPIO_ReadPin(BTN1_GPIO_PORT, BTN1_PIN) == BTN1_PRESSED) ? 1 : 0;
}

/**
 * @brief  调试串口初始化 (USART2, 115200, 8N1)
 */
void BSP_UART_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* 使能时钟 */
    DEBUG_UART_GPIO_CLK_ENABLE();
    DEBUG_UART_CLK_ENABLE();

    /* 配置 GPIO */
    GPIO_InitStruct.Pin       = DEBUG_UART_TX_PIN | DEBUG_UART_RX_PIN;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = DEBUG_UART_AF;
    HAL_GPIO_Init(DEBUG_UART_GPIO_PORT, &GPIO_InitStruct);

    /* 配置 UART */
    huart_debug.Instance          = DEBUG_UART;
    huart_debug.Init.BaudRate     = DEBUG_UART_BAUDRATE;
    huart_debug.Init.WordLength   = UART_WORDLENGTH_8B;
    huart_debug.Init.StopBits     = UART_STOPBITS_1;
    huart_debug.Init.Parity       = UART_PARITY_NONE;
    huart_debug.Init.Mode         = UART_MODE_TX_RX;
    huart_debug.Init.HwFlowCtl   = UART_HWCONTROL_NONE;
    huart_debug.Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_UART_Init(&huart_debug) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
 * @brief  串口打印字符串
 * @param  str: 字符串指针
 */
void BSP_UART_Print(const char *str)
{
    HAL_UART_Transmit(&huart_debug, (uint8_t *)str, strlen(str), 1000);
}

/**
 * @brief  串口格式化打印 (类似 printf)
 * @param  fmt: 格式字符串
 */
void BSP_UART_Printf(const char *fmt, ...)
{
    char buf[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    BSP_UART_Print(buf);
}

/**
 * @brief  printf 重定向到 UART (需要在 IDE 中启用 syscalls)
 */
int __io_putchar(int ch)
{
    HAL_UART_Transmit(&huart_debug, (uint8_t *)&ch, 1, 1000);
    return ch;
}

/**
 * @brief  错误处理函数
 * @note   发生错误时 LED 快速闪烁
 */
void Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
        HAL_GPIO_TogglePin(LED2_GPIO_PORT, LED2_PIN);
        HAL_Delay(100);
    }
}

/**
 * @brief  断言失败处理
 */
#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
    BSP_UART_Printf("Assert failed: file %s, line %lu\r\n", file, line);
    Error_Handler();
}
#endif