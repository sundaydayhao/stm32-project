/**
 ******************************************************************************
 * @file    15_Industrial/03_State_Machine/main.c
 * @brief   工业状态机例程 / Industrial State Machine Example
 * @board   NUCLEO-F401RE
 * @note    
 *   模拟工业设备控制状态机:
 *   IDLE -> STARTING -> RUNNING -> STOPPING -> IDLE
 *                    -> ERROR -> IDLE
 *   
 *   按键操作:
 *   - 短按 (<1s): 启动/停止
 *   - 长按 (>3s): 紧急停止/复位
 ******************************************************************************
 */

#include "stm32f4xx_hal.h"
#include <stdio.h>

/* 状态定义 */
typedef enum {
    STATE_IDLE     = 0,
    STATE_STARTING = 1,
    STATE_RUNNING  = 2,
    STATE_STOPPING = 3,
    STATE_ERROR    = 4,
} MachineState_t;

/* 事件定义 */
typedef enum {
    EVENT_NONE       = 0,
    EVENT_START      = 1,
    EVENT_STOP       = 2,
    EVENT_EMERGENCY  = 3,
    EVENT_RESET      = 4,
    EVENT_TIMEOUT    = 5,
    EVENT_ERROR      = 6,
} MachineEvent_t;

const char *state_names[] = {"IDLE", "STARTING", "RUNNING", "STOPPING", "ERROR"};
const char *event_names[] = {"NONE", "START", "STOP", "EMERGENCY", "RESET", "TIMEOUT", "ERROR"};

UART_HandleTypeDef huart2;
volatile MachineState_t current_state = STATE_IDLE;
volatile MachineEvent_t pending_event = EVENT_NONE;

uint32_t state_enter_time = 0;
uint32_t run_count = 0;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_Init(void);
static void State_Enter(MachineState_t new_state);
static void State_Process(void);
static MachineEvent_t Button_GetEvent(void);

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

    printf("\r\n=== Industrial State Machine ===\r\n");
    printf("Short press: Start/Stop\r\n");
    printf("Long press (>3s): Emergency/Reset\r\n\r\n");

    State_Enter(STATE_IDLE);

    while (1)
    {
        /* 获取按键事件 */
        MachineEvent_t evt = Button_GetEvent();
        if (evt != EVENT_NONE)
        {
            pending_event = evt;
            printf("[EVENT] %s in state %s\r\n",
                   event_names[evt], state_names[current_state]);
        }

        /* 处理状态机 */
        State_Process();

        HAL_Delay(10);
    }
}

/**
 * @brief  进入新状态
 */
static void State_Enter(MachineState_t new_state)
{
    printf("[STATE] %s -> %s\r\n",
           state_names[current_state], state_names[new_state]);
    current_state = new_state;
    state_enter_time = HAL_GetTick();

    /* 进入状态时的动作 */
    switch (new_state)
    {
        case STATE_IDLE:
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
            printf("  System IDLE. Press button to start.\r\n");
            break;
        case STATE_STARTING:
            printf("  Starting up... (2 seconds)\r\n");
            break;
        case STATE_RUNNING:
            run_count++;
            printf("  System RUNNING! Run count: %lu\r\n", run_count);
            break;
        case STATE_STOPPING:
            printf("  Stopping... (1 second)\r\n");
            break;
        case STATE_ERROR:
            printf("  *** ERROR! Long press to reset. ***\r\n");
            break;
    }
}

/**
 * @brief  状态机处理
 */
static void State_Process(void)
{
    uint32_t elapsed = HAL_GetTick() - state_enter_time;
    MachineEvent_t evt = pending_event;
    pending_event = EVENT_NONE;

    switch (current_state)
    {
        case STATE_IDLE:
            if (evt == EVENT_START)
                State_Enter(STATE_STARTING);
            break;

        case STATE_STARTING:
            /* LED 快闪表示启动中 */
            if ((HAL_GetTick() / 200) % 2)
                HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
            else
                HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);

            if (elapsed >= 2000)
                State_Enter(STATE_RUNNING);
            else if (evt == EVENT_EMERGENCY)
                State_Enter(STATE_ERROR);
            break;

        case STATE_RUNNING:
            /* LED 慢闪表示运行中 */
            if ((HAL_GetTick() / 500) % 2)
                HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
            else
                HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);

            /* 每 5 秒打印运行状态 */
            if (elapsed % 5000 < 10)
                printf("  Running... elapsed: %lu s\r\n", elapsed / 1000);

            if (evt == EVENT_STOP)
                State_Enter(STATE_STOPPING);
            else if (evt == EVENT_EMERGENCY || evt == EVENT_ERROR)
                State_Enter(STATE_ERROR);
            break;

        case STATE_STOPPING:
            /* LED 常亮表示停止中 */
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
            if (elapsed >= 1000)
                State_Enter(STATE_IDLE);
            break;

        case STATE_ERROR:
            /* LED 超快闪表示错误 */
            if ((HAL_GetTick() / 100) % 2)
                HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
            else
                HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);

            if (evt == EVENT_RESET)
                State_Enter(STATE_IDLE);
            break;
    }
}

/**
 * @brief  按键事件检测
 */
static MachineEvent_t Button_GetEvent(void)
{
    static uint8_t  btn_last = 0;
    static uint32_t btn_press_time = 0;
    static uint8_t  btn_pressed = 0;

    uint8_t btn_cur = (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == GPIO_PIN_RESET) ? 1 : 0;

    if (btn_cur && !btn_last)
    {
        btn_press_time = HAL_GetTick();
        btn_pressed = 1;
    }
    else if (!btn_cur && btn_last && btn_pressed)
    {
        btn_pressed = 0;
        uint32_t duration = HAL_GetTick() - btn_press_time;

        if (duration > 3000)
        {
            /* 长按: 紧急停止或复位 */
            return (current_state == STATE_ERROR) ? EVENT_RESET : EVENT_EMERGENCY;
        }
        else if (duration > 50)
        {
            /* 短按: 启动或停止 */
            return (current_state == STATE_IDLE) ? EVENT_START : EVENT_STOP;
        }
    }

    btn_last = btn_cur;
    return EVENT_NONE;
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
    __HAL_RCC_GPIOA_CLK_ENABLE(); __HAL_RCC_GPIOC_CLK_ENABLE();
    g.Pin = GPIO_PIN_5; g.Mode = GPIO_MODE_OUTPUT_PP; g.Pull = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_LOW; HAL_GPIO_Init(GPIOA, &g);
    g.Pin = GPIO_PIN_13; g.Mode = GPIO_MODE_INPUT; g.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &g);
}