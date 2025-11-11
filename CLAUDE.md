# EVCS 项目架构分析文档

## 项目概述

EVCS (Examination Voice Command System) 是一个专门为标准化考试设计的自动语音指令播放系统，使用 C++ 和 Windows API 开发。该系统能够根据预设的时间点自动播放相应的语音指令，支持多种考试科目和音频格式。

## 技术栈

- **开发语言**: C++17
- **GUI框架**: Windows API (原生Win32)
- **音频库**: BASS Audio Library (支持WAV、MP3等格式)
- **构建系统**: CMake 3.10+
- **开发环境**: Visual Studio 2022 (推荐)
- **目标平台**: Windows 7+ (64位)

## 项目架构设计

### 整体架构

项目采用典型的单窗口应用程序架构，主要包含以下核心组件：

1. **主入口模块** (`main.cpp`) - 程序启动和初始化
2. **主窗口模块** (`MainWindow`) - GUI界面和用户交互
3. **科目管理模块** (`Subject`) - 考试科目数据管理
4. **指令管理模块** (`Instruction`) - 语音指令生成和管理
5. **音频播放模块** (`AudioPlayer`) - 统一的音频播放接口

### 主要类关系图

```
MainWindow (主窗口)
├── 管理多个 Subject 对象
├── 管理多个 Instruction 对象  
├── 使用 AudioPlayer 进行音频播放
└── 提供GUI界面和用户交互

Subject (科目)
├── 存储科目基本信息 (名称、时长、开始时间)
├── 生成对应的 Instruction 序列
└── 支持不同科目类型的预设配置

Instruction (指令)
├── 存储指令信息 (名称、播放时间、音频文件)
├── 管理播放状态 (未播放、播放中、已播放、已跳过)
└── 提供状态检查和显示功能

AudioPlayer (音频播放器)
├── 统一的音频播放接口
├── 支持多种音频格式 (WAV、MP3)
└── 提供音量控制和时长获取
```

## 核心组件详细分析

### 1. MainWindow 类

**职责**: 程序的主窗口界面，负责所有用户交互和界面显示

**主要功能**:
- 创建和管理主窗口及子控件 (ListView、状态栏、状态面板)
- 处理用户输入 (菜单点击、列表选择、双击操作)
- 管理科目和指令列表的显示更新
- 协调音频播放和时间控制
- 提供DPI缩放支持

**关键特性**:
- **DPI感知**: 完整的高DPI显示器支持，使用Windows 7兼容的DPI API
- **实时更新**: 通过定时器每秒更新状态信息和界面
- **状态管理**: 完整的播放状态跟踪和显示
- **用户交互**: 支持右键菜单、双击播放等交互方式

### 2. Subject 类

**职责**: 管理考试科目信息，包含科目配置和验证功能

**数据结构**:
```cpp
struct Subject {
    int id;                          // 唯一标识符
    std::string name;                // 科目名称
    int durationMinutes;             // 考试时长(分钟)
    bool isDoubleSession;            // 是否为双场考试
    std::chrono::system_clock::time_point startTime; // 开始时间
    static int nextId;               // 静态ID计数器
}
```

**科目预设配置**:
- 语文: 150分钟，单场考试
- 数学: 120分钟，单场考试  
- 英语: 120分钟，单场考试 (含听力)
- 单科: 75分钟，单场考试
- 首选科目: 75分钟，单场考试
- 再选合堂: 160分钟，双场考试

### 3. Instruction 类

**职责**: 管理单个语音指令，包括时间控制、状态管理和文件验证

**数据结构**:
```cpp
struct Instruction {
    int subjectId;                    // 所属科目ID
    std::string subjectName;          // 科目名称(显示用)
    std::string name;                 // 指令名称
    std::chrono::system_clock::time_point playTime; // 播放时间
    std::string audioFile;            // 音频文件名
    PlaybackStatus status;           // 播放状态
}
```

**播放状态枚举**:
- `UNPLAYED`: 未播放
- `PLAYING`: 正在播放
- `PLAYED`: 已播放
- `SKIPPED`: 已跳过

### 4. AudioPlayer 类

**职责**: 统一的音频播放接口，封装BASS库的使用

**功能特性**:
- **多格式支持**: WAV、MP3等格式
- **统一接口**: 所有音频文件都通过BASS库播放
- **音量控制**: 获取系统音量百分比
- **时长获取**: 获取音频文件播放时长
- **路径管理**: 自动处理audio子目录路径

**依赖**: BASS Audio Library，通过动态链接库集成

## Windows 特定实现

### DPI 缩放支持

项目使用Windows 7兼容的DPI API：
- 使用 `SetProcessDPIAware()` 设置DPI感知
- 通过 `GetDeviceCaps(hdc, LOGPIXELSX)` 获取DPI值
- 实现自定义缩放算法处理控件大小和字体大小

### Unicode 支持

- 完整的Unicode字符串处理
- UTF-8到宽字符的转换
- 支持中文界面和文件名

### Windows API 特性

- 使用Win32 API创建和管理窗口
- ListView控件用于表格显示
- 状态栏和自定义状态面板
- 右键菜单和对话框
- 计时器驱动的实时更新

## 音频处理机制

### BASS 集成

