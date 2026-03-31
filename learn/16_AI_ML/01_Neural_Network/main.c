/**
 * @file    main.c
 * @brief   MLP 神经网络推理示例 (TinyML on STM32)
 * @details 在 STM32F401RE 上运行预训练的多层感知机 (MLP) 神经网络。
 *          网络结构: 输入(4) -> 隐藏层(8, ReLU) -> 输出(3, Softmax)
 *          应用场景: 传感器数据分类 (正常/警告/报警)
 *
 *          【神经网络推理流程】
 *          1. 读取传感器数据 (温度/湿度/光照/振动)
 *          2. 归一化到 [0,1]
 *          3. 前向传播: h = ReLU(W1*x + b1), y = Softmax(W2*h + b2)
 *          4. 取最大概率类别作为预测结果
 *
 *          【权重来源】
 *          使用 Python/TensorFlow 训练后导出为 C 数组:
 *          model = tf.keras.Sequential([Dense(8,'relu'), Dense(3,'softmax')])
 *          W1, b1 = model.layers[0].get_weights()
 *
 * @board   NUCLEO-F401RE (STM32F401RE @ 84MHz)
 * @author  STM32 学习项目
 * @version 2.0 (自包含版本，无需 bsp.h)
 */

#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdarg.h>

/* ==================== 网络结构参数 ==================== */
#define INPUT_SIZE      4   /* 输入特征数: 温度/湿度/光照/振动 */
#define HIDDEN_SIZE     8   /* 隐藏层神经元数 */
#define OUTPUT_SIZE     3   /* 输出类别数: 正常/警告/报警 */

/* ==================== 预训练权重 (float32) ==================== */
/*
 * 权重由 Python 训练后导出，存储在 Flash 中 (const)
 * 布局: W1[INPUT_SIZE][HIDDEN_SIZE], 列主序 (每列是一个神经元的权重)
 */
static const float W1[INPUT_SIZE][HIDDEN_SIZE] = {
    { 0.52f, -0.28f,  0.83f, -0.61f,  0.74f, -0.45f,  0.33f, -0.71f},
    {-0.31f,  0.65f, -0.19f,  0.87f, -0.52f,  0.76f, -0.44f,  0.58f},
    { 0.78f, -0.43f,  0.61f, -0.35f,  0.89f, -0.27f,  0.55f, -0.63f},
    {-0.24f,  0.81f, -0.55f,  0.42f, -0.33f,  0.69f, -0.78f,  0.36f}
};
static const float b1[HIDDEN_SIZE] = {
    0.12f, -0.08f, 0.21f, -0.15f, 0.09f, -0.18f, 0.14f, -0.11f
};

static const float W2[HIDDEN_SIZE][OUTPUT_SIZE] = {
    { 0.91f, -0.54f, -0.37f},
    {-0.47f,  0.83f, -0.36f},
    { 0.55f, -0.31f, -0.24f},
    {-0.38f,  0.67f, -0.29f},
    { 0.77f, -0.42f, -0.35f},
    {-0.89f,  0.51f,  0.38f},
    { 0.36f, -0.28f, -0.08f},
    {-0.44f,  0.33f,  0.11f}
};
static const float b2[OUTPUT_SIZE] = {0.05f, -0.03f, -0.02f};

/* ==================== 类别标签 ==================== */
static const char *CLASS_LABELS[OUTPUT_SIZE] = {"NORMAL", "WARNING", "ALARM"};

/* ==================== 推理结果结构体 ==================== */
typedef struct {
    float   hidden[HIDDEN_SIZE];    /* 隐藏层激活值 */
    float   output[OUTPUT_SIZE];    /* Softmax 输出概率 */
    uint8_t predicted;              /* 预测类别 */
    float   confidence;             /* 置信度 */
} NNResult_TypeDef;

/* ==================== 全局变量 ==================== */
UART_HandleTypeDef huart2;
static NNResult_TypeDef result;
static uint32_t infer_count = 0;
static uint32_t last_tick   = 0;

