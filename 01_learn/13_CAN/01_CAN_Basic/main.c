/**
 ******************************************************************************
 * @file    13_CAN/01_CAN_Basic/main.c
 * @brief   CAN 总线基础例程 / CAN Bus Basic Example
 * @board   NUCLEO-F401RE
 * @note    
 *   硬件连接 (需要 CAN 收发器, 如 TJA1050):
 *   - CAN1_TX: PB9 (或 PA12)
 *   - CAN1_RX: PB8 (或 PA11)
 *   - 收发器 VCC: 5V, GND: GND
 *   - CANH/CANL: 连接到 CAN 总线
 *   - 终端电阻: 120Ω (总线两端各一个)
 *   
 *   功能:
 *   - CAN 500Kbps 通信
 *   - 发送标准帧 (11位ID)
 *   - 接收并打印 CAN 消息
 *   - 过滤器配置
 ******************************************************************************
 */

#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <string.h>

CAN_HandleTypeDef hcan1;
UART_HandleTypeDef huart2;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_Init(void);
static void MX_CAN1_Init(void);
static void CAN_ConfigFilter(void);
static HAL_StatusTypeDef CAN_SendMessage(uint32_t id, uint8_t *data, uint8_t len);

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
    MX_CAN1_Init();
    CAN_ConfigFilter();

    /* 启动 CAN */
    HAL_CAN_Start(&hcan1);
    HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING);

    printf("\r\n=== CAN Bus Basic Example ===\r\n");
    printf("Baud: 500Kbps | TX: PB9 | RX: PB8\r\n\r\n");

    uint32_t tx_count = 0;
    uint8_t  tx_data[8];

    while (1)
    {
        tx_count++;

        /* 构造发送数据 */
        tx_data[0] = (tx_count >> 24) & 0xFF;
        tx_data[1] = (tx_count >> 16) & 0xFF;
        tx_data[2] = (tx_count >> 8) & 0xFF;
        tx_data[3] = tx_count & 0xFF;
        tx_data[4] = (uint8_t)(HAL_GetTick() & 0xFF);
        tx_data[5] = 0xAA;
        tx_data[6] = 0xBB;
        tx_data[7] = 0xCC;

        /* 发送 CAN 消息 (ID=0x123) */
        HAL_StatusTypeDef ret = CAN_SendMessage(0x123, tx_data, 8);
        if (ret == HAL_OK)
        {
            printf("TX [0x123] #%lu: %02X %02X %02X %02X %02X %02X %02X %02X\r\n",
                   tx_count,
                   tx_data[0], tx_data[1], tx_data[2], tx_data[3],
                   tx_data[4], tx_data[5], tx_data[6], tx_data[7]);
        }
        else
        {
            printf("TX failed! (no ACK or bus error)\r\n");
        }

        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
        HAL_Delay(1000);
    }
}

/**
 * @brief  CAN 接收中断回调
 */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    CAN_RxHeaderTypeDef rx_header;
    uint8_t rx_data[8];

    if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rx_header, rx_data) == HAL_OK)
    {
        printf("RX [0x%03lX] DLC=%d: ", rx_header.StdId, rx_header.DLC);
        for (uint8_t i = 0; i < rx_header.DLC; i++)
            printf("%02X ", rx_data[i]);
        printf("\r\n");
    }
}

/**
 * @brief  发送 CAN 消息
 */
static HAL_StatusTypeDef CAN_SendMessage(uint32_t id, uint8_t *data, uint8_t len)
{
    CAN_TxHeaderTypeDef tx_header;
    uint32_t tx_mailbox;

    tx_header.StdId              = id;
    tx_header.ExtId              = 0;
    tx_header.IDE                = CAN_ID_STD;
    tx_header.RTR                = CAN_RTR_DATA;
    tx_header.DLC                = len;
    tx_header.TransmitGlobalTime = DISABLE;

    return HAL_CAN_AddTxMessage(&hcan1, &tx_header, data, &tx_mailbox);
}

/**
 * @brief  配置 CAN 过滤器 (接收所有消息)
 */
static void CAN_ConfigFilter(void)
{
    CAN_FilterTypeDef filter;
    filter.FilterBank           = 0;
    filter.FilterMode           = CAN_FILTERMODE_IDMASK;
    filter.FilterScale          = CAN_FILTERSCALE_32BIT;
    filter.FilterIdHigh         = 0x0000;
    filter.FilterIdLow          = 0x0000;
    filter.FilterMaskIdHigh     = 0x0000;  /* 掩码全0 = 接收所有 */
    filter.FilterMaskIdLow      = 0x0000;
    filter.FilterFIFOAssignment = CAN_RX_FIFO0;
    filter.FilterActivation     = ENABLE;
    filter.SlaveStartFilterBank = 14;
    HAL_CAN_ConfigFilter(&hcan1, &filter);
}

/**
 * @brief  CAN1 初始化 (500Kbps @ 42MHz APB1)
 *         波特率 = APB1 / (Prescaler * (1 + BS1 + BS2))
 *         500K = 42MHz / (6 * (1 + 11 + 2)) = 500KHz
 */
static void MX_CAN1_Init(void)
{
    GPIO_InitTypeDef g = {0};
    __HAL_RCC_CAN1_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* PB8: CAN1_RX, PB9: CAN1_TX */
    g.Pin = GPIO_PIN_8 | GPIO_PIN_9;
    g.Mode = GPIO_MODE_AF_PP; g.Pull = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    g.Alternate = GPIO_AF9_CAN1;
    HAL_GPIO_Init(GPIOB, &g);

    hcan1.Instance = CAN1;
    hcan1.Init.Prescaler = 6;
    hcan1.Init.Mode = CAN_MODE_NORMAL;  /* 正常模式 (CAN_MODE_LOOPBACK 用于自测) */
    hcan1.Init.SyncJumpWidth = CAN_SJW_1TQ;
    hcan1.Init.TimeSeg1 = CAN_BS1_11TQ;
    hcan1.Init.TimeSeg2 = CAN_BS2_2TQ;
    hcan1.Init.TimeTriggeredMode = DISABLE;
    hcan1.Init.AutoBusOff = ENABLE;
    hcan1.Init.AutoWakeUp = DISABLE;
    hcan1.Init.AutoRetransmission = ENABLE;
    hcan1.Init.ReceiveFifoLocked = DISABLE;
    hcan1.Init.TransmitFifoPriority = DISABLE;
    HAL_CAN_Init(&hcan1);

    HAL_NVIC_SetPriority(CAN1_RX0_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(CAN1_RX0_IRQn);
}

void CAN1_RX0_IRQHandler(void) { HAL_CAN_IRQHandler(&hcan1); }

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