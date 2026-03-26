/**
 ******************************************************************************
 * @file    01_GPIO/03_LED_Pattern/main.c
 * @brief   LED 流水灯 / LED Running Light Pattern
 * @board   STM32F4DISCOVERY (4个LED) 或 外接 LED
 * @note    
 *   硬件连接 / Hardware Connection (STM32F4DISCOVERY):
 *   - LD3: PD13 (橙色)
 *   - LD4: PD12 (绿色)
 *   - LD5: PD14 (红色)
 *   - LD6: PD15 (蓝色)
 *   
 *   外接 LED (NUCLEO-F401RE):
 *   - LED1: PA5
 *   - LED2: PA6
 *   - LED3: PA7
 *   - LED4: PB6
 ******************************************************************************
 */

#include "stm32f4xx_hal.h"

/* 使用 STM32F4DISCOVERY 的 4 个 LED */
#define LED_PORT        GPIOD
#define LED_PIN_LD3     GPIO_PIN_13  /* 橙色 */
#define LED_PIN_LD4     GPIO_PIN_12  /* 绿色 */
#define LED_PIN_LD5     GPIO_PIN_14  /* 红色 */
#define LED_PIN_LD6     GPIO_PIN_15  /* 蓝色 */
#define LED_ALL_PINS    (LED_PIN_LD3 | LED_PIN_LD4 | LED_PIN_LD5 | LED_PIN_LD6)

/* 流水灯模式 */
typedef enum {
    PATTERN_FLOW_RIGHT = 0,  /* 向右流水 */
    PATTERN_FLOW_LEFT,       /* 向左流水 */
    PATTERN_BLINK_ALL,       /* 全部闪烁 */
    PATTERN_CHASE,           /* 追逐模式 */
    PATTERN_BREATHE,         /* 呼吸效果 (需要PWM) */
    PATTERN_MAX
} LED_Pattern_t;

/* 函数声明 */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void LED_SetPattern(LED_Pattern_t pattern, uint32_t delay_ms);
static void LED_AllOff(void);
static void LED_AllOn(void);

/**
 * @brief  主函数
 */
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();

    LED_Pattern_t current_pattern = PATTERN_FLOW_RIGHT;
    uint32_t pattern_timer = 0;

    while (1)
    {
        /* 每 5 秒切换一种模式 */
        if ((HAL_GetTick() - pattern_timer) >= 5000)
        {
            pattern_timer = HAL_GetTick();
            current_pattern = (LED_Pattern_t)((current_pattern + 1) % PATTERN_MAX);
            LED_AllOff();
        }

        LED_SetPattern(current_pattern, 200);
    }
}

/**
 * @brief  关闭所有 LED
 */
static void LED_AllOff(void)
{
    HAL_GPIO_WritePin(LED_PORT, LED_ALL_PINS, GPIO_PIN_RESET);
}

/**
 * @brief  点亮所有 LED
 */
static void LED_AllOn(void)
{
    HAL_GPIO_WritePin(LED_PORT, LED_ALL_PINS, GPIO_PIN_SET);
}

/**
 * @brief  执行 LED 模式
 * @param  pattern: 模式选择
 * @param  delay_ms: 每步延时 (ms)
 */
static void LED_SetPattern(LED_Pattern_t pattern, uint32_t delay_ms)
{
    static uint8_t step = 0;

    /* LED 引脚数组 */
    const uint16_t leds[] = {LED_PIN_LD4, LED_PIN_LD3, LED_PIN_LD5, LED_PIN_LD6};
    const uint8_t  led_count = 4;

    switch (pattern)
    {
        case PATTERN_FLOW_RIGHT:
            /* 向右流水: 依次点亮每个 LED */
            LED_AllOff();
            HAL_GPIO_WritePin(LED_PORT, leds[step % led_count], GPIO_PIN_SET);
            step++;
            HAL_Delay(delay_ms);
            break;

        case PATTERN_FLOW_LEFT:
            /* 向左流水 */
            LED_AllOff();
            HAL_GPIO_WritePin(LED_PORT, leds[(led_count - 1 - step % led_count)], GPIO_PIN_SET);
            step++;
            HAL_Delay(delay_ms);
            break;

        case PATTERN_BLINK_ALL:
            /* 全部闪烁 */
            LED_AllOn();
            HAL_Delay(delay_ms);
            LED_AllOff();
            HAL_Delay(delay_ms);
            break;

        case PATTERN_CHASE:
            /* 追逐模式: 累积点亮 */
            {
                uint8_t count = step % (led_count + 1);
                LED_AllOff();
                for (uint8_t i = 0; i < count; i++)
                {
                    HAL_GPIO_WritePin(LED_PORT, leds[i], GPIO_PIN_SET);
                }
                step++;
                HAL_Delay(delay_ms);
            }
            break;

        default:
            step = 0;
            break;
    }
}

/**
 * @brief  系统时钟配置 (168MHz for STM32F407)
 */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState       = RCC_HSE_ON;  /* DISCOVERY 有外部晶振 */
    RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM       = 8;
    RCC_OscInitStruct.PLL.PLLN       = 336;
    RCC_OscInitStruct.PLL.PLLP       = RCC_PLLP_DIV2;  /* 168MHz */
    RCC_OscInitStruct.PLL.PLLQ       = 7;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                     | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5);
}

/**
 * @brief  GPIO 初始化 (PD12-PD15 为 LED)
 */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOD_CLK_ENABLE();

    GPIO_InitStruct.Pin   = LED_ALL_PINS;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);

    LED_AllOff();
}