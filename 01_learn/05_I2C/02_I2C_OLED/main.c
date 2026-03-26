/**
 ******************************************************************************
 * @file    05_I2C/02_I2C_OLED/main.c
 * @brief   I2C OLED 显示屏驱动 / I2C OLED Display Driver (SSD1306)
 * @board   NUCLEO-F401RE
 * @note    
 *   硬件连接 / Hardware Connection:
 *   - I2C1_SCL: PB8 -> OLED SCL
 *   - I2C1_SDA: PB9 -> OLED SDA
 *   - OLED VCC: 3.3V
 *   - OLED GND: GND
 *   - I2C 地址: 0x3C (默认)
 *   
 *   OLED 规格 / OLED Spec:
 *   - 控制器: SSD1306
 *   - 分辨率: 128x64 像素
 *   - 接口: I2C
 ******************************************************************************
 */

#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <string.h>

/* OLED 参数 */
#define OLED_I2C_ADDR   0x3C    /* OLED I2C 地址 */
#define OLED_WIDTH      128
#define OLED_HEIGHT     64
#define OLED_PAGES      8       /* 64/8 = 8 页 */

I2C_HandleTypeDef hi2c1;
UART_HandleTypeDef huart2;

/* OLED 显存 */
static uint8_t oled_buf[OLED_PAGES][OLED_WIDTH];

/* 5x7 ASCII 字体 (部分) */
static const uint8_t font5x7[][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, /* 空格 */
    {0x00, 0x00, 0x5F, 0x00, 0x00}, /* ! */
    {0x00, 0x07, 0x00, 0x07, 0x00}, /* " */
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, /* # */
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, /* $ */
    {0x23, 0x13, 0x08, 0x64, 0x62}, /* % */
    {0x36, 0x49, 0x55, 0x22, 0x50}, /* & */
    {0x00, 0x05, 0x03, 0x00, 0x00}, /* ' */
    {0x00, 0x1C, 0x22, 0x41, 0x00}, /* ( */
    {0x00, 0x41, 0x22, 0x1C, 0x00}, /* ) */
    {0x08, 0x2A, 0x1C, 0x2A, 0x08}, /* * */
    {0x08, 0x08, 0x3E, 0x08, 0x08}, /* + */
    {0x00, 0x50, 0x30, 0x00, 0x00}, /* , */
    {0x08, 0x08, 0x08, 0x08, 0x08}, /* - */
    {0x00, 0x60, 0x60, 0x00, 0x00}, /* . */
    {0x20, 0x10, 0x08, 0x04, 0x02}, /* / */
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, /* 0 */
    {0x00, 0x42, 0x7F, 0x40, 0x00}, /* 1 */
    {0x42, 0x61, 0x51, 0x49, 0x46}, /* 2 */
    {0x21, 0x41, 0x45, 0x4B, 0x31}, /* 3 */
    {0x18, 0x14, 0x12, 0x7F, 0x10}, /* 4 */
    {0x27, 0x45, 0x45, 0x45, 0x39}, /* 5 */
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, /* 6 */
    {0x01, 0x71, 0x09, 0x05, 0x03}, /* 7 */
    {0x36, 0x49, 0x49, 0x49, 0x36}, /* 8 */
    {0x06, 0x49, 0x49, 0x29, 0x1E}, /* 9 */
    {0x00, 0x36, 0x36, 0x00, 0x00}, /* : */
    {0x00, 0x56, 0x36, 0x00, 0x00}, /* ; */
    {0x00, 0x08, 0x14, 0x22, 0x41}, /* < */
    {0x14, 0x14, 0x14, 0x14, 0x14}, /* = */
    {0x41, 0x22, 0x14, 0x08, 0x00}, /* > */
    {0x02, 0x01, 0x51, 0x09, 0x06}, /* ? */
    {0x32, 0x49, 0x79, 0x41, 0x3E}, /* @ */
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, /* A */
    {0x7F, 0x49, 0x49, 0x49, 0x36}, /* B */
    {0x3E, 0x41, 0x41, 0x41, 0x22}, /* C */
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, /* D */
    {0x7F, 0x49, 0x49, 0x49, 0x41}, /* E */
    {0x7F, 0x09, 0x09, 0x01, 0x01}, /* F */
    {0x3E, 0x41, 0x41, 0x51, 0x32}, /* G */
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, /* H */
    {0x00, 0x41, 0x7F, 0x41, 0x00}, /* I */
    {0x20, 0x40, 0x41, 0x3F, 0x01}, /* J */
    {0x7F, 0x08, 0x14, 0x22, 0x41}, /* K */
    {0x7F, 0x40, 0x40, 0x40, 0x40}, /* L */
    {0x7F, 0x02, 0x04, 0x02, 0x7F}, /* M */
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, /* N */
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, /* O */
    {0x7F, 0x09, 0x09, 0x09, 0x06}, /* P */
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, /* Q */
    {0x7F, 0x09, 0x19, 0x29, 0x46}, /* R */
    {0x46, 0x49, 0x49, 0x49, 0x31}, /* S */
    {0x01, 0x01, 0x7F, 0x01, 0x01}, /* T */
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, /* U */
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, /* V */
    {0x7F, 0x20, 0x18, 0x20, 0x7F}, /* W */
    {0x63, 0x14, 0x08, 0x14, 0x63}, /* X */
    {0x03, 0x04, 0x78, 0x04, 0x03}, /* Y */
    {0x61, 0x51, 0x49, 0x45, 0x43}, /* Z */
};

