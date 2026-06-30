# AGENTS.md

Agent 在此仓库工作时请遵循本文件。用户文档参见 [README.md](./README.md)。

## 项目简介

EVCS (Examination Voice Command System) — 面向标准化考试的自动语音指令播放系统。根据预设时间点自动播放考试语音指令，支持多科目与多种音频格式。C++17 + 原生 Win32 API + BASS 音频库，目标 Windows 7+ x64。

## 仓库导航

```
src/           源码（main, MainWindow, Subject, Instruction, AudioPlayer, ConfigManager, StringUtil）
resource/      资源（app.ico, app.manifest, resource.h, resources.rc）
config/        INI 配置（default/cz/czqm），由 ConfigManager 加载
script/        build.bat / release.bat / debug.bat / clean.bat
third_party/   BASS 音频库（x64）
docs/          配置文件功能说明
```

## 常用命令

```bash
# 编译（最简方式，VS2022 + Release x64）
script\build.bat

# 发布包（含 EVCS.exe、bass.dll、README.txt、config\*.ini）
script\release.bat

# Debug 构建
script\debug.bat

# 清理
script\clean.bat

# 手动 CMake（备选）
mkdir build && cd build
cmake -G "Visual Studio 17 2022" -A x64 ..
cmake --build . --config Release
```

产物：`build/Release/EVCS.exe`、`build/Debug/EVCS.exe`。

## 架构

单窗口应用程序，由 `main.cpp` 启动初始化。主要类关系：

```
MainWindow (主窗口)
├── 管理多个 Subject 对象
├── 管理多个 Instruction 对象
├── 使用 AudioPlayer 进行音频播放
└── 提供GUI界面和用户交互
Subject   存储科目信息，生成对应 Instruction 序列
Instruction 存储指令信息，管理播放状态
AudioPlayer 统一的音频播放接口（封装 BASS）
ConfigManager 解析外部 INI，动态加载科目与指令
StringUtil   UTF-8 ↔ 宽字符转换（全项目 Unicode 支持）
```

### MainWindow

- 主窗口 GUI 与所有用户交互；创建/管理主窗口、ListView、状态栏、状态面板。
- **DPI 感知**：`SetProcessDPIAware()` + `GetDeviceCaps(LOGPIXELSX)`，自定义缩放算法（Win7 兼容 API）。
- **实时更新**：定时器每秒更新状态信息与界面；完整的播放状态跟踪。
- 交互：右键菜单、双击播放、列表选择。

### Subject

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

预设（在 `Subject::createSubject` 添加新科目）：

| 科目 | 时长 | 类型 |
| --- | --- | --- |
| 语文 | 150 min | 单场 |
| 数学 | 120 min | 单场 |
| 英语（含听力） | 120 min | 单场 |
| 单科 | 75 min | 单场 |
| 首选科目 | 75 min | 单场 |
| 再选合堂 | 160 min | 双场 |

### Instruction

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

播放状态机 `UNPLAYED → PLAYING → PLAYED/SKIPPED`。新增指令模板在 `Instruction::generateInstructions`。

### AudioPlayer

- 统一 WAV/MP3 接口，封装 BASS：`BASS_Init(-1, 44100, ...)`、`BASS_StreamCreateFile(..., BASS_STREAM_AUTOFREE | BASS_UNICODE)`、`BASS_ChannelPlay`。
- 音量百分比查询、音频时长获取；自动处理 `audio/` 子目录与 Unicode 文件名。

### ConfigManager

解析 `config/*.ini`，动态加载科目与指令列表（外部配置文件系统）。

## Windows 特定实现

- **Unicode**：完整支持；UI/文件边界用宽字符，内部 UTF-8 经 `StringUtil` 转换。支持中文界面与文件名。
- **Win32 API**：ListView 表格、状态栏、自定义状态面板、右键菜单、对话框、计时器驱动更新。
- **DPI**：见上文 MainWindow。

## 资源与控件

```
resource/ app.ico, app.manifest, resource.h, resources.rc
```

关键控件 ID：`IDC_SUBJECT_LIST`、`IDC_INSTRUCTION_LIST`、`IDC_STATUS_PANEL`。
菜单 ID：`IDM_FILE_ADD_SUBJECT`、`IDM_DELETE_SUBJECT`、`IDM_PLAY_INSTRUCTION`、`IDM_HELP_HELP`、`IDM_HELP_ABOUT`。

## 编码约定

- C++17，MSVC 编译，启用 `/utf-8 /W4`、`UNICODE`/`_UNICODE`、`_CRT_SECURE_NO_WARNINGS`。
- 字符串在 UI/文件边界用宽字符，内部存储优先 `std::string` (UTF-8)，经 `StringUtil` 转换。
- 文件路径优先 `std::filesystem`；音频文件必须位于 `audio/`，实时存在性检查（不缓存）。
- 资源用 RAII / 智能指针；BASS 流使用 `BASS_STREAM_AUTOFREE` 自动释放。
- 强类型与枚举替代魔法数字；类职责单一；改动后同步更新 [version.h](./src/version.h)。

## 版本

`src/version.h` 定义 `EVCS_VERSION_{MAJOR,MINOR,PATCH,STRING}` 与 `EVCS_BUILD_DATE`，发版时一并更新。

## 测试与验证

无测试套件、无外部配置文件依赖（INI 除外）。改动需手动验证：科目增删、指令生成/播放、时间控制准确性、缺文件处理、系统时间变更、过期指令跳过逻辑等边界。

## 部署注意事项

- BASS 依赖 `third_party/bass/`，运行需 bass.dll；通过 `#pragma comment` 按架构自动链接 x64/x86，需手动将 bass.dll 放到程序目录。
- 所有设置通过界面操作完成（除 INI 配置外无外部配置文件）。

## 注意事项

- 中文路径与文件名为一等公民，改动路径处理时务必用宽字符 API。
- 仓库 `.gitignore` 已忽略 `build/`、`*.exe`、`*.dll`、音频文件、`.claude/` 等产物，勿提交。
