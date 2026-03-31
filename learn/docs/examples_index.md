# STM32 例程索引 / Examples Index

## 项目结构 / Project Structure

```
stm32_learn/
├── README.md                    # 项目说明
├── LICENSE                      # MIT 许可证
├── common/                      # 公共库
│   ├── Inc/
│   │   ├── bsp.h               # 板级支持包头文件
│   │   └── stm32f4xx_hal_conf.h # HAL 配置
│   └── Src/
│       └── bsp.c               # 板级支持包实现
├── docs/                        # 文档
│   ├── getting_started.md      # 快速入门
│   ├── toolchain_setup.md      # 工具链安装
│   ├── hardware_reference.md   # 硬件参考
│   └── examples_index.md       # 本文件
├── 01_GPIO/                     # GPIO 例程
├── 02_UART/                     # UART 例程
├── 03_Timer/                    # 定时器例程
├── 04_ADC/                      # ADC 例程
├── 05_I2C/                      # I2C 例程
├── 06_SPI/                      # SPI 例程
├── 07_DMA/                      # DMA 例程
├── 08_EXTI/                     # 外部中断例程
├── 09_RTC/                      # RTC 例程
├── 10_Watchdog/                 # 看门狗例程
├── 11_LowPower/                 # 低功耗例程
├── 12_FreeRTOS/                 # FreeRTOS 例程
└── 15_Industrial/               # 工业应用例程
```

---

## 01_GPIO - GPIO 输入输出

| 例程 | 文件 | 功能 | 难度 |
|------|------|------|------|
| LED 闪烁 | `01_LED_Blink/main.c` | PA5 LED 1Hz 闪烁, HAL_Delay | ⭐ |
| 按键控制 LED | `02_Button_LED/main.c` | PC13 按键控制 PA5 LED | ⭐ |
| LED 流水灯 | `03_LED_Pattern/main.c` | 多种 LED 花样, 按键切换模式 | ⭐⭐ |

**硬件**: PA5=LED, PC13=Button (NUCLEO 板载)

---

## 02_UART - 串口通信

| 例程 | 文件 | 功能 | 难度 |
|------|------|------|------|
| Printf 重定向 | `01_UART_Printf/main.c` | printf 输出到串口, 系统信息 | ⭐ |
| 串口回显 | `02_UART_Echo/main.c` | 接收数据并回显, 中断接收 | ⭐⭐ |

**硬件**: PA2=TX, PA3=RX (USART2, 连接 ST-Link VCP)  
**波特率**: 115200, 8N1

---

## 03_Timer - 定时器

| 例程 | 文件 | 功能 | 难度 |
|------|------|------|------|
| 基础定时中断 | `01_Timer_Basic/main.c` | TIM2 1秒中断, 翻转 LED | ⭐⭐ |
| PWM 输出 | `02_PWM_Output/main.c` | TIM2_CH3 PWM, 呼吸灯效果 | ⭐⭐ |
| 输入捕获 | `03_Input_Capture/main.c` | TIM5 测量频率和占空比 | ⭐⭐⭐ |

**定时器计算**:
- TIM2 时钟 = 84MHz (APB1 x2)
- 频率 = TIM_CLK / (PSC+1) / (ARR+1)

---

## 04_ADC - 模数转换

| 例程 | 文件 | 功能 | 难度 |
|------|------|------|------|
| 单次采样 | `01_ADC_Single/main.c` | PA0 单通道 12位 ADC | ⭐⭐ |
| DMA 多通道 | `02_ADC_DMA/main.c` | 3通道扫描+DMA+均值滤波 | ⭐⭐⭐ |

**硬件**: PA0=CH0, PA1=CH1, PA4=CH4  
**参数**: 12位, 3.3V参考, V = ADC * 3.3 / 4095

---

## 05_I2C - I2C 总线

| 例程 | 文件 | 功能 | 难度 |
|------|------|------|------|
| 设备扫描 | `01_I2C_Scanner/main.c` | 扫描 0x08~0x77 所有地址 | ⭐⭐ |
| OLED 显示 | `02_I2C_OLED/main.c` | SSD1306 128x64 OLED 驱动 | ⭐⭐⭐ |

**硬件**: PB8=SCL, PB9=SDA (I2C1, 400KHz)  
**常见设备**: 0x3C=OLED, 0x68=MPU6050, 0x76=BMP280

---

## 06_SPI - SPI 总线

| 例程 | 文件 | 功能 | 难度 |
|------|------|------|------|
| W25Q Flash | `01_SPI_W25Q/main.c` | W25Q 读写擦除, JEDEC ID | ⭐⭐⭐ |

**硬件**: PA5=SCK, PA6=MISO, PA7=MOSI, PB6=CS  
**支持**: W25Q16/32/64/128

---

## 07_DMA - 直接内存访问

| 例程 | 文件 | 功能 | 难度 |
|------|------|------|------|
| 内存拷贝 | `01_DMA_MemCopy/main.c` | DMA M2M 传输, 速度对比 | ⭐⭐⭐ |

---

## 08_EXTI - 外部中断

