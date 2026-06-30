#include "MainWindow.h"
#include "AudioPlayer.h"
#include "ConfigManager.h"
#include "resource.h"
#include "version.h"
#include "StringUtil.h"
#include "PathUtil.h"
#include <windowsx.h>
#include <CommCtrl.h>
#include <string>
#include <chrono>
#include <algorithm>
#include <filesystem>

#pragma comment(lib, "comctl32.lib")

// 听力文件名常量（英语科目）
static constexpr const char* LISTENING_AUDIO_FILE = "tl.mp3";

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
    // 初始化 COM
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    // 初始化通用控件
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_LISTVIEW_CLASSES | ICC_BAR_CLASSES;
    InitCommonControlsEx(&icex);

    // 加载默认配置文件
    auto& configManager = ConfigManager::getInstance();
    if (!configManager.loadDefaultConfig()) {
        // 默认配置加载失败，显示警告但不阻止启动
        // 此处窗口尚未创建，暂不弹消息框
    }

    m_lastVolumeCheck = std::chrono::steady_clock::time_point();
}

MainWindow::~MainWindow() {
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

    if (pThis) {
        switch (uMsg) {
            case WM_CREATE:
                pThis->CreateControls();
                pThis->UpdateDpiInfo();
                pThis->UpdateLayoutForDpi();
                SetTimer(hwnd, TIMER_ID, TIMER_INTERVAL, NULL);
                return 0;

            case WM_DESTROY:
                KillTimer(hwnd, TIMER_ID);
                AudioPlayer::stop();
                PostQuitMessage(0);
                return 0;

            case WM_DPICHANGED: {
                pThis->UpdateDpiInfo();
                RECT* const prcNewWindow = (RECT*)lParam;
                SetWindowPos(hwnd,
                    NULL,
                    prcNewWindow->left,
                    prcNewWindow->top,
                    prcNewWindow->right - prcNewWindow->left,
                    prcNewWindow->bottom - prcNewWindow->top,
                    SWP_NOZORDER | SWP_NOACTIVATE);
                pThis->UpdateLayoutForDpi();
                return 0;
            }

            case WM_SIZE:
                pThis->UpdateLayoutForDpi();
                return 0;

            case WM_TIMER:
                if (wParam == TIMER_ID) {
                    pThis->UpdateStatusBar();
                    pThis->UpdateStatusPanel();
                    pThis->CheckPlaybackCompletion();
                    pThis->UpdateNextInstruction();
                }
                return 0;

            case WM_NOTIFY: {
                LPNMHDR lpnmh = (LPNMHDR)lParam;
                if (lpnmh->hwndFrom == pThis->m_hwndSubjectList) {
                    pThis->HandleSubjectListNotify(lpnmh);
                } else if (lpnmh->hwndFrom == pThis->m_hwndInstructionList) {
                    return pThis->HandleInstructionListNotify(lpnmh);
                }
                return 0;
            }

            case WM_CTLCOLORBTN:
            case WM_CTLCOLORSTATIC:
                return DefWindowProc(hwnd, uMsg, wParam, lParam);

            case WM_COMMAND:
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
                        int selectedItem = ListView_GetNextItem(pThis->m_hwndSubjectList, -1, LVNI_SELECTED);
                        if (selectedItem >= 0) {
                            pThis->DeleteSubject(selectedItem);
                        }
                        return 0;
                    }
                    case IDM_PLAY_INSTRUCTION: {
                        int selectedItem = ListView_GetNextItem(pThis->m_hwndInstructionList, -1, LVNI_SELECTED);
                        if (selectedItem >= 0) {
                            pThis->PlayInstruction(selectedItem, true);
                        }
                        return 0;
                    }
                    case IDM_HELP_HELP:
                        pThis->ShowHelp();
                        return 0;
                    case IDM_HELP_ABOUT:
                        pThis->ShowAbout();
                        return 0;
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
    wc.lpszMenuName = MAKEINTRESOURCE(IDR_MAIN_MENU);

    RegisterClassW(&wc);

    // 创建窗口前获取系统 DPI 以确定窗口大小
    HDC hdcScreen = GetDC(NULL);
    int systemDpi = 96;
    if (hdcScreen) {
        systemDpi = GetDeviceCaps(hdcScreen, LOGPIXELSX);
        ReleaseDC(NULL, hdcScreen);
    }
    float dpiScale = systemDpi / 96.0f;

    DWORD style = WS_OVERLAPPEDWINDOW;
    int scaledWidth = static_cast<int>(800 * dpiScale);
    int scaledHeight = static_cast<int>(600 * dpiScale);
    RECT rc = { 0, 0, scaledWidth, scaledHeight };
    AdjustWindowRect(&rc, style, TRUE);

    m_hwnd = CreateWindowExW(
        0,
        WINDOW_CLASS_NAME,
        WINDOW_TITLE,
        style,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right - rc.left,
        rc.bottom - rc.top,
        NULL,
        NULL,
        GetModuleHandle(NULL),
        this);

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
        0,
        STATUSCLASSNAMEW,
        NULL,
        WS_CHILD | WS_VISIBLE,
        0, 0, 0, 0,
        m_hwnd,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    if (m_hwndStatusBar) {
        int statusWidths[] = { ScaleX(120), ScaleX(370), -1 };
        SendMessage(m_hwndStatusBar, SB_SETPARTS, 3, (LPARAM)statusWidths);
    }

    // 创建状态面板
    m_hwndStatusPanel = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        L"STATIC",
        L"下一指令: 无",
        WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE,
        ScaleX(10), ScaleY(10), ScaleX(780), ScaleY(50),
        m_hwnd,
        (HMENU)IDC_STATUS_PANEL,
        GetModuleHandle(NULL),
        NULL);

    if (m_hwndStatusPanel) {
        m_hStatusPanelFont = CreateFontW(
            ScaleY(16), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, NULL);
        if (m_hStatusPanelFont) {
            SendMessage(m_hwndStatusPanel, WM_SETFONT, (WPARAM)m_hStatusPanelFont, TRUE);
        }
    }

    // 创建科目列表
    m_hwndSubjectList = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        WC_LISTVIEWW,
        NULL,
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SINGLESEL,
        ScaleX(10), ScaleY(70), ScaleX(780), ScaleY(200),
        m_hwnd,
        (HMENU)IDC_SUBJECT_LIST,
        GetModuleHandle(NULL),
        NULL);

    ListView_SetExtendedListViewStyle(m_hwndSubjectList,
        LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_BORDERSELECT);

    LVCOLUMN lvc = {0};
    lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

    lvc.iSubItem = 0;
    lvc.pszText = (LPWSTR)L"科目";
    lvc.cx = ScaleX(240);
    ListView_InsertColumn(m_hwndSubjectList, 0, &lvc);

    lvc.iSubItem = 1;
    lvc.pszText = (LPWSTR)L"开始时间";
    lvc.cx = ScaleX(250);
    ListView_InsertColumn(m_hwndSubjectList, 1, &lvc);

    lvc.iSubItem = 2;
    lvc.pszText = (LPWSTR)L"结束时间";
    lvc.cx = ScaleX(250);
    ListView_InsertColumn(m_hwndSubjectList, 2, &lvc);

    // 创建指令列表
    m_hwndInstructionList = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        WC_LISTVIEWW,
        NULL,
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS,
        ScaleX(10), ScaleY(260), ScaleX(780), ScaleY(280),
        m_hwnd,
        (HMENU)IDC_INSTRUCTION_LIST,
        GetModuleHandle(NULL),
        NULL);

    ListView_SetExtendedListViewStyle(m_hwndInstructionList,
        LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_BORDERSELECT);

    lvc.iSubItem = 0;
    lvc.pszText = (LPWSTR)L"科目";
    lvc.cx = ScaleX(120);
    ListView_InsertColumn(m_hwndInstructionList, 0, &lvc);

    lvc.iSubItem = 1;
    lvc.pszText = (LPWSTR)L"指令";
    lvc.cx = ScaleX(260);
    ListView_InsertColumn(m_hwndInstructionList, 1, &lvc);

    lvc.iSubItem = 2;
    lvc.pszText = (LPWSTR)L"播放时间";
    lvc.cx = ScaleX(240);
    ListView_InsertColumn(m_hwndInstructionList, 2, &lvc);

    lvc.iSubItem = 3;
    lvc.pszText = (LPWSTR)L"状态";
    lvc.cx = ScaleX(100);
    ListView_InsertColumn(m_hwndInstructionList, 3, &lvc);
}

