/**
 * @file    main.c
 * @brief   K 近邻 (KNN) 分类器示例
 * @details 使用 ADC 采集振动信号，提取时域特征，通过 KNN 算法分类旋转机械状态。
 *
 *          【KNN 算法流程】
 *          1. 采样窗口: 32 点 ADC 数据 @ 1kHz
 *          2. 特征提取: 均值、标准差、峰值因子、峭度 (4维)
 *          3. Min-Max 归一化
 *          4. 计算到 15 个训练样本的欧氏距离
 *          5. 选 K=3 个最近邻，多数投票
 *
 *          硬件: PA0=ADC1_IN0(振动传感器), PA5=LED, PA2/PA3=UART
 *
 * @board   NUCLEO-F401RE (STM32F401RE @ 84MHz)
 * @version 2.0 (自包含版本)
 */

#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

/* ==================== 参数定义 ==================== */
#define WINDOW_SIZE     32
#define FEATURE_DIM     4
#define NUM_CLASSES     3
#define NUM_TRAIN       15
#define K_NEIGHBORS     3

static const char *CLASS_LABELS[NUM_CLASSES] = {"NORMAL", "UNBALANCE", "FAULT"};

/* ==================== 训练样本 (特征已归一化) ==================== */
static const float train_features[NUM_TRAIN][FEATURE_DIM] = {
    {0.50f, 0.05f, 0.15f, 0.10f}, {0.51f, 0.06f, 0.14f, 0.11f},
    {0.49f, 0.05f, 0.16f, 0.09f}, {0.50f, 0.04f, 0.13f, 0.10f},
    {0.52f, 0.06f, 0.15f, 0.12f},
    {0.50f, 0.25f, 0.45f, 0.35f}, {0.51f, 0.27f, 0.48f, 0.38f},
    {0.49f, 0.24f, 0.43f, 0.33f}, {0.50f, 0.26f, 0.46f, 0.36f},
    {0.52f, 0.28f, 0.50f, 0.40f},
    {0.50f, 0.65f, 0.85f, 0.80f}, {0.51f, 0.68f, 0.88f, 0.83f},
    {0.49f, 0.63f, 0.82f, 0.78f}, {0.50f, 0.66f, 0.86f, 0.81f},
    {0.52f, 0.70f, 0.90f, 0.85f}
};
static const uint8_t train_labels[NUM_TRAIN] = {
    0,0,0,0,0, 1,1,1,1,1, 2,2,2,2,2
};
static const float feat_min[FEATURE_DIM] = {0.48f, 0.03f, 0.10f, 0.08f};
static const float feat_max[FEATURE_DIM] = {0.53f, 0.72f, 0.92f, 0.87f};

/* ==================== 全局变量 ==================== */
UART_HandleTypeDef huart2;
ADC_HandleTypeDef  hadc1;
static float   adc_window[WINDOW_SIZE];
static uint8_t window_idx  = 0;
static uint8_t window_full = 0;
static uint32_t sample_tick = 0;
static uint32_t infer_count = 0;

/* ==================== 函数声明 ==================== */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_Init(void);
static void MX_ADC1_Init(void);
static void Feature_Extract(const float *win, float *feat);
static void Feature_Normalize(float *feat);
static uint8_t KNN_Classify(const float *feat);
static float   Euclidean_Dist(const float *a, const float *b, uint8_t dim);

int __io_putchar(int ch)
{
    HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}

/* ==================== 主函数 ==================== */
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART2_Init();
    MX_ADC1_Init();

    printf("\r\n========================================\r\n");
    printf("  KNN 振动故障诊断示例\r\n");
    printf("  特征: 均值/标准差/峰值因子/峭度\r\n");
    printf("  训练样本: %d, K=%d\r\n", NUM_TRAIN, K_NEIGHBORS);
    printf("========================================\r\n\r\n");

    while (1) {
        /* 1kHz 采样 */
        if (HAL_GetTick() - sample_tick >= 1) {
            sample_tick = HAL_GetTick();

            /* 读取 ADC */
            HAL_ADC_Start(&hadc1);
            HAL_ADC_PollForConversion(&hadc1, 5);
            uint32_t raw = HAL_ADC_GetValue(&hadc1);
            HAL_ADC_Stop(&hadc1);

            /* 归一化到 [0,1] */
            adc_window[window_idx] = (float)raw / 4095.0f;
            window_idx = (window_idx + 1) % WINDOW_SIZE;
            if (window_idx == 0) window_full = 1;
        }

        /* 窗口满时执行推理 */
        if (window_full) {
            window_full = 0;
            infer_count++;

            float feat[FEATURE_DIM];
            Feature_Extract(adc_window, feat);
            Feature_Normalize(feat);
            uint8_t cls = KNN_Classify(feat);

            printf("推理#%lu | 特征:[%.3f,%.3f,%.3f,%.3f] -> [%s]\r\n",
                   infer_count, feat[0], feat[1], feat[2], feat[3],
                   CLASS_LABELS[cls]);

            /* LED: 正常=常亮, 不平衡=慢闪, 故障=快闪 */
            if (cls == 0) {
                HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
            } else if (cls == 1) {
                HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
                HAL_Delay(500);
            } else {
                HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
                HAL_Delay(100);
            }
        }
    }
}

/* ==================== 特征提取 ==================== */
/**
 * @brief  从采样窗口提取 4 维时域特征
 * @param  win:  采样窗口数据 [WINDOW_SIZE]
 * @param  feat: 输出特征 [均值, 标准差, 峰值因子, 峭度]
 */
