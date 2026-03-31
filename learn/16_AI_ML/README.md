# 16_AI_ML - 嵌入式 AI/ML 应用 (TinyML on STM32)

## 模块简介

本模块介绍如何在 STM32 微控制器上运行 AI/ML 算法，即 **TinyML**（边缘 AI）。
无需云端服务器，直接在资源受限的嵌入式设备上完成智能推理。

```
16_AI_ML/
├── 01_Neural_Network/      # MLP 神经网络推理 (float32)
├── 02_KNN_Classifier/      # K 近邻分类器 + 特征提取
├── 03_Anomaly_Detection/   # 统计异常检测 (Z-Score + EMA)
├── 04_CMSIS_NN/            # ARM CMSIS-NN INT8 量化推理
└── 05_Gesture_Recognition/ # 手势识别 (MPU6050 + 模板匹配)
```

---

## 例程列表

| 编号 | 名称 | 算法 | 难度 | 应用场景 |
|------|------|------|------|----------|
| 01 | Neural_Network | MLP 前向推理 | ⭐⭐⭐ | 传感器状态分类 |
| 02 | KNN_Classifier | K 近邻 + 特征提取 | ⭐⭐⭐ | 振动故障诊断 |
| 03 | Anomaly_Detection | Z-Score + EMA + Welford | ⭐⭐⭐ | 设备健康监测 |
| 04 | CMSIS_NN | INT8 量化推理 + DWT 性能分析 | ⭐⭐⭐⭐ | 高效边缘推理 |
| 05 | Gesture_Recognition | MPU6050 + 模板匹配 | ⭐⭐⭐⭐ | 手势控制 |

---

## 核心概念速查

### 1. TinyML 工作流程

```
PC 端 (Python)                    STM32 端 (C)
─────────────────                 ──────────────────────
数据采集                          传感器读取 (ADC/I2C)
    ↓                                     ↓
数据标注                          特征提取 (均值/方差/峰值)
    ↓                                     ↓
模型训练                          模型推理 (前向传播)
(TensorFlow/PyTorch)                      ↓
    ↓                             结果输出 (LED/UART/控制)
权重导出为 C 数组
    ↓
烧录到 Flash
```

### 2. 各算法对比

| 算法 | 训练阶段 | 推理内存 | 推理速度 | 适用场景 |
|------|----------|----------|----------|----------|
| MLP 神经网络 | 离线 (PC) | ~200B | 快 | 多类别分类 |
| KNN 分类器 | 无（存储样本）| ~1KB | 中 | 小样本分类 |
| 统计异常检测 | 在线自学习 | ~100B | 极快 | 实时监测 |
| CMSIS-NN INT8 | 离线 (PC) | ~50B | 最快 | 资源极限场景 |
| 手势识别 | 无（模板）| ~500B | 中 | 人机交互 |

### 3. 内存使用估算

STM32F407 资源: Flash=1MB, RAM=192KB

| 模型 | Flash (权重) | RAM (激活) | 总计 |
|------|-------------|-----------|------|
| MLP float32 (4-8-3) | ~200B | ~100B | ~300B |
| MLP INT8 (4-8-3) | ~50B | ~50B | ~100B |
| KNN (15样本×4特征) | ~240B | ~100B | ~340B |
| 手势模板 (4×12) | ~192B | ~600B | ~800B |

---

## 例程详解

### 01_Neural_Network - MLP 神经网络推理

**网络结构:**
```
输入层 (4)  →  隐藏层 (8, ReLU)  →  输出层 (3, Softmax)
温度/湿度/光照/振动              正常/警告/报警
```

**关键代码:**
```c
/* 前向传播 */
// 第一层: h = ReLU(W1 * x + b1)
for (j = 0; j < HIDDEN_SIZE; j++) {
    acc = b1[j];
    for (i = 0; i < INPUT_SIZE; i++)
        acc += W1[i][j] * input[i];
    hidden[j] = (acc > 0.0f) ? acc : 0.0f;  // ReLU
}
// 第二层: y = Softmax(W2 * h + b2)
// Softmax: exp(x_i) / sum(exp(x_j))
```

**Python 训练脚本示例:**
```python
import tensorflow as tf
import numpy as np

model = tf.keras.Sequential([
    tf.keras.layers.Dense(8, activation='relu', input_shape=(4,)),
    tf.keras.layers.Dense(3, activation='softmax')
])
model.compile(optimizer='adam', loss='sparse_categorical_crossentropy')
model.fit(X_train, y_train, epochs=100)

# 导出权重为 C 数组
W1, b1 = model.layers[0].get_weights()
W2, b2 = model.layers[1].get_weights()
print("static const float W1[][] = {", W1.tolist(), "};")
```

---

### 02_KNN_Classifier - K 近邻分类器

**特征提取 (时域统计特征):**

| 特征 | 公式 | 物理意义 |
|------|------|----------|
| 均值 (Mean) | μ = Σx/N | 信号直流分量 |
| 标准差 (Std) | σ = √(Σ(x-μ)²/N) | 信号波动幅度 |
| 峰值因子 (Crest) | peak/RMS | 冲击特征，故障敏感 |
| 峭度 (Kurtosis) | E[(x-μ)⁴]/σ⁴ | 信号尖锐程度 |

**KNN 分类流程:**
```
采样窗口 (32点) → 特征提取 (4维) → Min-Max归一化
    → 计算到15个训练样本的欧氏距离
    → 选K=3个最近邻 → 多数投票 → 分类结果
```

