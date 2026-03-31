/**
 * @file    main.c
 * @brief   CMSIS-NN INT8 量化神经网络推理示例
 * @details 使用 INT8 量化权重在 STM32F401RE 上执行神经网络推理。
 *          展示量化原理及与 ARM CMSIS-NN 库的 API 对应关系。
 *          使用 DWT 周期计数器进行性能分析。
 *
 *          【量化原理】
 *          float32 -> INT8: q = clamp(round(r/scale + zero_point), -128, 127)
 *          INT8 -> float32: r = (q - zero_point) * scale
 *          节省内存: float32(4B) -> INT8(1B), 节省 75%
 *
 *          【CMSIS-NN API 对应关系】
 *          本示例手动实现  ->  CMSIS-NN API
 *          FC_ReLU6_INT8   ->  arm_fully_connected_s8() + arm_relu6_s8()
 *          FC_Softmax      ->  arm_fully_connected_s8() + arm_softmax_s8()
 *          输入量化         ->  arm_requantize_s8()
 *
 *          【DWT 性能分析】
 *          STM32F401RE @ 84MHz: 1 cycle ≈ 11.9 ns
 *          推理时间(μs) = cycles / 84
 *
 * @board   NUCLEO-F401RE (STM32F401RE @ 84MHz)
 * @version 2.0 (自包含版本)
 */

#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

/* ==================== 网络结构 ==================== */
#define NN_INPUT_SIZE   4
#define NN_HIDDEN_SIZE  8
#define NN_OUTPUT_SIZE  3

/* ==================== 量化参数 ==================== */
#define INPUT_SCALE         0.00784f
#define INPUT_ZERO_POINT    (-128)
#define W1_SCALE            0.00625f
#define LAYER1_OUT_SCALE    0.01f
#define LAYER1_OUT_ZERO     (-128)
#define W2_SCALE            0.00625f

/* ==================== INT8 量化权重 (存储在 Flash) ==================== */
static const int8_t W1_q[NN_HIDDEN_SIZE * NN_INPUT_SIZE] = {
     83, -45,  107, -78,
    -50,  104,   69,-115,
    125,  -30,  -88,  58,
     22,  103,  -61,  98,
    -99,   66,  126, -43,
     72,  -91,   34, 104,
     53,  115,  -70,  24,
   -114,   42,   93,-101
};
static const int32_t b1_q[NN_HIDDEN_SIZE] = {
    154, -103, 269, -192, 115, -231, 179, -141
};
static const int8_t W2_q[NN_OUTPUT_SIZE * NN_HIDDEN_SIZE] = {
    117, -90,  70, -61,  99,-114,  46,-103,
    -67, 109, -46,  82,  53, -77, 122, -98,
    -50, -19, 106, 107,-122,  94, -72, 118
};
static const int32_t b2_q[NN_OUTPUT_SIZE] = {64, -38, -26};

static const char *CLASS_LABELS[NN_OUTPUT_SIZE] = {"NORMAL", "WARNING", "ALARM"};

/* ==================== 推理结构体 ==================== */
typedef struct {
    int8_t   input_q[NN_INPUT_SIZE];
    int8_t   hidden_q[NN_HIDDEN_SIZE];
    float    output_f[NN_OUTPUT_SIZE];
    uint8_t  predicted;
    float    confidence;
    uint32_t cycles;
} QuantNN_t;

typedef struct {
    uint32_t count;
    uint32_t min_cyc;
    uint32_t max_cyc;
    float    avg_cyc;
} PerfStats_t;

/* ==================== 全局变量 ==================== */
UART_HandleTypeDef huart2;
static QuantNN_t  qnn;
static PerfStats_t perf;
static uint32_t infer_count = 0;
static uint32_t last_tick   = 0;

/* ==================== 函数声明 ==================== */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_Init(void);
static void QNN_QuantizeInput(QuantNN_t *net, const float *input_f);
static void QNN_FC_ReLU6(const int8_t *in, int8_t *out,
                          const int8_t *W, const int32_t *b,
                          uint16_t in_sz, uint16_t out_sz,
                          float in_sc, float w_sc, float out_sc, int32_t out_zp);
