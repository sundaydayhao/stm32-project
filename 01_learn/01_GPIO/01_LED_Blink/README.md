# 01 - LED 闪烁 / LED Blink

## 功能说明

最基础的 STM32 GPIO 输出例程，控制板载 LED 以 500ms 间隔闪烁。

## 硬件连接

| 信号 | 引脚 | 说明 |
|------|------|------|
| LED | PA5 | NUCLEO LD2 (板载绿色 LED) |

```
STM32 PA5 ──[330Ω]──┤>├── GND
                    LD2
(NUCLEO 板已内置此电路)
```

## 知识点

### GPIO 输出模式
- **推挽输出 (PP)**: 可以输出高低电平，驱动能力强
- **开漏输出 (OD)**: 只能拉低，需要外部上拉电阻

### HAL 库 GPIO 函数

```c
/* 初始化 GPIO */
HAL_GPIO_Init(GPIOx, &GPIO_InitStruct);

/* 写入 GPIO */
HAL_GPIO_WritePin(GPIOx, GPIO_Pin, PinState);

/* 读取 GPIO */
GPIO_PinState HAL_GPIO_ReadPin(GPIOx, GPIO_Pin);

/* 翻转 GPIO */
HAL_GPIO_TogglePin(GPIOx, GPIO_Pin);
```

### GPIO 速度选择

| 速度 | 最大频率 | 适用场景 |
|------|---------|---------|
| GPIO_SPEED_FREQ_LOW | 2 MHz | LED, 低速信号 |
| GPIO_SPEED_FREQ_MEDIUM | 12.5 MHz | 一般 GPIO |
| GPIO_SPEED_FREQ_HIGH | 50 MHz | SPI, I2C |
| GPIO_SPEED_FREQ_VERY_HIGH | 100 MHz | USB, 高速信号 |

## 代码解析

```c
/* 1. 使能 GPIO 时钟 (必须先使能时钟) */
__HAL_RCC_GPIOA_CLK_ENABLE();

/* 2. 配置 GPIO 参数 */
GPIO_InitTypeDef GPIO_InitStruct = {0};
GPIO_InitStruct.Pin   = GPIO_PIN_5;           // 引脚号
GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;  // 推挽输出
GPIO_InitStruct.Pull  = GPIO_NOPULL;          // 无上下拉
GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;  // 低速

/* 3. 初始化 GPIO */
HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

/* 4. 在循环中翻转 LED */
HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
HAL_Delay(500);
```

## 实验步骤

1. 打开 STM32CubeIDE
2. 导入本项目
3. 编译 (Ctrl+B)
4. 下载到 NUCLEO 板 (Run -> Debug)
5. 观察 LD2 绿色 LED 闪烁

## 扩展练习

1. 修改闪烁频率 (改变 `HAL_Delay` 参数)
2. 实现 SOS 摩斯码 (... --- ...)
3. 使用 `HAL_GPIO_WritePin` 替代 `HAL_GPIO_TogglePin`