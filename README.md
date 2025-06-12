# EVCS 项目编译指南

## 概述

EVCS (Exam Voice Command System) 是一个考试语音指令系统，使用 C++ 和 CMake 构建的 Windows 应用程序项目。该系统用于在考试期间按照预设时间播放语音指令，支持WAV和MP3格式的音频文件。本项目提供了多种编译脚本，位于 `script` 目录下，适用于 Windows 开发环境。

## 功能特性

- **多格式音频支持**：支持WAV和MP3格式的语音指令文件
- **精确时间控制**：按照预设时间点自动播放对应的语音指令
- **实时状态显示**：显示当前指令播放进度、剩余时长和下一条指令信息
- **音频设备管理**：支持选择不同的音频输出设备
- **科目管理**：支持多个考试科目的语音指令配置
- **播放控制**：支持暂停、恢复、停止等播放控制功能
- **直观界面**：简洁的Windows GUI界面，方便操作使用

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

## 使用说明

### 音频文件准备

1. **创建subjects目录**：在程序所在目录创建 `subjects` 文件夹
2. **音频文件格式**：支持WAV和MP3格式的音频文件
3. **文件命名规则**：音频文件名需要包含播放时间信息
   - 格式：`科目名_HH-MM-SS_指令描述.wav/mp3`
   - 示例：`数学_14-30-00_开始答题.wav`、`英语_15-45-30_还有15分钟.mp3`

### 基本使用流程

1. **启动程序**：运行编译生成的 `EVCS.exe`
2. **选择科目**：从下拉列表中选择要播放的考试科目
3. **选择音频设备**：根据需要选择音频输出设备
4. **开始播放**：点击"开始播放"按钮启动语音指令播放
5. **实时监控**：观察状态面板显示的当前指令信息和播放进度
6. **控制播放**：使用暂停、恢复、停止按钮控制播放状态

### 详细使用说明

更多详细的使用说明和配置方法，请查看程序目录下的 `README.txt` 文件。

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
│   ├── main.cpp           # 程序入口
│   ├── MainWindow.cpp     # 主窗口实现
│   ├── MainWindow.h       # 主窗口头文件
│   ├── Subject.cpp        # 科目管理实现
│   ├── Subject.h          # 科目管理头文件
│   ├── Instruction.cpp    # 指令管理实现
│   ├── Instruction.h      # 指令管理头文件
│   ├── AudioPlayer.cpp    # 音频播放器实现
│   └── AudioPlayer.h      # 音频播放器头文件
├── resource/               # 资源文件
│   ├── app.ico            # 应用程序图标
│   ├── app.manifest       # 应用程序清单
│   ├── resource.h         # 资源头文件
│   └── resources.rc       # 资源脚本
├── subjects/               # 音频文件目录（运行时创建）
│   ├── 数学/              # 数学科目音频文件
│   ├── 英语/              # 英语科目音频文件
│   └── ...                # 其他科目音频文件
├── build/                  # 构建目录（生成）
├── script/                 # 编译脚本目录
│   ├── build.bat          # Windows 批处理编译脚本
│   ├── build.ps1          # PowerShell 编译脚本
│   ├── clean.bat          # Windows 清理脚本
│   └── clean.ps1          # PowerShell 清理脚本
├── CMakeLists.txt         # CMake 配置文件
├── README.md              # 项目文档（本文件）
└── README.txt             # 用户使用说明
```

## 技术依赖

- **BASS音频库**：用于音频文件播放，支持多种音频格式
- **Windows API**：GUI界面和系统功能调用
- **CMake**：跨平台构建系统

## 开发说明

### 核心组件

1. **MainWindow**：主窗口类，负责GUI界面和用户交互
2. **Subject**：科目管理类，处理科目列表和音频文件扫描
3. **Instruction**：指令类，管理单个语音指令的时间和播放信息
4. **AudioPlayer**：音频播放器类，统一使用BASS库播放音频文件

### 编译要求

- Visual Studio 2019/2022
- CMake 3.10+
- BASS音频库（已包含在项目中）

## 联系信息

如果您在使用过程中遇到问题，请查看项目文档或访问项目主页：
https://github.com/reee/EVCS/

---

*考试语音指令系统 - 为考试管理提供精确的语音提示服务*
