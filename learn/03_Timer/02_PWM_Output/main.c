/**
 ******************************************************************************
 * @file    03_Timer/02_PWM_Output/main.c
 * @brief   PWM 输出例程 / PWM Output Example
 * @board   NUCLEO-F401RE
 * @note    
 *   硬件连接 / Hardware Connection:
 *   - PWM 输出: PB10 (TIM2_CH3, Arduino D6)
 *   - 可接 LED 或舵机
 *   
 *   PWM 参数 / PWM Parameters:
 *   - 频率: 1KHz (周期 1ms)
 *   - 占空比: 0% ~ 100% 可调
 *   
 *   功能 / Function:
 *   - 呼吸灯效果 (占空比渐变)
 *   - 舵机控制 (50Hz, 1ms~2ms 脉宽)
 ******************************************************************************
 */

#include "stm32f4xx_hal.h"
#include <stdio.h>

/* 宏定义 */
/* PWM 频率 = TIM_CLK / (PSC+1) / (ARR+1) */
/* TIM2_CLK = 84MHz, PSC=84-1, ARR=1000-1 -> 1KHz */
#define PWM_FREQ_HZ     1000
#define PWM_PERIOD      (1000 - 1)   /* ARR 值 */
#define PWM_PRESCALER   (84 - 1)     /* PSC 值 */

/* 舵机参数 (50Hz) */
/* TIM2_CLK = 84MHz, PSC=84-1, ARR=20000-1 -> 50Hz */
#define SERVO_FREQ_HZ   50
#define SERVO_PERIOD    (20000 - 1)
#define SERVO_PRESCALER (84 - 1)
#define SERVO_MIN_PULSE 1000   /* 1ms = 0度 */
#define SERVO_MAX_PULSE 2000   /* 2ms = 180度 */

/* 私有变量 */
TIM_HandleTypeDef htim2;
UART_HandleTypeDef huart2;

/* 函数声明 */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM2_PWM_Init(void);
static void PWM_SetDutyCycle(uint32_t duty_percent);
static void Servo_SetAngle(uint8_t angle);

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
    MX_TIM2_PWM_Init();

    printf("\r\n=== PWM Output Example ===\r\n");
    printf("PB10 (TIM2_CH3): PWM 1KHz\r\n\r\n");

    /* 启动 PWM 输出 */
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);

    uint32_t duty = 0;
    int8_t   direction = 1;  /* 1: 增加, -1: 减少 */

    while (1)
    {
        /* 呼吸灯效果: 占空比 0% -> 100% -> 0% */
        PWM_SetDutyCycle(duty);

        printf("PWM Duty: %3lu%%\r\n", duty);

        duty += direction * 5;

        if (duty >= 100)
        {
            duty = 100;
            direction = -1;
        }
        else if (duty == 0)
        {
            direction = 1;
        }

        HAL_Delay(50);
    }
}

/**
 * @brief  设置 PWM 占空比
 * @param  duty_percent: 占空比 (0-100)
 */
static void PWM_SetDutyCycle(uint32_t duty_percent)
{
    if (duty_percent > 100) duty_percent = 100;

    /* CCR = ARR * duty / 100 */
    uint32_t ccr = (PWM_PERIOD + 1) * duty_percent / 100;
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, ccr);
}

/**
 * @brief  设置舵机角度
 * @param  angle: 角度 (0-180)
 * @note   需要将 TIM2 配置为 50Hz
 */
static void Servo_SetAngle(uint8_t angle)
{
    if (angle > 180) angle = 180;

    /* 脉宽 = 1ms + angle/180 * 1ms */
    /* CCR = pulse_us (因为 PSC=84-1, 每个计数 = 1us) */
    uint32_t pulse = SERVO_MIN_PULSE + (uint32_t)angle * (SERVO_MAX_PULSE - SERVO_MIN_PULSE) / 180;
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, pulse);
}

/**
 * @brief  TIM2 PWM 初始化 (1KHz, CH3 -> PB10)
 */
static void MX_TIM2_PWM_Init(void)
{
    TIM_OC_InitTypeDef sConfigOC = {0};
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* 使能时钟 */
    __HAL_RCC_TIM2_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* PB10: TIM2_CH3 复用功能 */
    GPIO_InitStruct.Pin       = GPIO_PIN_10;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* 配置 TIM2 基础参数 */
    htim2.Instance               = TIM2;
    htim2.Init.Prescaler         = PWM_PRESCALER;
    htim2.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim2.Init.Period            = PWM_PERIOD;
    htim2.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    HAL_TIM_PWM_Init(&htim2);

    /* 配置 PWM 通道 3 */
    sConfigOC.OCMode       = TIM_OCMODE_PWM1;
    sConfigOC.Pulse        = 0;  /* 初始占空比 0% */
    sConfigOC.OCPolarity   = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode   = TIM_OCFAST_DISABLE;
    HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_3);
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

/**
 * @brief  系统时钟配置 (84MHz)
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