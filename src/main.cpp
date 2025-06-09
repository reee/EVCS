#include "MainWindow.h"
#include <shellscalingapi.h>

#pragma comment(lib, "shcore.lib")

int WINAPI wWinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, PWSTR /*pCmdLine*/, int nCmdShow) {
    // 设置DPI感知模式 - 支持每显示器DPI感知
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    
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
