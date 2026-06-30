# AGENTS.md

Agent 在此仓库工作时请遵循本文件。用户文档参见 [README.md](./README.md)。

## 项目简介

EVCS (Examination Voice Command System) — 面向**平时考试**的自动语音指令及听力播放系统。根据预设时间点自动播放考试语音指令（含英语听力），支持多科目与多种音频格式。C++17 + 原生 Win32 API + BASS 音频库，目标 Windows 7+ x64。

## SCENARIO INVARIANTS（场景不变量 — 考试指令及听力播放，硬约束）

下列约束源于产品定位：考试由考务操作员按预设时间表自动播放语音指令（含听力），单场持续数分钟至数小时，期间不得因任何软件原因漏播、错播或卡顿。**违反任一条即视为 P0 缺陷**，新代码与未来审计均不得对这些条目再次提出"优化建议"。

### 1. 不考虑省电
- 不引入、不评估任何省电/低功耗优化。当前唯一计时器为 1s UI timer（`MainWindow.cpp` 中 `SetTimer`，`TIMER_INTERVAL=1000`），用途是状态刷新与到点触发播放，**不得为省电改为 coalescable/低频 timer，也不得把"1s 轮询粒度"作为审计项要求降频**。
- 任何"为降低 CPU 占用而牺牲定时/播放实时性"的权衡均不成立。

### 2. 考试期间必须保持系统唤醒（当前为待补齐的 P0 缺口）
- 考试播放期间，系统进入休眠/熄屏会直接导致漏播，对考试场景属**高风险**。
- **强制不变量**：程序运行期间必须阻止系统休眠与熄屏。实现方式为启动时调用 `SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED)`，覆盖整个会话；退出时还原。**不要改为按播放状态启停**（按播放状态启停存在窗口期漏播风险）。
- ⚠️ **现状**：源码中尚未实现任何电源管理 API。此项为已识别的 P0 待补齐项；补齐实现前，新代码不得进一步加剧该风险（如新增可能阻塞消息泵、延长休眠敏感期的逻辑）。

### 3. 播放准确与流畅是首要目标
- 播放调度基于系统时钟（`std::chrono::system_clock`）：到点由 1s timer 轮询触发 `MainWindow::UpdateNextInstruction → PlayInstruction`。走时/到点判断必须基于 `system_clock` 实际时间，**不得改为基于播放进度或设备时钟估算**，避免与考试真实时间脱节。
- 音频输出经 BASS（`BASS_Init(-1,44100,...)` + `BASS_StreamCreateFile` + `BASS_ChannelPlay`）。BASS 内部线程的采样率/缓冲配置不得为省电或 CPU 而改动。
- 播放状态机 `UNPLAYED → PLAYING → PLAYED/SKIPPED` 语义不可放宽：过期(>60s) 未播指令必须置 `SKIPPED`；手动起播前未播指令必须经 `MarkPreviousAsSkipped`，保证整体进度一致。
- 音频文件存在性检查（`std::filesystem::exists`）必须**实时**（不缓存路径存在性；仅缓存聚合计数以减少文件系统扫描，见 MainWindow）。

### 4. 全流程防卡死/防超时
- **当前为单线程 GUI**：配置读取、指令生成、音频文件检查、ListView 重建都在 UI 线程同步执行。**新增耗时操作不得继续堆在 UI 线程上**：若引入 >100ms 的任务（大文件解码、批量校验、网络等），必须先引入后台线程机制（`std::jthread` / 后台队列），UI 线程上禁止阻塞 I/O（小配置文件除外）。
- **任何新增后台任务必须支持取消**：接受 `std::stop_token` 或等价取消信号，循环体内每个数据块之间检查取消请求；不检查取消的后台长任务视为 P0。
- **配置/音频文件解析必须有上限与边界检查**：`ConfigManager` 读取 INI 时对文件大小、行数、缓冲区应有防御性上限；未来任何自实现解码器（目前由 BASS 负责）必须对结构复杂度、迭代次数、缓冲区大小做防御性上限。
- **可重入/可刷新操作必须有 generation 守卫**：当前配置重载会先 `stop()` 当前播放再重生成；若未来引入异步加载/校验，需引入 generation 计数，防止过期结果回灌到 UI。

