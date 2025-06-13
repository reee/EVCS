# EVCS 项目编译指南

## 概述

EVCS (Exam Voice Command System) 是一个考试语音指令系统，使用 C++ 和 CMake 构建的 Windows 应用程序项目。该系统用于在考试期间按照预设时间播放语音指令，支持WAV和MP3格式的音频文件。本项目提供了多种编译脚本，位于 `script` 目录下，适用于 Windows 开发环境。

## 功能特性

- **多格式音频支持**：支持WAV和MP3格式的语音指令文件
- **精确时间控制**：按照预设时间点自动播放对应的语音指令
- **实时状态显示**：显示当前指令播放进度、剩余时长和下一条指令信息
- **科目管理**：支持多个考试科目的语音指令配置
- **直观界面**：简洁的Windows GUI界面，方便操作使用

## 使用说明

请查看README.txt获取详细的使用说明


## 编译要求

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
- 本项目不包含base.dll文件，请自行下载：[Base Audio Lib](https://www.un4seen.com/files/bass24.zip)，并把x64目录下的base.dll放置到EVCS.exe相同目录。

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