/* 函数声明 */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C1_Init(void);

/* OLED 函数 */
static void OLED_WriteCmd(uint8_t cmd);
static void OLED_WriteData(uint8_t *data, uint16_t len);
static void OLED_Init(void);
static void OLED_Clear(void);
static void OLED_Display(void);
static void OLED_SetPixel(uint8_t x, uint8_t y, uint8_t color);
static void OLED_DrawChar(uint8_t x, uint8_t page, char c);
static void OLED_DrawString(uint8_t x, uint8_t page, const char *str);
static void OLED_DrawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);
static void OLED_DrawRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h);

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
    MX_I2C1_Init();

    printf("\r\n=== I2C OLED Example ===\r\n");

    OLED_Init();
    OLED_Clear();

    /* 显示标题 */
    OLED_DrawString(0, 0, "STM32 NUCLEO");
    OLED_DrawString(0, 1, "OLED Demo");
    OLED_DrawLine(0, 16, 127, 16);

    uint32_t counter = 0;
    char buf[32];

    while (1)
    {
        counter++;

        /* 更新显示内容 */
        OLED_DrawString(0, 3, "Tick:");
        snprintf(buf, sizeof(buf), "%lu ms   ", HAL_GetTick());
        OLED_DrawString(40, 3, buf);

        OLED_DrawString(0, 4, "Count:");
        snprintf(buf, sizeof(buf), "%lu   ", counter);
        OLED_DrawString(48, 4, buf);

        /* 绘制进度条 */
        OLED_DrawRect(0, 48, 128, 8);
        uint8_t progress = (uint8_t)(counter % 128);
        for (uint8_t i = 0; i < progress; i++)
        {
            for (uint8_t j = 49; j < 55; j++)
                OLED_SetPixel(i, j, 1);
        }

        OLED_Display();

        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
        HAL_Delay(100);
    }
}

/**
 * @brief  发送命令到 OLED
 */
static void OLED_WriteCmd(uint8_t cmd)
{
    uint8_t buf[2] = {0x00, cmd};  /* 0x00: 命令模式 */
    HAL_I2C_Master_Transmit(&hi2c1, OLED_I2C_ADDR << 1, buf, 2, 100);
}

/**
 * @brief  发送数据到 OLED
 */
static void OLED_WriteData(uint8_t *data, uint16_t len)
{
    uint8_t buf[OLED_WIDTH + 1];
    buf[0] = 0x40;  /* 0x40: 数据模式 */
    memcpy(buf + 1, data, len);
    HAL_I2C_Master_Transmit(&hi2c1, OLED_I2C_ADDR << 1, buf, len + 1, 100);
}

/**
 * @brief  OLED 初始化序列 (SSD1306)
 */
