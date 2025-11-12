#include "MainWindow.h"
#include "AudioPlayer.h"
#include "ConfigManager.h"
#include "resource.h"
#include "version.h"
#include <windowsx.h>
#include <CommCtrl.h>
#include <string>
#include <chrono>
#include <algorithm>
#include <filesystem>

#pragma comment(lib, "comctl32.lib")

// 定义 UNICODE 版本的窗口类名和标题
#ifdef UNICODE
#define WINDOW_CLASS_NAME L"Examination Voice Command System"
#define WINDOW_TITLE L"考试语音指令系统"
#else
#define WINDOW_CLASS_NAME "Examination Voice Command System"
#define WINDOW_TITLE "考试语音指令系统"
#endif

MainWindow::MainWindow() : m_hwnd(NULL), m_hwndStatusBar(NULL), m_hwndStatusPanel(NULL), m_hStatusPanelFont(NULL),
    m_hwndSubjectList(NULL), m_hwndInstructionList(NULL), m_dpi(96), m_dpiScaleX(1.0f), m_dpiScaleY(1.0f),
    m_currentPlayingIndex(-1), m_nextInstructionIndex(-1) {
    // 初始化COM
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    // 初始化通用控件
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_LISTVIEW_CLASSES | ICC_BAR_CLASSES;
    InitCommonControlsEx(&icex);

    // 加载默认配置文件
    auto& configManager = ConfigManager::getInstance();
    if (!configManager.loadDefaultConfig()) {
        // 如果默认配置文件加载失败，显示警告但不阻止程序启动
        // 这里暂时不显示消息框，因为窗口还没有创建
    }
}

MainWindow::~MainWindow() {
    // 清理字体资源
    if (m_hStatusPanelFont) {
        DeleteObject(m_hStatusPanelFont);
    }
    CoUninitialize();
}

