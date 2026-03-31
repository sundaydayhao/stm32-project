/**
 ******************************************************************************
 * @file    01_GPIO/02_Button_LED/main.c
 * @brief   按键控制 LED 例程 / Button Control LED Example
 * @board   NUCLEO-F401RE
 * @note    
 *   硬件连接 / Hardware Connection:
 *   - LED:    PA5 (NUCLEO LD2)
 *   - Button: PC13 (NUCLEO B1, 低电平有效)
 *   
 *   现象 / Phenomenon:
 *   - 按下 B1 按键: LED 点亮
 *   - 释放 B1 按键: LED 熄灭
 *   - 长按 2 秒: LED 快速闪烁
 ******************************************************************************
 */

#include "stm32f4xx_hal.h"

/* 宏定义 */
#define LED_PIN         GPIO_PIN_5
#define LED_PORT        GPIOA
#define BTN_PIN         GPIO_PIN_13
#define BTN_PORT        GPIOC
#define BTN_PRESSED     GPIO_PIN_RESET  /* 低电平有效 */

/* 函数声明 */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static uint8_t Button_GetState(void);
static uint8_t Button_Debounce(void);

/**
 * @brief  主函数
 */
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();

    uint32_t press_time = 0;  /* 按键按下时间 */
    uint8_t  last_state = 0;  /* 上次按键状态 */

    while (1)
    {
        uint8_t btn_state = Button_Debounce();

        if (btn_state)
        {
            /* 按键按下 */
            HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);

            /* 计算按下时长 */
            if (!last_state)
            {
                press_time = HAL_GetTick();
            }

            /* 长按 2 秒: LED 快速闪烁 */
            if ((HAL_GetTick() - press_time) > 2000)
            {
                HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
                HAL_Delay(100);
            }
        }
        else
        {
            /* 按键释放 */
            HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);
            press_time = 0;
        }

        last_state = btn_state;
        HAL_Delay(10);
    }
}

/**
 * @brief  读取按键原始状态
 * @retval 1: 按下, 0: 释放
 */
static uint8_t Button_GetState(void)
{
    return (HAL_GPIO_ReadPin(BTN_PORT, BTN_PIN) == BTN_PRESSED) ? 1 : 0;
}

/**
 * @brief  按键消抖 (软件消抖, 连续读取 3 次)
 * @retval 1: 按下, 0: 释放
 */
static uint8_t Button_Debounce(void)
{
    static uint8_t count = 0;
    static uint8_t state = 0;

    uint8_t raw = Button_GetState();

    if (raw != state)
    {
        count++;
        if (count >= 3)  /* 连续 3 次相同才确认 */
        {
            state = raw;
            count = 0;
        }
    }
    else
    {
        count = 0;
    }

    return state;
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

/**
 * @brief  GPIO 初始化
 */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* 使能时钟 */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* LED: PA5 推挽输出 */
    GPIO_InitStruct.Pin   = LED_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);

    /* 按键: PC13 输入 (板载上拉电阻) */
    GPIO_InitStruct.Pin  = BTN_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;  /* 板载已有上拉 */
    HAL_GPIO_Init(BTN_PORT, &GPIO_InitStruct);
}