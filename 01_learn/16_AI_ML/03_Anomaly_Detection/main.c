/**
 * @file    main.c
 * @brief   统计异常检测示例 (Z-Score + EMA + Welford)
 * @details 两阶段检测: 基线学习 -> 实时 Z-Score 异常检测
 *          无需预训练，设备上电后自动学习正常工况基线。
 *
 *          【算法说明】
 *          阶段1 - 基线学习 (前 100 个样本):
 *            使用 Welford 在线算法计算均值 μ 和标准差 σ
 *            Welford 算法: 数值稳定，无需存储所有样本
 *
 *          阶段2 - 实时检测:
 *            Z-Score = |x - μ| / σ
 *            Z > 3.0 → 异常 (超过 3σ 范围)
 *            EMA 自适应基线: 缓慢跟踪正常工况漂移
 *
 *          硬件: PA0=ADC1_IN0(传感器), PA5=LED, PA2/PA3=UART
 *
 * @board   NUCLEO-F401RE (STM32F401RE @ 84MHz)
 * @version 2.0 (自包含版本)
 */

#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

/* ==================== 参数定义 ==================== */
#define BASELINE_SAMPLES    100     /* 基线学习样本数 */
#define ZSCORE_THRESHOLD    3.0f    /* Z-Score 异常阈值 (3σ 原则) */
#define EMA_ALPHA           0.01f   /* EMA 自适应速率 (越小越稳定) */
#define SAMPLE_INTERVAL_MS  100     /* 采样间隔 (ms) */

/* ==================== 检测器状态 ==================== */
typedef enum {
    DETECTOR_LEARNING = 0,  /* 基线学习阶段 */
    DETECTOR_READY    = 1,  /* 检测就绪 */
} DetectorState_t;

/* ==================== 检测器结构体 ==================== */
typedef struct {
    /* Welford 在线算法变量 */
    uint32_t n;         /* 样本计数 */
    float    mean;      /* 当前均值 */
    float    M2;        /* 方差累积量 */
    float    std;       /* 标准差 */

    /* EMA 自适应基线 */
    float    ema_mean;  /* EMA 均值 */
    float    ema_var;   /* EMA 方差 */

    /* 检测结果 */
    float    zscore;    /* 当前 Z-Score */
    uint8_t  is_anomaly;/* 是否异常 */
    uint32_t anomaly_count; /* 累计异常次数 */

    DetectorState_t state;
} AnomalyDetector_t;

/* ==================== 全局变量 ==================== */
UART_HandleTypeDef huart2;
ADC_HandleTypeDef  hadc1;
static AnomalyDetector_t detector;
static uint32_t sample_tick  = 0;
static uint32_t sample_count = 0;

/* ==================== 函数声明 ==================== */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_Init(void);
static void MX_ADC1_Init(void);
static void Detector_Init(AnomalyDetector_t *det);
static void Detector_Update(AnomalyDetector_t *det, float x);
static float ADC_ReadNormalized(void);

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
    Detector_Init(&detector);

    printf("\r\n========================================\r\n");
    printf("  统计异常检测示例\r\n");
    printf("  算法: Welford + Z-Score + EMA\r\n");
    printf("  基线学习: %d 个样本\r\n", BASELINE_SAMPLES);
    printf("  异常阈值: Z > %.1f\r\n", ZSCORE_THRESHOLD);
    printf("========================================\r\n");
    printf("正在学习基线...\r\n\r\n");

    while (1) {
        if (HAL_GetTick() - sample_tick >= SAMPLE_INTERVAL_MS) {
            sample_tick = HAL_GetTick();
            sample_count++;

            /* 读取传感器 */
            float x = ADC_ReadNormalized();

            /* 更新检测器 */
            Detector_Update(&detector, x);

            /* 打印状态 */
            if (detector.state == DETECTOR_LEARNING) {
                if (sample_count % 10 == 0) {
                    printf("学习中 [%lu/%d] 均值=%.4f 标准差=%.4f\r\n",
                           detector.n, BASELINE_SAMPLES,
                           detector.mean, detector.std);
                }
                /* 学习阶段 LED 慢闪 */
                HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
            } else {
                /* 检测阶段 */
                if (sample_count % 5 == 0) {
                    printf("[%lu] x=%.4f Z=%.2f %s\r\n",
                           sample_count, x, detector.zscore,
                           detector.is_anomaly ? "<<ANOMALY>>" : "normal");
                }
                /* 每 50 次打印统计 */
                if (sample_count % 50 == 0) {
                    printf("--- 统计: 总样本=%lu 异常=%lu (%.1f%%) ---\r\n",
                           sample_count,
                           detector.anomaly_count,
                           (float)detector.anomaly_count / sample_count * 100.0f);
                    printf("    EMA均值=%.4f EMA标准差=%.4f\r\n\r\n",
                           detector.ema_mean, sqrtf(detector.ema_var));
                }
                /* LED: 正常=常亮, 异常=快闪 */
                if (detector.is_anomaly) {
                    HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
                } else {
                    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
                }
            }
        }
    }
}

