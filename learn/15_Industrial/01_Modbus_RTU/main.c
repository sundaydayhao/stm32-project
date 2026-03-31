/**
 ******************************************************************************
 * @file    15_Industrial/01_Modbus_RTU/main.c
 * @brief   Modbus RTU 从站例程 / Modbus RTU Slave Example
 * @board   NUCLEO-F401RE
 * @note    
 *   硬件连接 (RS485):
 *   - USART1_TX: PA9  -> RS485 DI
 *   - USART1_RX: PA10 -> RS485 RO
 *   - DE/RE:     PA8  -> RS485 DE+RE (高=发送, 低=接收)
 *   
 *   Modbus 寄存器映射:
 *   - 线圈 (0x): 0000=LED控制
 *   - 离散输入 (1x): 0000=按键状态
 *   - 保持寄存器 (4x): 0000=ADC值, 0001=计数器
 *   - 输入寄存器 (3x): 0000=ADC电压(mV)
 *   
 *   支持功能码:
 *   - FC01: 读线圈
 *   - FC02: 读离散输入
 *   - FC03: 读保持寄存器
 *   - FC04: 读输入寄存器
 *   - FC05: 写单个线圈
 *   - FC06: 写单个寄存器
 ******************************************************************************
 */

#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <string.h>

/* Modbus 配置 */
#define MODBUS_SLAVE_ADDR   1       /* 从站地址 */
#define MODBUS_BAUD         9600
#define MODBUS_BUF_SIZE     256

/* Modbus 功能码 */
#define MB_FC_READ_COILS        0x01
#define MB_FC_READ_DISC_INPUT   0x02
#define MB_FC_READ_HOLD_REG     0x03
#define MB_FC_READ_INPUT_REG    0x04
#define MB_FC_WRITE_COIL        0x05
#define MB_FC_WRITE_REG         0x06

/* Modbus 异常码 */
#define MB_EX_ILLEGAL_FUNC      0x01
#define MB_EX_ILLEGAL_ADDR      0x02
#define MB_EX_ILLEGAL_VALUE     0x03

/* RS485 方向控制 */
#define RS485_TX()  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET)
#define RS485_RX()  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET)

/* Modbus 数据模型 */
#define COIL_COUNT      8
#define DISC_COUNT      8
#define HOLD_REG_COUNT  16
#define INPUT_REG_COUNT 8

static uint8_t  mb_coils[COIL_COUNT]         = {0};
static uint8_t  mb_disc_inputs[DISC_COUNT]   = {0};
static uint16_t mb_hold_regs[HOLD_REG_COUNT] = {0};
static uint16_t mb_input_regs[INPUT_REG_COUNT] = {0};

UART_HandleTypeDef huart1;  /* RS485 */
UART_HandleTypeDef huart2;  /* Debug */
ADC_HandleTypeDef  hadc1;

static uint8_t rx_buf[MODBUS_BUF_SIZE];
static uint8_t tx_buf[MODBUS_BUF_SIZE];
static volatile uint16_t rx_len = 0;
static volatile uint8_t  rx_done = 0;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_Init(void);
static void MX_USART2_Init(void);
static void MX_ADC1_Init(void);

static uint16_t CRC16(uint8_t *buf, uint16_t len);
static void Modbus_Process(void);
static void Modbus_SendResponse(uint8_t *data, uint16_t len);
static void Modbus_Exception(uint8_t func, uint8_t code);

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
    MX_USART1_Init();
    MX_ADC1_Init();

    RS485_RX();

    printf("\r\n=== Modbus RTU Slave ===\r\n");
    printf("Slave Address: %d\r\n", MODBUS_SLAVE_ADDR);
    printf("Baud Rate: %d\r\n", MODBUS_BAUD);
    printf("Waiting for Modbus commands...\r\n\r\n");

    /* 启动 UART 接收 */
    HAL_UART_Receive_IT(&huart1, rx_buf, 1);

    uint32_t last_update = 0;

    while (1)
    {
        /* 每 100ms 更新传感器数据 */
        if (HAL_GetTick() - last_update >= 100)
        {
            last_update = HAL_GetTick();

            /* 读取 ADC */
            HAL_ADC_Start(&hadc1);
            HAL_ADC_PollForConversion(&hadc1, 10);
            uint16_t adc = (uint16_t)HAL_ADC_GetValue(&hadc1);
            HAL_ADC_Stop(&hadc1);

            /* 更新 Modbus 寄存器 */
            mb_hold_regs[0] = adc;
            mb_hold_regs[1]++;  /* 计数器 */
            mb_input_regs[0] = (uint16_t)((uint32_t)adc * 3300 / 4095);  /* mV */

            /* 更新离散输入 (按键) */
            mb_disc_inputs[0] = (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == GPIO_PIN_RESET) ? 1 : 0;

            /* 线圈控制 LED */
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5,
                              mb_coils[0] ? GPIO_PIN_SET : GPIO_PIN_RESET);
        }

        /* 处理 Modbus 请求 */
        if (rx_done)
        {
            rx_done = 0;
            Modbus_Process();
        }
    }
}

