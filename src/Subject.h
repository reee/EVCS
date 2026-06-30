#pragma once
#include <string>
#include <vector>
#include <chrono>

struct Subject {
    static int nextId;  // 静态计数器，用于生成唯一 ID

    // 配置缺失时的默认时长（分钟）。单一来源。
    static constexpr int DEFAULT_DURATION_MINUTES = 90;

    int id;  // 唯一标识符
    std::string name;
    int durationMinutes;
    std::chrono::system_clock::time_point startTime;

    Subject() : id(++nextId) {}

    static Subject createSubject(const std::string& name);
    static std::vector<std::string> getAvailableSubjects();
    static bool isValidStartTime(const std::string& timeStr);
    static bool isValidDateTime(const std::string& dateStr, const std::string& timeStr);
    void setStartTime(const std::string& timeStr);
    void setStartDateTime(const std::string& dateStr, const std::string& timeStr);
    std::string getStartDateTimeString() const;
    std::string getEndDateTimeString() const;
};