static void Feature_Extract(const float *win, float *feat)
{
    uint8_t i;
    float sum = 0.0f, peak = 0.0f;

    /* 均值 */
    for (i = 0; i < WINDOW_SIZE; i++) {
        sum += win[i];
        if (win[i] > peak) peak = win[i];
    }
    feat[0] = sum / WINDOW_SIZE;  /* 均值 (Mean) */

    /* 标准差 */
    float var = 0.0f;
    for (i = 0; i < WINDOW_SIZE; i++) {
        float d = win[i] - feat[0];
        var += d * d;
    }
    var = var / WINDOW_SIZE;
    float std = sqrtf(var);
    feat[1] = std;  /* 标准差 (Std) */

    /* 峰值因子 = peak / RMS */
    float rms = sqrtf(var + feat[0] * feat[0]);
    feat[2] = (rms > 1e-6f) ? (peak / rms) : 0.0f;  /* 峰值因子 (Crest Factor) */

    /* 峭度 = E[(x-mean)^4] / std^4 */
    float kurt = 0.0f;
    for (i = 0; i < WINDOW_SIZE; i++) {
        float d = win[i] - feat[0];
        kurt += d * d * d * d;
    }
    kurt /= WINDOW_SIZE;
    float std4 = var * var;
    feat[3] = (std4 > 1e-12f) ? (kurt / std4) : 0.0f;  /* 峭度 (Kurtosis) */
}

/* ==================== Min-Max 归一化 ==================== */
static void Feature_Normalize(float *feat)
{
    for (uint8_t i = 0; i < FEATURE_DIM; i++) {
        float range = feat_max[i] - feat_min[i];
        if (range > 1e-6f) {
            feat[i] = (feat[i] - feat_min[i]) / range;
        }
        /* 裁剪到 [0,1] */
        if (feat[i] < 0.0f) feat[i] = 0.0f;
        if (feat[i] > 1.0f) feat[i] = 1.0f;
    }
}

/* ==================== KNN 分类 ==================== */
/**
 * @brief  K 近邻分类 (欧氏距离 + 多数投票)
 * @param  feat: 归一化特征向量
 * @retval 预测类别
 */
static uint8_t KNN_Classify(const float *feat)
{
    /* 计算到所有训练样本的距离 */
    float dists[NUM_TRAIN];
    for (uint8_t i = 0; i < NUM_TRAIN; i++) {
        dists[i] = Euclidean_Dist(feat, train_features[i], FEATURE_DIM);
    }

    /* 选 K 个最近邻 (简单选择排序) */
    uint8_t votes[NUM_CLASSES] = {0};
    for (uint8_t k = 0; k < K_NEIGHBORS; k++) {
        uint8_t min_idx = 0;
        float   min_d   = dists[0];
        for (uint8_t i = 1; i < NUM_TRAIN; i++) {
            if (dists[i] < min_d) { min_d = dists[i]; min_idx = i; }
        }
        votes[train_labels[min_idx]]++;
        dists[min_idx] = 1e10f;  /* 标记已选 */
    }

    /* 多数投票 */
    uint8_t best = 0;
    for (uint8_t c = 1; c < NUM_CLASSES; c++) {
        if (votes[c] > votes[best]) best = c;
    }
    return best;
}

/* ==================== 欧氏距离 ==================== */
static float Euclidean_Dist(const float *a, const float *b, uint8_t dim)
{
    float sum = 0.0f;
    for (uint8_t i = 0; i < dim; i++) {
        float d = a[i] - b[i];
        sum += d * d;
    }
    return sqrtf(sum);
}

/* ==================== ADC1 初始化 (PA0, 12位) ==================== */
static void MX_ADC1_Init(void)
{
    ADC_ChannelConfTypeDef s = {0};
    GPIO_InitTypeDef g = {0};
    __HAL_RCC_ADC1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    g.Pin = GPIO_PIN_0; g.Mode = GPIO_MODE_ANALOG; g.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &g);
    hadc1.Instance = ADC1;
    hadc1.Init.ClockPrescaler        = ADC_CLOCK_SYNC_PCLK_DIV4;
    hadc1.Init.Resolution            = ADC_RESOLUTION_12B;
    hadc1.Init.ScanConvMode          = DISABLE;
    hadc1.Init.ContinuousConvMode    = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc1.Init.ExternalTrigConv      = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion       = 1;
    hadc1.Init.DMAContinuousRequests = DISABLE;
    hadc1.Init.EOCSelection          = ADC_EOC_SINGLE_CONV;
    HAL_ADC_Init(&hadc1);
    s.Channel      = ADC_CHANNEL_0;
    s.Rank         = 1;
    s.SamplingTime = ADC_SAMPLETIME_56CYCLES;
    HAL_ADC_ConfigChannel(&hadc1, &s);
}

/* ==================== 系统时钟配置 (84MHz) ==================== */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef o = {0};
    RCC_ClkInitTypeDef c = {0};
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

/* ==================== GPIO 初始化 ==================== */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef g = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    g.Pin = GPIO_PIN_5; g.Mode = GPIO_MODE_OUTPUT_PP;
    g.Pull = GPIO_NOPULL; g.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &g);
    g.Pin = GPIO_PIN_13; g.Mode = GPIO_MODE_INPUT; g.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &g);
}

/* ==================== USART2 初始化 ==================== */
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
    if (huart->Instance == USART2) {
        __HAL_RCC_USART2_CLK_ENABLE(); __HAL_RCC_GPIOA_CLK_ENABLE();
        g.Pin = GPIO_PIN_2|GPIO_PIN_3; g.Mode = GPIO_MODE_AF_PP;
        g.Pull = GPIO_NOPULL; g.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        g.Alternate = GPIO_AF7_USART2; HAL_GPIO_Init(GPIOA, &g);
    }
}

void Error_Handler(void)
{
    __disable_irq();
    while (1) { HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5); HAL_Delay(100); }
}