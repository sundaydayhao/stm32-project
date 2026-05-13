# STM32F103 Test 工程编译与烧录说明

本工程是 STM32F103 系列的 CMake 工程，使用 `arm-none-eabi-gcc` 交叉编译工具链。

## 1. 环境要求

编译前确认以下工具已经安装并在 `PATH` 中：

```bash
cmake --version
make --version
arm-none-eabi-gcc --version
arm-none-eabi-objcopy --version
```

烧录时按使用的方式额外安装其中一种工具：

- `openocd`：配合 ST-Link 通过 SWD 烧录。
- `st-flash`：配合 ST-Link 烧录 `.bin` 文件。
- `STM32_Programmer_CLI`：STM32CubeProgrammer 命令行工具。

当前工程的 CMake preset 使用 `Unix Makefiles`，不依赖 Ninja。

## 2. 编译 Debug 版本

配置 Debug 构建目录：

```bash
cmake --preset Debug
```

执行编译：

```bash
cmake --build --preset Debug
```

编译成功后，主要产物位于：

```text
build/Debug/test.elf
build/Debug/test.map
```

其中：

- `test.elf`：可调试、可烧录的 ELF 文件。
- `test.map`：链接映射文件，用于查看符号、段分布和内存占用。

## 3. 编译 Release 版本

配置 Release 构建目录：

```bash
cmake --preset Release
```

执行编译：

```bash
cmake --build --preset Release
```

编译成功后，主要产物位于：

```text
build/Release/test.elf
build/Release/test.map
```

Release 版本使用 `-Os` 优化，通常用于最终烧录测试。

## 4. 重新生成构建目录

如果切换过工具链、CMake 配置异常，或之前误用了宿主机编译器，可以清理后重新配置：

```bash
rm -rf build/Debug build/Release
cmake --preset Debug
cmake --build --preset Debug
```

也可以只刷新根目录 `build`：

```bash
cmake --fresh -S . -B build -G "Unix Makefiles" \
  -DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake \
  -DCMAKE_BUILD_TYPE=Debug

cmake --build build
```

## 5. 生成 bin 和 hex 产物

CMake 默认生成 `.elf` 和 `.map`。如果需要 `.bin` 或 `.hex`，使用 `arm-none-eabi-objcopy` 从 `.elf` 转换。

生成 Debug 的 `.bin` 和 `.hex`：

```bash
arm-none-eabi-objcopy -O binary build/Debug/test.elf build/Debug/test.bin
arm-none-eabi-objcopy -O ihex build/Debug/test.elf build/Debug/test.hex
```

生成 Release 的 `.bin` 和 `.hex`：

```bash
arm-none-eabi-objcopy -O binary build/Release/test.elf build/Release/test.bin
arm-none-eabi-objcopy -O ihex build/Release/test.elf build/Release/test.hex
```

查看 ELF 文件大小：

```bash
arm-none-eabi-size build/Release/test.elf
```

产物用途：

- `.elf`：包含符号信息，适合调试，也可以直接用于 OpenOCD 或 STM32CubeProgrammer 烧录。
- `.bin`：裸二进制镜像，烧录时需要指定 Flash 起始地址。
- `.hex`：Intel HEX 格式，包含地址信息，常用于量产或通用烧录工具。
- `.map`：链接映射文件，不用于烧录。

STM32F103 内部 Flash 起始地址通常是：

```text
0x08000000
```

## 6. 使用 OpenOCD 烧录

连接 ST-Link 和目标板，确认 SWD 接线正确：

- `SWDIO`
- `SWCLK`
- `GND`
- `3V3` 或目标板自供电

推荐直接烧录 ELF：

```bash
openocd -f interface/stlink.cfg -f target/stm32f1x.cfg \
  -c "program build/Release/test.elf verify reset exit"
```

也可以烧录 `.bin`，但必须指定 Flash 地址：

```bash
arm-none-eabi-objcopy -O binary build/Release/test.elf build/Release/test.bin

openocd -f interface/stlink.cfg -f target/stm32f1x.cfg \
  -c "program build/Release/test.bin 0x08000000 verify reset exit"
```

## 7. 使用 st-flash 烧录

`st-flash` 通常烧录 `.bin` 文件：

```bash
arm-none-eabi-objcopy -O binary build/Release/test.elf build/Release/test.bin
st-flash --reset write build/Release/test.bin 0x08000000
```

烧录完成后目标板会复位运行。

## 8. 使用 STM32CubeProgrammer CLI 烧录

使用 SWD 连接时，可以直接烧录 ELF：

```bash
STM32_Programmer_CLI -c port=SWD -w build/Release/test.elf -v -rst
```

烧录 `.bin` 时需要指定 Flash 地址：

```bash
arm-none-eabi-objcopy -O binary build/Release/test.elf build/Release/test.bin
STM32_Programmer_CLI -c port=SWD -w build/Release/test.bin 0x08000000 -v -rst
```

## 9. 常见问题

### 误用宿主机 gcc 导致 CMSIS 汇编报错

如果看到类似下面的错误：

```text
Error: no such instruction: `isb 0xF'
Error: number of operands mismatch for `ds'
```

通常说明 CMake 缓存使用了 `/usr/bin/cc` 等宿主机编译器，而不是 `arm-none-eabi-gcc`。重新配置即可：

```bash
rm -rf build/Debug
cmake --preset Debug
cmake --build --preset Debug
```

### 找不到 arm-none-eabi-gcc

如果配置时报找不到交叉编译器，先确认工具链是否安装：

```bash
which arm-none-eabi-gcc
```

如果没有输出，需要安装 GNU Arm Embedded Toolchain，并把工具链的 `bin` 目录加入 `PATH`。

### 找不到烧录器

先确认系统能识别 ST-Link：

```bash
lsusb
```

如果 OpenOCD 或 st-flash 无法连接，优先检查：

- ST-Link 是否接好。
- 目标板是否供电。
- `SWDIO`、`SWCLK`、`GND` 是否连接正确。
- 芯片是否被读保护或处于异常低功耗状态。

