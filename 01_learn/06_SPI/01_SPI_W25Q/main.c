/**
 ******************************************************************************
 * @file    06_SPI/01_SPI_W25Q/main.c
 * @brief   SPI W25Q Flash 读写例程 / SPI W25Q Flash Read/Write
 * @board   NUCLEO-F401RE
 * @note    
 *   硬件连接:
 *   - SPI1_SCK:  PA5 (D13), SPI1_MISO: PA6 (D12)
 *   - SPI1_MOSI: PA7 (D11), CS: PB6 (D10)
 ******************************************************************************
 */

#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <string.h>

#define W25Q_CMD_WRITE_ENABLE  0x06
#define W25Q_CMD_READ_STATUS1  0x05
#define W25Q_CMD_PAGE_PROGRAM  0x02
#define W25Q_CMD_SECTOR_ERASE  0x20
#define W25Q_CMD_READ_DATA     0x03
#define W25Q_CMD_JEDEC_ID      0x9F
#define W25Q_STATUS_BUSY       0x01

#define W25Q_CS_LOW()   HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET)
#define W25Q_CS_HIGH()  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET)

SPI_HandleTypeDef hspi1;
UART_HandleTypeDef huart2;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_SPI1_Init(void);

static uint8_t  W25Q_Transfer(uint8_t data);
static void     W25Q_WriteEnable(void);
static void     W25Q_WaitBusy(void);
static uint32_t W25Q_ReadID(void);
static void     W25Q_SectorErase(uint32_t addr);
static void     W25Q_PageProgram(uint32_t addr, uint8_t *data, uint16_t len);
static void     W25Q_ReadData(uint32_t addr, uint8_t *buf, uint32_t len);

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
    MX_SPI1_Init();

    printf("\r\n=== SPI W25Q Flash Example ===\r\n");

    uint32_t id = W25Q_ReadID();
    printf("JEDEC ID: 0x%06lX\r\n", id);
    if ((id >> 16) == 0xEF)
    {
        uint8_t cap = id & 0xFF;
        printf("Winbond W25Q, Size: %lu KB\r\n", (uint32_t)(1 << cap) / 1024);
    }

    uint8_t wbuf[256], rbuf[256];
    for (int i = 0; i < 256; i++) wbuf[i] = (uint8_t)i;

    printf("Erasing sector 0...\r\n");
    W25Q_SectorErase(0);

    printf("Writing 256 bytes...\r\n");
    W25Q_PageProgram(0, wbuf, 256);

    printf("Reading 256 bytes...\r\n");
    W25Q_ReadData(0, rbuf, 256);

    uint8_t ok = (memcmp(wbuf, rbuf, 256) == 0);
    printf("Verify: %s\r\n", ok ? "PASS" : "FAIL");

    while (1)
    {
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
        HAL_Delay(ok ? 1000 : 100);
    }
}

static uint8_t W25Q_Transfer(uint8_t data)
{
    uint8_t rx;
    HAL_SPI_TransmitReceive(&hspi1, &data, &rx, 1, 100);
    return rx;
}

static void W25Q_WriteEnable(void)
{
    W25Q_CS_LOW();
    W25Q_Transfer(W25Q_CMD_WRITE_ENABLE);
    W25Q_CS_HIGH();
}

static void W25Q_WaitBusy(void)
{
    W25Q_CS_LOW();
    W25Q_Transfer(W25Q_CMD_READ_STATUS1);
    while (W25Q_Transfer(0xFF) & W25Q_STATUS_BUSY);
    W25Q_CS_HIGH();
}

static uint32_t W25Q_ReadID(void)
{
    uint32_t id;
    W25Q_CS_LOW();
    W25Q_Transfer(W25Q_CMD_JEDEC_ID);
    id  = (uint32_t)W25Q_Transfer(0xFF) << 16;
    id |= (uint32_t)W25Q_Transfer(0xFF) << 8;
    id |= (uint32_t)W25Q_Transfer(0xFF);
    W25Q_CS_HIGH();
    return id;
}

static void W25Q_SectorErase(uint32_t addr)
{
    W25Q_WriteEnable();
    W25Q_CS_LOW();
    W25Q_Transfer(W25Q_CMD_SECTOR_ERASE);
    W25Q_Transfer((addr >> 16) & 0xFF);
    W25Q_Transfer((addr >> 8) & 0xFF);
    W25Q_Transfer(addr & 0xFF);
    W25Q_CS_HIGH();
    W25Q_WaitBusy();
}

static void W25Q_PageProgram(uint32_t addr, uint8_t *data, uint16_t len)
{
    if (len > 256) len = 256;
    W25Q_WriteEnable();
    W25Q_CS_LOW();
    W25Q_Transfer(W25Q_CMD_PAGE_PROGRAM);
    W25Q_Transfer((addr >> 16) & 0xFF);
    W25Q_Transfer((addr >> 8) & 0xFF);
    W25Q_Transfer(addr & 0xFF);
    for (uint16_t i = 0; i < len; i++) W25Q_Transfer(data[i]);
    W25Q_CS_HIGH();
    W25Q_WaitBusy();
}

static void W25Q_ReadData(uint32_t addr, uint8_t *buf, uint32_t len)
{
    W25Q_CS_LOW();
    W25Q_Transfer(W25Q_CMD_READ_DATA);
    W25Q_Transfer((addr >> 16) & 0xFF);
    W25Q_Transfer((addr >> 8) & 0xFF);
    W25Q_Transfer(addr & 0xFF);
    for (uint32_t i = 0; i < len; i++) buf[i] = W25Q_Transfer(0xFF);
    W25Q_CS_HIGH();
}

static void MX_SPI1_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_SPI1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* PA5(SCK), PA6(MISO), PA7(MOSI) */
    GPIO_InitStruct.Pin = GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* PB6: CS (GPIO Output) */
    GPIO_InitStruct.Pin = GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    W25Q_CS_HIGH();

    hspi1.Instance               = SPI1;
    hspi1.Init.Mode              = SPI_MODE_MASTER;
    hspi1.Init.Direction         = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize          = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity       = SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase          = SPI_PHASE_1EDGE;
    hspi1.Init.NSS               = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4; /* 84/4=21MHz */
    hspi1.Init.FirstBit          = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode            = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
    HAL_SPI_Init(&hspi1);
}

static void MX_USART2_UART_Init(void)
{
    huart2.Instance = USART2; huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B; huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE; huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE; huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart2);
}

void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if (huart->Instance == USART2)
    {
        __HAL_RCC_USART2_CLK_ENABLE(); __HAL_RCC_GPIOA_CLK_ENABLE();
        GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3; GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL; GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
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
    RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 8; RCC_OscInitStruct.PLL.PLLN = 336;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4; RCC_OscInitStruct.PLL.PLLQ = 7;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
}

static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitStruct.Pin = GPIO_PIN_5; GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL; GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}