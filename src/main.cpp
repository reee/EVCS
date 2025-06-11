#include "MainWindow.h"

int WINAPI wWinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, PWSTR /*pCmdLine*/, int nCmdShow) {
    // 设置DPI感知模式 - 兼容Windows 7
    // 使用SetProcessDPIAware替代SetProcessDpiAwarenessContext
    SetProcessDPIAware();
    
    MainWindow mainWindow;
    
    if (!mainWindow.Create()) {
        return 0;
    }
    
    mainWindow.Show(nCmdShow);
    
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return 0;
}