| 例程 | 文件 | 功能 | 难度 |
|------|------|------|------|
| 外部中断 | `01_External_Interrupt/main.c` | PC13 下降沿中断, 软件消抖 | ⭐⭐ |

---

## 09_RTC - 实时时钟

| 例程 | 文件 | 功能 | 难度 |
|------|------|------|------|
| 日历时钟 | `01_RTC_Calendar/main.c` | 日期时间显示, 备份寄存器 | ⭐⭐⭐ |

**时钟源**: LSI (内部 ~32KHz) 或 LSE (外部 32.768KHz)

---

## 10_Watchdog - 看门狗

| 例程 | 文件 | 功能 | 难度 |
|------|------|------|------|
| IWDG | `01_IWDG/main.c` | 独立看门狗, 1秒超时, 复位检测 | ⭐⭐ |

---

## 11_LowPower - 低功耗

| 例程 | 文件 | 功能 | 难度 |
|------|------|------|------|
| 睡眠模式 | `01_Sleep_Mode/main.c` | Sleep 模式, 定时器/按键唤醒 | ⭐⭐⭐ |

**功耗对比**:
- 正常运行: ~30mA
- 睡眠模式: ~1-2mA  
- 停止模式: ~100-500uA
- 待机模式: ~2-5uA

---

## 12_FreeRTOS - 实时操作系统

| 例程 | 文件 | 功能 | 难度 |
|------|------|------|------|
| 基础任务 | `01_Task_Basic/main.c` | 3个任务, 互斥锁保护UART | ⭐⭐⭐⭐ |

**需要**: STM32CubeF4 FreeRTOS 中间件

---

## 15_Industrial - 工业应用

| 例程 | 文件 | 功能 | 难度 |
|------|------|------|------|
| Modbus RTU 从站 | `01_Modbus_RTU/main.c` | FC01-06, RS485, CRC16 | ⭐⭐⭐⭐ |
| PID 控制器 | `02_PID_Control/main.c` | 位置式+增量式PID, ADC反馈 | ⭐⭐⭐⭐ |
| 状态机 | `03_State_Machine/main.c` | 工业设备状态机, 长短按键 | ⭐⭐⭐ |

---

## 16_AI_ML - 嵌入式 AI/ML (TinyML)

| 例程 | 文件 | 功能 | 难度 |
|------|------|------|------|
| MLP 神经网络推理 | `01_Neural_Network/main.c` | 4→8→3 MLP, ReLU+Softmax, 传感器状态分类 | ⭐⭐⭐ |
| KNN 分类器 | `02_KNN_Classifier/main.c` | K=3近邻, 振动特征提取(均值/标准差/峰值因子/峭度) | ⭐⭐⭐ |
| 统计异常检测 | `03_Anomaly_Detection/main.c` | Z-Score+EMA+Welford在线算法, 自适应基线 | ⭐⭐⭐ |
| CMSIS-NN 量化推理 | `04_CMSIS_NN/main.c` | INT8量化, DWT性能分析, SIMD优化说明 | ⭐⭐⭐⭐ |
| 手势识别 | `05_Gesture_Recognition/main.c` | MPU6050+I2C, 手势分割, 模板匹配 | ⭐⭐⭐⭐ |

**算法说明**:
- `01`: 预训练权重嵌入 Flash，前向推理 <1ms
- `02`: 无需训练，存储 15 个标注样本，欧氏距离投票
- `03`: 两阶段：基线学习(100样本) → 实时 Z-Score 检测
- `04`: float32→INT8 量化节省 75% 内存，展示 CMSIS-NN API 对应关系
- `05`: MPU6050 三轴加速度，阈值分割手势，12维特征模板匹配

---

## 引脚分配总览 / Pin Assignment

| 引脚 | 功能 | 例程 |
|------|------|------|
| PA2 | USART2_TX (Debug) | 所有例程 |
| PA3 | USART2_RX (Debug) | 所有例程 |
| PA5 | LED LD2 | 所有例程 |
| PA0 | ADC1_IN0 / TIM5_CH1 | ADC, Timer |
| PA1 | ADC1_IN1 | ADC DMA |
| PA4 | ADC1_IN4 | ADC DMA |
| PA5 | SPI1_SCK | SPI |
| PA6 | SPI1_MISO | SPI |
| PA7 | SPI1_MOSI | SPI |
| PA8 | RS485 DE/RE | Modbus |
| PA9 | USART1_TX | Modbus |
| PA10 | USART1_RX | Modbus |
| PB6 | SPI CS / I2C1_SCL | SPI, I2C |
| PB8 | I2C1_SCL | I2C |
| PB9 | I2C1_SDA | I2C |
| PB10 | TIM2_CH3 (PWM) | Timer PWM, PID |
| PC13 | Button B1 | 所有按键例程 |

---

## 工具链要求 / Toolchain Requirements

- **编译器**: arm-none-eabi-gcc (GNU Arm Embedded Toolchain)
- **IDE**: STM32CubeIDE 或 Keil MDK 或 IAR EWARM
- **调试器**: ST-Link V2/V3 (NUCLEO 板载)
- **串口工具**: PuTTY / MobaXterm / 串口助手
- **固件库**: STM32CubeF4 (HAL 库)

详见 `docs/toolchain_setup.md`