LRESULT CALLBACK MainWindow::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    MainWindow* pThis = NULL;
    
    if (uMsg == WM_NCCREATE) {
        CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
        pThis = (MainWindow*)pCreate->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
        
        pThis->m_hwnd = hwnd;
    } else {
        pThis = (MainWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    }
    
    if (pThis) {        switch (uMsg) {            case WM_CREATE:
                pThis->CreateControls();
                pThis->UpdateDpiInfo();
                pThis->UpdateLayoutForDpi();  // 确保控件创建后立即使用正确的DPI布局
                SetTimer(hwnd, TIMER_ID, TIMER_INTERVAL, NULL);
                return 0;
                
            case WM_DESTROY:
                KillTimer(hwnd, TIMER_ID);
                PostQuitMessage(0);
                return 0;            case WM_DPICHANGED: {
                pThis->UpdateDpiInfo();
                
                // 获取新的窗口位置和大小
                RECT* const prcNewWindow = (RECT*)lParam;
                SetWindowPos(hwnd,
                    NULL,
                    prcNewWindow->left,
                    prcNewWindow->top,
                    prcNewWindow->right - prcNewWindow->left,
                    prcNewWindow->bottom - prcNewWindow->top,
                    SWP_NOZORDER | SWP_NOACTIVATE);
                  // 更新布局
                pThis->UpdateLayoutForDpi();
                
                return 0;
            }            case WM_SIZE: {
                pThis->UpdateLayoutForDpi();
                return 0;
            }            case WM_TIMER:
                if (wParam == TIMER_ID) {
                    pThis->UpdateStatusBar();
                    pThis->UpdateStatusPanel();  // 更新状态面板
                    pThis->CheckPlaybackCompletion();  // 检查播放完成状态
                    pThis->UpdateNextInstruction();
                }
                return 0;
                  case WM_NOTIFY: {
                LPNMHDR lpnmh = (LPNMHDR)lParam;
                if (lpnmh->hwndFrom == pThis->m_hwndSubjectList) {
                    pThis->HandleSubjectListNotify(lpnmh);
                } else if (lpnmh->hwndFrom == pThis->m_hwndInstructionList) {
                    return pThis->HandleInstructionListNotify(lpnmh);
                }                return 0;
            }
              case WM_CTLCOLORBTN:
            case WM_CTLCOLORSTATIC: {
                // 使用默认系统背景
                return DefWindowProc(hwnd, uMsg, wParam, lParam);
            }              case WM_COMMAND:
                switch (LOWORD(wParam)) {
                    case IDM_FILE_ADD_SUBJECT:
                        pThis->AddSubject();
                        return 0;
                    case IDM_FILE_LOAD_CONFIG:
                        pThis->LoadConfigFile();
                        return 0;
                    case IDM_FILE_RELOAD_CONFIG:
                        pThis->ReloadConfigFile();
                        return 0;
                    case IDM_DELETE_SUBJECT: {
                        // 获取当前选择的项目
                        int selectedItem = ListView_GetNextItem(pThis->m_hwndSubjectList, -1, LVNI_SELECTED);
                        if (selectedItem >= 0) {
                            pThis->DeleteSubject(selectedItem);
                        }
                        return 0;
                    }
                    case IDM_PLAY_INSTRUCTION: {
                        // 获取当前选择的指令项目
                        int selectedItem = ListView_GetNextItem(pThis->m_hwndInstructionList, -1, LVNI_SELECTED);
                        if (selectedItem >= 0) {
                            pThis->PlayInstruction(selectedItem, true);  // 手动播放
                        }
                        return 0;
                    }
                    case IDM_HELP_HELP: {
                        pThis->ShowHelp();
                        return 0;
                    }
                    case IDM_HELP_ABOUT: {
                        pThis->ShowAbout();
                        return 0;
                    }
                }
                break;
        }
    }
    
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

bool MainWindow::Create() {
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = WINDOW_CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName = MAKEINTRESOURCE(IDR_MAIN_MENU);  // 加载菜单
    
    RegisterClassW(&wc);
    
    // 在创建窗口前获取系统DPI以确定窗口大小
    HDC hdcScreen = GetDC(NULL);
    int systemDpi = 96; // 默认DPI
    if (hdcScreen) {
        systemDpi = GetDeviceCaps(hdcScreen, LOGPIXELSX);
        ReleaseDC(NULL, hdcScreen);
    }
    float dpiScale = systemDpi / 96.0f;
    
    DWORD style = WS_OVERLAPPEDWINDOW;
    // 使用DPI缩放的窗口大小
    int scaledWidth = static_cast<int>(800 * dpiScale);
    int scaledHeight = static_cast<int>(600 * dpiScale);    RECT rc = { 0, 0, scaledWidth, scaledHeight };
    AdjustWindowRect(&rc, style, TRUE);  // 传入TRUE表示有菜单
    
    m_hwnd = CreateWindowExW(
        0,                              // 扩展样式
        WINDOW_CLASS_NAME,              // 窗口类名
        WINDOW_TITLE,                   // 窗口标题
        style,                          // 窗口样式
        CW_USEDEFAULT, CW_USEDEFAULT,   // 位置
        rc.right - rc.left,            // 宽度
        rc.bottom - rc.top,            // 高度
        NULL,                          // 父窗口
        NULL,                          // 菜单
        GetModuleHandle(NULL),         // 实例句柄
        this                           // 额外参数
    );
      // 窗口创建后，初始化DPI信息
    if (m_hwnd) {
        UpdateDpiInfo();
    }
    
    return (m_hwnd != NULL);
}

void MainWindow::Show(int nCmdShow) {
    ShowWindow(m_hwnd, nCmdShow);
    UpdateWindow(m_hwnd);
}

void MainWindow::CreateControls() {
    // 创建状态栏
    m_hwndStatusBar = CreateWindowExW(
        0,                  // 扩展样式
        STATUSCLASSNAMEW,   // 类名
        NULL,              // 窗口文本
        WS_CHILD | WS_VISIBLE,  // 样式
        0, 0, 0, 0,       // 位置和大小
        m_hwnd,           // 父窗口
        NULL,             // 菜单
        GetModuleHandle(NULL),  // 实例句柄
        NULL              // 额外参数
    );    // 设置状态栏的分区 - 创建3个分区：系统音量，音频文件状态，当前时间
    if (m_hwndStatusBar) {
        // 使用DPI缩放的状态栏分区宽度
        // Part 0: Volume, Width: ScaleX(120) -> Right edge: ScaleX(120)
        // Part 1: Audio File Status, Width: ScaleX(250) -> Right edge: ScaleX(120 + 250) = ScaleX(370)
        // Part 2: Current Time, Width: -1 (remaining)
        int statusWidths[] = { ScaleX(120), ScaleX(370), -1 };
        SendMessage(m_hwndStatusBar, SB_SETPARTS, 3, (LPARAM)statusWidths);
    }    // 创建状态面板 - 显示下一指令信息
    m_hwndStatusPanel = CreateWindowExW(
        WS_EX_CLIENTEDGE,    // 扩展样式：添加凹陷边框
        L"STATIC",           // 类名
        L"下一指令: 无",       // 初始文本
        WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE,  // 样式：使用SS_CENTER让文字居中
        ScaleX(10), ScaleY(10), ScaleX(780), ScaleY(50),   // 位置和大小：高度从30增加到50
        m_hwnd,              // 父窗口
        (HMENU)IDC_STATUS_PANEL,  // 控件ID
        GetModuleHandle(NULL),    // 实例句柄
        NULL                 // 额外参数
    );    // 为状态面板设置更大的字体
    if (m_hwndStatusPanel) {
        m_hStatusPanelFont = CreateFontW(
            ScaleY(16),         // 字体高度（使用DPI缩放）
            0,                  // 字体宽度（0表示自动）
            0,                  // 文本角度
            0,                  // 基线角度
            FW_NORMAL,          // 字体粗细
            FALSE,              // 是否斜体
            FALSE,              // 是否下划线
            FALSE,              // 是否删除线
            DEFAULT_CHARSET,    // 字符集
            OUT_DEFAULT_PRECIS, // 输出精度
            CLIP_DEFAULT_PRECIS,// 裁剪精度
            DEFAULT_QUALITY,    // 输出质量
            DEFAULT_PITCH | FF_DONTCARE, // 字体间距和族
            NULL                // 使用系统默认字体
        );
        if (m_hStatusPanelFont) {
            SendMessage(m_hwndStatusPanel, WM_SETFONT, (WPARAM)m_hStatusPanelFont, TRUE);
        }
    }

    // 创建科目列表 - 使用DPI缩放，添加边框
    m_hwndSubjectList = CreateWindowExW(
        WS_EX_CLIENTEDGE,    // 扩展样式：添加凹陷边框
        WC_LISTVIEWW,       // 类名
        NULL,               // 窗口文本
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SINGLESEL,  // 样式
        ScaleX(10), ScaleY(70), ScaleX(780), ScaleY(200),   // 位置调整：Y从50改为70，为更高的状态面板留出空间
        m_hwnd,             // 父窗口
        (HMENU)IDC_SUBJECT_LIST,  // 控件ID
        GetModuleHandle(NULL),    // 实例句柄
        NULL                // 额外参数
    );
    
    // 启用完整行选择模式和网格线，添加边框效果
    ListView_SetExtendedListViewStyle(m_hwndSubjectList, 
        LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_BORDERSELECT);    // 添加科目列表的列 - 调整列宽以适应实际空间 (总宽约750px)
    LVCOLUMN lvc = {0};
    lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

    lvc.iSubItem = 0;
    lvc.pszText = (LPWSTR)L"科目";
    lvc.cx = ScaleX(240);  // 调整科目列宽度为240px
    ListView_InsertColumn(m_hwndSubjectList, 0, &lvc);

    lvc.iSubItem = 1;
    lvc.pszText = (LPWSTR)L"开始时间";
    lvc.cx = ScaleX(250);  // 调整开始时间列宽度为250px
    ListView_InsertColumn(m_hwndSubjectList, 1, &lvc);        lvc.iSubItem = 2;
    lvc.pszText = (LPWSTR)L"结束时间";
    lvc.cx = ScaleX(250);  // 调整结束时间列宽度为250px
    ListView_InsertColumn(m_hwndSubjectList, 2, &lvc);
        // 创建指令列表 - 使用DPI缩放，添加边框
    m_hwndInstructionList = CreateWindowExW(
        WS_EX_CLIENTEDGE,    // 扩展样式：添加凹陷边框
        WC_LISTVIEWW,       // 类名
        NULL,               // 窗口文本
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS,  // 样式
        ScaleX(10), ScaleY(260), ScaleX(780), ScaleY(280),  // 位置和大小
        m_hwnd,             // 父窗口
        (HMENU)IDC_INSTRUCTION_LIST,  // 控件ID
        GetModuleHandle(NULL),    // 实例句柄
        NULL                // 额外参数
    );
    
    // 启用完整行选择模式和网格线，添加边框效果
    ListView_SetExtendedListViewStyle(m_hwndInstructionList, 
        LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_BORDERSELECT);
    
    // 添加指令列表的列 - 使用DPI缩放的列宽
    lvc.iSubItem = 0;
    lvc.pszText = (LPWSTR)L"科目";
    lvc.cx = ScaleX(120);
    ListView_InsertColumn(m_hwndInstructionList, 0, &lvc);    lvc.iSubItem = 1;
    lvc.pszText = (LPWSTR)L"指令";
    lvc.cx = ScaleX(260);  // 指令列进一步加宽到280px，最大化利用窗口空间
    ListView_InsertColumn(m_hwndInstructionList, 1, &lvc);

    lvc.iSubItem = 2;
    lvc.pszText = (LPWSTR)L"播放时间";
    lvc.cx = ScaleX(240);  // 播放时间列加宽到240px (原100px)
    ListView_InsertColumn(m_hwndInstructionList, 2, &lvc);
      lvc.iSubItem = 3;
    lvc.pszText = (LPWSTR)L"状态";    lvc.cx = ScaleX(100);  // 状态列加宽到100px (原80px)
    ListView_InsertColumn(m_hwndInstructionList, 3, &lvc);
}

void MainWindow::AddSubject() {
    INT_PTR result = DialogBoxParamW(GetModuleHandle(NULL), 
                   MAKEINTRESOURCEW(IDD_ADD_SUBJECT), 
                   m_hwnd, 
                   AddSubjectDialogProc,
                   reinterpret_cast<LPARAM>(this));
                     // 检查对话框返回值
    if (result == -1) {
        DWORD error = GetLastError();
        wchar_t errorMsg[256];
        swprintf_s(errorMsg, _countof(errorMsg), L"对话框创建失败，错误代码: %d", error);
        MessageBoxW(m_hwnd, errorMsg, L"错误", MB_OK | MB_ICONERROR);
    }
}

void MainWindow::DeleteSubject(int index) {
    if (index < 0 || static_cast<size_t>(index) >= m_subjects.size()) {
        return;  // 无效索引
    }
    
    auto& subject = m_subjects[index];
    int subjectIdToDelete = subject.id;  // 获取要删除的科目ID
    
    // 删除相关的指令 - 使用科目ID而不是科目名称
    m_instructions.erase(
        std::remove_if(m_instructions.begin(), m_instructions.end(),
            [subjectIdToDelete](const Instruction& instr) {
                return instr.subjectId == subjectIdToDelete;
            }),
        m_instructions.end()
    );
    
    // 删除科目
    m_subjects.erase(m_subjects.begin() + index);
      // 更新列表
    UpdateSubjectList();
    UpdateInstructionList();
    UpdateStatusPanel();  // 更新状态面板
}

// 静态辅助函数：UTF-8字符串转换为宽字符串
std::wstring MainWindow::ConvertUtf8ToWide(const std::string& utf8Str) {
    if (utf8Str.empty()) {
        return std::wstring();
    }

    int size_needed = MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, NULL, 0);
    if (size_needed <= 0) {
        return std::wstring();
    }

    std::wstring result(size_needed - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, &result[0], size_needed);
    return result;
}

