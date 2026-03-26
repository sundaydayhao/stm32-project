/**
 ******************************************************************************
 * @file    12_FreeRTOS/01_Task_Basic/main.c
 * @brief   FreeRTOS 基础任务例程 / FreeRTOS Basic Task Example
 * @board   NUCLEO-F401RE
 * @note    需要 FreeRTOS 源码 (STM32CubeF4 Middlewares/Third_Party/FreeRTOS)
 *   功能: Task1-LED闪烁, Task2-串口监控, Task3-按键检测
 ******************************************************************************
 */

#include "stm32f4xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

TaskHandle_t xTask1Handle = NULL;
TaskHandle_t xTask2Handle = NULL;
TaskHandle_t xTask3Handle = NULL;
SemaphoreHandle_t xUartMutex = NULL;
UART_HandleTypeDef huart2;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void Task_LED(void *p);
static void Task_Monitor(void *p);
static void Task_Button(void *p);
static void RTOS_Printf(const char *fmt, ...);

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
    MX_USART2_UART_Init();

    printf("\r\n=== FreeRTOS Basic Task Example ===\r\n");

    xUartMutex = xSemaphoreCreateMutex();
    xTaskCreate(Task_LED,     "LED",     128, NULL, 2, &xTask1Handle);
    xTaskCreate(Task_Monitor, "Monitor", 256, NULL, 1, &xTask2Handle);
    xTaskCreate(Task_Button,  "Button",  128, NULL, 3, &xTask3Handle);

    vTaskStartScheduler();
    while (1);
}

static void Task_LED(void *p)
{
    (void)p;
    TickType_t xLast = xTaskGetTickCount();
    while (1)
    {
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
        vTaskDelayUntil(&xLast, pdMS_TO_TICKS(500));
    }
}

static void Task_Monitor(void *p)
{
    (void)p;
    uint32_t count = 0;
    while (1)
    {
        count++;
        RTOS_Printf("[Monitor] Tick:%lu Count:%lu Heap:%u\r\n",
                    xTaskGetTickCount(), count,
                    (unsigned int)xPortGetFreeHeapSize());
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void Task_Button(void *p)
{
    (void)p;
    uint8_t last = 0;
    uint32_t cnt = 0;
    while (1)
    {
        uint8_t cur = (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == GPIO_PIN_RESET) ? 1 : 0;
        if (cur && !last) { cnt++; RTOS_Printf("[Button] Press#%lu\r\n", cnt); }
        last = cur;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

static void RTOS_Printf(const char *fmt, ...)
{
    char buf[128];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    if (xUartMutex && xSemaphoreTake(xUartMutex, pdMS_TO_TICKS(100)) == pdTRUE)
    {
        HAL_UART_Transmit(&huart2, (uint8_t *)buf, strlen(buf), 1000);
        xSemaphoreGive(xUartMutex);
    }
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    printf("Stack overflow: %s\r\n", pcTaskName);
    while (1);
}

void vApplicationMallocFailedHook(void) { printf("Malloc failed!\r\n"); while (1); }

void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    GPIO_InitTypeDef g = {0};
    if (huart->Instance == USART2)
    {
        __HAL_RCC_USART2_CLK_ENABLE(); __HAL_RCC_GPIOA_CLK_ENABLE();
        g.Pin = GPIO_PIN_2 | GPIO_PIN_3; g.Mode = GPIO_MODE_AF_PP;
        g.Pull = GPIO_NOPULL; g.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        g.Alternate = GPIO_AF7_USART2;
        HAL_GPIO_Init(GPIOA, &g);
    }
}

static void MX_USART2_UART_Init(void)
{
    huart2.Instance = USART2; huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B; huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE; huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE; huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart2);
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