/* ==================== 函数声明 ==================== */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_Init(void);
static void NN_Forward(const float *input, NNResult_TypeDef *res);
static void Sensor_GetFeatures(float *feat);
static void Print_Result(const float *feat, NNResult_TypeDef *res);

/* ==================== printf 重定向 ==================== */
int __io_putchar(int ch)
{
    HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}

/* ==================== 主函数 ==================== */
int main(void)
{
    float features[INPUT_SIZE];

    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART2_Init();

    printf("\r\n");
    printf("========================================\r\n");
    printf("  MLP 神经网络推理示例 (TinyML)\r\n");
    printf("  网络: %d -> %d(ReLU) -> %d(Softmax)\r\n",
           INPUT_SIZE, HIDDEN_SIZE, OUTPUT_SIZE);
    printf("  板卡: NUCLEO-F401RE @ 84MHz\r\n");
    printf("========================================\r\n\r\n");

    while (1) {
        if (HAL_GetTick() - last_tick >= 1000) {
            last_tick = HAL_GetTick();
            infer_count++;

            /* 1. 获取传感器特征 */
            Sensor_GetFeatures(features);

            /* 2. 神经网络前向推理 */
            NN_Forward(features, &result);

            /* 3. 打印结果 */
            Print_Result(features, &result);

            /* 4. LED 指示: 正常=常亮, 警告=慢闪, 报警=快闪 */
            if (result.predicted == 0) {
                HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
            } else if (result.predicted == 1) {
                HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
            } else {
                HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
                HAL_Delay(100);
                HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
            }
        }
    }
}

/* ==================== 神经网络前向传播 ==================== */
/**
 * @brief  MLP 前向推理
 * @param  input: 输入特征向量 [INPUT_SIZE]，值域 [0,1]
 * @param  res:   推理结果结构体
 *
 * 计算流程:
 *   h_j = ReLU( sum_i(W1[i][j] * x[i]) + b1[j] )
 *   y_k = Softmax( sum_j(W2[j][k] * h_j) + b2[k] )
 */
static void NN_Forward(const float *input, NNResult_TypeDef *res)
{
    uint8_t i, j, k;
    float acc;

    /* === 第一层: 全连接 + ReLU === */
    for (j = 0; j < HIDDEN_SIZE; j++) {
        acc = b1[j];
        for (i = 0; i < INPUT_SIZE; i++) {
            acc += W1[i][j] * input[i];
        }
        res->hidden[j] = (acc > 0.0f) ? acc : 0.0f;  /* ReLU */
    }

    /* === 第二层: 全连接 === */
    float logits[OUTPUT_SIZE];
    for (k = 0; k < OUTPUT_SIZE; k++) {
        acc = b2[k];
        for (j = 0; j < HIDDEN_SIZE; j++) {
            acc += W2[j][k] * res->hidden[j];
        }
        logits[k] = acc;
    }

    /* === Softmax (数值稳定版本) === */
    /* 减去最大值防止 exp 溢出 */
    float max_val = logits[0];
    for (k = 1; k < OUTPUT_SIZE; k++) {
        if (logits[k] > max_val) max_val = logits[k];
    }
    float sum = 0.0f;
    for (k = 0; k < OUTPUT_SIZE; k++) {
        res->output[k] = expf(logits[k] - max_val);
        sum += res->output[k];
    }
    for (k = 0; k < OUTPUT_SIZE; k++) {
        res->output[k] /= sum;
    }

    /* === 取最大概率类别 === */
    res->predicted  = 0;
    res->confidence = res->output[0];
    for (k = 1; k < OUTPUT_SIZE; k++) {
        if (res->output[k] > res->confidence) {
            res->confidence = res->output[k];
            res->predicted  = k;
        }
    }
}

/* ==================== 传感器特征获取 ==================== */
/**
 * @brief  获取传感器特征 (实际项目替换为真实 ADC/I2C 读取)
 * @param  feat: 特征数组 [温度, 湿度, 光照, 振动]，归一化到 [0,1]
 */