### 5. 输入可信但解析仍需防御
- 考试音频与 INI 由统一渠道分发，理论上可信；但解析与边界检查仍属强制项——介质损坏、传输不完整、误传文件都可能让程序进入未定义行为。**防御性边界检查不是"过度防御"，是 P0 强制项。**
- 音频文件名/路径含中文，必须全程宽字符；BASS 以 `BASS_UNICODE` 打开。

## 仓库导航

```
src/           源码（main, MainWindow, Subject, Instruction, AudioPlayer,
                       ConfigManager, StringUtil, PathUtil, version.h）
resource/      资源（app.ico, app.manifest, resource.h, resources.rc）
config/        INI 配置（default/cz/czqm），由 ConfigManager 加载
script/        build.bat / release.bat / debug.bat / clean.bat (+ _deploy.bat / _detect.bat)
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

单窗口、单 GUI 线程应用程序，由 `main.cpp` 启动初始化并运行消息循环。音频输出由 BASS 内部线程负责，应用代码不直接接触。主要类关系：

```
MainWindow (主窗口，单例式 GUI)
├── 管理多个 Subject 对象
├── 管理多个 Instruction 对象（按 playTime 排序）
├── 使用 AudioPlayer 进行音频播放（封装 BASS）
├── 1s SetTimer 驱动：状态刷新 + 到点触发播放 + 播放完成检测
└── 提供GUI界面和用户交互
Subject       存储科目信息（名称、时长、开始时间），时长由 ConfigManager 提供
Instruction   存储指令信息，管理播放状态（UNPLAYED/PLAYING/PLAYED/SKIPPED）
              模板与播放偏移由 ConfigManager 的 InstructionTemplate 提供
AudioPlayer   统一的音频播放接口（封装 BASS），含系统音量查询
ConfigManager 解析外部 INI，动态加载科目与指令模板（单例）
StringUtil   UTF-8 ↔ 宽字符转换（全项目 Unicode 支持）
PathUtil      统一音频路径解析（audio/ 子目录）
```

### MainWindow

- 主窗口 GUI 与所有用户交互；创建/管理主窗口、ListView、状态栏、状态面板。
- **DPI 感知**：`SetProcessDPIAware()` + `GetDeviceCaps(LOGPIXELSX)`，自定义缩放算法（Win7 兼容 API）。
- **实时更新**：1s `SetTimer` 每秒驱动 `UpdateStatusBar` / `UpdateStatusPanel` / `CheckPlaybackCompletion` / `UpdateNextInstruction`；后者在到点时触发 `PlayInstruction`。
- **性能节流**：音频文件缺失计数缓存（`m_cachedMissingInstructionCount`，科目/指令变动时失效）；系统音量查询节流到每 5s 一次（COM 设备枚举开销大）。
- 交互：右键菜单、双击播放、列表选择。

### Subject

```cpp
struct Subject {
    static int nextId;  // 静态 ID 计数器
    static constexpr int DEFAULT_DURATION_MINUTES = 90;  // 配置缺失时的默认时长

