#include "Instruction.h"
#include "ConfigManager.h"
#include "PathUtil.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <windows.h>
#include <filesystem>

std::vector<Instruction> Instruction::generateInstructions(const Subject& subject) {
    std::vector<Instruction> instructions;

    auto& configManager = ConfigManager::getInstance();
    auto templates = configManager.getInstructionTemplates(subject.name);

    for (const auto& temp : templates) {
        Instruction instr;
        instr.subjectId = subject.id;
        instr.subjectName = subject.name;
        instr.name = temp.name;  // UTF-8 直接使用，无需往返转换
        instr.playTime = subject.startTime + std::chrono::seconds(temp.offsetSeconds);
        instr.audioFile = temp.audioFile;
        instructions.push_back(instr);
    }

    return instructions;
}

std::string Instruction::getPlayDateTimeString() const {
    auto time = std::chrono::system_clock::to_time_t(playTime);
    std::tm tm = {};
    localtime_s(&tm, &time);
    std::stringstream ss;
    ss << (tm.tm_year + 1900) << "-"
       << std::setfill('0') << std::setw(2) << (tm.tm_mon + 1) << "-"
       << std::setfill('0') << std::setw(2) << tm.tm_mday << " "
       << std::setfill('0') << std::setw(2) << tm.tm_hour << ":"
       << std::setfill('0') << std::setw(2) << tm.tm_min;
    return ss.str();
}

// 实时检查音频文件是否存在（不使用缓存）
bool Instruction::checkAudioFileExists() const {
    return std::filesystem::exists(PathUtil::getAudioPath(audioFile));
}

COLORREF Instruction::getStatusTextColor() const {
    switch (status) {
        case PlaybackStatus::UNPLAYED:
            return RGB(0, 0, 0);        // 黑色
        case PlaybackStatus::PLAYING:
            return RGB(0, 100, 0);      // 深绿色（高亮）
        case PlaybackStatus::PLAYED:
            return RGB(34, 139, 34);    // 森林绿
        case PlaybackStatus::SKIPPED:
            return RGB(128, 128, 128);  // 灰色
        default:
            return RGB(0, 0, 0);
    }
}

std::string Instruction::getStatusString() const {
    switch (status) {
        case PlaybackStatus::UNPLAYED:
            return "未播放";
        case PlaybackStatus::PLAYING:
            return "播放中";
        case PlaybackStatus::PLAYED:
            return "已播放";
        case PlaybackStatus::SKIPPED:
            return "已跳过";
        default:
            return "未知";
    }
}
