#include "MainWindow.h"
#include "AudioPlayer.h"
#include "resource.h"
#include <windowsx.h>
#include <CommCtrl.h>
#include <string>

#pragma comment(lib, "comctl32.lib")

// 定义 UNICODE 版本的窗口类名和标题
#ifdef UNICODE
#define WINDOW_CLASS_NAME L"ExamInstruPlayerWindow"
#define WINDOW_TITLE L"考试指令播放器"
#else
#define WINDOW_CLASS_NAME "ExamInstruPlayerWindow"
#define WINDOW_TITLE "考试指令播放器"
#endif

MainWindow::MainWindow() : m_hwnd(NULL), m_hwndStatusBar(NULL), 
    m_hwndSubjectList(NULL), m_hwndInstructionList(NULL), m_dpi(96), m_dpiScaleX(1.0f), m_dpiScaleY(1.0f),
    m_currentPlayingIndex(-1), m_nextInstructionIndex(-1) {
    // 初始化COM
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    
    // 初始化通用控件
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_LISTVIEW_CLASSES | ICC_BAR_CLASSES;
    InitCommonControlsEx(&icex);
}

MainWindow::~MainWindow() {
    DestroyFontAndBrushes();
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
    
    if (pThis) {        switch (uMsg) {
            case WM_CREATE:
                pThis->CreateControls();
                pThis->UpdateDpiInfo();
                SetTimer(hwnd, TIMER_ID, TIMER_INTERVAL, NULL);
                return 0;
                
            case WM_DESTROY:
                KillTimer(hwnd, TIMER_ID);
                PostQuitMessage(0);
                return 0;
                  case WM_DPICHANGED: {
                pThis->UpdateDpiInfo();
                
                // 重新创建字体以适应新的DPI
                pThis->CreateFontAndBrushes();
                
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
                  // 重新应用字体到所有控件
                pThis->ApplyDefaultFontToButton(GetDlgItem(hwnd, IDC_ADD_SUBJECT_BTN));  // 按钮使用默认字体
                pThis->ApplyFontToControl(pThis->m_hwndSubjectList);
                pThis->ApplyFontToControl(pThis->m_hwndInstructionList);
                pThis->ApplyFontToControl(pThis->m_hwndStatusBar);
                
                return 0;
            }            case WM_SIZE: {
                pThis->UpdateLayoutForDpi();
                return 0;
            }
                
            case WM_TIMER:
                if (wParam == TIMER_ID) {
                    pThis->UpdateStatusBar();
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
            }
              case WM_COMMAND:
                switch (LOWORD(wParam)) {                    case IDC_ADD_SUBJECT_BTN:
                        pThis->AddSubject();
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
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    
    RegisterClassW(&wc);
    
    DWORD style = WS_OVERLAPPEDWINDOW;
    RECT rc = { 0, 0, 800, 600 };
    AdjustWindowRect(&rc, style, FALSE);
    
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
    
    // 窗口创建后，初始化字体和画刷（依赖DPI信息）
    if (m_hwnd) {
        UpdateDpiInfo();
        CreateFontAndBrushes();
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
    );
      // 创建"添加科目"按钮 - 使用DPI缩放
    CreateWindowExW(
        0,
        L"BUTTON",
        L"添加科目",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        ScaleX(10), ScaleY(10), ScaleX(100), ScaleY(30),
        m_hwnd,
        (HMENU)IDC_ADD_SUBJECT_BTN,
        GetModuleHandle(NULL),
        NULL
    );    // 创建科目列表 - 使用DPI缩放，添加边框
    m_hwndSubjectList = CreateWindowExW(
        WS_EX_CLIENTEDGE,    // 扩展样式：添加凹陷边框
        WC_LISTVIEWW,       // 类名
        NULL,               // 窗口文本
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SINGLESEL,  // 样式
        ScaleX(10), ScaleY(50), ScaleX(780), ScaleY(200),   // 位置和大小
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
    lvc.cx = ScaleX(360);  // 指令列进一步加宽到360px，最大化利用窗口空间
    ListView_InsertColumn(m_hwndInstructionList, 1, &lvc);

    lvc.iSubItem = 2;
    lvc.pszText = (LPWSTR)L"播放时间";
    lvc.cx = ScaleX(140);  // 播放时间列加宽到140px (原100px)
    ListView_InsertColumn(m_hwndInstructionList, 2, &lvc);
      lvc.iSubItem = 3;
    lvc.pszText = (LPWSTR)L"状态";
    lvc.cx = ScaleX(120);  // 状态列加宽到120px (原80px)
    ListView_InsertColumn(m_hwndInstructionList, 3, &lvc);
    
      // 应用字体到所有控件
    ApplyDefaultFontToButton(GetDlgItem(m_hwnd, IDC_ADD_SUBJECT_BTN));  // 按钮使用默认字体
    ApplyFontToControl(m_hwndSubjectList);
    ApplyFontToControl(m_hwndInstructionList);
    ApplyFontToControl(m_hwndStatusBar);
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
    
    // 删除相关的指令
    m_instructions.erase(
        std::remove_if(m_instructions.begin(), m_instructions.end(),
            [&](const Instruction& instr) {
                return instr.subjectName == subject.name;
            }),
        m_instructions.end()
    );
    
    // 删除科目
    m_subjects.erase(m_subjects.begin() + index);
    
    // 更新列表
    UpdateSubjectList();
    UpdateInstructionList();
}

void MainWindow::UpdateStatusBar() {
    int volume = AudioPlayer::getSystemVolume();
    wchar_t statusText[256];
    swprintf_s(statusText, _countof(statusText), L"系统音量: %d%%", volume);
    SendMessage(m_hwndStatusBar, SB_SETTEXT, 0, (LPARAM)statusText);
}

void MainWindow::UpdateSubjectList() {
    ListView_DeleteAllItems(m_hwndSubjectList);
    
    for (size_t i = 0; i < m_subjects.size(); ++i) {
        const auto& subject = m_subjects[i];
        
        // 使用更安全的字符串转换方法
        std::wstring subjectName;
        std::wstring startTime;
        std::wstring endTime;
        
        // 转换科目名称
        if (!subject.name.empty()) {
            int size_needed = MultiByteToWideChar(CP_UTF8, 0, subject.name.c_str(), -1, NULL, 0);
            if (size_needed > 0) {
                subjectName.resize(size_needed - 1);
                MultiByteToWideChar(CP_UTF8, 0, subject.name.c_str(), -1, &subjectName[0], size_needed);
            }
        }
        
        // 转换开始时间
        std::string startTimeStr = subject.getStartTimeString();
        if (!startTimeStr.empty()) {
            int size_needed = MultiByteToWideChar(CP_UTF8, 0, startTimeStr.c_str(), -1, NULL, 0);
            if (size_needed > 0) {
                startTime.resize(size_needed - 1);
                MultiByteToWideChar(CP_UTF8, 0, startTimeStr.c_str(), -1, &startTime[0], size_needed);
            }
        }
        
        // 转换结束时间
        std::string endTimeStr = subject.getEndTimeString();
        if (!endTimeStr.empty()) {
            int size_needed = MultiByteToWideChar(CP_UTF8, 0, endTimeStr.c_str(), -1, NULL, 0);
            if (size_needed > 0) {
                endTime.resize(size_needed - 1);
                MultiByteToWideChar(CP_UTF8, 0, endTimeStr.c_str(), -1, &endTime[0], size_needed);
            }
        }
        
        LVITEM lvi = {0};
        lvi.mask = LVIF_TEXT;
        lvi.iItem = static_cast<int>(i);
        
        // 添加科目名称
        lvi.iSubItem = 0;
        lvi.pszText = const_cast<LPWSTR>(subjectName.c_str());
        ListView_InsertItem(m_hwndSubjectList, &lvi);        // 添加开始时间 - 使用SetItemText确保安全
        ListView_SetItemText(m_hwndSubjectList, static_cast<int>(i), 1, const_cast<LPWSTR>(startTime.c_str()));
        
        // 添加结束时间
        ListView_SetItemText(m_hwndSubjectList, static_cast<int>(i), 2, const_cast<LPWSTR>(endTime.c_str()));
    }
}

void MainWindow::UpdateInstructionList() {
    ListView_DeleteAllItems(m_hwndInstructionList);
    
    // 重置下一指令索引，因为列表已更新
    m_nextInstructionIndex = -1;
    
    for (size_t i = 0; i < m_instructions.size(); ++i) {
        const auto& instruction = m_instructions[i];
        
        // 使用更安全的字符串转换方法
        std::wstring subjectName;
        std::wstring instrName;
        std::wstring playTime;
        
        // 转换科目名称
        if (!instruction.subjectName.empty()) {
            int size_needed = MultiByteToWideChar(CP_UTF8, 0, instruction.subjectName.c_str(), -1, NULL, 0);
            if (size_needed > 0) {
                subjectName.resize(size_needed - 1);
                MultiByteToWideChar(CP_UTF8, 0, instruction.subjectName.c_str(), -1, &subjectName[0], size_needed);
            }
        }
        
        // 转换指令名称
        if (!instruction.name.empty()) {
            int size_needed = MultiByteToWideChar(CP_UTF8, 0, instruction.name.c_str(), -1, NULL, 0);
            if (size_needed > 0) {
                instrName.resize(size_needed - 1);
                MultiByteToWideChar(CP_UTF8, 0, instruction.name.c_str(), -1, &instrName[0], size_needed);
            }
        }
        
        // 转换播放时间
        std::string playTimeStr = instruction.getPlayTimeString();
        if (!playTimeStr.empty()) {
            int size_needed = MultiByteToWideChar(CP_UTF8, 0, playTimeStr.c_str(), -1, NULL, 0);
            if (size_needed > 0) {
                playTime.resize(size_needed - 1);
                MultiByteToWideChar(CP_UTF8, 0, playTimeStr.c_str(), -1, &playTime[0], size_needed);
            }
        }
        
        LVITEM lvi = {0};
        lvi.mask = LVIF_TEXT;
        lvi.iItem = static_cast<int>(i);
        
        // 添加科目名称
        lvi.iSubItem = 0;
        lvi.pszText = const_cast<LPWSTR>(subjectName.c_str());
        ListView_InsertItem(m_hwndInstructionList, &lvi);
          // 添加指令名称 - 使用SetItemText确保安全
        ListView_SetItemText(m_hwndInstructionList, static_cast<int>(i), 1, const_cast<LPWSTR>(instrName.c_str()));
        
        // 添加播放时间
        ListView_SetItemText(m_hwndInstructionList, static_cast<int>(i), 2, const_cast<LPWSTR>(playTime.c_str()));
        
        // 添加状态
        std::string statusStr = instruction.getStatusString();
        std::wstring statusWStr;
        if (!statusStr.empty()) {
            int size_needed = MultiByteToWideChar(CP_UTF8, 0, statusStr.c_str(), -1, NULL, 0);
            if (size_needed > 0) {
                statusWStr.resize(size_needed - 1);
                MultiByteToWideChar(CP_UTF8, 0, statusStr.c_str(), -1, &statusWStr[0], size_needed);
            }
        }
        ListView_SetItemText(m_hwndInstructionList, static_cast<int>(i), 3, const_cast<LPWSTR>(statusWStr.c_str()));
    }
}

void MainWindow::UpdateNextInstruction() {
    // 如果当前正在播放指令，不查找新的指令
    if (m_currentPlayingIndex >= 0 && 
        static_cast<size_t>(m_currentPlayingIndex) < m_instructions.size() &&
        m_instructions[m_currentPlayingIndex].status == PlaybackStatus::PLAYING) {
        return;  // 当前有指令正在播放，等待播放完成
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
    static const wchar_t* subjects[] = {
        L"语文", L"数学", L"英语", L"一上单科", L"首选科目", L"再选合堂"
    };
    
    MainWindow* pMainWindow = nullptr;
    if (msg == WM_INITDIALOG) {
        pMainWindow = reinterpret_cast<MainWindow*>(lParam);
        if (!pMainWindow) {
            EndDialog(hwnd, IDCANCEL);
            return FALSE;
        }
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pMainWindow));
        
        // 为对话框启用DPI感知
        SetDialogDpiChangeBehavior(hwnd, DDC_DISABLE_ALL, DDC_DISABLE_ALL);
        
    } else {
        pMainWindow = reinterpret_cast<MainWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        // 只在需要 pMainWindow 的消息中检查空指针
        if (!pMainWindow && (msg == WM_COMMAND || msg == WM_NOTIFY)) {
            return FALSE;
        }
    }

    switch (msg) {        case WM_INITDIALOG: {
            // 初始化科目下拉列表
            HWND hComboBox = GetDlgItem(hwnd, IDC_SUBJECT_COMBO);
            for (const auto& subject : subjects) {
                SendMessageW(hComboBox, CB_ADDSTRING, 0, (LPARAM)subject);
            }
            SendMessageW(hComboBox, CB_SETCURSEL, 0, 0);            // 应用系统默认字体到对话框控件
            if (pMainWindow) {
                pMainWindow->ApplyFontToControl(hwnd);  // 对话框本身
                pMainWindow->ApplyFontToControl(hComboBox);  // 下拉列表
                pMainWindow->ApplyFontToControl(GetDlgItem(hwnd, IDC_START_TIME_EDIT));  // 时间编辑框
                pMainWindow->ApplyDefaultFontToButton(GetDlgItem(hwnd, IDOK));  // 确定按钮使用默认字体
                pMainWindow->ApplyDefaultFontToButton(GetDlgItem(hwnd, IDCANCEL));  // 取消按钮使用默认字体
                
                // 应用到静态标签
                HWND hStatic1 = GetDlgItem(hwnd, IDC_STATIC);
                if (hStatic1) pMainWindow->ApplyFontToControl(hStatic1);
            }
              // 设置默认时间为当前时间的下一个整点
            SYSTEMTIME st;
            GetLocalTime(&st);
            st.wMinute = 0;
            st.wHour = (st.wHour + 1) % 24;
            
            wchar_t timeStr[10];
            swprintf_s(timeStr, _countof(timeStr), L"%02d:%02d", st.wHour, st.wMinute);
            SetDlgItemText(hwnd, IDC_START_TIME_EDIT, timeStr);
            
            return TRUE;
        }
        
        case WM_COMMAND: {
            switch (LOWORD(wParam)) {
                case IDOK: {
                    wchar_t subjectName[256] = {0};
                    wchar_t startTime[6] = {0};
                    
                    // 获取选中的科目
                    HWND hComboBox = GetDlgItem(hwnd, IDC_SUBJECT_COMBO);
                    int selectedIndex = SendMessageW(hComboBox, CB_GETCURSEL, 0, 0);
                    if (selectedIndex == CB_ERR) {
                        MessageBoxW(hwnd, L"请选择科目", L"错误", MB_OK | MB_ICONERROR);
                        return TRUE;
                    }
                    SendMessageW(hComboBox, CB_GETLBTEXT, selectedIndex, (LPARAM)subjectName);
                    
                    // 获取开始时间
                    if (GetDlgItemTextW(hwnd, IDC_START_TIME_EDIT, startTime, 6) == 0) {
                        MessageBoxW(hwnd, L"请输入开始时间", L"错误", MB_OK | MB_ICONERROR);
                        return TRUE;
                    }                    try {
                        // 使用Windows API进行字符转换 - 增加缓冲区大小以支持中文字符
                        char mbSubjectName[768] = {0};  // 增加缓冲区大小，中文字符可能需要更多字节
                        char mbStartTime[16] = {0};     // 增加时间缓冲区大小
                        
                        // 进行字符转换，并检查转换结果
                        int subjectResult = WideCharToMultiByte(CP_UTF8, 0, subjectName, -1, 
                                          mbSubjectName, sizeof(mbSubjectName), NULL, NULL);
                        int timeResult = WideCharToMultiByte(CP_UTF8, 0, startTime, -1, 
                                          mbStartTime, sizeof(mbStartTime), NULL, NULL);
                        
                        if (subjectResult == 0 || timeResult == 0) {
                            MessageBoxW(hwnd, L"字符转换失败", L"错误", MB_OK | MB_ICONERROR);
                            return TRUE;
                        }
                          std::string name(mbSubjectName);
                        std::string timeStr(mbStartTime);
                        
                        if (!Subject::isValidStartTime(timeStr)) {
                            MessageBoxW(hwnd, L"请输入正确的时间格式（HH:MM）", L"错误", MB_OK | MB_ICONERROR);
                            return TRUE;
                        }
                        
                        Subject subject = Subject::createSubject(name);
                        subject.setStartTime(timeStr);
                        
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

// DPI相关函数实现
void MainWindow::UpdateDpiInfo() {
    m_dpi = GetDpiForWindow(m_hwnd);
    m_dpiScaleX = m_dpi / 96.0f;
    m_dpiScaleY = m_dpi / 96.0f;
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
    int statusHeight = rcStatus.bottom - rcStatus.top;
    
    // 使用DPI缩放的尺寸重新布局控件
    MoveWindow(GetDlgItem(m_hwnd, IDC_ADD_SUBJECT_BTN),
        ScaleX(10), ScaleY(10), ScaleX(100), ScaleY(30), TRUE);
        
    MoveWindow(m_hwndSubjectList,
        ScaleX(10), ScaleY(50),
        rcClient.right - ScaleX(20),
        (rcClient.bottom - statusHeight - ScaleY(80)) / 2,
        TRUE);
        
    MoveWindow(m_hwndInstructionList,
        ScaleX(10), ScaleY(60) + (rcClient.bottom - statusHeight - ScaleY(80)) / 2,
        rcClient.right - ScaleX(20),
        (rcClient.bottom - statusHeight - ScaleY(100)) / 2,
        TRUE);
    
    // 更新状态栏
    SendMessage(m_hwndStatusBar, WM_SIZE, 0, 0);
}

// 字体管理函数
void MainWindow::CreateFontAndBrushes() {
    // 使用系统默认字体，不需要创建自定义字体
    // 保留此函数为了兼容性，但不执行任何操作
}

void MainWindow::DestroyFontAndBrushes() {
    // 使用系统默认字体，不需要销毁任何自定义字体
}

void MainWindow::ApplyFontToControl(HWND hwnd) {
    if (hwnd) {
        // 使用系统默认GUI字体
        HFONT hDefaultFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        if (hDefaultFont) {
            SendMessage(hwnd, WM_SETFONT, (WPARAM)hDefaultFont, TRUE);
        }
    }
}

void MainWindow::ApplyDefaultFontToButton(HWND hwnd) {
    if (hwnd) {
        // 获取系统默认GUI字体
        HFONT hDefaultFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        if (hDefaultFont) {
            SendMessage(hwnd, WM_SETFONT, (WPARAM)hDefaultFont, TRUE);
        }
    }
}

// 指令播放相关方法实现
void MainWindow::PlayInstruction(int index, bool isManualPlay) {
    if (index < 0 || static_cast<size_t>(index) >= m_instructions.size()) {
        return;  // 无效索引
    }
    
    auto& instruction = m_instructions[index];
    
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
    m_currentPlayingIndex = index;    // 播放音频文件
    AudioPlayer::playAudioFile(instruction.audioFile);
      // 更新显示
    UpdateInstructionListDisplay();
    
    // 简化处理：立即标记为已播放并设置下一指令
    instruction.status = PlaybackStatus::PLAYED;
    m_currentPlayingIndex = -1;
    
    // 设置下一指令
    if (isManualPlay) {
        // 手动播放：设置为被播放指令之后的第一个未播放指令
        m_nextInstructionIndex = FindNextUnplayedInstructionAfter(index);
    } else {
        // 自动播放：设置为列表中第一个未播放指令
        SetNextInstruction();
    }
      UpdateInstructionListDisplay();
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
    
    // 注意：文字颜色通过自定义绘制实现，需要处理 NM_CUSTOMDRAW 消息
    // 暂时通过状态列的文字来提供视觉反馈
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
    
    // 检查时间是否已到（精确到分钟）
    auto now = std::chrono::system_clock::now();
    auto nowTime = std::chrono::system_clock::to_time_t(now);
    auto instrTime = std::chrono::system_clock::to_time_t(instruction.playTime);
    
    // 将时间转换为分钟进行比较
    return (nowTime / 60) >= (instrTime / 60);
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