    int id;                          // 唯一标识符
    std::string name;                // 科目名称
    int durationMinutes;             // 考试时长(分钟)，由 ConfigManager 提供
    std::chrono::system_clock::time_point startTime; // 开始时间
}
```

**科目与时长均由 INI 配置驱动**（无硬编码科目表）：`Subject::createSubject` 从 `ConfigManager::getSubjectConfig(name)` 读取时长；配置缺失时回退 `DEFAULT_DURATION_MINUTES`。可用科目列表来自 `ConfigManager::getSubjectNames()`。

### Instruction

```cpp
struct Instruction {
    int subjectId;
    std::string subjectName;       // 显示用科目名称
    std::string name;
    std::chrono::system_clock::time_point playTime;  // 播放时间
    std::string audioFile;
    PlaybackStatus status;
    mutable double cachedDurationSeconds = 0.0;  // 缓存的音频时长(秒)，<=0 表示未取/失败
}
```

播放状态机 `UNPLAYED → PLAYING → PLAYED/SKIPPED`：
- 过期(>60s)未播 → `SKIPPED`；手动起播 → 之前的未播指令经 `MarkPreviousAsSkipped` 置 `SKIPPED`。
- 播放完成由 `CheckPlaybackCompletion` 轮询 `AudioPlayer::isPlaying()` 检测。

指令模板由 `ConfigManager` 的 `InstructionTemplate { offsetSeconds, name, audioFile }` 提供；`Instruction::generateInstructions` 按 `subject.startTime + offsetSeconds` 生成 `playTime`。

### AudioPlayer

- 统一 WAV/MP3 接口，封装 BASS：`BASS_Init(-1, 44100, ...)`、`BASS_StreamCreateFile(..., BASS_UNICODE)`、`BASS_ChannelPlay`。
- **故意不使用 `BASS_STREAM_AUTOFREE`**：保留句柄以便查询活跃状态（`isPlaying`），播放结束/出错时由 `stop()`/`cleanup()` 显式 `BASS_ChannelStop` + `BASS_StreamFree` 释放。
- 音量百分比查询（`getSystemVolume`，经 `IMMDeviceEnumerator`/`IAudioEndpointVolume`）；音频时长获取（`getAudioDuration` / `getCurrentStreamDuration`）。
- 自动处理 `audio/` 子目录（`PathUtil::getAudioPath`）与 Unicode 文件名。

### ConfigManager

单例，解析 `config/*.ini`，动态加载科目列表（`SubjectConfig`）与指令模板（`InstructionTemplate`）。支持运行时加载/重载配置文件。

## Windows 特定实现

- **Unicode**：完整支持；UI/文件边界用宽字符，内部 UTF-8 经 `StringUtil` 转换。支持中文界面与文件名。
- **Win32 API**：ListView 表格、状态栏、自定义状态面板、右键菜单、对话框、计时器驱动更新。
- **COM**：主线程 STA（`COINIT_APARTMENTTHREADED`），用于系统音量查询。
- **DPI**：见上文 MainWindow。

## 资源与控件

```
resource/ app.ico, app.manifest, resource.h, resources.rc
```

关键控件 ID：`IDC_SUBJECT_LIST`、`IDC_INSTRUCTION_LIST`、`IDC_STATUS_PANEL`、`IDC_SUBJECT_COMBO`、`IDC_START_DATE_EDIT`、`IDC_START_TIME_EDIT`。
菜单 ID：`IDM_FILE_ADD_SUBJECT`、`IDM_FILE_LOAD_CONFIG`、`IDM_FILE_RELOAD_CONFIG`、`IDM_DELETE_SUBJECT`、`IDM_PLAY_INSTRUCTION`、`IDM_HELP_HELP`、`IDM_HELP_ABOUT`。

## 编码约定

- C++17，MSVC 编译，启用 `/utf-8 /W4`、`UNICODE`/`_UNICODE`、`_CRT_SECURE_NO_WARNINGS`。
- 字符串在 UI/文件边界用宽字符，内部存储优先 `std::string` (UTF-8)，经 `StringUtil` 转换。
- 文件路径优先 `std::filesystem`；音频文件必须位于 `audio/`（经 `PathUtil` 解析），实时存在性检查（不缓存路径存在性）。
- 资源用 RAII / 显式释放；BASS 流**不使用** `BASS_STREAM_AUTOFREE`，由 `stop()`/`cleanup()` 手动释放。
- 强类型与枚举替代魔法数字；类职责单一；改动后同步更新 [version.h](./src/version.h)。

## 版本

`src/version.h` 定义 `EVCS_VERSION_{MAJOR,MINOR,PATCH,STRING}` 与 `EVCS_BUILD_DATE`，发版时一并更新。

## 测试与验证

无测试套件、无外部配置文件依赖（INI 除外）。改动需手动验证：科目增删、指令生成/播放、时间控制准确性、缺文件处理、系统时间变更、过期指令跳过、配置重载等边界。

## 部署注意事项

- BASS 依赖 `third_party/bass/`，运行需 bass.dll；通过 `#pragma comment` 按架构自动链接 x64/x86，发布脚本（`script/release.bat` 经 `_deploy.bat`）自动复制 bass.dll、`config/*.ini`、README.txt 到发布目录，需手动放置音频文件。
- 所有设置通过界面操作完成（除 INI 配置外无外部配置文件）。

## 注意事项

- 中文路径与文件名为一等公民，改动路径处理时务必用宽字符 API。
- batch 脚本（`script/*.bat`）必须使用 **CRLF** 行尾；`if (...)` 复合块内的 `echo` 文本含圆括号必须转义为 `^(` `^)`。
- 仓库 `.gitignore` 已忽略 `build/`、`*.exe`、`*.dll`、音频文件、`.claude/` 等产物，勿提交。
