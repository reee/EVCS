# EVCS 项目编译指南

## 概述

EVCS (Electronic Vehicle Charging System) 是一个使用 C++ 和 CMake 构建的 Windows 应用程序项目。本项目提供了多种编译脚本，位于 `script` 目录下，适用于 Windows 开发环境。

## 系统要求

### Windows
- CMake 3.10 或更高版本
- Visual Studio 2022 (推荐) 或 Visual Studio 2019
- Windows 10/11 操作系统

## 编译脚本说明

项目在 `script` 目录下提供了以下编译脚本：

### Windows 批处理脚本

#### `script\build.bat` - Windows 批处理编译脚本
最简单的编译方式，在项目根目录下运行：
```batch
script\build.bat
```

或者进入 script 目录后双击 `build.bat` 文件。

特性：
- 自动检测 CMake 安装
- 使用 Visual Studio 2022 生成器
- 编译 Release 版本
- 生成 x64 架构的可执行文件
- 自动处理相对路径，从 script 目录定位到项目根目录

#### `script\clean.bat` - Windows 清理脚本
清理构建目录和临时文件：
```batch
script\clean.bat
```

### PowerShell 脚本

#### `script\build.ps1` - PowerShell 编译脚本
功能更强大的编译脚本，支持多种参数：

```powershell
# 基本编译
script\build.ps1

# 编译 Debug 版本
script\build.ps1 -Config Debug

# 清理后重新编译
script\build.ps1 -Clean

# 使用不同的生成器
script\build.ps1 -Generator "Visual Studio 16 2019"

# 显示帮助信息
script\build.ps1 -Help
```

参数说明：
- `-Config <配置>`：编译配置 (Debug/Release，默认: Release)
- `-Generator <生成器>`：CMake 生成器 (默认: Visual Studio 17 2022)
- `-Clean`：清理构建目录后重新编译
- `-Help`：显示帮助信息

#### `script\clean.ps1` - PowerShell 清理脚本
```powershell
# 交互式清理
script\clean.ps1

# 强制清理（不询问确认）
script\clean.ps1 -Force

# 显示帮助
script\clean.ps1 -Help
```

### Linux/macOS 脚本

~~该项目专为 Windows 平台设计，不提供 Linux/macOS 编译脚本。~~

## 快速开始

### Windows 用户

1. **最简单的方式**：
   - 在项目根目录打开命令提示符
   - 运行 `script\build.bat`
   - 等待编译完成
   - 在 `build\Release\` 目录下找到 `EVCS.exe`

2. **使用 PowerShell**（推荐）：
   ```powershell
   script\build.ps1
   ```

3. **从 script 目录运行**：
   ```batch
   cd script
   build.bat
   ```
   或双击 `script\build.bat` 文件

### 替代方案

如果您更喜欢传统的方式，也可以直接使用 CMake：

```batch
mkdir build
cd build
cmake -G "Visual Studio 17 2022" -A x64 ..
cmake --build . --config Release
```

## 输出文件

编译成功后，可执行文件将位于以下位置：

- **Windows**: `build\Release\EVCS.exe` 或 `build\Debug\EVCS.exe`

## 故障排除

### 常见问题

1. **CMake 未找到**
   - 确保已安装 CMake 并添加到 PATH 环境变量
   - Windows: 从 [cmake.org](https://cmake.org/) 下载安装程序

2. **Visual Studio 版本不匹配**
   - 修改 PowerShell 脚本中的生成器参数
   - 或安装 Visual Studio 2022

3. **编译失败**
   - 检查是否缺少依赖库
   - 确保源代码文件完整
   - 查看编译输出中的错误信息

4. **脚本执行策略问题**（PowerShell）
   - 如果无法运行 PowerShell 脚本，请以管理员身份运行 PowerShell 并执行：
   ```powershell
   Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser
   ```

### 清理项目

如果遇到编译问题，可以尝试清理项目：

```batch
# Windows 命令提示符
script\clean.bat

# 或使用 PowerShell
script\clean.ps1

# 手动清理（如果脚本失败）
rmdir /s /q build
```

然后重新编译项目。

## 项目结构

```
EVCS/
├── src/                    # 源代码文件
│   ├── main.cpp
│   ├── MainWindow.cpp
│   ├── MainWindow.h
│   ├── Subject.cpp
│   ├── Subject.h
│   ├── Instruction.cpp
│   ├── Instruction.h
│   ├── AudioPlayer.cpp
│   └── AudioPlayer.h
├── resource/               # 资源文件
│   ├── app.ico
│   ├── app.manifest
│   ├── resource.h
│   └── resources.rc
├── build/                  # 构建目录（生成）
├── script/                 # 编译脚本目录
│   ├── build.bat          # Windows 批处理编译脚本
│   ├── build.ps1          # PowerShell 编译脚本
│   ├── clean.bat          # Windows 清理脚本
│   └── clean.ps1          # PowerShell 清理脚本
├── CMakeLists.txt         # CMake 配置文件
└── README.md              # 本文件
```

## 联系信息

如果您在使用过程中遇到问题，请查看项目文档或提交 Issue。

---

*最后更新: 2025-06-11*