void MainWindow::UpdateStatusBar() {
    // 获取当前时间
    SYSTEMTIME st;
    GetLocalTime(&st);
    wchar_t currentTimeText[64];
    swprintf_s(currentTimeText, _countof(currentTimeText), 
        L"当前时间: %02d:%02d:%02d", st.wHour, st.wMinute, st.wSecond);
      // 获取系统音量
    int volume = AudioPlayer::getSystemVolume();
    wchar_t volumeText[64];
    swprintf_s(volumeText, _countof(volumeText), L"系统音量: %d%%", volume);

    // 分别检查指令文件和听力文件状态
    wchar_t audioFileStatusText[512] = L"音频文件: 无指令";
    
    if (!m_instructions.empty()) {
        // 检查指令文件状态
        int missingInstructionCount = 0;
        for (const auto& instruction : m_instructions) {
            if (!instruction.checkAudioFileExists()) {
                missingInstructionCount++;
            }
        }
        
        // 检查听力文件(tl.mp3)状态
        char exePath[MAX_PATH];
        GetModuleFileNameA(NULL, exePath, MAX_PATH);
        std::filesystem::path exeDir = std::filesystem::path(exePath).parent_path();
        bool listeningFileExists = std::filesystem::exists(exeDir / "audio" / "tl.mp3");
        
        // 根据检查结果生成状态文本
        if (missingInstructionCount == 0 && listeningFileExists) {
            swprintf_s(audioFileStatusText, _countof(audioFileStatusText), L"音频文件: 全部存在, 听力文件存在");
        } else if (missingInstructionCount == 0 && !listeningFileExists) {
            swprintf_s(audioFileStatusText, _countof(audioFileStatusText), L"音频文件: 全部存在, 听力文件缺失");
        } else if (missingInstructionCount > 0 && listeningFileExists) {
            swprintf_s(audioFileStatusText, _countof(audioFileStatusText), L"音频文件: 缺失%d个, 听力文件存在", missingInstructionCount);
        } else {
            swprintf_s(audioFileStatusText, _countof(audioFileStatusText), L"音频文件: 缺失%d个, 听力文件缺失", missingInstructionCount);
        }
    } else {
        // 如果没有指令，只检查听力文件
        char exePath[MAX_PATH];
        GetModuleFileNameA(NULL, exePath, MAX_PATH);
        std::filesystem::path exeDir = std::filesystem::path(exePath).parent_path();
        bool listeningFileExists = std::filesystem::exists(exeDir / "audio" / "tl.mp3");
        if (listeningFileExists) {
            swprintf_s(audioFileStatusText, _countof(audioFileStatusText), L"音频文件: 无指令, 听力文件存在");
        } else {
            swprintf_s(audioFileStatusText, _countof(audioFileStatusText), L"音频文件: 无指令, 听力文件缺失");
        }
    }
    
    // 设置状态栏的各个分区文本
    SendMessage(m_hwndStatusBar, SB_SETTEXT, 0, (LPARAM)volumeText);           // 分区0: 系统音量
    SendMessage(m_hwndStatusBar, SB_SETTEXT, 1, (LPARAM)audioFileStatusText); // 分区1: 音频文件状态
    SendMessage(m_hwndStatusBar, SB_SETTEXT, 2, (LPARAM)currentTimeText);      // 分区2: 当前时间
}

void MainWindow::UpdateStatusPanel() {
    wchar_t statusText[512] = L"";
    bool hasCurrentInstruction = false;
    
    // 第一部分：检查是否有当前播放的指令
    if (m_currentPlayingIndex >= 0 && 
        static_cast<size_t>(m_currentPlayingIndex) < m_instructions.size()) {
        
        const auto& currentInstruction = m_instructions[m_currentPlayingIndex];
        
        if (currentInstruction.status == PlaybackStatus::PLAYING) {
            // 计算已播放时长和总时长
            auto now = std::chrono::system_clock::now();
            auto playedDuration = std::chrono::duration_cast<std::chrono::seconds>(
                now - m_currentPlayingStartTime).count();
            
            // 获取音频文件总时长
            double totalDuration = AudioPlayer::getAudioDuration(currentInstruction.audioFile);
            int totalSeconds = static_cast<int>(totalDuration);
            int remainingSeconds = (totalSeconds - static_cast<int>(playedDuration)) > 0 ? 
                                 (totalSeconds - static_cast<int>(playedDuration)) : 0;
            
            swprintf_s(statusText, _countof(statusText), 
                L"当前指令: %s (剩余 %d秒 / 总计 %d秒)", 
                ConvertUtf8ToWide(currentInstruction.name).c_str(),
                remainingSeconds,
                totalSeconds);
            
            hasCurrentInstruction = true;
        }
    }
    
    // 第二部分：如果没有当前指令，则显示下一指令信息
    if (!hasCurrentInstruction) {
        wcscpy_s(statusText, _countof(statusText), L"下一指令: 无");
        
        if (m_nextInstructionIndex >= 0 && 
            static_cast<size_t>(m_nextInstructionIndex) < m_instructions.size()) {
            
            const auto& instruction = m_instructions[m_nextInstructionIndex];
            
            // 只有未播放的指令才显示倒计时
            if (instruction.status == PlaybackStatus::UNPLAYED) {
                auto now = std::chrono::system_clock::now();
                auto nowTime = std::chrono::system_clock::to_time_t(now);
                auto instrTime = std::chrono::system_clock::to_time_t(instruction.playTime);
                
                int timeDiffMinutes = static_cast<int>((instrTime - nowTime) / 60);
                
                if (timeDiffMinutes > 0) {
                    // 距离播放时间还有分钟数
                    swprintf_s(statusText, _countof(statusText), 
                        L"下一指令: %s (%d分钟后)", 
                        ConvertUtf8ToWide(instruction.name).c_str(), timeDiffMinutes);
                } else if (timeDiffMinutes == 0) {
                    // 即将播放
                    swprintf_s(statusText, _countof(statusText), 
                        L"下一指令: %s (即将播放)", 
                        ConvertUtf8ToWide(instruction.name).c_str());
                } else {
                    // 已到播放时间
                    swprintf_s(statusText, _countof(statusText), 
                        L"下一指令: %s (播放时间已到)", 
                        ConvertUtf8ToWide(instruction.name).c_str());
                }
            }
        }
    }
    
    // 更新状态面板文本
    if (m_hwndStatusPanel) {
        SetWindowTextW(m_hwndStatusPanel, statusText);
    }
}

