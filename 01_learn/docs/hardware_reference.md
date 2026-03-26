# 硬件参考手册 / Hardware Reference Manual

## NUCLEO-F401RE 详细说明

### 板载资源

| 资源 | 描述 |
|------|------|
| MCU | STM32F401RET6 |
| 主频 | 84 MHz (最高) |
| Flash | 512 KB |
| SRAM | 96 KB |
| LED | LD2 (绿色, PA5) |
| 按键 | B1 (蓝色, PC13, 低电平有效) |
| 调试接口 | ST-Link V2-1 (板载) |
| USB | Micro-USB (ST-Link + 虚拟串口) |
| 电源 | USB 5V 或 外部 7-12V |

### 时钟配置

```
外部晶振: 无 (使用 ST-Link MCO 提供 8MHz)
HSE: 8 MHz (来自 ST-Link)
PLL: HSE * N/M/P = 8 * 336/8/2 = 84 MHz
APB1: 42 MHz (最大 42 MHz)
APB2: 84 MHz (最大 84 MHz)
```

### 引脚功能表

#### USART 接口

| 功能 | 引脚 | 说明 |
|------|------|------|
| USART2_TX | PA2 | 连接 ST-Link (虚拟串口) |
| USART2_RX | PA3 | 连接 ST-Link (虚拟串口) |
| USART1_TX | PA9/PB6 | 用户串口 |
| USART1_RX | PA10/PB7 | 用户串口 |
| USART6_TX | PC6 | 用户串口 |
| USART6_RX | PC7 | 用户串口 |

#### I2C 接口

| 功能 | 引脚 | 说明 |
|------|------|------|
| I2C1_SCL | PB8 | Arduino D15 |
| I2C1_SDA | PB9 | Arduino D14 |
| I2C2_SCL | PB10 | Arduino D6 |
| I2C2_SDA | PB3 | - |
| I2C3_SCL | PA8 | Arduino D7 |
| I2C3_SDA | PB4 | Arduino D5 |

#### SPI 接口

| 功能 | 引脚 | 说明 |
|------|------|------|
| SPI1_SCK | PA5 | Arduino D13 (LD2) |
| SPI1_MISO | PA6 | Arduino D12 |
| SPI1_MOSI | PA7 | Arduino D11 |
| SPI1_NSS | PA4/PB6 | Arduino A2/D10 |
| SPI2_SCK | PB13 | - |
| SPI2_MISO | PB14 | - |
| SPI2_MOSI | PB15 | - |

#### 定时器

| 定时器 | 通道 | 引脚 | 说明 |
|--------|------|------|------|
| TIM1 | CH1 | PA8 | 高级定时器 |
| TIM2 | CH1 | PA0/PA5/PA15 | 32位定时器 |
| TIM2 | CH2 | PA1/PB3 | |
| TIM2 | CH3 | PA2/PB10 | |
| TIM2 | CH4 | PA3/PB11 | |
| TIM3 | CH1 | PA6/PB4/PC6 | |
| TIM3 | CH2 | PA7/PB5/PC7 | |
| TIM4 | CH1 | PB6 | |
| TIM4 | CH2 | PB7 | |
| TIM5 | CH1 | PA0 | 32位定时器 |

#### ADC 通道

| 通道 | 引脚 | Arduino |
|------|------|---------|
| ADC1_IN0 | PA0 | A0 |
| ADC1_IN1 | PA1 | A1 |
| ADC1_IN4 | PA4 | A2 |
| ADC1_IN8 | PB0 | A3 |
| ADC1_IN11 | PC1 | A4 |
| ADC1_IN10 | PC0 | A5 |
| ADC1_IN16 | 内部温度传感器 | - |
| ADC1_IN17 | 内部参考电压 | - |
| ADC1_IN18 | VBAT | - |

## STM32F4DISCOVERY 详细说明

### 板载资源

| 资源 | 描述 |
|------|------|
| MCU | STM32F407VGT6 |
| 主频 | 168 MHz |
| Flash | 1 MB |
| SRAM | 192 KB (含 64KB CCM) |
| LED | LD3(PD13橙), LD4(PD12绿), LD5(PD14红), LD6(PD15蓝) |
| 按键 | B1 (PA0, 高电平有效) |
| 加速度计 | LIS302DL 或 LIS3DSH (SPI1) |
| 音频 | CS43L22 DAC (I2C1 + I2S3) |
| 麦克风 | MP45DT02 (I2S2) |
| USB OTG FS | PA11/PA12 |
| USB OTG HS | PB14/PB15 |