static void Sensor_GetFeatures(float *feat)
{
    /* 模拟正常工况 */
    feat[0] = 0.45f + (float)(infer_count % 10) * 0.005f;  /* 温度 ~45% */
    feat[1] = 0.60f + (float)(infer_count % 8)  * 0.005f;  /* 湿度 ~60% */
    feat[2] = 0.70f + (float)(infer_count % 12) * 0.005f;  /* 光照 ~70% */
    feat[3] = 0.05f + (float)(infer_count % 6)  * 0.002f;  /* 振动 ~5% */

    /* 每 10 次模拟警告状态 */
    if (infer_count % 10 == 0) {
        feat[0] = 0.72f;  /* 温度偏高 */
        feat[3] = 0.45f;  /* 振动增大 */
    }
    /* 每 20 次模拟报警状态 */
    if (infer_count % 20 == 0) {
        feat[0] = 0.91f;  /* 高温 */
        feat[3] = 0.88f;  /* 强振动 */
    }
}

/* ==================== 打印推理结果 ==================== */
static void Print_Result(const float *feat, NNResult_TypeDef *res)
{
    printf("--- 推理 #%lu ---\r\n", infer_count);
    printf("输入: 温度=%.2f 湿度=%.2f 光照=%.2f 振动=%.2f\r\n",
           feat[0], feat[1], feat[2], feat[3]);
    printf("概率: NORMAL=%.1f%% WARNING=%.1f%% ALARM=%.1f%%\r\n",
           res->output[0]*100.0f, res->output[1]*100.0f, res->output[2]*100.0f);
    printf("预测: [%s] 置信度=%.1f%%\r\n\r\n",
           CLASS_LABELS[res->predicted], res->confidence*100.0f);
}

/* ==================== 系统时钟配置 (84MHz) ==================== */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef o = {0};
    RCC_ClkInitTypeDef c = {0};
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);
    o.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    o.HSEState       = RCC_HSE_BYPASS;
    o.PLL.PLLState   = RCC_PLL_ON;
    o.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    o.PLL.PLLM = 8; o.PLL.PLLN = 336;
    o.PLL.PLLP = RCC_PLLP_DIV4; o.PLL.PLLQ = 7;
    HAL_RCC_OscConfig(&o);
    c.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                       RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    c.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    c.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    c.APB1CLKDivider = RCC_HCLK_DIV2;
    c.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&c, FLASH_LATENCY_2);
}

/* ==================== GPIO 初始化 ==================== */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef g = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    /* PA5: LED LD2 */
    g.Pin = GPIO_PIN_5; g.Mode = GPIO_MODE_OUTPUT_PP;
    g.Pull = GPIO_NOPULL; g.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &g);
    /* PC13: Button B1 */
    g.Pin = GPIO_PIN_13; g.Mode = GPIO_MODE_INPUT; g.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &g);
}

/* ==================== USART2 初始化 (115200, 8N1) ==================== */
static void MX_USART2_Init(void)
{
    huart2.Instance          = USART2;
    huart2.Init.BaudRate     = 115200;
    huart2.Init.WordLength   = UART_WORDLENGTH_8B;
    huart2.Init.StopBits     = UART_STOPBITS_1;
    huart2.Init.Parity       = UART_PARITY_NONE;
    huart2.Init.Mode         = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart2);
}

/* ==================== UART MSP 初始化回调 ==================== */
void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    GPIO_InitTypeDef g = {0};
    if (huart->Instance == USART2) {
        __HAL_RCC_USART2_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();
        /* PA2=TX, PA3=RX (AF7) */
        g.Pin       = GPIO_PIN_2 | GPIO_PIN_3;
        g.Mode      = GPIO_MODE_AF_PP;
        g.Pull      = GPIO_NOPULL;
        g.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
        g.Alternate = GPIO_AF7_USART2;
        HAL_GPIO_Init(GPIOA, &g);
    }
}

/* ==================== 错误处理 ==================== */
void Error_Handler(void)
{
    __disable_irq();
    while (1) {
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
        HAL_Delay(100);
    }
}