void MainWindow::UpdateSubjectList() {
    // 清空现有项目
    ListView_DeleteAllItems(m_hwndSubjectList);
    
    // 如果没有科目，直接返回
    if (m_subjects.empty()) {
        return;
    }
    
    // 预分配ListView项目数量以提升性能
    ListView_SetItemCount(m_hwndSubjectList, static_cast<int>(m_subjects.size()));
    
    for (size_t i = 0; i < m_subjects.size(); ++i) {
        const auto& subject = m_subjects[i];
        
        try {
            // 转换字符串
            std::wstring subjectName = ConvertUtf8ToWide(subject.name);
            std::wstring startTime = ConvertUtf8ToWide(subject.getStartDateTimeString());
            std::wstring endTime = ConvertUtf8ToWide(subject.getEndDateTimeString());
            
            // 创建ListView项目
            LVITEM lvi = {0};
            lvi.mask = LVIF_TEXT;
            lvi.iItem = static_cast<int>(i);
            lvi.iSubItem = 0;
            lvi.pszText = const_cast<LPWSTR>(subjectName.c_str());
            
            // 插入主项目
            int itemIndex = ListView_InsertItem(m_hwndSubjectList, &lvi);
            if (itemIndex != -1) {
                // 设置子项目文本
                ListView_SetItemText(m_hwndSubjectList, itemIndex, 1, 
                                   const_cast<LPWSTR>(startTime.c_str()));
                ListView_SetItemText(m_hwndSubjectList, itemIndex, 2, 
                                   const_cast<LPWSTR>(endTime.c_str()));
            }
        }
        catch (const std::exception& e) {
            // 记录错误但继续处理其他项目
            OutputDebugStringA("UpdateSubjectList error: ");
            OutputDebugStringA(e.what());
            OutputDebugStringA("\n");
            continue;
        }
        catch (...) {
            // 处理其他异常
            OutputDebugStringA("UpdateSubjectList unknown error\n");
            continue;
        }
    }
}

void MainWindow::UpdateInstructionList() {
    // 清空现有项目
    ListView_DeleteAllItems(m_hwndInstructionList);

    // 重置下一指令索引，因为列表已更新
    m_nextInstructionIndex = -1;

    // 如果没有指令，直接返回
    if (m_instructions.empty()) {
        return;
    }

    // 预分配ListView项目数量以提升性能
    ListView_SetItemCount(m_hwndInstructionList, static_cast<int>(m_instructions.size()));

    for (size_t i = 0; i < m_instructions.size(); ++i) {
        const auto& instruction = m_instructions[i];

        try {
            // 转换字符串
            std::wstring subjectName = ConvertUtf8ToWide(instruction.subjectName);
            std::wstring instrName = ConvertUtf8ToWide(instruction.name);
            std::wstring playTime = ConvertUtf8ToWide(instruction.getPlayDateTimeString());
            std::wstring status = ConvertUtf8ToWide(instruction.getStatusString());

            // 创建ListView项目
            LVITEM lvi = {0};
            lvi.mask = LVIF_TEXT;
            lvi.iItem = static_cast<int>(i);
            lvi.iSubItem = 0;
            lvi.pszText = const_cast<LPWSTR>(subjectName.c_str());

            // 插入主项目
            int itemIndex = ListView_InsertItem(m_hwndInstructionList, &lvi);
            if (itemIndex != -1) {
                // 设置子项目文本
                ListView_SetItemText(m_hwndInstructionList, itemIndex, 1,
                                   const_cast<LPWSTR>(instrName.c_str()));
                ListView_SetItemText(m_hwndInstructionList, itemIndex, 2,
                                   const_cast<LPWSTR>(playTime.c_str()));
                ListView_SetItemText(m_hwndInstructionList, itemIndex, 3,
                                   const_cast<LPWSTR>(status.c_str()));
            }
        }
        catch (const std::exception& e) {
            // 记录错误但继续处理其他项目
            OutputDebugStringA("UpdateInstructionList error: ");
            OutputDebugStringA(e.what());
            OutputDebugStringA("\n");
            continue;
        }
        catch (...) {
            // 处理其他异常
            OutputDebugStringA("UpdateInstructionList unknown error\n");
            continue;
        }
    }

    // 确保焦点跟随当前播放/即将播放的指令
    EnsureInstructionListFocus();
}

void MainWindow::UpdateNextInstruction() {
    // 如果当前正在播放指令，不查找新的指令
    if (m_currentPlayingIndex >= 0 && 
        static_cast<size_t>(m_currentPlayingIndex) < m_instructions.size() &&
        m_instructions[m_currentPlayingIndex].status == PlaybackStatus::PLAYING) {
        return;  // 当前有指令正在播放，等待播放完成
    }
      // 检查并标记过期超过60秒的指令为跳过
    auto now = std::chrono::system_clock::now();
    auto nowTimestamp = std::chrono::duration_cast<std::chrono::seconds>(
        now.time_since_epoch()).count();
    
    bool hasExpiredInstructions = false;
    for (auto& instruction : m_instructions) {
        if (instruction.status == PlaybackStatus::UNPLAYED) {
            auto instructionTimestamp = std::chrono::duration_cast<std::chrono::seconds>(
                instruction.playTime.time_since_epoch()).count();
            
            // 只检查过去的指令是否过期
            // 如果指令时间已过且过期超过60秒，标记为跳过
            if (instructionTimestamp < nowTimestamp && 
                (nowTimestamp - instructionTimestamp) > 60) {
                instruction.status = PlaybackStatus::SKIPPED;
                hasExpiredInstructions = true;
            }
        }
    }
    
    // 如果有指令被标记为跳过，更新显示
    if (hasExpiredInstructions) {
        UpdateInstructionListDisplay();
    }
    
    // 检查下一指令是否有效且仍未播放
    if (m_nextInstructionIndex < 0 || 
        static_cast<size_t>(m_nextInstructionIndex) >= m_instructions.size() ||
        m_instructions[m_nextInstructionIndex].status != PlaybackStatus::UNPLAYED) {
        // 重新设置下一指令
        SetNextInstruction();
    }
    
    // 检查是否到时间播放下一指令
    if (m_nextInstructionIndex >= 0 && IsTimeToPlayNextInstruction()) {
        // 自动播放下一指令
        PlayInstruction(m_nextInstructionIndex, false);  // 自动播放，不跳过之前的指令
        // 注意：PlayInstruction 方法会自动设置下一指令
    }
    // 如果没有到播放时间，确保焦点显示在下一指令上
    else if (m_nextInstructionIndex >= 0) {
        EnsureInstructionListFocus();
    }
}

