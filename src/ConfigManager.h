#pragma once
#include <string>
#include <map>
#include <vector>

struct SubjectConfig {
    std::string name;
    int durationMinutes;
};

struct InstructionTemplate {
    int offsetSeconds;
    std::string name;
    std::string audioFile;
};

struct SubjectFullConfig {
    SubjectConfig subjectInfo;
    std::vector<InstructionTemplate> instructions;
};

class ConfigManager {
public:
    static ConfigManager& getInstance();

    bool loadConfig(const std::string& filePath);
    bool loadDefaultConfig();

    std::vector<SubjectConfig> getSubjects() const;
    SubjectConfig getSubjectConfig(const std::string& subjectName) const;
    std::vector<InstructionTemplate> getInstructionTemplates(const std::string& subjectName) const;
    std::vector<std::string> getSubjectNames() const;

    std::string getCurrentConfigPath() const { return m_currentConfigPath; }

private:
    std::string getDefaultConfigPath() const;

private:
    ConfigManager() = default;
    ~ConfigManager() = default;
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    bool parseConfigLine(const std::string& line, std::string& key, std::string& value);
    bool parseInstructionLine(const std::string& timeKey, const std::string& config,
                             InstructionTemplate& instruction);
    std::string trim(const std::string& str);

private:
    std::string m_currentConfigPath;
    std::map<std::string, SubjectFullConfig> m_subjectConfigs;
};