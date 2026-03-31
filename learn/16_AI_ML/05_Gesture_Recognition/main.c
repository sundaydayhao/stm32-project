/**
 * @file    main.c
 * @brief   手势识别示例 (MPU6050 + 模板匹配)
 * @details 通过 I2C 读取 MPU6050 三轴加速度，检测并识别 4 种手势。
 *
 *          【手势识别流程】
 *          1. MPU6050 I2C 读取 ax/ay/az @ 50Hz
 *          2. 阈值分割: 合加速度 > 0.3g 开始采集，连续 10 帧 < 0.1g 结束
 *          3. 特征提取: 各轴均值/标准差/最大值 (12维)
 *          4. 模板匹配: 欧氏距离最近邻
 *          5. LED 闪烁次数指示手势类别
 *
 *          硬件连接:
 *          MPU6050 SCL -> PB8 (I2C1_SCL)
 *          MPU6050 SDA -> PB9 (I2C1_SDA)
 *          MPU6050 VCC -> 3.3V, GND -> GND, AD0 -> GND (地址 0x68)
 *
 * @board   NUCLEO-F401RE (STM32F401RE @ 84MHz)
 * @version 2.0 (自包含版本)
 */

#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

/* ==================== MPU6050 寄存器 ==================== */
#define MPU6050_ADDR        (0x68 << 1)
#define MPU6050_PWR_MGMT_1  0x6B
#define MPU6050_SMPLRT_DIV  0x19
#define MPU6050_ACCEL_CFG   0x1C
#define MPU6050_ACCEL_XOUT  0x3B
#define MPU6050_SCALE       16384.0f   /* ±2g: 16384 LSB/g */

/* ==================== 手势参数 ==================== */
#define GESTURE_CLASSES     4
#define GESTURE_MAX_LEN     50
#define GESTURE_MIN_LEN     10
#define FEATURE_SIZE        12
#define SAMPLE_RATE_HZ      50
#define START_THRESH        0.3f
#define END_THRESH          0.1f
#define END_FRAMES          10

static const char *GESTURE_LABELS[GESTURE_CLASSES] = {
    "SWIPE_UP", "SWIPE_DOWN", "SWIPE_LEFT", "SWIPE_RIGHT"
};

/* ==================== 数据结构 ==================== */
typedef struct { float ax, ay, az, mag; } AccelData_t;

typedef struct {
    AccelData_t frames[GESTURE_MAX_LEN];
    uint8_t  len;
    uint8_t  capturing;
    uint8_t  end_cnt;
    uint8_t  ready;
} GestureBuf_t;

/* ==================== 手势模板 (12维特征) ==================== */
/*
 * 特征顺序: [ax_mean, ax_std, ax_max,
 *            ay_mean, ay_std, ay_max,
 *            az_mean, az_std, az_max,
 *            mag_mean, mag_std, mag_max]
 */
static const float templates[GESTURE_CLASSES][FEATURE_SIZE] = {
    /* SWIPE_UP: ay 正向 */
    { 0.05f, 0.10f, 0.30f,  0.60f, 0.25f, 1.20f,
      0.02f, 0.08f, 0.20f,  1.10f, 0.20f, 1.50f },
    /* SWIPE_DOWN: ay 负向 */
    { 0.05f, 0.10f, 0.30f, -0.60f, 0.25f, 0.10f,
      0.02f, 0.08f, 0.20f,  1.10f, 0.20f, 1.50f },
    /* SWIPE_LEFT: ax 负向 */
    {-0.60f, 0.25f, 0.10f,  0.05f, 0.10f, 0.30f,
      0.02f, 0.08f, 0.20f,  1.10f, 0.20f, 1.50f },
    /* SWIPE_RIGHT: ax 正向 */
    { 0.60f, 0.25f, 1.20f,  0.05f, 0.10f, 0.30f,
      0.02f, 0.08f, 0.20f,  1.10f, 0.20f, 1.50f }
};

/* ==================== 全局变量 ==================== */
UART_HandleTypeDef huart2;
I2C_HandleTypeDef  hi2c1;
static GestureBuf_t gbuf;
static uint32_t sample_tick   = 0;
static uint32_t gesture_count = 0;

