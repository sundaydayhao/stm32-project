# STM32 全套例程库 / STM32 Complete Example Library

[![STM32](https://img.shields.io/badge/MCU-STM32F4-blue)](https://www.st.com/en/microcontrollers-microprocessors/stm32f4-series.html)
[![HAL](https://img.shields.io/badge/Library-STM32%20HAL-green)](https://www.st.com/en/embedded-software/stm32cube-mcu-mpu-packages.html)
[![License](https://img.shields.io/badge/License-MIT-yellow)](LICENSE)

## 简介 / Introduction

本项目是一套完整的 STM32 学习例程，适用于：
- 🎓 **学生** - 系统学习嵌入式开发
- 🔧 **创客** - 快速原型开发参考
- 🏭 **工业应用** - 可靠的工业级代码参考

This is a comprehensive STM32 example library suitable for students, makers, and industrial applications.

## 硬件参考 / Hardware Reference

| 开发板 | MCU | 主频 | Flash | RAM |
|--------|-----|------|-------|-----|
| NUCLEO-F401RE | STM32F401RE | 84 MHz | 512 KB | 96 KB |
| NUCLEO-F103RB | STM32F103RB | 72 MHz | 128 KB | 20 KB |
| NUCLEO-F446RE | STM32F446RE | 180 MHz | 512 KB | 128 KB |
| STM32F4DISCOVERY | STM32F407VG | 168 MHz | 1 MB | 192 KB |
| NUCLEO-H743ZI | STM32H743ZI | 480 MHz | 2 MB | 1 MB |

> **主要参考板**: NUCLEO-F401RE (STM32F401RE)

## 目录结构 / Directory Structure

```
stm32_learn/
├── docs/                    # 文档
│   ├── getting_started.md   # 快速入门
│   ├── toolchain_setup.md   # 工具链配置
│   └── hardware_reference.md # 硬件参考
├── common/                  # 公共库
├── 01_GPIO/                 # GPIO 例程
├── 02_UART/                 # 串口通信
├── 03_Timer/                # 定时器
├── 04_ADC/                  # 模数转换
├── 05_I2C/                  # I2C 通信
├── 06_SPI/                  # SPI 通信
├── 07_DMA/                  # DMA 传输
├── 08_EXTI/                 # 外部中断
├── 09_RTC/                  # 实时时钟
├── 10_Watchdog/             # 看门狗
├── 11_LowPower/             # 低功耗
├── 12_FreeRTOS/             # 实时操作系统
├── 13_CAN/                  # CAN 总线
├── 14_USB/                  # USB 通信
├── 15_Industrial/           # 工业应用
└── 16_AI_ML/                # 嵌入式 AI/ML (TinyML)
```

## 例程列表 / Example List

### 01 GPIO
| 编号 | 名称 | 说明 |
|------|------|------|
| 01 | LED_Blink | LED 闪烁基础 |
| 02 | Button_LED | 按键控制 LED |
| 03 | LED_Pattern | LED 流水灯 |

### 02 UART
| 编号 | 名称 | 说明 |
|------|------|------|
| 01 | UART_Printf | 串口打印调试 |
| 02 | UART_Echo | 串口回显 |
| 03 | UART_DMA | DMA 串口传输 |

### 03 Timer
| 编号 | 名称 | 说明 |
|------|------|------|
| 01 | Timer_Basic | 基础定时器中断 |
| 02 | PWM_Output | PWM 输出 |
| 03 | Input_Capture | 输入捕获测频率 |

### 04 ADC
| 编号 | 名称 | 说明 |
|------|------|------|
| 01 | ADC_Single | 单次 ADC 采样 |
| 02 | ADC_DMA | DMA 连续采样 |
| 03 | ADC_Temperature | 内部温度传感器 |

### 05 I2C
| 编号 | 名称 | 说明 |
|------|------|------|
| 01 | I2C_Scanner | I2C 设备扫描 |
| 02 | I2C_OLED | OLED 显示屏驱动 |
| 03 | I2C_MPU6050 | MPU6050 姿态传感器 |

### 06 SPI
| 编号 | 名称 | 说明 |
|------|------|------|
| 01 | SPI_Basic | SPI 基础通信 |
| 02 | SPI_W25Q | W25Q Flash 读写 |

### 07 DMA
| 编号 | 名称 | 说明 |
|------|------|------|
| 01 | DMA_MemToMem | 内存到内存传输 |
| 02 | DMA_UART | UART DMA 传输 |

### 08 EXTI
| 编号 | 名称 | 说明 |
|------|------|------|
| 01 | External_Interrupt | 外部中断基础 |

### 09 RTC
| 编号 | 名称 | 说明 |
|------|------|------|
| 01 | RTC_Calendar | RTC 日历功能 |
| 02 | RTC_Alarm | RTC 闹钟唤醒 |

### 10 Watchdog
| 编号 | 名称 | 说明 |
|------|------|------|
| 01 | IWDG | 独立看门狗 |
| 02 | WWDG | 窗口看门狗 |

### 11 LowPower
| 编号 | 名称 | 说明 |
|------|------|------|
| 01 | Sleep_Mode | 睡眠模式 |
| 02 | Stop_Mode | 停止模式 |
| 03 | Standby_Mode | 待机模式 |

### 12 FreeRTOS
| 编号 | 名称 | 说明 |
|------|------|------|
| 01 | Task_Basic | 任务创建基础 |
| 02 | Queue | 消息队列 |
| 03 | Semaphore | 信号量 |
| 04 | Mutex | 互斥锁 |

### 13 CAN
| 编号 | 名称 | 说明 |
|------|------|------|
| 01 | CAN_Basic | CAN 基础收发 |
| 02 | CAN_Filter | CAN 过滤器配置 |

### 14 USB
| 编号 | 名称 | 说明 |
|------|------|------|
| 01 | USB_CDC | USB 虚拟串口 |

### 15 Industrial
| 编号 | 名称 | 说明 |
|------|------|------|
| 01 | Modbus_RTU | Modbus RTU 从站 |
| 02 | PID_Controller | PID 控制器 |
| 03 | State_Machine | 状态机框架 |

### 16 AI/ML (TinyML)
| 编号 | 名称 | 说明 |
|------|------|------|
| 01 | Neural_Network | MLP 神经网络推理 (float32) |
| 02 | KNN_Classifier | K 近邻分类器 + 振动特征提取 |
| 03 | Anomaly_Detection | 统计异常检测 (Z-Score + EMA) |
| 04 | CMSIS_NN | ARM CMSIS-NN INT8 量化推理 |
| 05 | Gesture_Recognition | MPU6050 手势识别 |

## 快速开始 / Quick Start

1. 安装工具链: 参考 [docs/toolchain_setup.md](docs/toolchain_setup.md)
2. 克隆本仓库
3. 选择例程目录
4. 使用 STM32CubeIDE 或 Keil MDK 打开项目
5. 编译并下载到开发板

## 开发环境 / Development Environment

- **IDE**: STM32CubeIDE / Keil MDK / IAR EWARM / VSCode + PlatformIO
- **调试器**: ST-Link V2/V3 (NUCLEO 板载)
- **HAL 库**: STM32CubeF4 v1.27+
- **编译器**: ARM GCC 10.3+

## 贡献 / Contributing

欢迎提交 Issue 和 Pull Request！

## 许可证 / License

MIT License - 详见 [LICENSE](LICENSE)