/**
 * @brief  CRC16 计算 (Modbus 标准)
 */
static uint16_t CRC16(uint8_t *buf, uint16_t len)
{
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < len; i++)
    {
        crc ^= buf[i];
        for (uint8_t j = 0; j < 8; j++)
        {
            if (crc & 0x0001)
                crc = (crc >> 1) ^ 0xA001;
            else
                crc >>= 1;
        }
    }
    return crc;
}

/**
 * @brief  处理 Modbus 请求
 */
static void Modbus_Process(void)
{
    if (rx_len < 4) return;

    /* 验证从站地址 */
    if (rx_buf[0] != MODBUS_SLAVE_ADDR) return;

    /* 验证 CRC */
    uint16_t crc_recv = (uint16_t)(rx_buf[rx_len-1] << 8) | rx_buf[rx_len-2];
    uint16_t crc_calc = CRC16(rx_buf, rx_len - 2);
    if (crc_recv != crc_calc)
    {
        printf("CRC error! recv=0x%04X calc=0x%04X\r\n", crc_recv, crc_calc);
        return;
    }

    uint8_t  func = rx_buf[1];
    uint16_t addr = (uint16_t)(rx_buf[2] << 8) | rx_buf[3];
    uint16_t qty  = (rx_len >= 6) ? ((uint16_t)(rx_buf[4] << 8) | rx_buf[5]) : 0;

    printf("FC:%02X Addr:%04X Qty:%d\r\n", func, addr, qty);

    switch (func)
    {
        case MB_FC_READ_COILS:
        {
            if (addr + qty > COIL_COUNT) { Modbus_Exception(func, MB_EX_ILLEGAL_ADDR); return; }
            uint8_t byte_cnt = (qty + 7) / 8;
            tx_buf[0] = MODBUS_SLAVE_ADDR;
            tx_buf[1] = func;
            tx_buf[2] = byte_cnt;
            memset(tx_buf + 3, 0, byte_cnt);
            for (uint16_t i = 0; i < qty; i++)
                if (mb_coils[addr + i]) tx_buf[3 + i/8] |= (1 << (i%8));
            uint16_t crc = CRC16(tx_buf, 3 + byte_cnt);
            tx_buf[3+byte_cnt] = crc & 0xFF;
            tx_buf[4+byte_cnt] = crc >> 8;
            Modbus_SendResponse(tx_buf, 5 + byte_cnt);
            break;
        }
        case MB_FC_READ_DISC_INPUT:
        {
            if (addr + qty > DISC_COUNT) { Modbus_Exception(func, MB_EX_ILLEGAL_ADDR); return; }
            uint8_t byte_cnt = (qty + 7) / 8;
            tx_buf[0] = MODBUS_SLAVE_ADDR; tx_buf[1] = func; tx_buf[2] = byte_cnt;
            memset(tx_buf + 3, 0, byte_cnt);
            for (uint16_t i = 0; i < qty; i++)
                if (mb_disc_inputs[addr + i]) tx_buf[3 + i/8] |= (1 << (i%8));
            uint16_t crc = CRC16(tx_buf, 3 + byte_cnt);
            tx_buf[3+byte_cnt] = crc & 0xFF; tx_buf[4+byte_cnt] = crc >> 8;
            Modbus_SendResponse(tx_buf, 5 + byte_cnt);
            break;
        }
        case MB_FC_READ_HOLD_REG:
        {
            if (addr + qty > HOLD_REG_COUNT) { Modbus_Exception(func, MB_EX_ILLEGAL_ADDR); return; }
            tx_buf[0] = MODBUS_SLAVE_ADDR; tx_buf[1] = func; tx_buf[2] = qty * 2;
            for (uint16_t i = 0; i < qty; i++)
            {
                tx_buf[3 + i*2]   = mb_hold_regs[addr+i] >> 8;
                tx_buf[3 + i*2+1] = mb_hold_regs[addr+i] & 0xFF;
            }
            uint16_t crc = CRC16(tx_buf, 3 + qty*2);
            tx_buf[3+qty*2] = crc & 0xFF; tx_buf[4+qty*2] = crc >> 8;
            Modbus_SendResponse(tx_buf, 5 + qty*2);
            break;
        }
        case MB_FC_READ_INPUT_REG:
        {
            if (addr + qty > INPUT_REG_COUNT) { Modbus_Exception(func, MB_EX_ILLEGAL_ADDR); return; }
            tx_buf[0] = MODBUS_SLAVE_ADDR; tx_buf[1] = func; tx_buf[2] = qty * 2;
            for (uint16_t i = 0; i < qty; i++)
            {
                tx_buf[3 + i*2]   = mb_input_regs[addr+i] >> 8;
                tx_buf[3 + i*2+1] = mb_input_regs[addr+i] & 0xFF;
            }
            uint16_t crc = CRC16(tx_buf, 3 + qty*2);
            tx_buf[3+qty*2] = crc & 0xFF; tx_buf[4+qty*2] = crc >> 8;
            Modbus_SendResponse(tx_buf, 5 + qty*2);
            break;
        }
        case MB_FC_WRITE_COIL:
        {
            if (addr >= COIL_COUNT) { Modbus_Exception(func, MB_EX_ILLEGAL_ADDR); return; }
            mb_coils[addr] = (qty == 0xFF00) ? 1 : 0;
            /* 回显请求 */
            memcpy(tx_buf, rx_buf, 6);
            uint16_t crc = CRC16(tx_buf, 6);
            tx_buf[6] = crc & 0xFF; tx_buf[7] = crc >> 8;
            Modbus_SendResponse(tx_buf, 8);
            break;
        }
        case MB_FC_WRITE_REG:
        {
            if (addr >= HOLD_REG_COUNT) { Modbus_Exception(func, MB_EX_ILLEGAL_ADDR); return; }
            mb_hold_regs[addr] = qty;
            memcpy(tx_buf, rx_buf, 6);
            uint16_t crc = CRC16(tx_buf, 6);
            tx_buf[6] = crc & 0xFF; tx_buf[7] = crc >> 8;
            Modbus_SendResponse(tx_buf, 8);
            break;
        }
        default:
            Modbus_Exception(func, MB_EX_ILLEGAL_FUNC);
            break;
    }
}