void MainWindow::AddSubject() {
    INT_PTR result = DialogBoxParamW(GetModuleHandle(NULL),
                   MAKEINTRESOURCEW(IDD_ADD_SUBJECT),
                   m_hwnd,
                   AddSubjectDialogProc,
                   reinterpret_cast<LPARAM>(this));

    if (result == -1) {
        DWORD error = GetLastError();
        wchar_t errorMsg[256];
        swprintf_s(errorMsg, _countof(errorMsg), L"对话框创建失败，错误代码: %d", error);
        MessageBoxW(m_hwnd, errorMsg, L"错误", MB_OK | MB_ICONERROR);
    }
}

void MainWindow::DeleteSubject(int index) {
    if (index < 0 || static_cast<size_t>(index) >= m_subjects.size()) {
        return;
    }

    auto& subject = m_subjects[index];
    int subjectIdToDelete = subject.id;

    m_instructions.erase(
        std::remove_if(m_instructions.begin(), m_instructions.end(),
            [subjectIdToDelete](const Instruction& instr) {
                return instr.subjectId == subjectIdToDelete;
            }),
        m_instructions.end());

    m_subjects.erase(m_subjects.begin() + index);

    InvalidateAudioCache();
    UpdateSubjectList();
    UpdateInstructionList();
    UpdateStatusPanel();
}

