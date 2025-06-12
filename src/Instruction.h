#pragma once
#include <string>
#include <vector>
#include <chrono>
#include <windows.h>  // 添加此行以支持COLORREF类型
#include "Subject.h"
#include <filesystem> // 添加此行以支持 std::filesystem::exists

// 指令播放状态枚举
enum class PlaybackStatus {
    UNPLAYED,   // 未播放（默认状态）
    PLAYING,    // 正在播放
    PLAYED,     // 已播放
    SKIPPED     // 已跳过
};

struct Instruction {
    int subjectId;  // 改为使用科目ID而不是科目名称
    std::string subjectName;  // 保留科目名称用于显示
    std::string name;
    std::chrono::system_clock::time_point playTime;
    std::string audioFile;
    PlaybackStatus status;  // 添加播放状态字段
    mutable bool audioFileExists; // 缓存音频文件是否存在状态
    mutable bool audioFileStatusChecked; // 标记是否已检查过音频文件状态

    // 构造函数，默认状态为未播放
    Instruction() : subjectId(0), status(PlaybackStatus::UNPLAYED), audioFileExists(false), audioFileStatusChecked(false) {}    static std::vector<Instruction> generateInstructions(const Subject& subject);
    std::string getPlayDateTimeString() const;
    bool shouldPlayNow() const;
    bool checkAudioFileExists() const; // 检查音频文件是否存在的方法
    
    COLORREF getStatusTextColor() const;  // 获取状态对应的文本颜色
    std::string getStatusString() const;
};
