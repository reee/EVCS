#include "MainWindow.h"
#include "AudioPlayer.h"

int WINAPI wWinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, PWSTR /*pCmdLine*/, int nCmdShow) {
    // 设置DPI感知模式 - 兼容Windows 7
    // 使用SetProcessDPIAware替代SetProcessDpiAwarenessContext
    SetProcessDPIAware();
    
    // 初始化音频播放器
    if (!AudioPlayer::initialize()) {
        MessageBoxW(NULL, L"音频系统初始化失败", L"错误", MB_OK | MB_ICONERROR);
        return -1;
    }
    
    MainWindow mainWindow;
    
    if (!mainWindow.Create()) {
        AudioPlayer::cleanup();
        return 0;
    }
    
    mainWindow.Show(nCmdShow);
    
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    // 清理音频播放器
    AudioPlayer::cleanup();
    
    return 0;
}
