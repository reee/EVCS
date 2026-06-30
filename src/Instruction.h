#pragma once
#include <string>
#include <vector>
#include <chrono>
#include <windows.h>  // COLORREF
#include "Subject.h"
#include <filesystem>

// 指令播放状态枚举
enum class PlaybackStatus {
    UNPLAYED,   // 未播放（默认状态）
    PLAYING,    // 正在播放
    PLAYED,     // 已播放
    SKIPPED     // 已跳过
};

struct Instruction {
    int subjectId;
    std::string subjectName;  // 显示用科目名称
    std::string name;
    std::chrono::system_clock::time_point playTime;
    std::string audioFile;
    PlaybackStatus status;

    // 缓存的音频时长（秒）。<=0 表示未取或取失败
    mutable double cachedDurationSeconds = 0.0;

    Instruction() : subjectId(0), status(PlaybackStatus::UNPLAYED) {}

    static std::vector<Instruction> generateInstructions(const Subject& subject);
    std::string getPlayDateTimeString() const;
    bool checkAudioFileExists() const;

    COLORREF getStatusTextColor() const;
    std::string getStatusString() const;
};
