/**
 ******************************************************************************
 * @file    02_UART/02_UART_Echo/main.c
 * @brief   UART 串口回显例程 / UART Echo Example
 * @board   NUCLEO-F401RE
 * @note    
 *   功能 / Function:
 *   - 接收串口数据并回显
 *   - 支持命令解析: "led on", "led off", "led toggle"
 *   - 支持中断接收模式
 ******************************************************************************
 */

#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

/* 宏定义 */
#define RX_BUFFER_SIZE  128
#define CMD_BUFFER_SIZE 64

/* 私有变量 */
UART_HandleTypeDef huart2;
uint8_t rx_byte;                        /* 单字节接收缓冲 */
uint8_t rx_buffer[RX_BUFFER_SIZE];      /* 接收缓冲区 */
uint8_t cmd_buffer[CMD_BUFFER_SIZE];    /* 命令缓冲区 */
volatile uint8_t rx_index = 0;          /* 接收索引 */
volatile uint8_t cmd_ready = 0;         /* 命令就绪标志 */

/* 函数声明 */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void Process_Command(const char *cmd);
static void UART_SendString(const char *str);

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

    printf("\r\n=== UART Echo Example ===\r\n");
    printf("Commands:\r\n");
    printf("  led on     - Turn on LED\r\n");
    printf("  led off    - Turn off LED\r\n");
    printf("  led toggle - Toggle LED\r\n");
    printf("  status     - Show system status\r\n");
    printf("> ");

    /* 启动中断接收 (每次接收 1 字节) */
    HAL_UART_Receive_IT(&huart2, &rx_byte, 1);

    while (1)
    {
        /* 检查是否有命令就绪 */
        if (cmd_ready)
        {
            cmd_ready = 0;
            printf("\r\n");

            /* 处理命令 */
            Process_Command((char *)cmd_buffer);

            printf("> ");
        }

        /* 主循环可以做其他事情 */
        HAL_Delay(10);
    }
}

/**
 * @brief  UART 接收完成回调 (中断方式)
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        /* 回显字符 */
        HAL_UART_Transmit(&huart2, &rx_byte, 1, 100);

        if (rx_byte == '\r' || rx_byte == '\n')
        {
            /* 回车: 命令结束 */
            if (rx_index > 0)
            {
                cmd_buffer[rx_index] = '\0';
                rx_index = 0;
                cmd_ready = 1;
            }
        }
        else if (rx_byte == 0x08 || rx_byte == 0x7F)
        {
            /* 退格键 */
            if (rx_index > 0)
            {
                rx_index--;
                /* 清除终端上的字符 */
                uint8_t backspace[] = {' ', 0x08};
                HAL_UART_Transmit(&huart2, backspace, 2, 100);
            }
        }
        else if (rx_index < CMD_BUFFER_SIZE - 1)
        {
            /* 存储字符 */
            cmd_buffer[rx_index++] = rx_byte;
        }

        /* 继续接收下一字节 */
        HAL_UART_Receive_IT(&huart2, &rx_byte, 1);
    }
}

/**
 * @brief  命令处理
 * @param  cmd: 命令字符串
 */
static void Process_Command(const char *cmd)
{
    /* 转换为小写 */
    char lower_cmd[CMD_BUFFER_SIZE];
    for (int i = 0; cmd[i] && i < CMD_BUFFER_SIZE - 1; i++)
    {
        lower_cmd[i] = tolower((unsigned char)cmd[i]);
        lower_cmd[i + 1] = '\0';
    }

    if (strcmp(lower_cmd, "led on") == 0)
    {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
        printf("LED turned ON\r\n");
    }
    else if (strcmp(lower_cmd, "led off") == 0)
    {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
        printf("LED turned OFF\r\n");
    }
    else if (strcmp(lower_cmd, "led toggle") == 0)
    {
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
        printf("LED toggled\r\n");
    }
    else if (strcmp(lower_cmd, "status") == 0)
    {
        printf("=== System Status ===\r\n");
        printf("Tick:    %lu ms\r\n", HAL_GetTick());
        printf("SYSCLK:  %lu MHz\r\n", HAL_RCC_GetSysClockFreq() / 1000000);
        printf("LED:     %s\r\n", HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_5) ? "ON" : "OFF");
        printf("Button:  %s\r\n", HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) ? "Released" : "Pressed");
    }
    else if (strlen(lower_cmd) == 0)
    {
        /* 空命令 */
    }
    else
    {
        printf("Unknown command: '%s'\r\n", cmd);
        printf("Type 'led on', 'led off', 'led toggle', or 'status'\r\n");
    }
}

/**
 * @brief  发送字符串
 */
static void UART_SendString(const char *str)
{
    HAL_UART_Transmit(&huart2, (uint8_t *)str, strlen(str), 1000);
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

    /* 使能 USART2 中断 */
    HAL_NVIC_SetPriority(USART2_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART2_IRQn);
}

/**
 * @brief  USART2 中断处理函数
 */
void USART2_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart2);
}

/**
 * @brief  HAL_UART_MspInit 回调
 */
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

/**
 * @brief  GPIO 初始化
 */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitStruct.Pin   = GPIO_PIN_5;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin  = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}