---

### 03_Anomaly_Detection - 统计异常检测

**两阶段检测:**
1. **基线学习阶段** (100个样本): 使用 Welford 在线算法计算均值 μ 和标准差 σ
2. **实时检测阶段**: 计算 Z-Score = |x - μ| / σ，超过阈值则报警

**Welford 在线算法** (数值稳定，无需存储所有样本):
```c
// 每来一个新样本 x_n:
delta  = x_n - mean;
mean  += delta / n;
delta2 = x_n - mean;
M2    += delta * delta2;
// 方差 = M2 / (n-1)
```

**EMA 自适应基线:**
```c
ema_mean = α * x + (1-α) * ema_mean;  // α=0.1
ema_var  = α * (x-ema_mean)² + (1-α) * ema_var;
```

---

### 04_CMSIS_NN - INT8 量化推理

**量化原理:**
```
float32 权重 → INT8 量化 → 节省 75% 内存
float: -0.8 → int8: round(-0.8 / 0.00625) = -128
反量化: -128 * 0.00625 = -0.8
```

**CMSIS-NN API 对应关系:**

| 本示例函数 | CMSIS-NN API | 说明 |
|-----------|-------------|------|
| `QNN_FC_ReLU6_INT8()` | `arm_fully_connected_s8()` | INT8 全连接层 |
| `QNN_FC_Softmax()` | `arm_softmax_s8()` | Softmax 激活 |
| 输入量化 | `arm_requantize_s8()` | 重量化 |

**DWT 性能分析** (Cortex-M4):
```c
CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
DWT->CYCCNT = 0;
DWT->CTRL  |= DWT_CTRL_CYCCNTENA_Msk;
uint32_t t0 = DWT->CYCCNT;
// ... 推理代码 ...
uint32_t cycles = DWT->CYCCNT - t0;
// STM32F407@168MHz: cycles/168 = 推理时间(μs)
```

---

### 05_Gesture_Recognition - 手势识别

**硬件连接 (MPU6050):**
```
MPU6050 SCL → PB8 (I2C1_SCL)
MPU6050 SDA → PB9 (I2C1_SDA)
MPU6050 VCC → 3.3V
MPU6050 GND → GND
MPU6050 AD0 → GND  (I2C 地址 0x68)
```

**手势分割状态机:**
```
IDLE ──(mag > 0.3g)──→ CAPTURING ──(连续10帧 < 0.1g)──→ READY
  ↑                                                          |
  └──────────────────────────────────────────────────────────┘
                        处理完成后重置
```

**支持手势:**
| 手势 | 主要特征 | LED 指示 |
|------|----------|----------|
| 上挥 (SWIPE_UP) | ay_mean > 0 | 闪1次 |
| 下挥 (SWIPE_DOWN) | ay_mean < 0 | 闪2次 |
| 左挥 (SWIPE_LEFT) | ax_mean < 0 | 闪3次 |
| 右挥 (SWIPE_RIGHT) | ax_mean > 0 | 闪4次 |

---

## 推荐学习路径

```
初学者:
  03_Anomaly_Detection  →  无需训练，立即可用，理解统计ML

进阶:
  01_Neural_Network     →  理解神经网络推理原理
  02_KNN_Classifier     →  理解特征工程和距离度量

高级:
  04_CMSIS_NN           →  理解量化和ARM优化
  05_Gesture_Recognition →  综合应用：传感器+信号处理+ML
```

---

## 扩展资源

### 工具链
- **TensorFlow Lite for Microcontrollers**: https://www.tensorflow.org/lite/microcontrollers
- **Edge Impulse**: https://edgeimpulse.com (可视化训练+部署)
- **CMSIS-NN**: https://github.com/ARM-software/CMSIS-NN
- **STM32Cube.AI**: ST 官方 AI 工具，支持 Keras/ONNX 模型转换

### Python 权重导出脚本
```python
import numpy as np

def export_weights_to_c(model, filename="weights.h"):
    """将 Keras 模型权重导出为 C 头文件"""
    with open(filename, 'w') as f:
        f.write("/* 自动生成的权重文件 */\n\n")
        for i, layer in enumerate(model.layers):
            weights = layer.get_weights()
            if len(weights) == 2:
                W, b = weights
                # 导出 float32 权重
                f.write(f"static const float W{i+1}[{W.shape[0]}][{W.shape[1]}] = {{\n")
                for row in W:
                    f.write("    {" + ", ".join(f"{v:.6f}f" for v in row) + "},\n")
                f.write("};\n\n")
                # 导出 INT8 量化权重
                scale = np.max(np.abs(W)) / 127.0
                W_int8 = np.clip(np.round(W / scale), -128, 127).astype(np.int8)
                f.write(f"/* INT8 量化 (scale={scale:.6f}) */\n")
                f.write(f"static const int8_t W{i+1}_q[] = {{\n")
                f.write("    " + ", ".join(str(v) for v in W_int8.flatten()) + "\n")
                f.write("};\n\n")
    print(f"权重已导出到 {filename}")
```

### 相关文档
- [STM32 AI 生态系统](https://www.st.com/en/embedded-software/x-cube-ai.html)
- [ARM ML 示例](https://github.com/ARM-software/ML-examples)
- [TinyML 书籍](https://www.oreilly.com/library/view/tinyml/9781492052036/)