static void QNN_FC_Softmax(const int8_t *in, float *out,
                            const int8_t *W, const int32_t *b,
                            uint16_t in_sz, uint16_t out_sz,
                            float in_sc, float w_sc);
static void QNN_Forward(QuantNN_t *net);
static void Perf_Update(PerfStats_t *p, uint32_t cyc);
static void Sensor_GetFeatures(float *feat);

int __io_putchar(int ch)
{
    HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}

/* ==================== 主函数 ==================== */
int main(void)
{
    float features[NN_INPUT_SIZE];

    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART2_Init();

    memset(&qnn,  0, sizeof(qnn));
    memset(&perf, 0, sizeof(perf));
    perf.min_cyc = 0xFFFFFFFFU;

    /* 启用 DWT 周期计数器 */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL  |= DWT_CTRL_CYCCNTENA_Msk;

    printf("\r\n========================================\r\n");
    printf("  CMSIS-NN INT8 量化推理示例\r\n");
    printf("  网络: %d->%d(ReLU6)->%d(Softmax)\r\n",
           NN_INPUT_SIZE, NN_HIDDEN_SIZE, NN_OUTPUT_SIZE);
    printf("  权重: INT8 (节省75%% vs float32)\r\n");
    printf("  时钟: 84MHz, DWT 性能分析\r\n");
    printf("========================================\r\n\r\n");

    while (1) {
        if (HAL_GetTick() - last_tick >= 500) {
            last_tick = HAL_GetTick();
            infer_count++;

            Sensor_GetFeatures(features);
            QNN_QuantizeInput(&qnn, features);

            /* DWT 计时推理 */
            uint32_t t0 = DWT->CYCCNT;
            QNN_Forward(&qnn);
            qnn.cycles = DWT->CYCCNT - t0;
            Perf_Update(&perf, qnn.cycles);

            /* 取最大概率 */
            qnn.predicted  = 0;
            qnn.confidence = qnn.output_f[0];
            for (uint8_t k = 1; k < NN_OUTPUT_SIZE; k++) {
                if (qnn.output_f[k] > qnn.confidence) {
                    qnn.confidence = qnn.output_f[k];
                    qnn.predicted  = k;
                }
            }

            /* 每 5 次打印一次 */
            if (infer_count % 5 == 1) {
                printf("[#%lu] 输入(int8):[%d,%d,%d,%d] -> [%s] %.1f%% | %lu cyc (%.2f us)\r\n",
                       infer_count,
                       qnn.input_q[0], qnn.input_q[1],
                       qnn.input_q[2], qnn.input_q[3],
                       CLASS_LABELS[qnn.predicted],
                       qnn.confidence * 100.0f,
                       qnn.cycles,
                       (float)qnn.cycles / 84.0f);
            }

            /* 每 50 次打印性能统计 */
            if (infer_count % 50 == 0) {
                printf("--- 性能统计 (%lu次) ---\r\n", perf.count);
                printf("  最小: %lu cyc (%.2f us)\r\n",
                       perf.min_cyc, (float)perf.min_cyc / 84.0f);
                printf("  最大: %lu cyc (%.2f us)\r\n",
                       perf.max_cyc, (float)perf.max_cyc / 84.0f);
                printf("  平均: %.0f cyc (%.2f us)\r\n\r\n",
                       perf.avg_cyc, perf.avg_cyc / 84.0f);
            }

            HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
        }
    }
}

/* ==================== 输入量化 float -> int8 ==================== */
static void QNN_QuantizeInput(QuantNN_t *net, const float *input_f)
{
    for (uint8_t i = 0; i < NN_INPUT_SIZE; i++) {
        float q = input_f[i] / INPUT_SCALE + (float)INPUT_ZERO_POINT;
        if (q >  127.0f) q =  127.0f;
        if (q < -128.0f) q = -128.0f;
        net->input_q[i] = (int8_t)(int32_t)q;
    }
}

