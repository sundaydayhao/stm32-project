# 快速入门指南 / Getting Started Guide

## 1. 硬件准备 / Hardware Preparation

### NUCLEO-F401RE 开发板

```
┌─────────────────────────────────────────────────────┐
│                  NUCLEO-F401RE                       │
│                                                      │
│  ┌──────────┐    ┌─────────────────────────────┐    │
│  │ ST-Link  │    │      STM32F401RE             │    │
│  │  V2-1    │    │   84MHz, 512KB Flash         │    │
│  │          │    │   96KB RAM                   │    │
│  └──────────┘    └─────────────────────────────┘    │
│                                                      │
│  LED: LD2 (PA5 - Green)                             │
│  Button: B1 (PC13 - Blue, 按下为低电平)              │
│  UART: USART2 (PA2-TX, PA3-RX) -> ST-Link USB       │
│                                                      │
│  Arduino 兼容接口 (CN5, CN6, CN8, CN9)              │
│  Morpho 接口 (CN7, CN10)                            │
└─────────────────────────────────────────────────────┘
```

### STM32F4DISCOVERY 开发板

```
┌─────────────────────────────────────────────────────┐
│                STM32F4DISCOVERY                      │
│                                                      │
│  MCU: STM32F407VGT6                                 │
│  主频: 168MHz                                        │
│  Flash: 1MB, RAM: 192KB                             │
│                                                      │
│  LED: LD3(PD13-橙), LD4(PD12-绿)                   │
│       LD5(PD14-红), LD6(PD15-蓝)                   │
│  Button: B1 (PA0 - 按下为高电平)                    │
│  加速度计: LIS302DL/LIS3DSH (SPI1)                  │
│  音频: CS43L22 (I2C1 + I2S3)                       │
│  USB OTG FS: PA11/PA12                              │
│  USB OTG HS: PB14/PB15                             │
└─────────────────────────────────────────────────────┘
```

## 2. 软件工具 / Software Tools

### 方案一: STM32CubeIDE (推荐)

1. 下载 [STM32CubeIDE](https://www.st.com/en/development-tools/stm32cubeide.html)
2. 安装 STM32CubeIDE
3. 安装 STM32CubeF4 固件包
4. 导入例程项目

### 方案二: Keil MDK

1. 下载 [Keil MDK](https://www.keil.com/download/product/)
2. 安装 STM32F4xx DFP 包
3. 打开 `.uvprojx` 文件

### 方案三: VSCode + PlatformIO

1. 安装 VSCode
2. 安装 PlatformIO 插件
3. 使用 `platformio.ini` 配置

### 方案四: Makefile + ARM GCC (命令行)

```bash
# 安装 ARM GCC
# Windows: 下载 arm-none-eabi-gcc
# Linux: sudo apt install gcc-arm-none-eabi
# macOS: brew install arm-none-eabi-gcc

# 编译
make all

# 下载 (需要 OpenOCD)
make flash

# 清理
make clean
```

## 3. 第一个程序 / First Program

### LED 闪烁 (01_GPIO/01_LED_Blink)

```c
#include "stm32f4xx_hal.h"

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    
    // 初始化 LED (PA5)
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    while (1)
    {
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
        HAL_Delay(500);  // 500ms
    }
}
```

## 4. 调试技巧 / Debug Tips

### 串口调试
- NUCLEO 板通过 ST-Link 虚拟串口连接 PC
- 波特率: 115200, 8N1
- 使用 `printf` 重定向到 USART2

### SWD 调试
- 使用 STM32CubeIDE 内置调试器
- 支持断点、单步、变量监视
- 使用 Live Expressions 实时监视变量

### 常见问题
1. **程序无法下载**: 检查 ST-Link 驱动是否安装
2. **串口无输出**: 检查波特率设置
3. **程序跑飞**: 检查堆栈大小，使用看门狗

## 5. 引脚分配 / Pin Assignment

### NUCLEO-F401RE Arduino 接口

| Arduino | STM32 | 功能 |
|---------|-------|------|
| D0 | PA3 | USART2_RX |
| D1 | PA2 | USART2_TX |
| D2 | PA10 | GPIO |
| D3 | PB3 | TIM2_CH2/PWM |
| D4 | PB5 | GPIO |
| D5 | PB4 | TIM3_CH1/PWM |
| D6 | PB10 | TIM2_CH3/PWM |
| D7 | PA8 | GPIO |
| D8 | PA9 | GPIO |
| D9 | PC7 | TIM3_CH2/PWM |
| D10 | PB6 | SPI1_CS/TIM4_CH1 |
| D11 | PA7 | SPI1_MOSI |
| D12 | PA6 | SPI1_MISO |
| D13 | PA5 | SPI1_SCK/LD2 |
| A0 | PA0 | ADC1_IN0 |
| A1 | PA1 | ADC1_IN1 |
| A2 | PA4 | ADC1_IN4 |
| A3 | PB0 | ADC1_IN8 |
| A4 | PC1 | ADC1_IN11/I2C |
| A5 | PC0 | ADC1_IN10/I2C |