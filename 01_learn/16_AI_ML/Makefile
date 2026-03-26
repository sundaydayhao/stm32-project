##############################################################################
# STM32F401RE AI/ML 例程 Makefile
#
# 用法:
#   make EXAMPLE=01_Neural_Network        # 编译指定例程
#   make EXAMPLE=01_Neural_Network flash  # 编译并烧录
#   make EXAMPLE=01_Neural_Network clean  # 清理
#   make all                              # 编译所有例程
#
# 依赖:
#   - arm-none-eabi-gcc (GNU Arm Embedded Toolchain)
#   - STM32CubeF4 固件包 (设置 CUBE_PATH)
#   - OpenOCD (用于烧录, 可选)
#
# STM32CubeF4 下载: https://www.st.com/en/embedded-software/stm32cubef4.html
##############################################################################

# ==================== 用户配置 ====================
# STM32CubeF4 固件包路径 (根据实际安装位置修改)
CUBE_PATH ?= C:/ST/STM32CubeF4

# 要编译的例程 (默认第一个)
EXAMPLE ?= 01_Neural_Network

# 所有例程列表
ALL_EXAMPLES := \
    01_Neural_Network \
    02_KNN_Classifier \
    03_Anomaly_Detection \
    04_CMSIS_NN \
    05_Gesture_Recognition

# ==================== 工具链 ====================
PREFIX  = arm-none-eabi-
CC      = $(PREFIX)gcc
AS      = $(PREFIX)gcc -x assembler-with-cpp
CP      = $(PREFIX)objcopy
SZ      = $(PREFIX)size
HEX     = $(CP) -O ihex
BIN     = $(CP) -O binary -S

# ==================== 目标配置 ====================
TARGET  = $(EXAMPLE)
MCU     = -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard

# ==================== 编译定义 ====================
DEFS = \
    -DSTM32F401xE \
    -DUSE_HAL_DRIVER \
    -DHSE_VALUE=8000000U

# ==================== 头文件路径 ====================
INCS = \
    -I$(CUBE_PATH)/Drivers/STM32F4xx_HAL_Driver/Inc \
    -I$(CUBE_PATH)/Drivers/STM32F4xx_HAL_Driver/Inc/Legacy \
    -I$(CUBE_PATH)/Drivers/CMSIS/Device/ST/STM32F4xx/Include \
    -I$(CUBE_PATH)/Drivers/CMSIS/Include \
    -I../common/Inc

# ==================== HAL 源文件 ====================
HAL_SRC_DIR = $(CUBE_PATH)/Drivers/STM32F4xx_HAL_Driver/Src
CMSIS_SRC   = $(CUBE_PATH)/Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates

HAL_SRCS = \
    $(HAL_SRC_DIR)/stm32f4xx_hal.c \
    $(HAL_SRC_DIR)/stm32f4xx_hal_cortex.c \
    $(HAL_SRC_DIR)/stm32f4xx_hal_gpio.c \
    $(HAL_SRC_DIR)/stm32f4xx_hal_rcc.c \
    $(HAL_SRC_DIR)/stm32f4xx_hal_rcc_ex.c \
    $(HAL_SRC_DIR)/stm32f4xx_hal_uart.c \
    $(HAL_SRC_DIR)/stm32f4xx_hal_pwr.c \
    $(HAL_SRC_DIR)/stm32f4xx_hal_pwr_ex.c \
    $(HAL_SRC_DIR)/stm32f4xx_hal_flash.c \
    $(HAL_SRC_DIR)/stm32f4xx_hal_flash_ex.c \
    $(HAL_SRC_DIR)/stm32f4xx_hal_dma.c \
    $(HAL_SRC_DIR)/stm32f4xx_hal_dma_ex.c \
    $(HAL_SRC_DIR)/stm32f4xx_hal_i2c.c \
    $(HAL_SRC_DIR)/stm32f4xx_hal_i2c_ex.c \
    $(CMSIS_SRC)/system_stm32f4xx.c

# ==================== 应用源文件 ====================
APP_SRCS = \
    $(EXAMPLE)/main.c

# ==================== 启动文件 ====================
ASM_SRCS = common/startup_stm32f401xe.s

# ==================== 链接脚本 ====================
LDSCRIPT = common/STM32F401RETx_FLASH.ld

# ==================== 编译选项 ====================
CFLAGS  = $(MCU) $(DEFS) $(INCS) \
          -O2 -Wall -Wextra \
          -fdata-sections -ffunction-sections \
          -std=c11 \
          --specs=nano.specs

ASFLAGS = $(MCU) -Wall -fdata-sections -ffunction-sections