```cpp
// 初始化BASS库
BASS_Init(-1, 44100, 0, NULL, NULL);

// 创建音频流
HSTREAM stream = BASS_StreamCreateFile(FALSE, wideFilePath.c_str(), 0, 0, 
                                     BASS_STREAM_AUTOFREE | BASS_UNICODE);

// 播放音频
BASS_ChannelPlay(stream, FALSE);
```

**音频特性**:
- 自动内存管理 (`BASS_STREAM_AUTOFREE`)
- Unicode文件名支持
- 支持流式播放
- 自动释放资源

### 文件管理

- 音频文件必须放在 `audio/` 子目录
- 支持实时文件存在性检查
- 自动处理路径转换和文件格式验证

## 构建和开发流程

### CMake 配置

```cmake
cmake_minimum_required(VERSION 3.10)
project(EVCS)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${HEADERS} ${RESOURCES})

target_link_libraries(${PROJECT_NAME} PRIVATE
    winmm
    comctl32
)
```

### 编译脚本

项目提供了两个Windows批处理脚本：

#### script/build.bat
- 自动检测CMake安装
- 使用Visual Studio 2022生成器
- 编译Release版本
- 输出x64架构可执行文件

#### script/clean.bat  
- 清理构建目录和临时文件

### 第三方依赖

#### BASS Audio Library
- 位置: `third_party/bass/`
- 包含x64和x86两个版本
- 通过`#pragma comment`自动链接对应版本
- 需要手动将bass.dll放到程序目录

## 资源文件管理

### 资源文件结构
```
resource/
├── app.ico          # 应用程序图标
├── app.manifest     # 应用程序清单  
├── resource.h       # 资源头文件
├── resources.rc     # 资源脚本
└── icon.svg         # 矢量图标(备选)
```

### 资源定义

**控件ID**:
- `IDC_SUBJECT_LIST`: 科目列表
- `IDC_INSTRUCTION_LIST`: 指令列表
- `IDC_STATUS_PANEL`: 状态面板

**菜单ID**:
- `IDM_FILE_ADD_SUBJECT`: 添加科目
- `IDM_DELETE_SUBJECT`: 删除科目
- `IDM_PLAY_INSTRUCTION`: 立即播放
- `IDM_HELP_HELP`: 帮助
- `IDM_HELP_ABOUT`: 关于

### 版本管理

版本信息定义在`src/version.h`:
```cpp
#define EVCS_VERSION_MAJOR    1
#define EVCS_VERSION_MINOR    1  
#define EVCS_VERSION_PATCH    0
#define EVCS_VERSION_STRING   "1.1.0"
#define EVCS_BUILD_DATE       "2025年7月"
```

## 开发相关实用信息

### 开发环境设置

1. **必需软件**:
   - Visual Studio 2022 (推荐)
   - CMake 3.10+
   - BASS Audio Library

2. **构建步骤**:
   ```bash
   cd /path/to/EVCS
   script\build.bat
   ```

3. **手动构建** (备选):
   ```bash
   mkdir build
   cd build
   cmake -G "Visual Studio 17 2022" -A x64 ..
   cmake --build . --config Release
   ```

### 代码组织原则

1. **模块化**: 每个类都有明确的单一职责
2. **类型安全**: 使用强类型和枚举避免魔法数字
3. **错误处理**: 适当的异常处理和错误检查
4. **内存管理**: 使用RAII和智能指针管理资源
5. **跨平台考虑**: 虽然针对Windows，但尽量使用标准库

### 测试和调试

1. **功能测试**:
   - 科目添加/删除功能
   - 指令生成和播放
   - 时间控制准确性
   - 音频文件检查

2. **边界情况处理**:
   - 过期指令跳过逻辑
   - 文件不存在时的处理
   - 系统时间变化的影响

### 部署注意事项

1. **依赖要求**:
   - 需要手动下载并放置bass.dll
   - 确保audio目录包含所需音频文件
   - Windows 7+系统兼容性

2. **配置文件**:
   - 无外部配置文件
   - 所有设置通过界面操作完成

### 性能优化

1. **界面更新**:
   - 使用ListView预分配提升性能
   - 定时器驱动的批量更新
   - 避免频繁的重绘操作

2. **内存管理**:
   - BASS库的自动内存管理
   - 及时释放不再使用的资源
   - 避免内存泄漏

3. **文件IO**:
   - 使用C++17文件系统库
   - 实时文件检查，不依赖缓存
   - 路径处理优化

## 扩展性和维护性

### 可扩展点

1. **新科目类型**: 在`Subject::createSubject`中添加新配置
2. **新指令类型**: 在`Instruction::generateInstructions`中添加模板
3. **新音频格式**: 通过BASS库自动支持
4. **新UI组件**: 基于现有架构添加

### 维护考虑

1. **向后兼容**: 保持现有API的稳定性
2. **代码风格**: 统一的命名规范和代码结构
3. **文档完善**: 注释和文档的及时更新
4. **测试覆盖**: 关键功能的单元测试

## 总结

EVCS是一个设计良好的Windows桌面应用程序，具有清晰的架构和良好的可维护性。项目采用了现代C++特性，结合Windows API和BASS音频库，实现了专业的考试语音指令播放功能。代码结构清晰，职责分明，适合进一步的扩展和维护。