/* ==================== INT8 全连接 + ReLU6 ==================== */
/**
 * @note 对应 CMSIS-NN: arm_fully_connected_s8() + arm_relu6_s8()
 *       SIMD 优化: Cortex-M4 DSP 扩展 SMLAD 指令一次处理 2 个 int16 乘加
 */
static void QNN_FC_ReLU6(const int8_t *in, int8_t *out,
                          const int8_t *W, const int32_t *b,
                          uint16_t in_sz, uint16_t out_sz,
                          float in_sc, float w_sc, float out_sc, int32_t out_zp)
{
    float requant = (in_sc * w_sc) / out_sc;
    float relu6_max = 6.0f / out_sc + (float)out_zp;

    for (uint16_t j = 0; j < out_sz; j++) {
        int32_t acc = b[j];
        const int8_t *row = &W[j * in_sz];
        for (uint16_t i = 0; i < in_sz; i++) {
            acc += (int32_t)row[i] * (int32_t)in[i];
        }
        float acc_f = (float)acc * requant + (float)out_zp;
        if (acc_f < (float)out_zp) acc_f = (float)out_zp;
        if (acc_f > relu6_max)     acc_f = relu6_max;
        int32_t q = (int32_t)(acc_f + 0.5f);
        if (q >  127) q =  127;
        if (q < -128) q = -128;
        out[j] = (int8_t)q;
    }
}

/* ==================== INT8 全连接 + Softmax ==================== */
/**
 * @note 对应 CMSIS-NN: arm_fully_connected_s8() + arm_softmax_s8()
 */
static void QNN_FC_Softmax(const int8_t *in, float *out,
                            const int8_t *W, const int32_t *b,
                            uint16_t in_sz, uint16_t out_sz,
                            float in_sc, float w_sc)
{
    float logits[NN_OUTPUT_SIZE];
    for (uint16_t j = 0; j < out_sz; j++) {
        int32_t acc = b[j];
        const int8_t *row = &W[j * in_sz];
        for (uint16_t i = 0; i < in_sz; i++) {
            acc += (int32_t)row[i] * (int32_t)in[i];
        }
        logits[j] = (float)acc * in_sc * w_sc;
    }
    /* 数值稳定 Softmax */
    float max_v = logits[0];
    for (uint8_t k = 1; k < out_sz; k++) {
        if (logits[k] > max_v) max_v = logits[k];
    }
    float sum = 0.0f;
    for (uint8_t k = 0; k < out_sz; k++) {
        out[k] = expf(logits[k] - max_v);
        sum += out[k];
    }
    for (uint8_t k = 0; k < out_sz; k++) out[k] /= sum;
}

/* ==================== 量化前向推理 ==================== */
static void QNN_Forward(QuantNN_t *net)
{
    QNN_FC_ReLU6(net->input_q, net->hidden_q, W1_q, b1_q,
                 NN_INPUT_SIZE, NN_HIDDEN_SIZE,
                 INPUT_SCALE, W1_SCALE, LAYER1_OUT_SCALE, LAYER1_OUT_ZERO);
    QNN_FC_Softmax(net->hidden_q, net->output_f, W2_q, b2_q,
                   NN_HIDDEN_SIZE, NN_OUTPUT_SIZE,
                   LAYER1_OUT_SCALE, W2_SCALE);
}

/* ==================== 性能统计更新 ==================== */
static void Perf_Update(PerfStats_t *p, uint32_t cyc)
{
    p->count++;
    if (cyc < p->min_cyc) p->min_cyc = cyc;
    if (cyc > p->max_cyc) p->max_cyc = cyc;
    p->avg_cyc = p->avg_cyc * 0.9f + (float)cyc * 0.1f;
}

/* ==================== 传感器特征获取 ==================== */
static void Sensor_GetFeatures(float *feat)
{
    feat[0] = 0.44f + (float)(infer_count % 100) * 0.001f;
    feat[1] = 0.50f + (float)(infer_count % 80)  * 0.001f;
    feat[2] = 0.61f + (float)(infer_count % 120) * 0.001f;
    feat[3] = 0.02f + (float)(infer_count % 50)  * 0.001f;
    if (infer_count % 30 == 0) { feat[0] = 0.85f; feat[3] = 0.92f; }
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