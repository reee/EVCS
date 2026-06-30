#include "Subject.h"
#include "ConfigManager.h"
#include <map>
#include <sstream>
#include <iomanip>

// 初始化静态 ID 计数器
int Subject::nextId = 0;

Subject Subject::createSubject(const std::string& name) {
    Subject subject;
    subject.name = name;

    auto& configManager = ConfigManager::getInstance();
    SubjectConfig config = configManager.getSubjectConfig(name);

    if (!config.name.empty()) {
        subject.durationMinutes = config.durationMinutes;
    } else {
        subject.durationMinutes = DEFAULT_DURATION_MINUTES;
    }

    return subject;
}

std::vector<std::string> Subject::getAvailableSubjects() {
    auto& configManager = ConfigManager::getInstance();
    return configManager.getSubjectNames();
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

bool Subject::isValidDateTime(const std::string& dateStr, const std::string& timeStr) {
    if (!isValidStartTime(timeStr)) return false;

    if (dateStr.length() != 10) return false;
    if (dateStr[4] != '-' || dateStr[7] != '-') return false;

    try {
        int year = std::stoi(dateStr.substr(0, 4));
        int month = std::stoi(dateStr.substr(5, 2));
        int day = std::stoi(dateStr.substr(8, 2));

        return year >= 2000 && year <= 2100 &&
               month >= 1 && month <= 12 &&
               day >= 1 && day <= 31;
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
    localtime_s(&tm, &now);

    int hours = std::stoi(timeStr.substr(0, 2));
    int minutes = std::stoi(timeStr.substr(3, 2));

    tm.tm_hour = hours;
    tm.tm_min = minutes;
    tm.tm_sec = 0;

    startTime = std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

void Subject::setStartDateTime(const std::string& dateStr, const std::string& timeStr) {
    if (!isValidDateTime(dateStr, timeStr)) {
        throw std::invalid_argument("Invalid date/time format");
    }

    std::tm tm = {};

    int year = std::stoi(dateStr.substr(0, 4));
    int month = std::stoi(dateStr.substr(5, 2));
    int day = std::stoi(dateStr.substr(8, 2));

    int hours = std::stoi(timeStr.substr(0, 2));
    int minutes = std::stoi(timeStr.substr(3, 2));

    tm.tm_year = year - 1900;
    tm.tm_mon = month - 1;
    tm.tm_mday = day;
    tm.tm_hour = hours;
    tm.tm_min = minutes;
    tm.tm_sec = 0;
    tm.tm_isdst = -1;

    startTime = std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

std::string Subject::getStartDateTimeString() const {
    auto time = std::chrono::system_clock::to_time_t(startTime);
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

std::string Subject::getEndDateTimeString() const {
    auto endTime = startTime + std::chrono::minutes(durationMinutes);
    auto time = std::chrono::system_clock::to_time_t(endTime);
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