LDFLAGS = $(MCU) \
          --specs=nano.specs \
          --specs=nosys.specs \
          -T$(LDSCRIPT) \
          -lc -lm \
          -Wl,--gc-sections \
          -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref

# ==================== 构建目录 ====================
BUILD_DIR = build/$(EXAMPLE)

# ==================== 目标文件列表 ====================
# HAL 目标文件
HAL_OBJS  = $(addprefix $(BUILD_DIR)/hal/, $(notdir $(HAL_SRCS:.c=.o)))
# 应用目标文件
APP_OBJS  = $(addprefix $(BUILD_DIR)/app/, $(notdir $(APP_SRCS:.c=.o)))
# 启动文件目标
ASM_OBJS  = $(addprefix $(BUILD_DIR)/asm/, $(notdir $(ASM_SRCS:.s=.o)))

ALL_OBJS  = $(HAL_OBJS) $(APP_OBJS) $(ASM_OBJS)

# ==================== 构建规则 ====================
.PHONY: all clean flash size help $(ALL_EXAMPLES)

all: $(ALL_EXAMPLES)

$(ALL_EXAMPLES):
	$(MAKE) EXAMPLE=$@ build_one

build_one: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin
	@echo ""
	@echo "========================================="
	@echo "  编译完成: $(EXAMPLE)"
	@echo "  输出目录: $(BUILD_DIR)/"
	@echo "========================================="
	$(SZ) $(BUILD_DIR)/$(TARGET).elf

# 链接
$(BUILD_DIR)/$(TARGET).elf: $(ALL_OBJS)
	@echo "[LD] $@"
	$(CC) $(ALL_OBJS) $(LDFLAGS) -o $@

# 生成 HEX
$(BUILD_DIR)/$(TARGET).hex: $(BUILD_DIR)/$(TARGET).elf
	@echo "[HEX] $@"
	$(HEX) $< $@

# 生成 BIN
$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	@echo "[BIN] $@"
	$(BIN) $< $@

# 编译 HAL 源文件
$(BUILD_DIR)/hal/%.o: $(HAL_SRC_DIR)/%.c | $(BUILD_DIR)/hal
	@echo "[CC] $(notdir $<)"
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/hal/system_stm32f4xx.o: $(CMSIS_SRC)/system_stm32f4xx.c | $(BUILD_DIR)/hal
	@echo "[CC] system_stm32f4xx.c"
	$(CC) -c $(CFLAGS) $< -o $@

# 编译应用源文件
$(BUILD_DIR)/app/%.o: $(EXAMPLE)/%.c | $(BUILD_DIR)/app
	@echo "[CC] $(notdir $<)"
	$(CC) -c $(CFLAGS) $< -o $@

# 编译启动文件
$(BUILD_DIR)/asm/%.o: common/%.s | $(BUILD_DIR)/asm
	@echo "[AS] $(notdir $<)"
	$(AS) -c $(ASFLAGS) $< -o $@

# 创建目录
$(BUILD_DIR)/hal:
	mkdir -p $@

$(BUILD_DIR)/app:
	mkdir -p $@

$(BUILD_DIR)/asm:
	mkdir -p $@

# ==================== 烧录 ====================
flash: build_one
	@echo "烧录 $(EXAMPLE) 到 STM32F401RE..."
	openocd \
	    -f interface/stlink.cfg \
	    -f target/stm32f4x.cfg \
	    -c "program $(BUILD_DIR)/$(TARGET).bin 0x08000000 verify reset exit"

# 使用 ST-Link CLI 烧录 (Windows)
flash-stlink: build_one
	ST-LINK_CLI.exe -c SWD -P $(BUILD_DIR)/$(TARGET).hex -Rst

# ==================== 清理 ====================
clean:
	rm -rf build/

clean_one:
	rm -rf $(BUILD_DIR)/

# ==================== 帮助 ====================
help:
	@echo "STM32F401RE AI/ML 例程构建系统"
	@echo ""
	@echo "用法:"
	@echo "  make EXAMPLE=<名称>          编译指定例程"
	@echo "  make EXAMPLE=<名称> flash    编译并烧录 (需要 OpenOCD)"
	@echo "  make all                     编译所有例程"
	@echo "  make clean                   清理所有构建文件"
	@echo ""
	@echo "可用例程:"
	@for e in $(ALL_EXAMPLES); do echo "  $$e"; done
	@echo ""
	@echo "配置:"
	@echo "  CUBE_PATH = $(CUBE_PATH)"
	@echo "  EXAMPLE   = $(EXAMPLE)"
	@echo ""
	@echo "示例:"
	@echo "  make EXAMPLE=01_Neural_Network"
	@echo "  make CUBE_PATH=/opt/STM32CubeF4 EXAMPLE=03_Anomaly_Detection"