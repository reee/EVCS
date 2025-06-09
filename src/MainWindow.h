#pragma once
#include <windows.h>
#include <commctrl.h>
#include <vector>
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
    HWND m_hwndStatusBar;    HWND m_hwndSubjectList;
    HWND m_hwndInstructionList;
    std::vector<Subject> m_subjects;
    std::vector<Instruction> m_instructions;
      // DPI相关成员
    UINT m_dpi;
    float m_dpiScaleX;
    float m_dpiScaleY;
      // 指令列表状态相关
    int m_currentPlayingIndex;  // 当前播放的指令索引，-1表示没有播放
    int m_nextInstructionIndex; // 下一个要播放的指令索引，-1表示没有下一个指令

    void CreateControls();
    void AddSubject();
    void DeleteSubject(int index);    void UpdateSubjectList();
    void UpdateInstructionList();    void HandleSubjectListNotify(LPNMHDR lpnmh);
    LRESULT HandleInstructionListNotify(LPNMHDR lpnmh);
    void ShowSubjectContextMenu(int x, int y, int itemIndex);
    void ShowInstructionContextMenu(int x, int y, int itemIndex);  // 新增：指令右键菜单
    void UpdateNextInstruction();
    void UpdateStatusBar();    // 指令播放相关方法
    void PlayInstruction(int index, bool isManualPlay = false);  // 播放指定指令
    void MarkPreviousAsSkipped(int playIndex);  // 标记之前未播放的指令为跳过
    void UpdateInstructionListDisplay();  // 更新指令列表的显示
    int FindNextUnplayedInstruction() const;  // 查找下一个未播放的指令
    int FindNextUnplayedInstructionAfter(int index) const;  // 查找指定索引之后的下一个未播放指令
    void SetNextInstruction();  // 设置下一个要播放的指令
    bool IsTimeToPlayNextInstruction() const;  // 检查是否到时间播放下一个指令
      // DPI相关函数
    void UpdateDpiInfo();
    int ScaleX(int x) const;
    int ScaleY(int y) const;
    void UpdateLayoutForDpi();    // 字体函数（使用系统默认字体）
    void CreateFontAndBrushes();  // 保留以兼容DPI变化处理
    void DestroyFontAndBrushes(); // 保留以兼容现有代码
    void ApplyFontToControl(HWND hwnd);  // 应用系统默认字体
    void ApplyDefaultFontToButton(HWND hwnd);  // 为按钮应用默认系统字体
    
    static constexpr int TIMER_ID = 1;
    static constexpr int TIMER_INTERVAL = 1000;  // 1秒
    
    // 对话框过程
    static INT_PTR CALLBACK AddSubjectDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};