void MainWindow::UpdateStatusBar() {
    // 当前时间
    SYSTEMTIME st;
    GetLocalTime(&st);
    wchar_t currentTimeText[64];
    swprintf_s(currentTimeText, _countof(currentTimeText),
        L"当前时间: %02d:%02d:%02d", st.wHour, st.wMinute, st.wSecond);

    // 系统音量：节流，避免每秒做 COM 设备枚举
    auto now = std::chrono::steady_clock::now();
    if (m_lastVolumeCheck == std::chrono::steady_clock::time_point() ||
        std::chrono::duration_cast<std::chrono::seconds>(now - m_lastVolumeCheck).count() >= VOLUME_REFRESH_SECONDS) {
        m_cachedSystemVolume = AudioPlayer::getSystemVolume();
        m_lastVolumeCheck = now;
    }
    wchar_t volumeText[64];
    swprintf_s(volumeText, _countof(volumeText), L"系统音量: %d%%", m_cachedSystemVolume);

    // 音频文件状态：指令缺失数走缓存；听力文件单文件、exists 便宜，每秒实时查
    wchar_t audioFileStatusText[512] = L"音频文件: 无指令";

    bool listeningFileExists = std::filesystem::exists(PathUtil::getAudioPath(LISTENING_AUDIO_FILE));

    if (!m_instructions.empty()) {
        if (m_cachedMissingInstructionCount < 0) {
            int missing = 0;
            for (const auto& instruction : m_instructions) {
                if (!instruction.checkAudioFileExists()) {
                    missing++;
                }
            }
            m_cachedMissingInstructionCount = missing;
        }
        int missingCount = m_cachedMissingInstructionCount;

        if (missingCount == 0 && listeningFileExists) {
            swprintf_s(audioFileStatusText, _countof(audioFileStatusText), L"音频文件: 全部存在, 听力文件存在");
        } else if (missingCount == 0 && !listeningFileExists) {
            swprintf_s(audioFileStatusText, _countof(audioFileStatusText), L"音频文件: 全部存在, 听力文件缺失");
        } else if (missingCount > 0 && listeningFileExists) {
            swprintf_s(audioFileStatusText, _countof(audioFileStatusText), L"音频文件: 缺失%d个, 听力文件存在", missingCount);
        } else {
            swprintf_s(audioFileStatusText, _countof(audioFileStatusText), L"音频文件: 缺失%d个, 听力文件缺失", missingCount);
        }
    } else {
        if (listeningFileExists) {
            swprintf_s(audioFileStatusText, _countof(audioFileStatusText), L"音频文件: 无指令, 听力文件存在");
        } else {
            swprintf_s(audioFileStatusText, _countof(audioFileStatusText), L"音频文件: 无指令, 听力文件缺失");
        }
    }

    SendMessage(m_hwndStatusBar, SB_SETTEXT, 0, (LPARAM)volumeText);
    SendMessage(m_hwndStatusBar, SB_SETTEXT, 1, (LPARAM)audioFileStatusText);
    SendMessage(m_hwndStatusBar, SB_SETTEXT, 2, (LPARAM)currentTimeText);
}

void MainWindow::UpdateStatusPanel() {
    wchar_t statusText[512] = L"";
    bool hasCurrentInstruction = false;

    if (m_currentPlayingIndex >= 0 &&
        static_cast<size_t>(m_currentPlayingIndex) < m_instructions.size()) {

        const auto& currentInstruction = m_instructions[m_currentPlayingIndex];

        if (currentInstruction.status == PlaybackStatus::PLAYING) {
            auto now = std::chrono::system_clock::now();
            auto playedDuration = std::chrono::duration_cast<std::chrono::seconds>(
                now - m_currentPlayingStartTime).count();

            // 使用缓存的音频时长（播放开始时已取），避免每秒重开文件
            double totalDuration = currentInstruction.cachedDurationSeconds;
            int totalSeconds = static_cast<int>(totalDuration);
            int remainingSeconds = (totalSeconds - static_cast<int>(playedDuration)) > 0 ?
                                 (totalSeconds - static_cast<int>(playedDuration)) : 0;

            swprintf_s(statusText, _countof(statusText),
                L"当前指令: %s (剩余 %d秒 / 总计 %d秒)",
                StringUtil::utf8ToWide(currentInstruction.name).c_str(),
                remainingSeconds,
                totalSeconds);

            hasCurrentInstruction = true;
        }
    }

    if (!hasCurrentInstruction) {
        wcscpy_s(statusText, _countof(statusText), L"下一指令: 无");

        if (m_nextInstructionIndex >= 0 &&
            static_cast<size_t>(m_nextInstructionIndex) < m_instructions.size()) {

            const auto& instruction = m_instructions[m_nextInstructionIndex];

            if (instruction.status == PlaybackStatus::UNPLAYED) {
                auto now = std::chrono::system_clock::now();
                auto nowTime = std::chrono::system_clock::to_time_t(now);
                auto instrTime = std::chrono::system_clock::to_time_t(instruction.playTime);

                int timeDiffMinutes = static_cast<int>((instrTime - nowTime) / 60);

                if (timeDiffMinutes > 0) {
                    swprintf_s(statusText, _countof(statusText),
                        L"下一指令: %s (%d分钟后)",
                        StringUtil::utf8ToWide(instruction.name).c_str(), timeDiffMinutes);
                } else if (timeDiffMinutes == 0) {
                    swprintf_s(statusText, _countof(statusText),
                        L"下一指令: %s (即将播放)",
                        StringUtil::utf8ToWide(instruction.name).c_str());
                } else {
                    swprintf_s(statusText, _countof(statusText),
                        L"下一指令: %s (播放时间已到)",
                        StringUtil::utf8ToWide(instruction.name).c_str());
                }
            }
        }
    }

    if (m_hwndStatusPanel) {
        SetWindowTextW(m_hwndStatusPanel, statusText);
    }
}

