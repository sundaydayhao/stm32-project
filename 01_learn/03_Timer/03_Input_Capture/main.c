/**
 ******************************************************************************
 * @file    03_Timer/03_Input_Capture/main.c
 * @brief   输入捕获测量频率和占空比 / Input Capture - Measure Frequency & Duty
 * @board   NUCLEO-F401RE
 * @note    
 *   硬件连接 / Hardware Connection:
 *   - 输入信号: PA0 (TIM5_CH1, Arduino A0)
 *   - 可将 PB10 (PWM输出) 连接到 PA0 进行测试
 *   
 *   原理 / Principle:
 *   - 使用 TIM5 输入捕获模式
 *   - CH1 捕获上升沿, CH2 捕获下降沿
 *   - 计算周期和高电平时间 -> 频率和占空比
 ******************************************************************************
 */

#include "stm32f4xx_hal.h"
#include <stdio.h>

/* 私有变量 */
TIM_HandleTypeDef htim5;
UART_HandleTypeDef huart2;

volatile uint32_t capture_rise  = 0;   /* 上升沿捕获值 */
volatile uint32_t capture_fall  = 0;   /* 下降沿捕获值 */
volatile uint32_t period        = 0;   /* 周期 (计数值) */
volatile uint32_t high_time     = 0;   /* 高电平时间 (计数值) */
volatile uint8_t  capture_state = 0;   /* 捕获状态 */
volatile uint8_t  data_ready    = 0;   /* 数据就绪标志 */

/* 函数声明 */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM5_IC_Init(void);

/* printf 重定向 */
int __io_putchar(int ch)
{
    HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}

/**
 * @brief  主函数
 */
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART2_UART_Init();
    MX_TIM5_IC_Init();

    printf("\r\n=== Input Capture Example ===\r\n");
    printf("Input: PA0 (TIM5_CH1)\r\n");
    printf("Measuring frequency and duty cycle...\r\n\r\n");

    /* 启动输入捕获 */
    HAL_TIM_IC_Start_IT(&htim5, TIM_CHANNEL_1);
    HAL_TIM_IC_Start_IT(&htim5, TIM_CHANNEL_2);

    while (1)
    {
        if (data_ready)
        {
            data_ready = 0;

            /* TIM5 时钟 = 84MHz, PSC = 84-1 -> 1MHz (1us/count) */
            float freq     = 0.0f;
            float duty     = 0.0f;

            if (period > 0)
            {
                freq = 1000000.0f / period;  /* Hz */
                duty = (float)high_time / period * 100.0f;  /* % */
            }

            printf("Period: %lu us | Freq: %.1f Hz | Duty: %.1f%%\r\n",
                   period, freq, duty);
        }

        HAL_Delay(500);
    }
}

/**
 * @brief  输入捕获回调
 */
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM5)
    {
        if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1)
        {
            /* 上升沿捕获 */
            capture_rise = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);

            if (capture_state == 1)
            {
                /* 计算周期 */
                if (capture_rise >= capture_fall)
                    period = capture_rise - capture_fall + high_time;
                else
                    period = (0xFFFFFFFF - capture_fall) + capture_rise + high_time;

                data_ready = 1;
            }
            capture_state = 1;
        }
        else if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2)
        {
            /* 下降沿捕获 */
            capture_fall = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_2);

            if (capture_state == 1)
            {
                /* 计算高电平时间 */
                if (capture_fall >= capture_rise)
                    high_time = capture_fall - capture_rise;
                else
                    high_time = (0xFFFFFFFF - capture_rise) + capture_fall;
            }
        }
    }
}

/**
 * @brief  TIM5 输入捕获初始化
 *         CH1: 上升沿, CH2: 下降沿 (PWM 输入模式)
 */
static void MX_TIM5_IC_Init(void)
{
    TIM_IC_InitTypeDef sConfigIC = {0};
    TIM_SlaveConfigTypeDef sSlaveConfig = {0};
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* 使能时钟 */
    __HAL_RCC_TIM5_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* PA0: TIM5_CH1 */
    GPIO_InitStruct.Pin       = GPIO_PIN_0;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_PULLDOWN;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF2_TIM5;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* TIM5 基础配置 */
    htim5.Instance               = TIM5;
    htim5.Init.Prescaler         = 84 - 1;      /* 84MHz / 84 = 1MHz */
    htim5.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim5.Init.Period            = 0xFFFFFFFF;   /* 32位最大值 */
    htim5.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim5.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    HAL_TIM_IC_Init(&htim5);

    /* CH1: 上升沿捕获 */
    sConfigIC.ICPolarity  = TIM_INPUTCHANNELPOLARITY_RISING;
    sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
    sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
    sConfigIC.ICFilter    = 0;
    HAL_TIM_IC_ConfigChannel(&htim5, &sConfigIC, TIM_CHANNEL_1);

    /* CH2: 下降沿捕获 (间接输入, 来自 TI1) */
    sConfigIC.ICPolarity  = TIM_INPUTCHANNELPOLARITY_FALLING;
    sConfigIC.ICSelection = TIM_ICSELECTION_INDIRECTTI;
    HAL_TIM_IC_ConfigChannel(&htim5, &sConfigIC, TIM_CHANNEL_2);

    /* 配置中断 */
    HAL_NVIC_SetPriority(TIM5_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(TIM5_IRQn);
}

/**
 * @brief  TIM5 中断处理
 */
void TIM5_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&htim5);
}

/**
 * @brief  USART2 初始化
 */
static void MX_USART2_UART_Init(void)
{
    huart2.Instance          = USART2;
    huart2.Init.BaudRate     = 115200;
    huart2.Init.WordLength   = UART_WORDLENGTH_8B;
    huart2.Init.StopBits     = UART_STOPBITS_1;
    huart2.Init.Parity       = UART_PARITY_NONE;
    huart2.Init.Mode         = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl   = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart2);
}

void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if (huart->Instance == USART2)
    {
        __HAL_RCC_USART2_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();
        GPIO_InitStruct.Pin       = GPIO_PIN_2 | GPIO_PIN_3;
        GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull      = GPIO_NOPULL;
        GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }
}

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

static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitStruct.Pin   = GPIO_PIN_5;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}