/* ==================== 函数声明 ==================== */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_Init(void);
static void MX_I2C1_Init(void);
static HAL_StatusTypeDef MPU6050_Init(void);
static HAL_StatusTypeDef MPU6050_Read(AccelData_t *d);
static void Gesture_Update(GestureBuf_t *buf, AccelData_t *d);
static uint8_t Gesture_Classify(GestureBuf_t *buf);
static float   Vec_Dist(const float *a, const float *b, uint8_t n);

int __io_putchar(int ch)
{
    HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}

/* ==================== 主函数 ==================== */
int main(void)
{
    AccelData_t accel;

    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART2_Init();
    MX_I2C1_Init();

    printf("\r\n========================================\r\n");
    printf("  手势识别示例 - MPU6050 + 模板匹配\r\n");
    printf("  支持: 上挥/下挥/左挥/右挥\r\n");
    printf("  I2C: PB8=SCL, PB9=SDA\r\n");
    printf("========================================\r\n");

    if (MPU6050_Init() != HAL_OK) {
        printf("MPU6050 初始化失败! 检查 I2C 连接\r\n");
        while (1) { HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5); HAL_Delay(100); }
    }
    printf("MPU6050 就绪, 请做手势...\r\n\r\n");

    while (1) {
        if (HAL_GetTick() - sample_tick >= (1000U / SAMPLE_RATE_HZ)) {
            sample_tick = HAL_GetTick();

            if (MPU6050_Read(&accel) == HAL_OK) {
                Gesture_Update(&gbuf, &accel);

                if (gbuf.ready) {
                    gesture_count++;
                    uint8_t cls = Gesture_Classify(&gbuf);
                    printf(">>> 手势 #%lu: [%s] <<<\r\n\r\n",
                           gesture_count, GESTURE_LABELS[cls]);

                    /* LED 闪烁次数 = 类别+1 */
                    for (uint8_t i = 0; i <= cls; i++) {
                        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
                        HAL_Delay(150);
                        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
                        HAL_Delay(150);
                    }
                    memset(&gbuf, 0, sizeof(gbuf));
                }
            }
        }
    }
}

/* ==================== MPU6050 初始化 ==================== */
static HAL_StatusTypeDef MPU6050_Init(void)
{
    uint8_t d;
    HAL_StatusTypeDef r;
    d = 0x00;  /* 唤醒 */
    r = HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR, MPU6050_PWR_MGMT_1, 1, &d, 1, 100);
    if (r != HAL_OK) return r;
    d = 19;    /* 采样率 = 1kHz/(1+19) = 50Hz */
    r = HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR, MPU6050_SMPLRT_DIV, 1, &d, 1, 100);
    if (r != HAL_OK) return r;
    d = 0x00;  /* ±2g */
    return HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR, MPU6050_ACCEL_CFG, 1, &d, 1, 100);
}

/* ==================== MPU6050 读取加速度 ==================== */
static HAL_StatusTypeDef MPU6050_Read(AccelData_t *d)
{
    uint8_t raw[6];
    HAL_StatusTypeDef r;

    r = HAL_I2C_Mem_Read(&hi2c1, MPU6050_ADDR, MPU6050_ACCEL_XOUT, 1, raw, 6, 100);
    if (r != HAL_OK) {
        /* I2C 失败时使用模拟数据 */
        static uint32_t t = 0; t++;
        d->ax = 0.05f * sinf(t * 0.1f);
        d->ay = 0.05f * cosf(t * 0.1f);
        d->az = 0.0f;
        d->mag = sqrtf(d->ax*d->ax + d->ay*d->ay);
        return HAL_OK;
    }

    int16_t rx = (int16_t)((raw[0]<<8)|raw[1]);
    int16_t ry = (int16_t)((raw[2]<<8)|raw[3]);
    int16_t rz = (int16_t)((raw[4]<<8)|raw[5]);
    d->ax = (float)rx / MPU6050_SCALE;
    d->ay = (float)ry / MPU6050_SCALE;
    d->az = (float)rz / MPU6050_SCALE - 1.0f;  /* 去重力 */
    d->mag = sqrtf(d->ax*d->ax + d->ay*d->ay + d->az*d->az);
    return HAL_OK;
}

