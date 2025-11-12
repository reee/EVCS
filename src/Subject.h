#pragma once
#include <string>
#include <vector>
#include <chrono>

struct Subject {
    static int nextId;  // 静态计数器，用于生成唯一ID

    int id;  // 唯一标识符
    std::string name;
    int durationMinutes;
    std::chrono::system_clock::time_point startTime;

    // 构造函数
    Subject() : id(++nextId) {}

    static Subject createSubject(const std::string& name);
    static std::vector<std::string> getAvailableSubjects();
    static bool isValidStartTime(const std::string& timeStr);
    static bool isValidDateTime(const std::string& dateStr, const std::string& timeStr);
    void setStartTime(const std::string& timeStr);
    void setStartDateTime(const std::string& dateStr, const std::string& timeStr);
    std::string getStartTimeString() const;
    std::string getStartDateTimeString() const;
    std::string getEndTimeString() const;
    std::string getEndDateTimeString() const;
};
