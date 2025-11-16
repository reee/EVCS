#include "Instruction.h"
#include "ConfigManager.h"
#include "StringUtil.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <windows.h>
#include <filesystem> // 添加此行

namespace {
    struct LegacyInstructionTemplate {
        int offsetSeconds;  // 改为秒为单位，支持更精确的时间控制
        std::wstring name;
        std::string file;
    };    // 添加指令到列表的辅助函数
    void AddInstruction(std::vector<Instruction>& instructions,
                       const Subject& subject,
                       const LegacyInstructionTemplate& temp) {
        Instruction instr;
        instr.subjectId = subject.id;  // 设置科目ID
        instr.subjectName = subject.name;  // 保留科目名称用于显示
        instr.name = StringUtil::wideToUtf8(temp.name);
        instr.playTime = subject.startTime + std::chrono::seconds(temp.offsetSeconds);
        instr.audioFile = temp.file;
        instructions.push_back(instr);
    }
}

std::vector<Instruction> Instruction::generateInstructions(const Subject& subject) {
    std::vector<Instruction> instructions;

    // 从配置管理器获取指令模板
    auto& configManager = ConfigManager::getInstance();
    auto templates = configManager.getInstructionTemplates(subject.name);

    for (const auto& temp : templates) {
        // 转换为宽字符串
        std::wstring wideName = StringUtil::utf8ToWide(temp.name);

        // 创建内部模板结构
        LegacyInstructionTemplate internalTemp;
        internalTemp.offsetSeconds = temp.offsetSeconds;
        internalTemp.name = wideName;
        internalTemp.file = temp.audioFile;

        AddInstruction(instructions, subject, internalTemp);
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

bool Instruction::shouldPlayNow() const {    auto now = std::chrono::system_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::seconds>(now - playTime).count();
    return diff >= 0 && diff < 60;  // 在当前分钟内
}

// 新增方法：检查音频文件是否存在（实时检查，不使用缓存）
bool Instruction::checkAudioFileExists() const {
    // 获取可执行文件所在目录
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    std::filesystem::path exeDir = std::filesystem::path(exePath).parent_path();
    std::filesystem::path filePath = exeDir / "audio" / audioFile;
    return std::filesystem::exists(filePath);
}

COLORREF Instruction::getStatusTextColor() const {
    switch (status) {
        case PlaybackStatus::UNPLAYED:
            return RGB(0, 0, 0);        // 黑色文字
        case PlaybackStatus::PLAYING:
            return RGB(0, 100, 0);      // 深绿色文字（高亮状态）
        case PlaybackStatus::PLAYED:
            return RGB(34, 139, 34);    // 森林绿文字
        case PlaybackStatus::SKIPPED:
            return RGB(128, 128, 128);  // 灰色文字
        default:
            return RGB(0, 0, 0);        // 默认黑色
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
