#pragma once
#include <string>
#include <chrono>

struct Subject {
    std::string name;
    int durationMinutes;
    bool isDoubleSession;
    std::chrono::system_clock::time_point startTime;

    static Subject createSubject(const std::string& name);
    static bool isValidStartTime(const std::string& timeStr);
    void setStartTime(const std::string& timeStr);
    std::string getStartTimeString() const;
    std::string getEndTimeString() const;
};