void MainWindow::UpdateSubjectList() {
    ListView_DeleteAllItems(m_hwndSubjectList);

    if (m_subjects.empty()) {
        return;
    }

    ListView_SetItemCount(m_hwndSubjectList, static_cast<int>(m_subjects.size()));

    for (size_t i = 0; i < m_subjects.size(); ++i) {
        const auto& subject = m_subjects[i];

        try {
            std::wstring subjectName = StringUtil::utf8ToWide(subject.name);
            std::wstring startTime = StringUtil::utf8ToWide(subject.getStartDateTimeString());
            std::wstring endTime = StringUtil::utf8ToWide(subject.getEndDateTimeString());

            LVITEM lvi = {0};
            lvi.mask = LVIF_TEXT;
            lvi.iItem = static_cast<int>(i);
            lvi.iSubItem = 0;
            lvi.pszText = const_cast<LPWSTR>(subjectName.c_str());

            int itemIndex = ListView_InsertItem(m_hwndSubjectList, &lvi);
            if (itemIndex != -1) {
                ListView_SetItemText(m_hwndSubjectList, itemIndex, 1,
                                   const_cast<LPWSTR>(startTime.c_str()));
                ListView_SetItemText(m_hwndSubjectList, itemIndex, 2,
                                   const_cast<LPWSTR>(endTime.c_str()));
            }
        } catch (const std::exception& e) {
            OutputDebugStringA("UpdateSubjectList error: ");
            OutputDebugStringA(e.what());
            OutputDebugStringA("\n");
            continue;
        } catch (...) {
            OutputDebugStringA("UpdateSubjectList unknown error\n");
            continue;
        }
    }
}

void MainWindow::UpdateInstructionList() {
    ListView_DeleteAllItems(m_hwndInstructionList);

    m_nextInstructionIndex = -1;

    if (m_instructions.empty()) {
        return;
    }

    ListView_SetItemCount(m_hwndInstructionList, static_cast<int>(m_instructions.size()));

    for (size_t i = 0; i < m_instructions.size(); ++i) {
        const auto& instruction = m_instructions[i];

        try {
            std::wstring subjectName = StringUtil::utf8ToWide(instruction.subjectName);
            std::wstring instrName = StringUtil::utf8ToWide(instruction.name);
            std::wstring playTime = StringUtil::utf8ToWide(instruction.getPlayDateTimeString());
            std::wstring status = StringUtil::utf8ToWide(instruction.getStatusString());

            LVITEM lvi = {0};
            lvi.mask = LVIF_TEXT;
            lvi.iItem = static_cast<int>(i);
            lvi.iSubItem = 0;
            lvi.pszText = const_cast<LPWSTR>(subjectName.c_str());

            int itemIndex = ListView_InsertItem(m_hwndInstructionList, &lvi);
            if (itemIndex != -1) {
                ListView_SetItemText(m_hwndInstructionList, itemIndex, 1,
                                   const_cast<LPWSTR>(instrName.c_str()));
                ListView_SetItemText(m_hwndInstructionList, itemIndex, 2,
                                   const_cast<LPWSTR>(playTime.c_str()));
                ListView_SetItemText(m_hwndInstructionList, itemIndex, 3,
                                   const_cast<LPWSTR>(status.c_str()));
            }
        } catch (const std::exception& e) {
            OutputDebugStringA("UpdateInstructionList error: ");
            OutputDebugStringA(e.what());
            OutputDebugStringA("\n");
            continue;
        } catch (...) {
            OutputDebugStringA("UpdateInstructionList unknown error\n");
            continue;
        }
    }

    EnsureInstructionListFocus();
}

void MainWindow::UpdateNextInstruction() {
    // 当前有指令正在播放时，等待播放完成
    if (m_currentPlayingIndex >= 0 &&
        static_cast<size_t>(m_currentPlayingIndex) < m_instructions.size() &&
        m_instructions[m_currentPlayingIndex].status == PlaybackStatus::PLAYING) {
        return;
    }

    // 标记过期超过 60 秒的指令为跳过
    auto now = std::chrono::system_clock::now();
    auto nowTimestamp = std::chrono::duration_cast<std::chrono::seconds>(
        now.time_since_epoch()).count();

    bool hasExpiredInstructions = false;
    for (auto& instruction : m_instructions) {
        if (instruction.status == PlaybackStatus::UNPLAYED) {
            auto instructionTimestamp = std::chrono::duration_cast<std::chrono::seconds>(
                instruction.playTime.time_since_epoch()).count();

            if (instructionTimestamp < nowTimestamp &&
                (nowTimestamp - instructionTimestamp) > 60) {
                instruction.status = PlaybackStatus::SKIPPED;
                hasExpiredInstructions = true;
            }
        }
    }

    if (hasExpiredInstructions) {
        UpdateInstructionListDisplay();
    }

    if (m_nextInstructionIndex < 0 ||
        static_cast<size_t>(m_nextInstructionIndex) >= m_instructions.size() ||
        m_instructions[m_nextInstructionIndex].status != PlaybackStatus::UNPLAYED) {
        SetNextInstruction();
    }

    if (m_nextInstructionIndex >= 0 && IsTimeToPlayNextInstruction()) {
        PlayInstruction(m_nextInstructionIndex, false);
    } else if (m_nextInstructionIndex >= 0) {
        EnsureInstructionListFocus();
    }
}

