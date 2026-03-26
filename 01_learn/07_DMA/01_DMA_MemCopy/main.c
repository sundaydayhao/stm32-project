/**
 ******************************************************************************
 * @file    07_DMA/01_DMA_MemCopy/main.c
 * @brief   DMA 内存到内存传输例程 / DMA Memory-to-Memory Transfer
 * @board   NUCLEO-F401RE
 * @note    
 *   功能:
 *   - DMA2 Stream0 内存到内存传输
 *   - 对比 DMA 传输与 CPU memcpy 的速度
 *   - 传输完成中断回调
 ******************************************************************************
 */

#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <string.h>

#define BUF_SIZE    1024

DMA_HandleTypeDef hdma;
UART_HandleTypeDef huart2;

static uint32_t src_buf[BUF_SIZE];
static uint32_t dst_buf[BUF_SIZE];
volatile uint8_t dma_done = 0;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_Init(void);
static void MX_DMA_Init(void);

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
    MX_DMA_Init();

    printf("\r\n=== DMA Memory-to-Memory Transfer ===\r\n");
    printf("Buffer size: %d x 4 bytes = %d bytes\r\n\r\n", BUF_SIZE, BUF_SIZE * 4);

    /* 填充源缓冲区 */
    for (int i = 0; i < BUF_SIZE; i++) src_buf[i] = i;

    /* --- 测试 1: CPU memcpy --- */
    memset(dst_buf, 0, sizeof(dst_buf));
    uint32_t t1 = HAL_GetTick();
    memcpy(dst_buf, src_buf, sizeof(src_buf));
    uint32_t cpu_time = HAL_GetTick() - t1;
    uint8_t cpu_ok = (memcmp(src_buf, dst_buf, sizeof(src_buf)) == 0);
    printf("CPU memcpy: %lu ms, verify: %s\r\n", cpu_time, cpu_ok ? "OK" : "FAIL");

    /* --- 测试 2: DMA 传输 --- */
    memset(dst_buf, 0, sizeof(dst_buf));
    dma_done = 0;
    uint32_t t2 = HAL_GetTick();
    HAL_DMA_Start_IT(&hdma, (uint32_t)src_buf, (uint32_t)dst_buf, BUF_SIZE);

    /* 等待 DMA 完成 */
    while (!dma_done) { /* CPU 可以做其他事 */ }
    uint32_t dma_time = HAL_GetTick() - t2;
    uint8_t dma_ok = (memcmp(src_buf, dst_buf, sizeof(src_buf)) == 0);
    printf("DMA transfer: %lu ms, verify: %s\r\n", dma_time, dma_ok ? "OK" : "FAIL");

    printf("\r\nDMA frees CPU during transfer!\r\n");

    while (1)
    {
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
        HAL_Delay(500);
    }
}

void HAL_DMA_XferCpltCallback(DMA_HandleTypeDef *hdma_cb)
{
    (void)hdma_cb;
    dma_done = 1;
}

void DMA2_Stream0_IRQHandler(void) { HAL_DMA_IRQHandler(&hdma); }

static void MX_DMA_Init(void)
{
    __HAL_RCC_DMA2_CLK_ENABLE();
    hdma.Instance                 = DMA2_Stream0;
    hdma.Init.Channel             = DMA_CHANNEL_0;
    hdma.Init.Direction           = DMA_MEMORY_TO_MEMORY;
    hdma.Init.PeriphInc           = DMA_PINC_ENABLE;
    hdma.Init.MemInc              = DMA_MINC_ENABLE;
    hdma.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    hdma.Init.MemDataAlignment    = DMA_MDATAALIGN_WORD;
    hdma.Init.Mode                = DMA_NORMAL;
    hdma.Init.Priority            = DMA_PRIORITY_HIGH;
    hdma.Init.FIFOMode            = DMA_FIFOMODE_ENABLE;
    hdma.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
    hdma.Init.MemBurst            = DMA_MBURST_INC4;
    hdma.Init.PeriphBurst         = DMA_PBURST_INC4;
    HAL_DMA_Init(&hdma);
    hdma.XferCpltCallback = HAL_DMA_XferCpltCallback;
    HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);
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