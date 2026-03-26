/**
 ******************************************************************************
 * @file    15_Industrial/02_PID_Control/main.c
 * @brief   PID 控制器例程 / PID Controller Example
 * @board   NUCLEO-F401RE
 * @note    
 *   功能:
 *   - 增量式 PID 控制器
 *   - ADC 读取反馈值 (PA0)
 *   - PWM 输出控制量 (PB10, TIM2_CH3)
 *   - 串口设置目标值和 PID 参数
 *   
 *   应用场景:
 *   - 温度控制 (PTC加热 + NTC测温)
 *   - 电机转速控制
 *   - 液位控制
 ******************************************************************************
 */

#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* PID 参数结构体 */
typedef struct {
    float Kp;           /* 比例系数 */
    float Ki;           /* 积分系数 */
    float Kd;           /* 微分系数 */
    float setpoint;     /* 目标值 */
    float output;       /* 输出值 */
    float output_min;   /* 输出下限 */
    float output_max;   /* 输出上限 */
    float integral;     /* 积分累积 */
    float prev_error;   /* 上次误差 */
    float prev_error2;  /* 上上次误差 (增量式) */
} PID_TypeDef;

/* 全局变量 */
TIM_HandleTypeDef htim2;
ADC_HandleTypeDef hadc1;
UART_HandleTypeDef huart2;

PID_TypeDef pid = {
    .Kp = 2.0f,
    .Ki = 0.1f,
    .Kd = 0.5f,
    .setpoint = 2048.0f,  /* 目标 ADC 值 (约 1.65V) */
    .output = 0.0f,
    .output_min = 0.0f,
    .output_max = 1000.0f,
    .integral = 0.0f,
    .prev_error = 0.0f,
    .prev_error2 = 0.0f
};

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_Init(void);
static void MX_ADC1_Init(void);
static void MX_TIM2_PWM_Init(void);
static float PID_Compute(PID_TypeDef *pid, float feedback);
static float PID_Compute_Incremental(PID_TypeDef *pid, float feedback);

int __io_putchar(int ch)
{
    HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART2_Init();
    MX_ADC1_Init();
    MX_TIM2_PWM_Init();

    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);

    printf("\r\n=== PID Controller Example ===\r\n");
    printf("Setpoint: %.0f | Kp:%.2f Ki:%.2f Kd:%.2f\r\n",
           pid.setpoint, pid.Kp, pid.Ki, pid.Kd);
    printf("ADC feedback on PA0, PWM output on PB10\r\n\r\n");

    uint32_t last_print = 0;
    uint32_t ctrl_tick = 0;

    while (1)
    {
        /* 控制周期: 10ms */
        if (HAL_GetTick() - ctrl_tick >= 10)
        {
            ctrl_tick = HAL_GetTick();

            /* 读取 ADC 反馈 */
            HAL_ADC_Start(&hadc1);
            HAL_ADC_PollForConversion(&hadc1, 10);
            float feedback = (float)HAL_ADC_GetValue(&hadc1);
            HAL_ADC_Stop(&hadc1);

            /* 计算 PID 输出 */
            float output = PID_Compute(&pid, feedback);

            /* 设置 PWM 占空比 */
            uint32_t ccr = (uint32_t)output;
            if (ccr > 1000) ccr = 1000;
            __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, ccr);

            /* 每 500ms 打印一次 */
            if (HAL_GetTick() - last_print >= 500)
            {
                last_print = HAL_GetTick();
                float error = pid.setpoint - feedback;
                printf("SP:%.0f FB:%.0f Err:%.1f Out:%.1f PWM:%lu%%\r\n",
                       pid.setpoint, feedback, error, output,
                       ccr / 10);
            }
        }

        HAL_Delay(1);
    }
}

/**
 * @brief  位置式 PID 计算
 * @param  pid: PID 结构体
 * @param  feedback: 反馈值
 * @retval 控制输出
 */
static float PID_Compute(PID_TypeDef *pid, float feedback)
{
    float error = pid->setpoint - feedback;

    /* 积分 */
    pid->integral += error;

    /* 积分限幅 (防止积分饱和) */
    float integral_max = pid->output_max / pid->Ki;
    if (pid->integral > integral_max)  pid->integral = integral_max;
    if (pid->integral < -integral_max) pid->integral = -integral_max;

    /* 微分 */
    float derivative = error - pid->prev_error;

    /* PID 输出 */
    pid->output = pid->Kp * error
                + pid->Ki * pid->integral
                + pid->Kd * derivative;

    /* 输出限幅 */
    if (pid->output > pid->output_max) pid->output = pid->output_max;
    if (pid->output < pid->output_min) pid->output = pid->output_min;

    pid->prev_error = error;
    return pid->output;
}