### 时钟配置

```
外部晶振: 8 MHz (HSE)
PLL: 8 * 336/8/2 = 168 MHz
APB1: 42 MHz
APB2: 84 MHz
```

## 电路连接参考

### LED 连接

```
STM32 PA5 ──[330Ω]──┤>├── GND
                    LED
(NUCLEO LD2 已板载此电路)
```

### 按键连接

```
3.3V ──[10kΩ]── STM32 PC13 ──[按键]── GND
(NUCLEO B1 已板载此电路, 按下为低电平)
```

### UART 连接 (外部设备)

```
STM32 TX (PA2) ──────── RX (USB-TTL)
STM32 RX (PA3) ──────── TX (USB-TTL)
STM32 GND      ──────── GND (USB-TTL)
```

### I2C 连接

```
STM32 PB8 (SCL) ──[4.7kΩ]── 3.3V
                └─────────── 传感器 SCL
STM32 PB9 (SDA) ──[4.7kΩ]── 3.3V
                └─────────── 传感器 SDA
```

### SPI 连接

```
STM32 PA5 (SCK)  ──── 设备 SCK
STM32 PA6 (MISO) ──── 设备 MISO
STM32 PA7 (MOSI) ──── 设备 MOSI
STM32 PB6 (CS)   ──── 设备 CS
```

## 电源说明

### NUCLEO 电源

| 电源 | 电压 | 说明 |
|------|------|------|
| USB | 5V | 通过 Micro-USB 供电 |
| VIN | 7-12V | 外部电源输入 |
| 3.3V | 3.3V | 板载 LDO 输出 |
| 5V | 5V | USB 直通 |
| GND | 0V | 公共地 |

### 电流消耗参考

| 模式 | 电流 | 说明 |
|------|------|------|
| 正常运行 | ~20-30 mA | 84MHz, 所有外设关闭 |
| 睡眠模式 | ~1-2 mA | CPU 停止, 外设运行 |
| 停止模式 | ~100-500 μA | 大部分时钟停止 |
| 待机模式 | ~2-5 μA | 最低功耗 |

## 常用外设模块接线

### OLED 显示屏 (I2C, 0.96寸)

```
OLED VCC  ──── 3.3V
OLED GND  ──── GND
OLED SCL  ──── PB8 (I2C1_SCL)
OLED SDA  ──── PB9 (I2C1_SDA)
I2C 地址: 0x3C 或 0x3D
```

### DHT11/DHT22 温湿度传感器

```
DHT VCC   ──── 3.3V
DHT GND   ──── GND
DHT DATA  ──[4.7kΩ]── 3.3V
          └──────────── PA1 (GPIO)
```

### HC-SR04 超声波传感器

```
HC-SR04 VCC   ──── 5V
HC-SR04 GND   ──── GND
HC-SR04 TRIG  ──── PA8 (GPIO_OUTPUT)
HC-SR04 ECHO  ──[分压]── PA9 (GPIO_INPUT)
注: ECHO 输出 5V, 需要分压到 3.3V
```

### W25Q64 SPI Flash

```
W25Q64 VCC  ──── 3.3V
W25Q64 GND  ──── GND
W25Q64 CLK  ──── PA5 (SPI1_SCK)
W25Q64 DO   ──── PA6 (SPI1_MISO)
W25Q64 DI   ──── PA7 (SPI1_MOSI)
W25Q64 CS   ──── PB6 (GPIO_OUTPUT)
W25Q64 WP   ──── 3.3V
W25Q64 HOLD ──── 3.3V
```

### CAN 总线 (TJA1050 收发器)

```
STM32 PB8 (CAN_RX) ──── TJA1050 RXD
STM32 PB9 (CAN_TX) ──── TJA1050 TXD
TJA1050 VCC ──── 5V
TJA1050 GND ──── GND
TJA1050 CANH ──── CAN 总线 H
TJA1050 CANL ──── CAN 总线 L
终端电阻: 120Ω (总线两端各一个)