void MainWindow::HandleSubjectListNotify(LPNMHDR lpnmh) {
    switch (lpnmh->code) {
        case NM_RCLICK: {
            // 处理右键点击
            LPNMITEMACTIVATE lpnmitem = (LPNMITEMACTIVATE)lpnmh;
            if (lpnmitem->iItem >= 0 && static_cast<size_t>(lpnmitem->iItem) < m_subjects.size()) {
                // 选择被右键点击的项目
                ListView_SetItemState(m_hwndSubjectList, lpnmitem->iItem, 
                    LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
                
                // 显示右键菜单
                POINT pt;
                GetCursorPos(&pt);
                ShowSubjectContextMenu(pt.x, pt.y, lpnmitem->iItem);
            }
            break;
        }
        
        case LVN_ITEMCHANGED: {
            // 处理选择变化
            LPNMLISTVIEW pnmv = (LPNMLISTVIEW)lpnmh;
            if ((pnmv->uChanged & LVIF_STATE) && (pnmv->uNewState & LVIS_SELECTED)) {
                // 科目选择发生变化，更新指令列表
                UpdateInstructionList();
            }
            break;
        }
    }
}

LRESULT MainWindow::HandleInstructionListNotify(LPNMHDR lpnmh) {
    switch (lpnmh->code) {
        case NM_CUSTOMDRAW: {
            // 处理自定义绘制消息以设置文字颜色
            LPNMLVCUSTOMDRAW lpCustomDraw = (LPNMLVCUSTOMDRAW)lpnmh;
            
            switch (lpCustomDraw->nmcd.dwDrawStage) {
                case CDDS_PREPAINT:
                    // 请求子项绘制通知
                    return CDRF_NOTIFYITEMDRAW;
                      case CDDS_ITEMPREPAINT: {
                    // 为每个项目设置文字颜色
                    int itemIndex = (int)lpCustomDraw->nmcd.dwItemSpec;
                    if (itemIndex >= 0 && static_cast<size_t>(itemIndex) < m_instructions.size()) {
                        // 获取指令状态对应的文字颜色
                        COLORREF textColor = m_instructions[itemIndex].getStatusTextColor();
                        lpCustomDraw->clrText = textColor;
                    }
                    return CDRF_NEWFONT;
                }
                
                default:
                    return CDRF_DODEFAULT;
            }
        }
        
        case NM_DBLCLK: {
            LPNMITEMACTIVATE lpnmitem = (LPNMITEMACTIVATE)lpnmh;
            if (lpnmitem->iItem >= 0 && static_cast<size_t>(lpnmitem->iItem) < m_instructions.size()) {
                // 使用新的播放方法，支持状态跟踪
                PlayInstruction(lpnmitem->iItem, true);  // 手动播放
            }
            break;
        }
        
        case NM_RCLICK: {
            // 处理右键点击
            LPNMITEMACTIVATE lpnmitem = (LPNMITEMACTIVATE)lpnmh;
            if (lpnmitem->iItem >= 0 && static_cast<size_t>(lpnmitem->iItem) < m_instructions.size()) {
                // 选择被右键点击的项目
                ListView_SetItemState(m_hwndInstructionList, lpnmitem->iItem, 
                    LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
                
                // 显示右键菜单
                POINT pt;
                GetCursorPos(&pt);
                ShowInstructionContextMenu(pt.x, pt.y, lpnmitem->iItem);
            }
            break;
        }
    }
    
    return 0;
}

void MainWindow::ShowSubjectContextMenu(int x, int y, int itemIndex) {
    // 创建右键菜单
    HMENU hMenu = CreatePopupMenu();
    if (hMenu) {
        // 添加删除菜单项
        AppendMenuW(hMenu, MF_STRING, IDM_DELETE_SUBJECT, L"删除科目");
        
        // 显示菜单并获取用户选择
        int command = TrackPopupMenu(hMenu, 
            TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD,
            x, y, 0, m_hwnd, NULL);
        
        // 处理菜单选择
        if (command == IDM_DELETE_SUBJECT) {
            // 确认删除
            auto& subject = m_subjects[itemIndex];
            
            // 转换科目名称用于显示
            std::wstring subjectName;
            if (!subject.name.empty()) {
                int size_needed = MultiByteToWideChar(CP_UTF8, 0, subject.name.c_str(), -1, NULL, 0);
                if (size_needed > 0) {
                    subjectName.resize(size_needed - 1);
                    MultiByteToWideChar(CP_UTF8, 0, subject.name.c_str(), -1, &subjectName[0], size_needed);
                }
            }
            
            wchar_t confirmMsg[512];
            swprintf_s(confirmMsg, _countof(confirmMsg), 
                L"确定要删除科目 \"%s\" 及其所有相关指令吗？", 
                subjectName.c_str());
                
            int result = MessageBoxW(m_hwnd, confirmMsg, L"确认删除", 
                MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);
                
            if (result == IDYES) {
                DeleteSubject(itemIndex);
            }
        }
        
        // 销毁菜单
        DestroyMenu(hMenu);
    }
}

INT_PTR CALLBACK MainWindow::AddSubjectDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    // 这个静态数组已不再使用，科目列表现在从ConfigManager动态获取
    // 移除所有对subjects静态数组的使用
      MainWindow* pMainWindow = nullptr;
    if (msg == WM_INITDIALOG) {
        pMainWindow = reinterpret_cast<MainWindow*>(lParam);
        if (!pMainWindow) {
            EndDialog(hwnd, IDCANCEL);
            return FALSE;
        }
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pMainWindow));
        
        // Windows 7兼容：移除SetDialogDpiChangeBehavior调用
        
    } else {
        pMainWindow = reinterpret_cast<MainWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        // 只在需要 pMainWindow 的消息中检查空指针
        if (!pMainWindow && (msg == WM_COMMAND || msg == WM_NOTIFY)) {
            return FALSE;
        }
    }

    switch (msg) {        case WM_INITDIALOG: {
            // 初始化科目下拉列表，添加时长信息
            HWND hComboBox = GetDlgItem(hwnd, IDC_SUBJECT_COMBO);
            auto& configManager = ConfigManager::getInstance();
            auto subjects = configManager.getSubjects();
            for (const auto& subject : subjects) {
                wchar_t displayText[128];
                std::wstring wideName = ConvertUtf8ToWide(subject.name);
                swprintf_s(displayText, _countof(displayText),
                    L"%s (%d分钟)", wideName.c_str(), subject.durationMinutes);
                SendMessageW(hComboBox, CB_ADDSTRING, 0, (LPARAM)displayText);
            }            SendMessageW(hComboBox, CB_SETCURSEL, 0, 0);
              // 设置默认日期为今天
            SYSTEMTIME st;
            GetLocalTime(&st);
            
            wchar_t dateStr[12];
            swprintf_s(dateStr, _countof(dateStr), L"%04d-%02d-%02d", st.wYear, st.wMonth, st.wDay);
            SetDlgItemText(hwnd, IDC_START_DATE_EDIT, dateStr);
            
            // 设置默认时间为当前时间的下一个整点
            st.wMinute = 0;
            st.wHour = (st.wHour + 1) % 24;
            
            wchar_t timeStr[10];
            swprintf_s(timeStr, _countof(timeStr), L"%02d:%02d", st.wHour, st.wMinute);
            SetDlgItemText(hwnd, IDC_START_TIME_EDIT, timeStr);
            
            return TRUE;
        }
        
        case WM_COMMAND: {
            switch (LOWORD(wParam)) {                case IDOK: {
                    wchar_t subjectName[256] = {0};
                    wchar_t startDate[12] = {0};
                    wchar_t startTime[6] = {0};

                    // 获取选中的科目
                    HWND hComboBox = GetDlgItem(hwnd, IDC_SUBJECT_COMBO);
                    int selectedIndex = static_cast<int>(SendMessageW(hComboBox, CB_GETCURSEL, 0, 0));

                    // 获取科目列表来验证索引
                    auto& configManager = ConfigManager::getInstance();
                    auto subjects = configManager.getSubjects();

                    if (selectedIndex == CB_ERR || selectedIndex >= subjects.size()) {
                        MessageBoxW(hwnd, L"请选择科目", L"错误", MB_OK | MB_ICONERROR);
                        return TRUE;
                    }

                    // 从配置管理器获取科目名称并转换
                    std::wstring wideSubjectName = ConvertUtf8ToWide(subjects[selectedIndex].name);
                    wcscpy_s(subjectName, wideSubjectName.c_str());
                    
                    // 获取考试日期
                    if (GetDlgItemTextW(hwnd, IDC_START_DATE_EDIT, startDate, 12) == 0) {
                        MessageBoxW(hwnd, L"请输入考试日期", L"错误", MB_OK | MB_ICONERROR);
                        return TRUE;
                    }
                    
                    // 获取开始时间
                    if (GetDlgItemTextW(hwnd, IDC_START_TIME_EDIT, startTime, 6) == 0) {
                        MessageBoxW(hwnd, L"请输入开始时间", L"错误", MB_OK | MB_ICONERROR);
                        return TRUE;
                    }                    try {
                        // 使用Windows API进行字符转换 - 增加缓冲区大小以支持中文字符
                        char mbSubjectName[768] = {0};  // 增加缓冲区大小，中文字符可能需要更多字节
                        char mbStartDate[32] = {0};     // 日期缓冲区
                        char mbStartTime[16] = {0};     // 增加时间缓冲区大小
                        
                        // 进行字符转换，并检查转换结果
                        int subjectResult = WideCharToMultiByte(CP_UTF8, 0, subjectName, -1, 
                                          mbSubjectName, sizeof(mbSubjectName), NULL, NULL);
                        int dateResult = WideCharToMultiByte(CP_UTF8, 0, startDate, -1, 
                                          mbStartDate, sizeof(mbStartDate), NULL, NULL);
                        int timeResult = WideCharToMultiByte(CP_UTF8, 0, startTime, -1, 
                                          mbStartTime, sizeof(mbStartTime), NULL, NULL);
                        
                        if (subjectResult == 0 || dateResult == 0 || timeResult == 0) {
                            MessageBoxW(hwnd, L"字符转换失败", L"错误", MB_OK | MB_ICONERROR);
                            return TRUE;
                        }
                          std::string name(mbSubjectName);
                        std::string dateStr(mbStartDate);
                        std::string timeStr(mbStartTime);
                        
                        if (!Subject::isValidDateTime(dateStr, timeStr)) {
                            MessageBoxW(hwnd, L"请输入正确的日期格式（YYYY-MM-DD）和时间格式（HH:MM）", L"错误", MB_OK | MB_ICONERROR);
                            return TRUE;
                        }
                        
                        Subject subject = Subject::createSubject(name);
                        subject.setStartDateTime(dateStr, timeStr);
                        
                        pMainWindow->m_subjects.push_back(subject);
                        pMainWindow->UpdateSubjectList();
                        
                        // 生成并更新指令列表
                        auto instructions = Instruction::generateInstructions(subject);
                          pMainWindow->m_instructions.insert(
                            pMainWindow->m_instructions.end(),
                            instructions.begin(),
                            instructions.end()
                        );
                        pMainWindow->UpdateInstructionList();
                        pMainWindow->UpdateStatusPanel();  // 更新状态面板
                        
                        EndDialog(hwnd, IDOK);
                        return TRUE;
                    }                    catch (const std::exception& e) {
                        // 显示具体的异常信息 - 使用更安全的字符串转换
                        std::string error = e.what();
                        std::wstring wError;
                        
                        if (!error.empty()) {
                            int size_needed = MultiByteToWideChar(CP_UTF8, 0, error.c_str(), -1, NULL, 0);
                            if (size_needed > 0) {
                                wError.resize(size_needed - 1);
                                MultiByteToWideChar(CP_UTF8, 0, error.c_str(), -1, &wError[0], size_needed);
                            }
                        }
                        
                        wchar_t errorMsg[512];
                        swprintf_s(errorMsg, _countof(errorMsg), L"添加科目时发生错误: %s", wError.c_str());
                        MessageBoxW(hwnd, errorMsg, L"错误", MB_OK | MB_ICONERROR);
                        return TRUE;
                    }
                    catch (...) {
                        MessageBoxW(hwnd, L"添加科目时发生未知错误", L"错误", MB_OK | MB_ICONERROR);
                        return TRUE;
                    }
                }
                
                case IDCANCEL:
                    EndDialog(hwnd, IDCANCEL);
                    return TRUE;
            }
            break;
        }
    }
    
    return FALSE;
}

