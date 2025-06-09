#pragma once
#include <string>
#include <vector>
#include <chrono>
#include <windows.h>  // 添加此行以支持COLORREF类型
#include "Subject.h"

// 指令播放状态枚举
enum class PlaybackStatus {
    UNPLAYED,   // 未播放（默认状态，白色背景）
    PLAYING,    // 正在播放（高亮绿色背景）
    PLAYED,     // 已播放（浅绿色背景）
    SKIPPED     // 已跳过（浅灰色背景）
};

struct Instruction {
    std::string subjectName;
    std::string name;
    std::chrono::system_clock::time_point playTime;
    std::string audioFile;
    PlaybackStatus status;  // 添加播放状态字段

    // 构造函数，默认状态为未播放
    Instruction() : status(PlaybackStatus::UNPLAYED) {}

    static std::vector<Instruction> generateInstructions(const Subject& subject);
    std::string getPlayTimeString() const;
    bool shouldPlayNow() const;
    
    // 新增方法：获取状态对应的背景颜色
    COLORREF getStatusBackgroundColor() const;
    COLORREF getStatusTextColor() const;  // 新增：获取状态对应的文本颜色
    std::string getStatusString() const;
};