static void OLED_Init(void)
{
    HAL_Delay(100);  /* 等待 OLED 上电稳定 */

    OLED_WriteCmd(0xAE);  /* 关闭显示 */
    OLED_WriteCmd(0x20);  /* 设置内存地址模式 */
    OLED_WriteCmd(0x00);  /* 水平地址模式 */
    OLED_WriteCmd(0xB0);  /* 设置页起始地址 */
    OLED_WriteCmd(0xC8);  /* COM 扫描方向 */
    OLED_WriteCmd(0x00);  /* 低列地址 */
    OLED_WriteCmd(0x10);  /* 高列地址 */
    OLED_WriteCmd(0x40);  /* 起始行 */
    OLED_WriteCmd(0x81);  /* 对比度设置 */
    OLED_WriteCmd(0xFF);  /* 最大对比度 */
    OLED_WriteCmd(0xA1);  /* 段重映射 */
    OLED_WriteCmd(0xA6);  /* 正常显示 */
    OLED_WriteCmd(0xA8);  /* 多路复用比 */
    OLED_WriteCmd(0x3F);  /* 64 行 */
    OLED_WriteCmd(0xA4);  /* 输出跟随 RAM */
    OLED_WriteCmd(0xD3);  /* 显示偏移 */
    OLED_WriteCmd(0x00);
    OLED_WriteCmd(0xD5);  /* 时钟分频 */
    OLED_WriteCmd(0xF0);
    OLED_WriteCmd(0xD9);  /* 预充电周期 */
    OLED_WriteCmd(0x22);
    OLED_WriteCmd(0xDA);  /* COM 引脚配置 */
    OLED_WriteCmd(0x12);
    OLED_WriteCmd(0xDB);  /* VCOMH 电压 */
    OLED_WriteCmd(0x20);
    OLED_WriteCmd(0x8D);  /* 充电泵 */
    OLED_WriteCmd(0x14);  /* 使能充电泵 */
    OLED_WriteCmd(0xAF);  /* 开启显示 */
}

/**
 * @brief  清除显存
 */
static void OLED_Clear(void)
{
    memset(oled_buf, 0, sizeof(oled_buf));
}

/**
 * @brief  将显存内容发送到 OLED
 */
static void OLED_Display(void)
{
    for (uint8_t page = 0; page < OLED_PAGES; page++)
    {
        OLED_WriteCmd(0xB0 + page);  /* 设置页地址 */
        OLED_WriteCmd(0x00);          /* 列地址低位 */
        OLED_WriteCmd(0x10);          /* 列地址高位 */
        OLED_WriteData(oled_buf[page], OLED_WIDTH);
    }
}

/**
 * @brief  设置像素点
 */
static void OLED_SetPixel(uint8_t x, uint8_t y, uint8_t color)
{
    if (x >= OLED_WIDTH || y >= OLED_HEIGHT) return;
    uint8_t page = y / 8;
    uint8_t bit  = y % 8;
    if (color)
        oled_buf[page][x] |= (1 << bit);
    else
        oled_buf[page][x] &= ~(1 << bit);
}

/**
 * @brief  在指定位置绘制字符
 */
static void OLED_DrawChar(uint8_t x, uint8_t page, char c)
{
    if (c < ' ' || c > 'Z') c = ' ';
    uint8_t idx = c - ' ';
    if (idx >= sizeof(font5x7) / sizeof(font5x7[0])) return;

    for (uint8_t i = 0; i < 5; i++)
    {
        if (x + i < OLED_WIDTH)
            oled_buf[page][x + i] = font5x7[idx][i];
    }
    if (x + 5 < OLED_WIDTH)
        oled_buf[page][x + 5] = 0x00;  /* 字符间距 */
}

/**
 * @brief  在指定位置绘制字符串
 */
static void OLED_DrawString(uint8_t x, uint8_t page, const char *str)
{
    while (*str && x < OLED_WIDTH)
    {
        OLED_DrawChar(x, page, *str++);
        x += 6;
    }
}

/**
 * @brief  绘制直线 (Bresenham 算法)
 */
static void OLED_DrawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
    int dx = (int)x1 - x0, dy = (int)y1 - y0;
    int sx = dx > 0 ? 1 : -1, sy = dy > 0 ? 1 : -1;
    dx = dx < 0 ? -dx : dx;
    dy = dy < 0 ? -dy : dy;
    int err = dx - dy;
    while (1)
    {
        OLED_SetPixel(x0, y0, 1);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx)  { err += dx; y0 += sy; }
    }
}

/**
 * @brief  绘制矩形框
 */
static void OLED_DrawRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
    OLED_DrawLine(x, y, x + w - 1, y);
    OLED_DrawLine(x, y + h - 1, x + w - 1, y + h - 1);
    OLED_DrawLine(x, y, x, y + h - 1);
    OLED_DrawLine(x + w - 1, y, x + w - 1, y + h - 1);
}

static void MX_I2C1_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_I2C1_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    hi2c1.Instance = I2C1; hi2c1.Init.ClockSpeed = 400000;
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2; hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0; hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    HAL_I2C_Init(&hi2c1);
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