# 项目文件结构 / Project File Structure

## 完整文件列表

```
stm32_learn/
│
├── README.md                           # 项目总说明 (中英双语)
├── LICENSE                             # MIT 开源许可证
│
├── .vscode/
│   ├── c_cpp_properties.json          # C/C++ IntelliSense 配置
│   └── settings.json                  # VS Code 编辑器设置
│
├── common/                             # 公共库 (所有例程共用)
│   ├── Inc/
│   │   ├── bsp.h                      # 板级支持包 (BSP) 头文件
│   │   └── stm32f4xx_hal_conf.h       # HAL 库配置文件
│   └── Src/
│       └── bsp.c                      # BSP 实现 (LED/Button/UART 封装)
│
├── docs/                               # 文档目录
│   ├── getting_started.md             # 快速入门指南
│   ├── toolchain_setup.md             # 工具链安装配置
│   ├── hardware_reference.md          # 硬件参考 (引脚图/原理图)
│   ├── examples_index.md              # 例程索引
│   └── project_structure.md           # 本文件
│
├── 01_GPIO/                            # GPIO 例程
│   ├── 01_LED_Blink/
│   │   ├── main.c                     # LED 闪烁
│   │   └── README.md                  # 例程说明
│   ├── 02_Button_LED/
│   │   └── main.c                     # 按键控制 LED
│   └── 03_LED_Pattern/
│       └── main.c                     # LED 花样 (流水灯/跑马灯)
│
├── 02_UART/                            # UART 串口例程
│   ├── 01_UART_Printf/
│   │   └── main.c                     # printf 重定向到串口
│   └── 02_UART_Echo/
│       └── main.c                     # 串口回显 (中断接收)
│
├── 03_Timer/                           # 定时器例程
│   ├── 01_Timer_Basic/
│   │   └── main.c                     # TIM2 基础定时中断 (1秒)
│   ├── 02_PWM_Output/
│   │   └── main.c                     # TIM2_CH3 PWM 输出 (呼吸灯)
│   └── 03_Input_Capture/
│       └── main.c                     # TIM5 输入捕获 (测频率/占空比)
│
├── 04_ADC/                             # ADC 模数转换例程
│   ├── 01_ADC_Single/
│   │   └── main.c                     # ADC1 单通道单次采样
│   └── 02_ADC_DMA/
│       └── main.c                     # ADC1 3通道 DMA 连续采样+均值滤波
│
├── 05_I2C/                             # I2C 总线例程
│   ├── 01_I2C_Scanner/
│   │   └── main.c                     # I2C 设备地址扫描
│   └── 02_I2C_OLED/
│       └── main.c                     # SSD1306 OLED 显示驱动
│
├── 06_SPI/                             # SPI 总线例程
│   └── 01_SPI_W25Q/
│       └── main.c                     # W25Q Flash 读写擦除
│
├── 07_DMA/                             # DMA 直接内存访问例程
│   └── 01_DMA_MemCopy/
│       └── main.c                     # DMA 内存到内存传输
│
├── 08_EXTI/                            # 外部中断例程
│   └── 01_External_Interrupt/
│       └── main.c                     # PC13 外部中断 (软件消抖)
│
├── 09_RTC/                             # 实时时钟例程
│   └── 01_RTC_Calendar/
│       └── main.c                     # RTC 日历 (LSI时钟, 备份寄存器)
│
├── 10_Watchdog/                        # 看门狗例程
│   └── 01_IWDG/
│       └── main.c                     # 独立看门狗 (1秒超时)
│
├── 11_LowPower/                        # 低功耗例程
│   └── 01_Sleep_Mode/
│       └── main.c                     # 睡眠模式 (定时器/按键唤醒)
│
├── 12_FreeRTOS/                        # FreeRTOS 实时操作系统例程
│   └── 01_Task_Basic/
│       └── main.c                     # 3任务 + 互斥锁 (需要FreeRTOS源码)
│
├── 13_CAN/                             # CAN 总线例程
│   └── 01_CAN_Basic/
│       └── main.c                     # CAN 500Kbps 收发 (需要收发器)
│
├── 15_Industrial/                      # 工业应用例程
│   ├── 01_Modbus_RTU/
│   │   └── main.c                     # Modbus RTU 从站 (FC01-06, RS485)
│   ├── 02_PID_Control/
│   │   └── main.c                     # PID 控制器 (位置式+增量式)
│   └── 03_State_Machine/
│       └── main.c                     # 工业设备状态机
│
└── 16_AI_ML/                           # 嵌入式 AI/ML 例程 (TinyML)
    ├── README.md                      # AI/ML 模块说明文档
    ├── 01_Neural_Network/
    │   └── main.c                     # MLP 神经网络推理 (float32, ReLU+Softmax)
    ├── 02_KNN_Classifier/
    │   └── main.c                     # K 近邻分类器 (振动故障诊断)
    ├── 03_Anomaly_Detection/
    │   └── main.c                     # 统计异常检测 (Z-Score + EMA + Welford)
    ├── 04_CMSIS_NN/
    │   └── main.c                     # CMSIS-NN INT8 量化推理 + DWT 性能分析
    └── 05_Gesture_Recognition/
        └── main.c                     # MPU6050 手势识别 (模板匹配)
```

## 例程总数统计

| 类别 | 例程数 | 说明 |
|------|--------|------|
| GPIO | 3 | 基础输入输出 |
| UART | 2 | 串口通信 |
| Timer | 3 | 定时/PWM/捕获 |
| ADC | 2 | 模数转换 |
| I2C | 2 | I2C总线 |
| SPI | 1 | SPI总线 |
| DMA | 1 | 直接内存访问 |
| EXTI | 1 | 外部中断 |
| RTC | 1 | 实时时钟 |
| Watchdog | 1 | 看门狗 |
| LowPower | 1 | 低功耗 |
| FreeRTOS | 1 | 实时操作系统 |
| CAN | 1 | CAN总线 |
| Industrial | 3 | 工业应用 |
| AI/ML | 5 | 嵌入式 AI/TinyML |
| **合计** | **28** | |

## 适用人群

- 🎓 **学生**: 从 GPIO 开始, 循序渐进学习嵌入式开发
- 🔧 **创客**: 快速上手各种外设, 实现创意项目
- 🏭 **工业**: Modbus/PID/状态机等工业级应用参考
- 🤖 **AI/ML**: TinyML 边缘推理，神经网络/KNN/异常检测/手势识别