/* ==================== 手势分割状态机 ==================== */
static void Gesture_Update(GestureBuf_t *buf, AccelData_t *d)
{
    if (buf->ready) return;

    if (!buf->capturing) {
        if (d->mag > START_THRESH) {
            buf->capturing = 1; buf->len = 0; buf->end_cnt = 0;
            printf("手势开始...\r\n");
        }
    } else {
        if (buf->len < GESTURE_MAX_LEN) buf->frames[buf->len++] = *d;
        if (d->mag < END_THRESH) {
            if (++buf->end_cnt >= END_FRAMES) {
                buf->capturing = 0;
                if (buf->len >= GESTURE_MIN_LEN) {
                    buf->ready = 1;
                    printf("手势结束 (%d帧)\r\n", buf->len);
                } else {
                    printf("手势太短 (%d帧), 忽略\r\n", buf->len);
                    buf->len = 0;
                }
            }
        } else {
            buf->end_cnt = 0;
        }
    }
}

/* ==================== 手势分类 (特征提取 + 模板匹配) ==================== */
static uint8_t Gesture_Classify(GestureBuf_t *buf)
{
    uint8_t n = buf->len;
    float feat[FEATURE_SIZE];

    /* 提取各轴统计特征 */
    float *axes[3][3] = {
        {&feat[0], &feat[1], &feat[2]},   /* ax: mean, std, max */
        {&feat[3], &feat[4], &feat[5]},   /* ay */
        {&feat[6], &feat[7], &feat[8]}    /* az */
    };

    for (uint8_t a = 0; a < 3; a++) {
        float sum = 0.0f, mx = -1e9f;
        for (uint8_t i = 0; i < n; i++) {
            float v = (a==0) ? buf->frames[i].ax :
                      (a==1) ? buf->frames[i].ay : buf->frames[i].az;
            sum += v;
            if (v > mx) mx = v;
        }
        float mean = sum / n;
        float var  = 0.0f;
        for (uint8_t i = 0; i < n; i++) {
            float v = (a==0) ? buf->frames[i].ax :
                      (a==1) ? buf->frames[i].ay : buf->frames[i].az;
            float d = v - mean; var += d*d;
        }
        *axes[a][0] = mean;
        *axes[a][1] = sqrtf(var / n);
        *axes[a][2] = mx;
    }

    /* mag 统计 */
    float ms = 0.0f, mmx = 0.0f;
    for (uint8_t i = 0; i < n; i++) {
        ms += buf->frames[i].mag;
        if (buf->frames[i].mag > mmx) mmx = buf->frames[i].mag;
    }
    float mm = ms / n, mv = 0.0f;
    for (uint8_t i = 0; i < n; i++) {
        float d = buf->frames[i].mag - mm; mv += d*d;
    }
    feat[9]  = mm;
    feat[10] = sqrtf(mv / n);
    feat[11] = mmx;

    /* 模板匹配 */
    uint8_t best = 0;
    float   best_d = 1e10f;
    for (uint8_t c = 0; c < GESTURE_CLASSES; c++) {
        float dist = Vec_Dist(feat, templates[c], FEATURE_SIZE);
        printf("  [%s] dist=%.4f\r\n", GESTURE_LABELS[c], dist);
        if (dist < best_d) { best_d = dist; best = c; }
    }
    return best;
}

/* ==================== 欧氏距离 ==================== */
static float Vec_Dist(const float *a, const float *b, uint8_t n)
{
    float s = 0.0f;
    for (uint8_t i = 0; i < n; i++) { float d = a[i]-b[i]; s += d*d; }
    return sqrtf(s);
}

/* ==================== I2C1 初始化 (PB8=SCL, PB9=SDA, 400kHz) ==================== */
static void MX_I2C1_Init(void)
{
    GPIO_InitTypeDef g = {0};
    __HAL_RCC_I2C1_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    g.Pin       = GPIO_PIN_8 | GPIO_PIN_9;
    g.Mode      = GPIO_MODE_AF_OD;
    g.Pull      = GPIO_PULLUP;
    g.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    g.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOB, &g);
    hi2c1.Instance             = I2C1;
    hi2c1.Init.ClockSpeed      = 400000;
    hi2c1.Init.DutyCycle       = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1     = 0;
    hi2c1.Init.AddressingMode  = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode   = I2C_NOSTRETCH_DISABLE;
    HAL_I2C_Init(&hi2c1);
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