/* ==================== 检测器初始化 ==================== */
static void Detector_Init(AnomalyDetector_t *det)
{
    memset(det, 0, sizeof(AnomalyDetector_t));
    det->state = DETECTOR_LEARNING;
}

/* ==================== 检测器更新 (Welford + EMA + Z-Score) ==================== */
/**
 * @brief  处理一个新样本
 * @param  det: 检测器指针
 * @param  x:   新样本值
 *
 * Welford 在线算法 (数值稳定):
 *   delta  = x - mean
 *   mean  += delta / n
 *   delta2 = x - mean
 *   M2    += delta * delta2
 *   var    = M2 / (n-1)  (n >= 2)
 */
static void Detector_Update(AnomalyDetector_t *det, float x)
{
    det->n++;

    if (det->state == DETECTOR_LEARNING) {
        /* === Welford 在线算法更新均值和方差 === */
        float delta  = x - det->mean;
        det->mean   += delta / (float)det->n;
        float delta2 = x - det->mean;
        det->M2     += delta * delta2;

        if (det->n >= 2) {
            det->std = sqrtf(det->M2 / (float)(det->n - 1));
        }

        /* 基线学习完成 */
        if (det->n >= BASELINE_SAMPLES) {
            det->state    = DETECTOR_READY;
            det->ema_mean = det->mean;
            det->ema_var  = det->std * det->std;
            printf("\r\n基线学习完成!\r\n");
            printf("  均值 μ = %.4f\r\n", det->mean);
            printf("  标准差 σ = %.4f\r\n", det->std);
            printf("  异常范围: [%.4f, %.4f]\r\n\r\n",
                   det->mean - ZSCORE_THRESHOLD * det->std,
                   det->mean + ZSCORE_THRESHOLD * det->std);
        }
    } else {
        /* === 实时 Z-Score 检测 === */
        float sigma = sqrtf(det->ema_var);
        if (sigma < 1e-6f) sigma = 1e-6f;
        det->zscore    = fabsf(x - det->ema_mean) / sigma;
        det->is_anomaly = (det->zscore > ZSCORE_THRESHOLD) ? 1 : 0;
        if (det->is_anomaly) det->anomaly_count++;

        /* === EMA 自适应基线更新 (仅正常样本参与更新) === */
        if (!det->is_anomaly) {
            float diff      = x - det->ema_mean;
            det->ema_mean  += EMA_ALPHA * diff;
            det->ema_var    = (1.0f - EMA_ALPHA) * (det->ema_var + EMA_ALPHA * diff * diff);
        }
    }
}

/* ==================== ADC 读取并归一化 ==================== */
static float ADC_ReadNormalized(void)
{
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, 10);
    uint32_t raw = HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);

    float val = (float)raw / 4095.0f;

    /* 模拟偶发异常 (实际项目删除此段) */
    static uint32_t sim_cnt = 0;
    sim_cnt++;
    if (sim_cnt % 80 == 0) {
        val += 0.35f;  /* 注入异常偏移 */
        if (val > 1.0f) val = 1.0f;
    }
    return val;
}

/* ==================== ADC1 初始化 ==================== */
static void MX_ADC1_Init(void)
{
    ADC_ChannelConfTypeDef s = {0};
    GPIO_InitTypeDef g = {0};
    __HAL_RCC_ADC1_CLK_ENABLE(); __HAL_RCC_GPIOA_CLK_ENABLE();
    g.Pin = GPIO_PIN_0; g.Mode = GPIO_MODE_ANALOG; g.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &g);
    hadc1.Instance = ADC1;
    hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
    hadc1.Init.Resolution = ADC_RESOLUTION_12B;
    hadc1.Init.ScanConvMode = DISABLE; hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT; hadc1.Init.NbrOfConversion = 1;
    hadc1.Init.DMAContinuousRequests = DISABLE; hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    HAL_ADC_Init(&hadc1);
    s.Channel = ADC_CHANNEL_0; s.Rank = 1; s.SamplingTime = ADC_SAMPLETIME_56CYCLES;
    HAL_ADC_ConfigChannel(&hadc1, &s);
}

/* ==================== 系统时钟配置 (84MHz) ==================== */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef o = {0}; RCC_ClkInitTypeDef c = {0};
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
    __HAL_RCC_GPIOA_CLK_ENABLE(); __HAL_RCC_GPIOC_CLK_ENABLE();
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