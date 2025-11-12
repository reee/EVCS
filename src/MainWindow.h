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
    HWND m_hwndStatusPanel;  // 新增：状态面板
    HFONT m_hStatusPanelFont;  // 新增：状态面板字体句柄
    HWND m_hwndSubjectList;
    HWND m_hwndInstructionList;    std::vector<Subject> m_subjects;
    std::vector<Instruction> m_instructions;
    
    // DPI相关成员
    UINT m_dpi;
    float m_dpiScaleX;
    float m_dpiScaleY;
    
    // 指令列表状态相关
    int m_currentPlayingIndex;  // 当前播放的指令索引，-1表示没有播放
    int m_nextInstructionIndex; // 下一个要播放的指令索引，-1表示没有下一个指令
    std::chrono::system_clock::time_point m_currentPlayingStartTime; // 当前播放指令的开始时间

    void CreateControls();
    void AddSubject();
    void DeleteSubject(int index);
    void UpdateSubjectList();
    void UpdateInstructionList();
    void HandleSubjectListNotify(LPNMHDR lpnmh);
    LRESULT HandleInstructionListNotify(LPNMHDR lpnmh);
    void ShowSubjectContextMenu(int x, int y, int itemIndex);
    void ShowInstructionContextMenu(int x, int y, int itemIndex);  // 新增：指令右键菜单
    void UpdateNextInstruction();
    void UpdateStatusBar();
    void UpdateStatusPanel();  // 新增：更新状态面板
    void CheckPlaybackCompletion();  // 新增：检查播放完成状态
    
    // 菜单相关方法
    void ShowHelp();     // 显示帮助信息
    void ShowAbout();    // 显示关于对话框
    void LoadConfigFile();     // 加载配置文件
    void ReloadConfigFile();   // 重新加载配置文件
    
    // 指令播放相关方法
    void PlayInstruction(int index, bool isManualPlay = false);  // 播放指定指令
    void MarkPreviousAsSkipped(int playIndex);  // 标记之前未播放的指令为跳过
    void UpdateInstructionListDisplay();  // 更新指令列表的显示
    void EnsureInstructionListFocus();  // 确保焦点跟随当前播放/即将播放的指令
    int FindNextUnplayedInstruction() const;  // 查找下一个未播放的指令
    int FindNextUnplayedInstructionAfter(int index) const;  // 查找指定索引之后的下一个未播放指令
    void SetNextInstruction();  // 设置下一个要播放的指令
    bool IsTimeToPlayNextInstruction() const;  // 检查是否到时间播放下一个指令
    
    // DPI相关函数
    void UpdateDpiInfo();
    int ScaleX(int x) const;
    int ScaleY(int y) const;
    void UpdateLayoutForDpi();
    
    // 辅助函数
    static std::wstring ConvertUtf8ToWide(const std::string& utf8Str);
    
    static constexpr int TIMER_ID = 1;
    static constexpr int TIMER_INTERVAL = 1000;  // 1秒
    
    // 对话框过程
    static INT_PTR CALLBACK AddSubjectDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};
