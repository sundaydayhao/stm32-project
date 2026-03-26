# 工具链配置指南 / Toolchain Setup Guide

## STM32CubeIDE 配置

### 安装步骤

1. 访问 https://www.st.com/en/development-tools/stm32cubeide.html
2. 下载对应操作系统版本
3. 运行安装程序（需要管理员权限）
4. 安装完成后，首次启动会提示安装固件包

### 固件包安装

```
Help -> Manage Embedded Software Packages
-> STMicroelectronics -> STM32F4 -> 选择最新版本 -> Install
```

### 创建新项目

```
File -> New -> STM32 Project
-> 搜索 "NUCLEO-F401RE"
-> 选择开发板
-> 配置项目名称
-> 完成
```

## Keil MDK 配置

### 安装步骤

1. 下载 Keil MDK-ARM: https://www.keil.com/download/product/
2. 安装 MDK
3. 打开 Pack Installer
4. 安装 `Keil::STM32F4xx_DFP`

### 编译设置

```
Project -> Options for Target
-> Device: STM32F401RE
-> Target: ARM Compiler 6
-> C/C++: -O1 -std=c99
-> Linker: 使用 STM32F401RETx_FLASH.ld
```

## ARM GCC + Makefile 配置

### Windows 安装

```powershell
# 方法1: 使用 winget
winget install Arm.GnuArmEmbeddedToolchain

# 方法2: 手动下载
# https://developer.arm.com/downloads/-/gnu-rm
# 下载 gcc-arm-none-eabi-10.3-2021.10-win32.exe
```

### Linux 安装

```bash
# Ubuntu/Debian
sudo apt update
sudo apt install gcc-arm-none-eabi binutils-arm-none-eabi

# 验证安装
arm-none-eabi-gcc --version
```

### macOS 安装

```bash
# 使用 Homebrew
brew install arm-none-eabi-gcc

# 或者使用 xPack
npm install -g xpm
xpm install -g @xpack-dev-tools/arm-none-eabi-gcc@latest
```

### OpenOCD 安装 (用于下载程序)

```bash
# Ubuntu/Debian
sudo apt install openocd

# macOS
brew install openocd

# Windows: 下载预编译版本
# https://github.com/openocd-org/openocd/releases
```

### 下载程序命令

```bash
# 使用 OpenOCD + ST-Link
openocd -f interface/stlink.cfg \
        -f target/stm32f4x.cfg \
        -c "program build/project.elf verify reset exit"

# 使用 STM32_Programmer_CLI
STM32_Programmer_CLI -c port=SWD -w build/project.hex -v -rst
```

## VSCode + PlatformIO 配置

### 安装步骤

1. 安装 VSCode: https://code.visualstudio.com/
2. 安装 PlatformIO IDE 插件
3. 重启 VSCode

### platformio.ini 配置示例

```ini
[env:nucleo_f401re]
platform = ststm32
board = nucleo_f401re
framework = stm32cube
monitor_speed = 115200

; 调试配置
debug_tool = stlink
upload_protocol = stlink

; 编译选项
build_flags = 
    -DUSE_HAL_DRIVER
    -DSTM32F401xE
    -O2
```

## ST-Link 驱动安装

### Windows

1. 下载 ST-Link 驱动: https://www.st.com/en/development-tools/stsw-link009.html
2. 运行安装程序
3. 连接 NUCLEO 板后，设备管理器应显示 "STMicroelectronics STLink Virtual COM Port"

### Linux

```bash
# 添加 udev 规则
sudo cp 49-stlinkv2.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules
sudo udevadm trigger

# 将用户添加到 dialout 组
sudo usermod -a -G dialout $USER
```

## 串口调试工具

| 工具 | 平台 | 说明 |
|------|------|------|
| PuTTY | Windows | 经典串口终端 |
| MobaXterm | Windows | 功能强大的终端 |
| CoolTerm | 跨平台 | 简单易用 |
| minicom | Linux/macOS | 命令行串口工具 |
| screen | Linux/macOS | `screen /dev/ttyACM0 115200` |

## 常用调试命令

```bash
# 查看串口设备 (Linux)
ls /dev/ttyACM* /dev/ttyUSB*

# 使用 minicom
minicom -D /dev/ttyACM0 -b 115200

# 使用 screen
screen /dev/ttyACM0 115200

# 查看串口设备 (Windows PowerShell)
Get-WMIObject Win32_SerialPort | Select-Object Name, DeviceID