/**
 ******************************************************************************
 * @file    01_GPIO/01_LED_Blink/main.c
 * @brief   LED 闪烁例程 / LED Blink Example
 * @board   NUCLEO-F401RE
 * @note    
 *   硬件连接 / Hardware Connection:
 *   - LED: PA5 (NUCLEO LD2, 板载绿色 LED)
 *   
 *   现象 / Phenomenon:
 *   - LD2 绿色 LED 以 500ms 间隔闪烁
 ******************************************************************************
 */

#include "stm32f4xx_hal.h"

/* 函数声明 */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);

/**
 * @brief  主函数
 */
int main(void)
{
    /* HAL 库初始化 */
    HAL_Init();

    /* 配置系统时钟 84MHz */
    SystemClock_Config();

    /* 初始化 GPIO */
    MX_GPIO_Init();

    /* 主循环 */
    while (1)
    {
        /* 翻转 LED (PA5) */
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);

        /* 延时 500ms */
        HAL_Delay(500);
    }
}

/**
 * @brief  系统时钟配置
 *         SYSCLK = 84MHz (HSE 8MHz -> PLL)
 */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState       = RCC_HSE_BYPASS;
    RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM       = 8;
    RCC_OscInitStruct.PLL.PLLN       = 336;
    RCC_OscInitStruct.PLL.PLLP       = RCC_PLLP_DIV4;
    RCC_OscInitStruct.PLL.PLLQ       = 7;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                     | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
}

/**
 * @brief  GPIO 初始化
 *         PA5: 推挽输出 (LED)
 */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* 使能 GPIOA 时钟 */
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* 配置 PA5 为推挽输出 */
    GPIO_InitStruct.Pin   = GPIO_PIN_5;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;  /* 推挽输出 */
    GPIO_InitStruct.Pull  = GPIO_NOPULL;           /* 无上下拉 */
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;   /* 低速 */
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* 初始状态: LED 关闭 */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
}