/**
 * @brief  增量式 PID 计算 (适合执行器控制)
 */
static float PID_Compute_Incremental(PID_TypeDef *pid, float feedback)
{
    float error = pid->setpoint - feedback;

    /* 增量 = Kp*(e[k]-e[k-1]) + Ki*e[k] + Kd*(e[k]-2*e[k-1]+e[k-2]) */
    float delta = pid->Kp * (error - pid->prev_error)
                + pid->Ki * error
                + pid->Kd * (error - 2*pid->prev_error + pid->prev_error2);

    pid->output += delta;

    if (pid->output > pid->output_max) pid->output = pid->output_max;
    if (pid->output < pid->output_min) pid->output = pid->output_min;

    pid->prev_error2 = pid->prev_error;
    pid->prev_error  = error;
    return pid->output;
}

static void MX_TIM2_PWM_Init(void)
{
    TIM_OC_InitTypeDef oc = {0};
    GPIO_InitTypeDef g = {0};
    __HAL_RCC_TIM2_CLK_ENABLE(); __HAL_RCC_GPIOB_CLK_ENABLE();
    g.Pin = GPIO_PIN_10; g.Mode = GPIO_MODE_AF_PP; g.Pull = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_LOW; g.Alternate = GPIO_AF1_TIM2;
    HAL_GPIO_Init(GPIOB, &g);
    htim2.Instance = TIM2; htim2.Init.Prescaler = 84 - 1;
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP; htim2.Init.Period = 1000 - 1;
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    HAL_TIM_PWM_Init(&htim2);
    oc.OCMode = TIM_OCMODE_PWM1; oc.Pulse = 0;
    oc.OCPolarity = TIM_OCPOLARITY_HIGH; oc.OCFastMode = TIM_OCFAST_DISABLE;
    HAL_TIM_PWM_ConfigChannel(&htim2, &oc, TIM_CHANNEL_3);
}

static void MX_ADC1_Init(void)
{
    GPIO_InitTypeDef g = {0};
    __HAL_RCC_ADC1_CLK_ENABLE(); __HAL_RCC_GPIOA_CLK_ENABLE();
    g.Pin = GPIO_PIN_0; g.Mode = GPIO_MODE_ANALOG; g.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &g);
    hadc1.Instance = ADC1; hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
    hadc1.Init.Resolution = ADC_RESOLUTION_12B; hadc1.Init.ScanConvMode = DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE; hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START; hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 1; hadc1.Init.DMAContinuousRequests = DISABLE;
    hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    HAL_ADC_Init(&hadc1);
    ADC_ChannelConfTypeDef s = {0};
    s.Channel = ADC_CHANNEL_0; s.Rank = 1; s.SamplingTime = ADC_SAMPLETIME_480CYCLES;
    HAL_ADC_ConfigChannel(&hadc1, &s);
}

static void MX_USART2_Init(void)
{
    huart2.Instance = USART2; huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B; huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE; huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE; huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart2);
}

void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    GPIO_InitTypeDef g = {0};
    if (huart->Instance == USART2)
    {
        __HAL_RCC_USART2_CLK_ENABLE(); __HAL_RCC_GPIOA_CLK_ENABLE();
        g.Pin = GPIO_PIN_2 | GPIO_PIN_3; g.Mode = GPIO_MODE_AF_PP;
        g.Pull = GPIO_NOPULL; g.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        g.Alternate = GPIO_AF7_USART2; HAL_GPIO_Init(GPIOA, &g);
    }
}

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef o = {0}; RCC_ClkInitTypeDef c = {0};
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);
    o.OscillatorType = RCC_OSCILLATORTYPE_HSE; o.HSEState = RCC_HSE_BYPASS;
    o.PLL.PLLState = RCC_PLL_ON; o.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    o.PLL.PLLM = 8; o.PLL.PLLN = 336; o.PLL.PLLP = RCC_PLLP_DIV4; o.PLL.PLLQ = 7;
    HAL_RCC_OscConfig(&o);
    c.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    c.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK; c.AHBCLKDivider = RCC_SYSCLK_DIV1;
    c.APB1CLKDivider = RCC_HCLK_DIV2; c.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&c, FLASH_LATENCY_2);
}

static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef g = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();
    g.Pin = GPIO_PIN_5; g.Mode = GPIO_MODE_OUTPUT_PP; g.Pull = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_LOW; HAL_GPIO_Init(GPIOA, &g);
}