// 菜单相关方法实现
void MainWindow::ShowHelp() {
    const wchar_t* helpText = 
        L"考试语音指令系统 - 帮助\n\n"
        L"详细使用说明请查看程序目录下的 README.txt 文件。\n\n"
        L"重要提醒：\n"
        L"• 请确保程序目录下的 audio 文件夹包含所需的音频文件\n"
        L"• 英语科目需要自行准备听力音频文件（sy.mp3 和 tl.mp3）\n"
        L"• 音频文件格式支持：WAV、MP3\n\n"
        L"基本操作：\n"
        L"• 添加科目：文件菜单 -> 添加科目\n"
        L"• 删除科目：右键点击科目列表项目\n"
        L"• 播放指令：双击指令或右键立即播放\n"
        L"• 自动播放：系统根据设定时间自动播放\n\n"
        L"如需更多帮助，请参阅 README.txt 文档。";
    
    MessageBoxW(m_hwnd, helpText, L"帮助", MB_OK | MB_ICONINFORMATION);
}

void MainWindow::ShowAbout() {
    const wchar_t* aboutText = 
        L"" EVCS_PRODUCT_NAME L"\n"
        L"" EVCS_PRODUCT_NAME_EN L" (EVCS)\n\n"
        L"版本：" EVCS_VERSION_STRING L"\n"
        L"构建日期：" EVCS_BUILD_DATE L"\n\n"
        L"项目地址：\n"
        L"https://github.com/reee/EVCS/\n\n"
        L"功能特性：\n"
        L"• 自动语音指令播放\n"
        L"• 多种考试科目支持\n"
        L"• DPI缩放适配\n"
        L"• Windows 7+ 兼容\n"
        L"• 实时状态监控\n\n"
        L"适用于标准化考试的语音指令播放管理。\n"
        L"支持音频文件：WAV、MP3等格式";
    
    MessageBoxW(m_hwnd, aboutText, L"关于", MB_OK | MB_ICONINFORMATION);
}

// DPI相关函数实现
void MainWindow::UpdateDpiInfo() {
    // 使用GetDeviceCaps替代GetDpiForWindow以兼容Windows 7
    HDC hdc = GetDC(m_hwnd);
    if (hdc) {
        m_dpi = GetDeviceCaps(hdc, LOGPIXELSX);
        ReleaseDC(m_hwnd, hdc);
    } else {
        m_dpi = 96; // 默认DPI
    }
    m_dpiScaleX = m_dpi / 96.0f;
    m_dpiScaleY = m_dpi / 96.0f;
    
    // 调试输出DPI信息
    wchar_t debugMsg[256];
    swprintf_s(debugMsg, _countof(debugMsg), 
        L"DPI: %d, ScaleX: %.2f, ScaleY: %.2f", m_dpi, m_dpiScaleX, m_dpiScaleY);
    OutputDebugStringW(debugMsg);
    OutputDebugStringW(L"\n");
}

int MainWindow::ScaleX(int x) const {
    return static_cast<int>(x * m_dpiScaleX);
}

int MainWindow::ScaleY(int y) const {
    return static_cast<int>(y * m_dpiScaleY);
}

