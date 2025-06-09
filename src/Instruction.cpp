#include "Instruction.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <windows.h>

namespace {
    struct InstructionTemplate {
        int offsetMinutes;
        const wchar_t* name;
        const char* file;
    };    // 辅助函数：将wstring转换为string
    std::string WideToUtf8(const wchar_t* wstr) {
        if (!wstr) return std::string();
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
        if (size_needed <= 1) return std::string();  // 只有null终止符或转换失败
        std::string strTo(size_needed - 1, 0);  // 减1去掉null终止符
        WideCharToMultiByte(CP_UTF8, 0, wstr, -1, &strTo[0], size_needed, NULL, NULL);
        return strTo;
    }

    // 添加指令到列表的辅助函数
    void AddInstruction(std::vector<Instruction>& instructions,
                       const Subject& subject,
                       const InstructionTemplate& temp) {
        Instruction instr;
        instr.subjectName = subject.name;
        instr.name = WideToUtf8(temp.name);
        instr.playTime = subject.startTime + std::chrono::minutes(temp.offsetMinutes);
        instr.audioFile = temp.file;
        instructions.push_back(instr);
    }
}

std::vector<Instruction> Instruction::generateInstructions(const Subject& subject) {
    std::vector<Instruction> instructions;
    
    if (!subject.isDoubleSession) {
        // 常规考试指令
        static const InstructionTemplate templates[] = {
            {-12, L"考前12分钟", "1kq12.wav"},
            {-10, L"考前10分钟", "2kq10.wav"},
            {-5,  L"考前5分钟", "3kq5.wav"},
            {0,   L"开始考试", "4ksks.wav"},
            {subject.durationMinutes - 15, L"结束前15分钟", "5jsq15.wav"},
            {subject.durationMinutes, L"考试结束", "6ksjs.wav"}
        };
        
        for (const auto& temp : templates) {
            AddInstruction(instructions, subject, temp);
        }
    } else {
        // 合堂考试指令
        static const InstructionTemplate templates[] = {
            {-12, L"第一堂考前12分钟", "1kq12.wav"},
            {-10, L"第一堂考前10分钟", "2kq10.wav"},
            {-5,  L"第一堂考前5分钟", "3kq5.wav"},
            {0,   L"第一堂开始考试", "4ksks.wav"},
            {60,  L"第一堂结束前15分钟", "5jsq15.wav"},
            {75,  L"第一堂考试结束", "6ksjs.wav"},
            {80,  L"第二堂考前5分钟", "3kq5.wav"},
            {85,  L"第二堂开始考试", "4ksks.wav"},
            {subject.durationMinutes - 15, L"第二堂结束前15分钟", "5jsq15.wav"},
            {subject.durationMinutes, L"第二堂考试结束", "6ksjs.wav"}
        };
        
        for (const auto& temp : templates) {
            AddInstruction(instructions, subject, temp);
        }
    }
    
    return instructions;
}

std::string Instruction::getPlayTimeString() const {
    auto time = std::chrono::system_clock::to_time_t(playTime);
    std::tm tm = {};
    localtime_s(&tm, &time);
    std::stringstream ss;
    ss << std::setfill('0') << std::setw(2) << tm.tm_hour << ":"
       << std::setfill('0') << std::setw(2) << tm.tm_min;
    return ss.str();
}

bool Instruction::shouldPlayNow() const {    auto now = std::chrono::system_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::seconds>(now - playTime).count();
    return diff >= 0 && diff < 60;  // 在当前分钟内
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
