#include "MainWindow.h"
#include "AudioPlayer.h"

namespace {
// RAII 守卫：考试期间持续阻止系统休眠与熄屏（AGENTS.md 不变量 §2）。
// 构造时设 ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED，
// 析构时还原为 ES_CONTINUOUS（清除所有执行状态请求）。
// 按播放状态启停存在窗口期漏播风险，故整个会话常开。
class PowerStateGuard {
public:
    PowerStateGuard() {
        SetThreadExecutionState(
            ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED);
    }
    ~PowerStateGuard() {
        SetThreadExecutionState(ES_CONTINUOUS);
    }
    PowerStateGuard(const PowerStateGuard&) = delete;
    PowerStateGuard& operator=(const PowerStateGuard&) = delete;
};
}  // namespace

int WINAPI wWinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, PWSTR /*pCmdLine*/, int nCmdShow) {
    // 设置DPI感知模式 - 兼容Windows 7
    // 使用SetProcessDPIAware替代SetProcessDpiAwarenessContext
    SetProcessDPIAware();

    // 阻止系统休眠/熄屏，覆盖整个考试会话（不变量 §2）。
    // 必须先于任何可能阻塞消息泵的逻辑；RAII 保证任何退出路径都还原。
    PowerStateGuard powerGuard;

    // 初始化音频播放器
    if (!AudioPlayer::initialize()) {
        MessageBoxW(NULL, L"音频系统初始化失败", L"错误", MB_OK | MB_ICONERROR);
        return 1;
    }

    MainWindow mainWindow;

    if (!mainWindow.Create()) {
        AudioPlayer::cleanup();
        return 1;
    }

    mainWindow.Show(nCmdShow);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // 清理音频播放器
    AudioPlayer::cleanup();

    return static_cast<int>(msg.wParam);
}