void MainWindow::HandleSubjectListNotify(LPNMHDR lpnmh) {
    switch (lpnmh->code) {
        case NM_RCLICK: {
            LPNMITEMACTIVATE lpnmitem = (LPNMITEMACTIVATE)lpnmh;
            if (lpnmitem->iItem >= 0 && static_cast<size_t>(lpnmitem->iItem) < m_subjects.size()) {
                ListView_SetItemState(m_hwndSubjectList, lpnmitem->iItem,
                    LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);

                POINT pt;
                GetCursorPos(&pt);
                ShowSubjectContextMenu(pt.x, pt.y, lpnmitem->iItem);
            }
            break;
        }

        case LVN_ITEMCHANGED: {
            LPNMLISTVIEW pnmv = (LPNMLISTVIEW)lpnmh;
            if ((pnmv->uChanged & LVIF_STATE) && (pnmv->uNewState & LVIS_SELECTED)) {
                UpdateInstructionList();
            }
            break;
        }
    }
}

LRESULT MainWindow::HandleInstructionListNotify(LPNMHDR lpnmh) {
    switch (lpnmh->code) {
        case NM_CUSTOMDRAW: {
            LPNMLVCUSTOMDRAW lpCustomDraw = (LPNMLVCUSTOMDRAW)lpnmh;

            switch (lpCustomDraw->nmcd.dwDrawStage) {
                case CDDS_PREPAINT:
                    return CDRF_NOTIFYITEMDRAW;

                case CDDS_ITEMPREPAINT: {
                    int itemIndex = (int)lpCustomDraw->nmcd.dwItemSpec;
                    if (itemIndex >= 0 && static_cast<size_t>(itemIndex) < m_instructions.size()) {
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
                PlayInstruction(lpnmitem->iItem, true);
            }
            break;
        }

        case NM_RCLICK: {
            LPNMITEMACTIVATE lpnmitem = (LPNMITEMACTIVATE)lpnmh;
            if (lpnmitem->iItem >= 0 && static_cast<size_t>(lpnmitem->iItem) < m_instructions.size()) {
                ListView_SetItemState(m_hwndInstructionList, lpnmitem->iItem,
                    LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);

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
    HMENU hMenu = CreatePopupMenu();
    if (hMenu) {
        AppendMenuW(hMenu, MF_STRING, IDM_DELETE_SUBJECT, L"删除科目");

        int command = TrackPopupMenu(hMenu,
            TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD,
            x, y, 0, m_hwnd, NULL);

        if (command == IDM_DELETE_SUBJECT) {
            auto& subject = m_subjects[itemIndex];
            std::wstring subjectName = StringUtil::utf8ToWide(subject.name);

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

        DestroyMenu(hMenu);
    }
}

INT_PTR CALLBACK MainWindow::AddSubjectDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    MainWindow* pMainWindow = nullptr;
    if (msg == WM_INITDIALOG) {
        pMainWindow = reinterpret_cast<MainWindow*>(lParam);
        if (!pMainWindow) {
            EndDialog(hwnd, IDCANCEL);
            return FALSE;
        }
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pMainWindow));
    } else {
        pMainWindow = reinterpret_cast<MainWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        if (!pMainWindow && (msg == WM_COMMAND || msg == WM_NOTIFY)) {
            return FALSE;
        }
    }

    switch (msg) {
        case WM_INITDIALOG: {
            HWND hComboBox = GetDlgItem(hwnd, IDC_SUBJECT_COMBO);
            auto& configManager = ConfigManager::getInstance();
            auto subjects = configManager.getSubjects();
            for (const auto& subject : subjects) {
                wchar_t displayText[128];
                std::wstring wideName = StringUtil::utf8ToWide(subject.name);
                swprintf_s(displayText, _countof(displayText),
                    L"%s (%d分钟)", wideName.c_str(), subject.durationMinutes);
                SendMessageW(hComboBox, CB_ADDSTRING, 0, (LPARAM)displayText);
            }
            SendMessageW(hComboBox, CB_SETCURSEL, 0, 0);

            // 默认日期为今天
            SYSTEMTIME st;
            GetLocalTime(&st);

            wchar_t dateStr[12];
            swprintf_s(dateStr, _countof(dateStr), L"%04d-%02d-%02d", st.wYear, st.wMonth, st.wDay);
            SetDlgItemText(hwnd, IDC_START_DATE_EDIT, dateStr);

            // 默认时间为当前时间的下一个整点
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
                    wchar_t startDate[12] = {0};
                    wchar_t startTime[6] = {0};

                    HWND hComboBox = GetDlgItem(hwnd, IDC_SUBJECT_COMBO);
                    int selectedIndex = static_cast<int>(SendMessageW(hComboBox, CB_GETCURSEL, 0, 0));

                    auto& configManager = ConfigManager::getInstance();
                    auto subjects = configManager.getSubjects();

                    if (selectedIndex == CB_ERR || selectedIndex >= (int)subjects.size()) {
                        MessageBoxW(hwnd, L"请选择科目", L"错误", MB_OK | MB_ICONERROR);
                        return TRUE;
                    }

                    std::wstring wideSubjectName = StringUtil::utf8ToWide(subjects[selectedIndex].name);
                    wcscpy_s(subjectName, wideSubjectName.c_str());

                    if (GetDlgItemTextW(hwnd, IDC_START_DATE_EDIT, startDate, 12) == 0) {
                        MessageBoxW(hwnd, L"请输入考试日期", L"错误", MB_OK | MB_ICONERROR);
                        return TRUE;
                    }

                    if (GetDlgItemTextW(hwnd, IDC_START_TIME_EDIT, startTime, 6) == 0) {
                        MessageBoxW(hwnd, L"请输入开始时间", L"错误", MB_OK | MB_ICONERROR);
                        return TRUE;
                    }

                    try {
                        std::string name = StringUtil::wideToUtf8(std::wstring(subjectName));
                        std::string dateStr = StringUtil::wideToUtf8(std::wstring(startDate));
                        std::string timeStr = StringUtil::wideToUtf8(std::wstring(startTime));

                        if (!Subject::isValidDateTime(dateStr, timeStr)) {
                            MessageBoxW(hwnd, L"请输入正确的日期格式（YYYY-MM-DD）和时间格式（HH:MM）", L"错误", MB_OK | MB_ICONERROR);
                            return TRUE;
                        }

                        Subject subject = Subject::createSubject(name);
                        subject.setStartDateTime(dateStr, timeStr);

                        pMainWindow->m_subjects.push_back(subject);
                        pMainWindow->UpdateSubjectList();

                        auto instructions = Instruction::generateInstructions(subject);
                        pMainWindow->m_instructions.insert(
                            pMainWindow->m_instructions.end(),
                            instructions.begin(),
                            instructions.end());
                        pMainWindow->InvalidateAudioCache();
                        pMainWindow->UpdateInstructionList();
                        pMainWindow->UpdateStatusPanel();

                        EndDialog(hwnd, IDOK);
                        return TRUE;
                    } catch (const std::exception& e) {
                        std::string error = e.what();
                        std::wstring wError = StringUtil::utf8ToWide(error);

                        wchar_t errorMsg[512];
                        swprintf_s(errorMsg, _countof(errorMsg), L"添加科目时发生错误: %s", wError.c_str());
                        MessageBoxW(hwnd, errorMsg, L"错误", MB_OK | MB_ICONERROR);
                        return TRUE;
                    } catch (...) {
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
    // 中文产品名为本地化显示文本，不进 version.h（RC 预处理器无法消化中文）。
    const wchar_t* aboutText =
        L"考试语音指令系统\n"
        L"Examination Voice Command System (EVCS)\n\n"
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

// DPI 相关函数
void MainWindow::UpdateDpiInfo() {
    HDC hdc = GetDC(m_hwnd);
    if (hdc) {
        m_dpi = GetDeviceCaps(hdc, LOGPIXELSX);
        ReleaseDC(m_hwnd, hdc);
    } else {
        m_dpi = 96;
    }
    m_dpiScaleX = m_dpi / 96.0f;
    m_dpiScaleY = m_dpi / 96.0f;

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
    RECT rcClient;
    GetClientRect(m_hwnd, &rcClient);

    RECT rcStatus;
    GetWindowRect(m_hwndStatusBar, &rcStatus);
    int statusHeight = rcStatus.bottom - rcStatus.top;

    if (m_hwndStatusBar) {
        int statusWidths[] = { ScaleX(250), ScaleX(500), -1 };
        SendMessage(m_hwndStatusBar, SB_SETPARTS, 3, (LPARAM)statusWidths);
    }

    if (m_hwndStatusPanel) {
        MoveWindow(m_hwndStatusPanel,
            ScaleX(10), ScaleY(10),
            rcClient.right - ScaleX(20),
            ScaleY(50),
            TRUE);

        // 重建字体以适应新 DPI
        if (m_hStatusPanelFont) {
            DeleteObject(m_hStatusPanelFont);
        }
        m_hStatusPanelFont = CreateFontW(
            ScaleY(16), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, NULL);
        if (m_hStatusPanelFont) {
            SendMessage(m_hwndStatusPanel, WM_SETFONT, (WPARAM)m_hStatusPanelFont, TRUE);
        }
    }

    int availableHeight = rcClient.bottom - statusHeight - ScaleY(100);
    int subjectListHeight = availableHeight / 3;
    int instructionListHeight = availableHeight * 2 / 3 - ScaleY(20);

    MoveWindow(m_hwndSubjectList,
        ScaleX(10), ScaleY(70),
        rcClient.right - ScaleX(20),
        subjectListHeight,
        TRUE);

    MoveWindow(m_hwndInstructionList,
        ScaleX(10), ScaleY(80) + subjectListHeight,
        rcClient.right - ScaleX(20),
        instructionListHeight,
        TRUE);

    if (m_hwndSubjectList) {
        ListView_SetColumnWidth(m_hwndSubjectList, 0, ScaleX(240));
        ListView_SetColumnWidth(m_hwndSubjectList, 1, ScaleX(250));
        ListView_SetColumnWidth(m_hwndSubjectList, 2, ScaleX(250));
    }

    if (m_hwndInstructionList) {
        ListView_SetColumnWidth(m_hwndInstructionList, 0, ScaleX(120));
        ListView_SetColumnWidth(m_hwndInstructionList, 1, ScaleX(260));
        ListView_SetColumnWidth(m_hwndInstructionList, 2, ScaleX(240));
        ListView_SetColumnWidth(m_hwndInstructionList, 3, ScaleX(100));
    }

    SendMessage(m_hwndStatusBar, WM_SIZE, 0, 0);
}

// 指令播放相关方法
void MainWindow::PlayInstruction(int index, bool isManualPlay) {
    if (index < 0 || static_cast<size_t>(index) >= m_instructions.size()) {
        return;
    }

    auto& instruction = m_instructions[index];

    // 过期检查（仅自动播放）
    if (!isManualPlay) {
        auto now = std::chrono::system_clock::now();
        auto nowTimestamp = std::chrono::duration_cast<std::chrono::seconds>(
            now.time_since_epoch()).count();
        auto instructionTimestamp = std::chrono::duration_cast<std::chrono::seconds>(
            instruction.playTime.time_since_epoch()).count();

        if (instructionTimestamp < nowTimestamp &&
            (nowTimestamp - instructionTimestamp) > 60) {
            instruction.status = PlaybackStatus::SKIPPED;
            UpdateInstructionListDisplay();
            SetNextInstruction();
            return;
        }
    }

    if (isManualPlay) {
        MarkPreviousAsSkipped(index);
    }

    // 之前在播放的指令置为已播放
    if (m_currentPlayingIndex >= 0 &&
        static_cast<size_t>(m_currentPlayingIndex) < m_instructions.size()) {
        m_instructions[m_currentPlayingIndex].status = PlaybackStatus::PLAYED;
    }

    // 先尝试播放音频文件
    bool ok = AudioPlayer::playAudioFile(instruction.audioFile);
    if (!ok) {
        // 播放失败：标记已播放，不进入 PLAYING
        instruction.status = PlaybackStatus::PLAYED;
        m_currentPlayingIndex = -1;
        InvalidateAudioCache();
        UpdateInstructionListDisplay();
        // 自动播放失败仅记录日志，避免模态框阻塞定时器消息循环
        // （手动播放才弹窗提示用户）
        if (isManualPlay) {
            MessageBoxW(m_hwnd, L"音频文件播放失败，请检查文件是否存在或格式是否支持。",
                L"播放错误", MB_OK | MB_ICONWARNING);
        } else {
            std::string dbgName = instruction.audioFile;
            OutputDebugStringA("[EVCS] 自动播放失败: ");
            OutputDebugStringA(dbgName.c_str());
            OutputDebugStringA("\n");
        }
        SetNextInstruction();
        return;
    }

    // 播放成功：从当前播放流直接取时长（避免再开一路流），进入 PLAYING
    instruction.cachedDurationSeconds = AudioPlayer::getCurrentStreamDuration();
    instruction.status = PlaybackStatus::PLAYING;
    m_currentPlayingIndex = index;
    m_currentPlayingStartTime = std::chrono::system_clock::now();

    InvalidateAudioCache();
    UpdateInstructionListDisplay();
    EnsureInstructionListFocus();

    if (isManualPlay) {
        m_nextInstructionIndex = FindNextUnplayedInstructionAfter(index);
    }
}

void MainWindow::MarkPreviousAsSkipped(int playIndex) {
    for (size_t i = 0; i < static_cast<size_t>(playIndex) && i < m_instructions.size(); ++i) {
        auto& instruction = m_instructions[i];
        if (instruction.status == PlaybackStatus::UNPLAYED) {
            instruction.status = PlaybackStatus::SKIPPED;
        }
    }
}

void MainWindow::UpdateInstructionListDisplay() {
    UpdateInstructionList();
    UpdateStatusPanel();
}

void MainWindow::EnsureInstructionListFocus() {
    if (m_instructions.empty() || !m_hwndInstructionList) {
        return;
    }

    int focusIndex = -1;

    if (m_currentPlayingIndex >= 0 &&
        static_cast<size_t>(m_currentPlayingIndex) < m_instructions.size()) {
        focusIndex = m_currentPlayingIndex;
    } else if (m_nextInstructionIndex >= 0 &&
             static_cast<size_t>(m_nextInstructionIndex) < m_instructions.size()) {
        focusIndex = m_nextInstructionIndex;
    } else {
        for (size_t i = 0; i < m_instructions.size(); ++i) {
            if (m_instructions[i].status == PlaybackStatus::UNPLAYED) {
                focusIndex = static_cast<int>(i);
                break;
            }
        }
    }

    if (focusIndex >= 0) {
        ListView_SetItemState(m_hwndInstructionList, focusIndex,
                            LVIS_SELECTED | LVIS_FOCUSED,
                            LVIS_SELECTED | LVIS_FOCUSED);
        ListView_EnsureVisible(m_hwndInstructionList, focusIndex, FALSE);
    }
}

int MainWindow::FindNextUnplayedInstruction() const {
    for (size_t i = 0; i < m_instructions.size(); ++i) {
        if (m_instructions[i].status == PlaybackStatus::UNPLAYED) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

int MainWindow::FindNextUnplayedInstructionAfter(int index) const {
    for (size_t i = index + 1; i < m_instructions.size(); ++i) {
        if (m_instructions[i].status == PlaybackStatus::UNPLAYED) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void MainWindow::SetNextInstruction() {
    m_nextInstructionIndex = FindNextUnplayedInstruction();
}

bool MainWindow::IsTimeToPlayNextInstruction() const {
    if (m_nextInstructionIndex < 0 ||
        static_cast<size_t>(m_nextInstructionIndex) >= m_instructions.size()) {
        return false;
    }

    const auto& instruction = m_instructions[m_nextInstructionIndex];

    if (instruction.status != PlaybackStatus::UNPLAYED) {
        return false;
    }

    auto now = std::chrono::system_clock::now();
    auto nowTimestamp = std::chrono::duration_cast<std::chrono::seconds>(
        now.time_since_epoch()).count();
    auto instructionTimestamp = std::chrono::duration_cast<std::chrono::seconds>(
        instruction.playTime.time_since_epoch()).count();

    if (instructionTimestamp < nowTimestamp &&
        (nowTimestamp - instructionTimestamp) > 60) {
        return false;
    }

    return instructionTimestamp <= nowTimestamp;
}

void MainWindow::ShowInstructionContextMenu(int x, int y, int itemIndex) {
    HMENU hMenu = CreatePopupMenu();
    if (hMenu) {
        AppendMenuW(hMenu, MF_STRING, IDM_PLAY_INSTRUCTION, L"立即播放");

        int command = TrackPopupMenu(hMenu,
            TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD,
            x, y, 0, m_hwnd, NULL);

        if (command == IDM_PLAY_INSTRUCTION) {
            PlayInstruction(itemIndex, true);
        }

        DestroyMenu(hMenu);
    }
}

void MainWindow::CheckPlaybackCompletion() {
    if (m_currentPlayingIndex < 0 ||
        static_cast<size_t>(m_currentPlayingIndex) >= m_instructions.size()) {
        return;
    }

    auto& currentInstruction = m_instructions[m_currentPlayingIndex];

    if (currentInstruction.status != PlaybackStatus::PLAYING) {
        return;
    }

    // 基于 BASS 通道活跃状态判断播放是否结束（推荐做法，与真实音频输出严格对齐）
    if (!AudioPlayer::isPlaying()) {
        currentInstruction.status = PlaybackStatus::PLAYED;
        m_currentPlayingIndex = -1;

        SetNextInstruction();
        UpdateInstructionListDisplay();
        EnsureInstructionListFocus();
    }
}

void MainWindow::LoadConfigFile() {
    OPENFILENAMEW ofn;
    wchar_t szFile[260] = {0};
    wchar_t configDir[MAX_PATH] = {0};

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = m_hwnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = _countof(szFile);
    ofn.lpstrFilter = L"INI Files\0*.ini\0All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;

    // 定位到程序所在目录下的 config 子目录
    GetModuleFileNameW(NULL, configDir, MAX_PATH);
    wchar_t* lastSlash = wcsrchr(configDir, L'\\');
    if (lastSlash) {
        *(lastSlash + 1) = L'\0';
        wcscat_s(configDir, L"config");
    }

    ofn.lpstrInitialDir = configDir;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

    if (GetOpenFileNameW(&ofn)) {
        auto& configManager = ConfigManager::getInstance();
        if (configManager.loadConfig(szFile)) {
            RegenerateInstructions();

            std::wstring message = L"配置文件加载成功！\n\n";
            message += L"配置文件：";
            message += szFile;
            MessageBoxW(m_hwnd, message.c_str(), L"加载成功", MB_OK | MB_ICONINFORMATION);
        } else {
            std::wstring message = L"配置文件加载失败！\n\n";
            message += L"文件：";
            message += szFile;
            message += L"\n\n请检查文件格式是否正确。";
            MessageBoxW(m_hwnd, message.c_str(), L"加载失败", MB_OK | MB_ICONERROR);
        }
    }
}

void MainWindow::ReloadConfigFile() {
    auto& configManager = ConfigManager::getInstance();
    std::wstring currentConfigPath = configManager.getCurrentConfigPath();

    if (currentConfigPath.empty()) {
        if (configManager.loadDefaultConfig()) {
            currentConfigPath = configManager.getCurrentConfigPath();
        } else {
            MessageBoxW(m_hwnd, L"无法加载默认配置文件！", L"重新加载失败", MB_OK | MB_ICONERROR);
            return;
        }
    } else {
        if (!configManager.loadConfig(currentConfigPath)) {
            std::wstring message = L"重新加载配置文件失败！\n\n";
            message += L"文件：";
            message += currentConfigPath;
            MessageBoxW(m_hwnd, message.c_str(), L"重新加载失败", MB_OK | MB_ICONERROR);
            return;
        }
    }

    RegenerateInstructions();

    std::wstring message = L"配置文件重新加载成功！\n\n";
    message += L"配置文件：";
    message += currentConfigPath;
    MessageBoxW(m_hwnd, message.c_str(), L"重新加载成功", MB_OK | MB_ICONINFORMATION);
}

// 使音频文件状态缓存失效（科目/指令变动后调用）
void MainWindow::InvalidateAudioCache() {
    m_cachedMissingInstructionCount = -1;
}

// 根据当前科目与配置重生成指令列表，按播放时间排序并重置播放状态
void MainWindow::RegenerateInstructions() {
    if (m_currentPlayingIndex >= 0) {
        AudioPlayer::stop();
    }

    m_instructions.clear();
    for (const auto& subject : m_subjects) {
        auto subjectInstructions = Instruction::generateInstructions(subject);
        m_instructions.insert(m_instructions.end(),
            subjectInstructions.begin(), subjectInstructions.end());
    }

    std::sort(m_instructions.begin(), m_instructions.end(),
        [](const Instruction& a, const Instruction& b) {
            return a.playTime < b.playTime;
        });

    m_currentPlayingIndex = -1;
    m_nextInstructionIndex = -1;

    InvalidateAudioCache();
    UpdateInstructionList();
    UpdateStatusPanel();
}
