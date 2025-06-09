#pragma once
#include <string>
#include <vector>
#include <chrono>
#include <windows.h>  // 添加此行以支持COLORREF类型
#include "Subject.h"

// 指令播放状态枚举
enum class PlaybackStatus {
    UNPLAYED,   // 未播放（默认状态）
    PLAYING,    // 正在播放
    PLAYED,     // 已播放
    SKIPPED     // 已跳过
};

struct Instruction {
    std::string subjectName;
    std::string name;
    std::chrono::system_clock::time_point playTime;
    std::string audioFile;
    PlaybackStatus status;  // 添加播放状态字段

    // 构造函数，默认状态为未播放
    Instruction() : status(PlaybackStatus::UNPLAYED) {}    static std::vector<Instruction> generateInstructions(const Subject& subject);
    std::string getPlayTimeString() const;
    bool shouldPlayNow() const;
    
    COLORREF getStatusTextColor() const;  // 获取状态对应的文本颜色
    std::string getStatusString() const;
};