void MainWindow::UpdateLayoutForDpi() {
    // 获取客户区大小
    RECT rcClient;
    GetClientRect(m_hwnd, &rcClient);
    
    // 获取状态栏高度
    RECT rcStatus;
    GetWindowRect(m_hwndStatusBar, &rcStatus);
    int statusHeight = rcStatus.bottom - rcStatus.top;    // 重新设置状态栏分区宽度（使用DPI缩放）
    if (m_hwndStatusBar) {
        // Part 0: Volume, Width: ScaleX(120) -> Right edge: ScaleX(120)
        // Part 1: Audio File Status, Width: ScaleX(250) -> Right edge: ScaleX(120 + 250) = ScaleX(370)
        // Part 2: Current Time, Width: -1 (remaining)
        int statusWidths[] = { ScaleX(250), ScaleX(500), -1 };
        SendMessage(m_hwndStatusBar, SB_SETPARTS, 3, (LPARAM)statusWidths);
    }    // 更新状态面板位置和大小
    if (m_hwndStatusPanel) {
        MoveWindow(m_hwndStatusPanel,
            ScaleX(10), ScaleY(10),
            rcClient.right - ScaleX(20),
            ScaleY(50),  // 高度从30增加到50
            TRUE);
        
        // 重新创建字体以适应新的DPI
        if (m_hStatusPanelFont) {
            DeleteObject(m_hStatusPanelFont);
        }        m_hStatusPanelFont = CreateFontW(
            ScaleY(16),         // 字体高度（使用DPI缩放）
            0,                  // 字体宽度（0表示自动）
            0,                  // 文本角度
            0,                  // 基线角度
            FW_NORMAL,          // 字体粗细
            FALSE,              // 是否斜体
            FALSE,              // 是否下划线
            FALSE,              // 是否删除线
            DEFAULT_CHARSET,    // 字符集
            OUT_DEFAULT_PRECIS, // 输出精度
            CLIP_DEFAULT_PRECIS,// 裁剪精度
            DEFAULT_QUALITY,    // 输出质量
            DEFAULT_PITCH | FF_DONTCARE, // 字体间距和族
            NULL                // 使用系统默认字体
        );
        if (m_hStatusPanelFont) {
            SendMessage(m_hwndStatusPanel, WM_SETFONT, (WPARAM)m_hStatusPanelFont, TRUE);
        }
    }
    
    // 计算可用高度并按比例分配：科目列表占1/3，指令列表占2/3
    int availableHeight = rcClient.bottom - statusHeight - ScaleY(100);  // 为更高的状态面板预留更多空间
    int subjectListHeight = availableHeight / 3;
    int instructionListHeight = availableHeight * 2 / 3 - ScaleY(20);
      MoveWindow(m_hwndSubjectList,
        ScaleX(10), ScaleY(70),  // Y坐标调整为70，为更高的状态面板留出空间
        rcClient.right - ScaleX(20),
        subjectListHeight,
        TRUE);
          MoveWindow(m_hwndInstructionList,
        ScaleX(10), ScaleY(80) + subjectListHeight,  // 相应调整指令列表位置，从60改为80
        rcClient.right - ScaleX(20),
        instructionListHeight,
        TRUE);
    
    // 重新设置科目列表的列宽（使用DPI缩放）
    if (m_hwndSubjectList) {
        ListView_SetColumnWidth(m_hwndSubjectList, 0, ScaleX(240));  // 科目列
        ListView_SetColumnWidth(m_hwndSubjectList, 1, ScaleX(250));  // 开始时间列
        ListView_SetColumnWidth(m_hwndSubjectList, 2, ScaleX(250));  // 结束时间列
    }
    
    // 重新设置指令列表的列宽（使用DPI缩放）
    if (m_hwndInstructionList) {
        ListView_SetColumnWidth(m_hwndInstructionList, 0, ScaleX(120));  // 科目列
        ListView_SetColumnWidth(m_hwndInstructionList, 1, ScaleX(260));  // 指令列
        ListView_SetColumnWidth(m_hwndInstructionList, 2, ScaleX(240));  // 播放时间列
        ListView_SetColumnWidth(m_hwndInstructionList, 3, ScaleX(100));  // 状态列
    }
    
    // 更新状态栏
    SendMessage(m_hwndStatusBar, WM_SIZE, 0, 0);
}

// 指令播放相关方法实现
void MainWindow::PlayInstruction(int index, bool isManualPlay) {
    if (index < 0 || static_cast<size_t>(index) >= m_instructions.size()) {
        return;  // 无效索引
    }
    
    auto& instruction = m_instructions[index];
    
    // 检查指令是否已过期超过60秒（只对自动播放生效）
    if (!isManualPlay) {
        auto now = std::chrono::system_clock::now();
        auto nowTimestamp = std::chrono::duration_cast<std::chrono::seconds>(
            now.time_since_epoch()).count();
        auto instructionTimestamp = std::chrono::duration_cast<std::chrono::seconds>(
            instruction.playTime.time_since_epoch()).count();
          // 如果指令已过期超过60秒，标记为跳过并返回
        if (instructionTimestamp < nowTimestamp && 
            (nowTimestamp - instructionTimestamp) > 60) {
            instruction.status = PlaybackStatus::SKIPPED;
            UpdateInstructionListDisplay();
            
            // 设置下一指令
            SetNextInstruction();
            return;
        }
    }
    
    // 如果是手动播放，标记之前未播放的指令为跳过
    if (isManualPlay) {
        MarkPreviousAsSkipped(index);
    }
    
    // 设置当前播放状态
    if (m_currentPlayingIndex >= 0 && 
        static_cast<size_t>(m_currentPlayingIndex) < m_instructions.size()) {
        m_instructions[m_currentPlayingIndex].status = PlaybackStatus::PLAYED;
    }
      instruction.status = PlaybackStatus::PLAYING;
    m_currentPlayingIndex = index;
    
    // 记录播放开始时间
    m_currentPlayingStartTime = std::chrono::system_clock::now();
    
    // 播放音频文件
    AudioPlayer::playAudioFile(instruction.audioFile);
    
    // 更新显示
    UpdateInstructionListDisplay();

    // 确保焦点跟随到当前播放的指令
    EnsureInstructionListFocus();

    // 注意：不再立即标记为已播放，等待CheckPlaybackCompletion检查播放完成
    // 设置下一指令（如果需要）
    if (isManualPlay) {
        // 手动播放：设置为被播放指令之后的第一个未播放指令
        m_nextInstructionIndex = FindNextUnplayedInstructionAfter(index);
    } else {
        // 自动播放：保持当前的下一指令设置
        // 下一指令的设置会在播放完成后自动处理
    }
}

void MainWindow::MarkPreviousAsSkipped(int playIndex) {
    // 将指定索引之前的所有未播放指令标记为跳过
    for (size_t i = 0; i < static_cast<size_t>(playIndex) && i < m_instructions.size(); ++i) {
        auto& instruction = m_instructions[i];
        
        // 只有未播放的指令才标记为跳过
        if (instruction.status == PlaybackStatus::UNPLAYED) {
            instruction.status = PlaybackStatus::SKIPPED;
        }
    }
}

void MainWindow::UpdateInstructionListDisplay() {
    // 更新指令列表显示（包括状态文字）
    UpdateInstructionList();

    // 更新状态面板
    UpdateStatusPanel();

    // 注意：文字颜色通过自定义绘制实现，需要处理 NM_CUSTOMDRAW 消息
    // 暂时通过状态列的文字来提供视觉反馈
}

void MainWindow::EnsureInstructionListFocus() {
    // 确保指令列表有内容
    if (m_instructions.empty() || !m_hwndInstructionList) {
        return;
    }

    int focusIndex = -1;

    // 优先选择当前正在播放的指令
    if (m_currentPlayingIndex >= 0 &&
        static_cast<size_t>(m_currentPlayingIndex) < m_instructions.size()) {
        focusIndex = m_currentPlayingIndex;
    }
    // 如果没有正在播放的指令，选择下一个即将播放的指令
    else if (m_nextInstructionIndex >= 0 &&
             static_cast<size_t>(m_nextInstructionIndex) < m_instructions.size()) {
        focusIndex = m_nextInstructionIndex;
    }
    // 如果都没有，尝试找到第一个未播放的指令
    else {
        for (size_t i = 0; i < m_instructions.size(); ++i) {
            if (m_instructions[i].status == PlaybackStatus::UNPLAYED) {
                focusIndex = static_cast<int>(i);
                break;
            }
        }
    }

    // 如果找到了要聚焦的指令索引
    if (focusIndex >= 0) {
        // 选择并设置焦点到该指令
        ListView_SetItemState(m_hwndInstructionList, focusIndex,
                            LVIS_SELECTED | LVIS_FOCUSED,
                            LVIS_SELECTED | LVIS_FOCUSED);

        // 确保该指令可见（滚动到视图中）
        ListView_EnsureVisible(m_hwndInstructionList, focusIndex, FALSE);
    }
}

int MainWindow::FindNextUnplayedInstruction() const {
    // 返回列表中第一个未播放的指令，不考虑时间
    for (size_t i = 0; i < m_instructions.size(); ++i) {
        const auto& instruction = m_instructions[i];
        
        // 只查找未播放的指令
        if (instruction.status == PlaybackStatus::UNPLAYED) {
            return static_cast<int>(i);
        }
    }
    
    return -1;  // 没有找到未播放的指令
}

int MainWindow::FindNextUnplayedInstructionAfter(int index) const {
    // 查找指定索引之后的第一个未播放指令
    for (size_t i = index + 1; i < m_instructions.size(); ++i) {
        const auto& instruction = m_instructions[i];
        
        // 只查找未播放的指令
        if (instruction.status == PlaybackStatus::UNPLAYED) {
            return static_cast<int>(i);
        }
    }
    
    return -1;  // 没有找到未播放的指令
}

