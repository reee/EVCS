#include "Subject.h"
#include <map>
#include <sstream>
#include <iomanip>

// 初始化静态ID计数器
int Subject::nextId = 0;

// 预设科目信息
static const std::map<std::string, std::pair<int, bool>> subjectPresets = {
    {"语文", {150, false}},
    {"数学", {120, false}},
    {"英语", {120, false}},
    {"一上单科", {75, false}},
    {"首选科目", {75, false}},
    {"再选合堂", {160, true}}
};

Subject Subject::createSubject(const std::string& name) {
    Subject subject;
    subject.name = name;
    subject.durationMinutes = 90;  // 默认90分钟
    subject.isDoubleSession = false;  // 默认单场考试
    
    auto it = subjectPresets.find(name);
    if (it != subjectPresets.end()) {
        subject.durationMinutes = it->second.first;
        subject.isDoubleSession = it->second.second;
    }
    
    return subject;
}

bool Subject::isValidStartTime(const std::string& timeStr) {
    if (timeStr.length() != 5) return false;
    if (timeStr[2] != ':') return false;
    
    try {
        int hours = std::stoi(timeStr.substr(0, 2));
        int minutes = std::stoi(timeStr.substr(3, 2));
        return hours >= 0 && hours < 24 && minutes >= 0 && minutes < 60;
    } catch (...) {
        return false;
    }
}

void Subject::setStartTime(const std::string& timeStr) {
    if (!isValidStartTime(timeStr)) {
        throw std::invalid_argument("Invalid time format");
    }

    std::tm tm = {};
    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    tm = *std::localtime(&now);
    
    int hours = std::stoi(timeStr.substr(0, 2));
    int minutes = std::stoi(timeStr.substr(3, 2));
    
    tm.tm_hour = hours;
    tm.tm_min = minutes;
    tm.tm_sec = 0;
    
    startTime = std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

std::string Subject::getStartTimeString() const {
    auto time = std::chrono::system_clock::to_time_t(startTime);
    std::tm tm = {};
    localtime_s(&tm, &time);
    std::stringstream ss;
    ss << std::setfill('0') << std::setw(2) << tm.tm_hour << ":"
       << std::setfill('0') << std::setw(2) << tm.tm_min;
    return ss.str();
}

std::string Subject::getEndTimeString() const {
    auto endTime = startTime + std::chrono::minutes(durationMinutes);
    auto time = std::chrono::system_clock::to_time_t(endTime);
    std::tm tm = {};
    localtime_s(&tm, &time);
    std::stringstream ss;
    ss << std::setfill('0') << std::setw(2) << tm.tm_hour << ":"
       << std::setfill('0') << std::setw(2) << tm.tm_min;
    return ss.str();
}