static void Modbus_SendResponse(uint8_t *data, uint16_t len)
{
    RS485_TX();
    HAL_UART_Transmit(&huart1, data, len, 100);
    HAL_Delay(1);
    RS485_RX();
}

static void Modbus_Exception(uint8_t func, uint8_t code)
{
    tx_buf[0] = MODBUS_SLAVE_ADDR;
    tx_buf[1] = func | 0x80;
    tx_buf[2] = code;
    uint16_t crc = CRC16(tx_buf, 3);
    tx_buf[3] = crc & 0xFF; tx_buf[4] = crc >> 8;
    Modbus_SendResponse(tx_buf, 5);
}

/* UART1 接收中断 - 简单帧检测 (3.5字符超时用定时器实现) */
static uint16_t rx_idx = 0;
static uint32_t last_rx_tick = 0;

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        rx_idx++;
        last_rx_tick = HAL_GetTick();
        HAL_UART_Receive_IT(&huart1, rx_buf + rx_idx, 1);
    }
}

/* 在主循环中检测帧超时 (3.5字符 @ 9600bps ≈ 4ms) */
/* 实际应用中应使用定时器中断实现精确超时 */

static void MX_USART1_Init(void)
{
    GPIO_InitTypeDef g = {0};
    __HAL_RCC_USART1_CLK_ENABLE(); __HAL_RCC_GPIOA_CLK_ENABLE();
    /* PA9: TX, PA10: RX */
    g.Pin = GPIO_PIN_9 | GPIO_PIN_10; g.Mode = GPIO_MODE_AF_PP;
    g.Pull = GPIO_NOPULL; g.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    g.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &g);
    /* PA8: DE/RE */
    g.Pin = GPIO_PIN_8; g.Mode = GPIO_MODE_OUTPUT_PP;
    g.Pull = GPIO_NOPULL; g.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &g);

    huart1.Instance = USART1; huart1.Init.BaudRate = MODBUS_BAUD;
    huart1.Init.WordLength = UART_WORDLENGTH_8B; huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE; huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE; huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart1);
    HAL_NVIC_SetPriority(USART1_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
}

void USART1_IRQHandler(void) { HAL_UART_IRQHandler(&huart1); }

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
        g.Alternate = GPIO_AF7_USART2;
        HAL_GPIO_Init(GPIOA, &g);
    }
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