void MainWindow::SetNextInstruction() {
    // 查找列表中第一个未播放的指令
    m_nextInstructionIndex = FindNextUnplayedInstruction();
}

bool MainWindow::IsTimeToPlayNextInstruction() const {
    // 检查下一指令是否有效
    if (m_nextInstructionIndex < 0 || 
        static_cast<size_t>(m_nextInstructionIndex) >= m_instructions.size()) {
        return false;
    }
    
    const auto& instruction = m_instructions[m_nextInstructionIndex];
    
    // 检查指令是否仍然处于未播放状态
    if (instruction.status != PlaybackStatus::UNPLAYED) {
        return false;
    }
    
    // 获取当前时间和指令播放时间的Unix时间戳
    auto now = std::chrono::system_clock::now();
    auto instructionTime = instruction.playTime;
    
    // 转换为Unix时间戳（秒）
    auto nowTimestamp = std::chrono::duration_cast<std::chrono::seconds>(
        now.time_since_epoch()).count();
    auto instructionTimestamp = std::chrono::duration_cast<std::chrono::seconds>(
        instructionTime.time_since_epoch()).count();
      // 检查指令是否已过期超过60秒
    if (instructionTimestamp < nowTimestamp && 
        (nowTimestamp - instructionTimestamp) > 60) {
        // 指令已过期超过60秒，不播放
        return false;
    }
    
    // 比较时间戳：当指令时间已到且未过期超过60秒时返回true
    return instructionTimestamp <= nowTimestamp;
}

void MainWindow::ShowInstructionContextMenu(int x, int y, int itemIndex) {
    // 创建右键菜单
    HMENU hMenu = CreatePopupMenu();
    if (hMenu) {
        // 添加立即播放菜单项
        AppendMenuW(hMenu, MF_STRING, IDM_PLAY_INSTRUCTION, L"立即播放");
        
        // 显示菜单并获取用户选择
        int command = TrackPopupMenu(hMenu, 
            TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD,
            x, y, 0, m_hwnd, NULL);
        
        // 处理菜单选择
        if (command == IDM_PLAY_INSTRUCTION) {
            PlayInstruction(itemIndex, true);  // 手动播放
        }
        
        // 销毁菜单
        DestroyMenu(hMenu);
    }
}

void MainWindow::CheckPlaybackCompletion() {
    // 检查是否有指令正在播放
    if (m_currentPlayingIndex < 0 ||
        static_cast<size_t>(m_currentPlayingIndex) >= m_instructions.size()) {
        return;  // 没有指令正在播放
    }

    auto& currentInstruction = m_instructions[m_currentPlayingIndex];

    // 确认指令状态为播放中
    if (currentInstruction.status != PlaybackStatus::PLAYING) {
        return;  // 指令不是播放中状态
    }

    // 获取音频文件总时长
    double totalDuration = AudioPlayer::getAudioDuration(currentInstruction.audioFile);
    if (totalDuration <= 0.0) {
        // 无法获取音频时长，假设播放完成
        currentInstruction.status = PlaybackStatus::PLAYED;
        m_currentPlayingIndex = -1;

        // 设置下一指令
        SetNextInstruction();
        UpdateInstructionListDisplay();
        return;
    }

    // 计算已播放时长
    auto now = std::chrono::system_clock::now();
    auto playedDuration = std::chrono::duration_cast<std::chrono::seconds>(
        now - m_currentPlayingStartTime).count();

    // 如果已播放时长超过总时长，认为播放完成
    if (playedDuration >= static_cast<int>(totalDuration)) {
        currentInstruction.status = PlaybackStatus::PLAYED;
        m_currentPlayingIndex = -1;

        // 设置下一指令
        SetNextInstruction();
        UpdateInstructionListDisplay();

        // 播放完成后立即更新焦点到下一个指令
        EnsureInstructionListFocus();
    }
}

void MainWindow::LoadConfigFile() {
    // 创建文件选择对话框
    OPENFILENAMEA ofn;
    char szFile[260] = {0};  // 文件名缓冲区

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = m_hwnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "INI Files\0*.ini\0All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

    // 显示文件选择对话框
    if (GetOpenFileNameA(&ofn)) {
        // 加载选中的配置文件
        auto& configManager = ConfigManager::getInstance();
        if (configManager.loadConfig(szFile)) {
            // 配置加载成功，重新生成指令列表
            m_instructions.clear();
            for (const auto& subject : m_subjects) {
                auto subjectInstructions = Instruction::generateInstructions(subject);
                m_instructions.insert(m_instructions.end(),
                    subjectInstructions.begin(), subjectInstructions.end());
            }

            // 按播放时间排序
            std::sort(m_instructions.begin(), m_instructions.end(),
                [](const Instruction& a, const Instruction& b) {
                    return a.playTime < b.playTime;
                });

            // 重置播放状态
            m_currentPlayingIndex = -1;
            m_nextInstructionIndex = -1;

            // 更新界面显示
            UpdateInstructionList();
            UpdateStatusPanel();

            // 显示成功消息
            std::wstring message = L"配置文件加载成功！\n\n";
            message += L"配置文件：";
            message += ConvertUtf8ToWide(szFile);
            MessageBoxW(m_hwnd, message.c_str(), L"加载成功", MB_OK | MB_ICONINFORMATION);
        } else {
            // 配置加载失败
            std::wstring message = L"配置文件加载失败！\n\n";
            message += L"文件：";
            message += ConvertUtf8ToWide(szFile);
            message += L"\n\n请检查文件格式是否正确。";
            MessageBoxW(m_hwnd, message.c_str(), L"加载失败", MB_OK | MB_ICONERROR);
        }
    }
}

void MainWindow::ReloadConfigFile() {
    auto& configManager = ConfigManager::getInstance();
    std::string currentConfigPath = configManager.getCurrentConfigPath();

    // 如果没有当前配置文件，使用默认配置
    if (currentConfigPath.empty()) {
        if (configManager.loadDefaultConfig()) {
            currentConfigPath = configManager.getCurrentConfigPath();
        } else {
            MessageBoxW(m_hwnd, L"无法加载默认配置文件！", L"重新加载失败", MB_OK | MB_ICONERROR);
            return;
        }
    } else {
        // 重新加载当前配置文件
        if (!configManager.loadConfig(currentConfigPath)) {
            std::wstring message = L"重新加载配置文件失败！\n\n";
            message += L"文件：";
            message += ConvertUtf8ToWide(currentConfigPath);
            MessageBoxW(m_hwnd, message.c_str(), L"重新加载失败", MB_OK | MB_ICONERROR);
            return;
        }
    }

    // 重新生成指令列表
    m_instructions.clear();
    for (const auto& subject : m_subjects) {
        auto subjectInstructions = Instruction::generateInstructions(subject);
        m_instructions.insert(m_instructions.end(),
            subjectInstructions.begin(), subjectInstructions.end());
    }

    // 按播放时间排序
    std::sort(m_instructions.begin(), m_instructions.end(),
        [](const Instruction& a, const Instruction& b) {
            return a.playTime < b.playTime;
        });

    // 重置播放状态
    m_currentPlayingIndex = -1;
    m_nextInstructionIndex = -1;

    // 更新界面显示
    UpdateInstructionList();
    UpdateStatusPanel();

    // 显示成功消息
    std::wstring message = L"配置文件重新加载成功！\n\n";
    message += L"配置文件：";
    message += ConvertUtf8ToWide(currentConfigPath);
    MessageBoxW(m_hwnd, message.c_str(), L"重新加载成功", MB_OK | MB_ICONINFORMATION);
}
