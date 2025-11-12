# EVCS 项目文档

## 概述

EVCS (Examination Voice Command System) 是一个专为标准化考试设计的自动语音指令播放系统，使用 C++17 和 Windows API 开发。该系统能够根据预设的时间点自动播放相应的语音指令，支持多种考试科目和音频格式。

## 功能特性

- **自动播放指令**：根据预设时间自动播放考试相关语音指令
- **多科目支持**：支持语文、数学、英语、单科、首选科目、再选合堂等多种考试科目
- **配置文件系统**：支持通过外部INI配置文件自定义科目和指令列表
- **实时状态监控**：显示当前播放指令的进度或下一指令的倒计时
- **高DPI支持**：完整的高DPI显示器适配
- **灵活时间安排**：支持自定义考试时间和指令安排
- **手动播放控制**：支持手动播放或跳过指令

## 使用说明

### 📖 详细使用指南
请查看 **[README.txt](README.txt)** 获取完整的用户使用说明，包括：
- 快速开始指南
- 音频文件准备要求
- 配置文件系统详解
- 各科目类型说明
- 故障排除指南

### 🔧 编译和部署
请查看下方 **编译指南** 部分了解如何编译项目

### 📁 发布版本
使用 `script\release.bat` 可以生成包含所有必要文件的完整发布包

## 系统要求

- **操作系统**：Windows 7 及以上版本 (64位)
- **依赖库**：BASS Audio Library (需要手动下载 bass.dll)


## 编译要求

### Windows
- CMake 3.10 或更高版本
- Visual Studio 2022 (推荐) 或 Visual Studio 2019
- Windows 10/11 操作系统

## 编译指南

### 🔨 编译脚本说明

项目在 `script` 目录下提供了以下编译脚本：

#### `script\build.bat` - Windows 批处理编译脚本
最简单的编译方式，在项目根目录下运行：
```batch
script\build.bat
```

或者进入 script 目录后双击 `build.bat` 文件。

**特性**：
- 自动检测 CMake 安装
- 使用 Visual Studio 2022 生成器
- 编译 Release 版本
- 生成 x64 架构的可执行文件
- 自动处理相对路径，从 script 目录定位到项目根目录

#### `script\release.bat` - Windows 发布脚本 ⭐ **推荐**
生成完整的发布包，包含所有必要文件：
```batch
script\release.bat
```

**包含文件**：
- EVCS.exe（主程序）
- bass.dll（音频库）
- README.txt（用户文档）
- config\*.ini（配置文件）
- audio\ 目录（音频文件目录，用户需自行添加）

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

### 📦 BASS 音频库

本项目使用 BASS Audio Library 进行音频播放，**不包含** bass.dll 文件。请按以下步骤获取：

1. 下载：[BASS Audio Library](https://www.un4seen.com/files/bass24.zip)
2. 解压后将 `x64\bass.dll` 复制到可执行文件相同目录
3. 或者使用 `script\release.bat` 自动复制

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
│   ├── AudioPlayer.h      # 音频播放器头文件
│   ├── ConfigManager.cpp  # 配置管理器实现
│   └── ConfigManager.h    # 配置管理器头文件
├── resource/               # 资源文件
│   ├── app.ico            # 应用程序图标
│   ├── app.manifest       # 应用程序清单
│   ├── resource.h         # 资源头文件
│   └── resources.rc       # 资源脚本
├── config/                 # 配置文件目录
│   ├── default.ini        # 默认配置（新高考）
│   ├── cz.ini             # 传统高考配置
│   └── czqm.ini           # 传统高考配置（另版）
├── script/                 # 编译脚本目录
│   ├── build.bat          # Windows 编译脚本
│   ├── release.bat        # Windows 发布脚本 ⭐
│   └── clean.bat          # Windows 清理脚本
├── third_party/            # 第三方库
│   └── bass/              # BASS音频库
│       └── x64/bass.dll   # x64版本DLL
├── build/                  # 构建目录（生成）
├── release/                # 发布目录（生成）
├── CMakeLists.txt         # CMake 配置文件
├── README.md              # 项目文档（本文件）
└── README.txt             # 用户使用说明 📖
```

## 技术栈

- **开发语言**：C++17
- **GUI框架**：Windows API (原生Win32)
- **音频库**：BASS Audio Library (支持WAV、MP3等格式)
- **构建系统**：CMake 3.10+
- **开发环境**：Visual Studio 2022 (推荐)
- **目标平台**：Windows 7+ (64位)

## 🏗️ 核心架构

### 主要组件

1. **MainWindow**：主窗口类，负责GUI界面和用户交互
   - 完整的DPI缩放支持
   - 实时状态更新和显示
   - 菜单和用户交互处理

2. **Subject**：科目管理类，处理考试科目信息
   - 支持多种预设科目类型
   - 科目时长和配置管理

3. **Instruction**：指令类，管理单个语音指令
   - 时间控制和播放状态管理
   - 支持多种播放状态（未播放、播放中、已播放、已跳过）

4. **AudioPlayer**：音频播放器类，统一音频播放接口
   - 支持WAV、MP3等格式
   - 音量控制和时长获取

5. **ConfigManager**：配置管理器类（新增）
   - 外部INI配置文件解析
   - 动态科目和指令加载

## 🚀 快速开始

### 1. 编译项目
```batch
script\release.bat
```

### 2. 准备音频文件
将音频文件放入 `audio/` 目录：
- 1kq12.mp3, 2kq10.mp3, 3kq5.mp3
- 4ksks.mp3, 5jsq15.mp3, 6ksjs.mp3
- sy.mp3, tl.mp3 (英语科目)

### 3. 运行程序
双击 `release\EVCS.exe` 开始使用

详细使用说明请查看 **[README.txt](README.txt)**

## 📄 许可证

本项目为开源项目，遵循相应的开源协议。
详细信息请查看项目页面：https://github.com/reee/EVCS/
