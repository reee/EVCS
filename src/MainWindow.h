#pragma once
#include <windows.h>
#include <commctrl.h>
#include <vector>
#include <chrono>
#include "Subject.h"
#include "Instruction.h"
#include "resource.h"

class MainWindow {
public:
    MainWindow();
    ~MainWindow();

    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    bool Create();
    void Show(int nCmdShow);

private:
    HWND m_hwnd;
    HWND m_hwndStatusBar;
    HWND m_hwndStatusPanel;
    HFONT m_hStatusPanelFont;
    HWND m_hwndSubjectList;
    HWND m_hwndInstructionList;

    std::vector<Subject> m_subjects;
    std::vector<Instruction> m_instructions;

    // DPI 相关成员
    UINT m_dpi;
    float m_dpiScaleX;
    float m_dpiScaleY;

    // 指令列表状态相关
    int m_currentPlayingIndex;  // 当前播放的指令索引，-1 表示无
    int m_nextInstructionIndex; // 下一个要播放的指令索引，-1 表示无
    std::chrono::system_clock::time_point m_currentPlayingStartTime;

    // 音频文件状态缓存（避免每秒全量扫描文件系统）
    int m_cachedMissingInstructionCount = -1;  // <0 表示缓存失效

    // 系统音量节流（避免每秒做 COM 设备枚举）
    int m_cachedSystemVolume = 0;
    std::chrono::steady_clock::time_point m_lastVolumeCheck;
    static constexpr int VOLUME_REFRESH_SECONDS = 5;

    // 「文件存在」列周期补刷节流（避免每秒全量扫描文件系统）
    std::chrono::steady_clock::time_point m_lastFileExistRefresh;
    static constexpr int FILE_EXIST_REFRESH_SECONDS = 5;
    void RefreshFileExistColumn();

    void CreateControls();
    void AddSubject();
    void DeleteSubject(int index);
    void UpdateSubjectList();
    void UpdateInstructionList();
    void HandleSubjectListNotify(LPNMHDR lpnmh);
    LRESULT HandleInstructionListNotify(LPNMHDR lpnmh);
    void ShowSubjectContextMenu(int x, int y, int itemIndex);
    void ShowInstructionContextMenu(int x, int y, int itemIndex);
    void UpdateNextInstruction();
    void UpdateStatusBar();
    void UpdateStatusPanel();
    void CheckPlaybackCompletion();

    // 菜单相关方法
    void ShowHelp();
    void ShowAbout();
    void LoadConfigFile();
    void ReloadConfigFile();
    void InvalidateAudioCache();  // 科目/指令变动时调用，使音频文件状态缓存失效
    void RegenerateInstructions();  // 根据当前科目与配置重生成并排序指令列表

    // 指令播放相关方法
    void PlayInstruction(int index, bool isManualPlay = false);
    void MarkPreviousAsSkipped(int playIndex);
    void UpdateInstructionListDisplay();
    void EnsureInstructionListFocus();
    int FindNextUnplayedInstruction() const;
    int FindNextUnplayedInstructionAfter(int index) const;
    void SetNextInstruction();
    bool IsTimeToPlayNextInstruction() const;

    // DPI 相关函数
    void UpdateDpiInfo();
    int ScaleX(int x) const;
    int ScaleY(int y) const;
    void UpdateLayoutForDpi();

    // 辅助函数
    static constexpr int TIMER_ID = 1;
    static constexpr int TIMER_INTERVAL = 1000;  // 1 秒

    // 对话框过程
    static INT_PTR CALLBACK AddSubjectDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};
