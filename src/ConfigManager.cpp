#include "ConfigManager.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <windows.h>

ConfigManager& ConfigManager::getInstance() {
    static ConfigManager instance;
    return instance;
}

bool ConfigManager::loadConfig(const std::string& filePath) {
    m_currentConfigPath = filePath;

    // 清空现有配置
    m_subjectConfigs.clear();

    std::ifstream file(filePath);
    if (!file.is_open()) {
        return false;
    }

    std::string line;
    std::string currentSection;
    int lineNum = 0;

    while (std::getline(file, line)) {
        lineNum++;

        // 去除BOM标记（如果有）
        if (lineNum == 1 && line.length() >= 3 &&
            (unsigned char)line[0] == 0xEF &&
            (unsigned char)line[1] == 0xBB &&
            (unsigned char)line[2] == 0xBF) {
            line = line.substr(3);
        }

        // 去除前后空格
        line = trim(line);

        // 跳过空行和注释行
        if (line.empty() || line[0] == ';' || line[0] == '#') {
            continue;
        }

        // 检查是否为节标题（科目名称）
        if (line[0] == '[' && line.back() == ']') {
            currentSection = line.substr(1, line.length() - 2);
            currentSection = trim(currentSection);

            // 如果不是空节标题，初始化科目配置
            if (!currentSection.empty()) {
                SubjectFullConfig& config = m_subjectConfigs[currentSection];
                config.subjectInfo.name = currentSection;
                config.subjectInfo.durationMinutes = 90;  // 默认值
            }
            continue;
        }

        // 解析键值对
        std::string key, value;
        if (parseConfigLine(line, key, value)) {
            if (!currentSection.empty() && m_subjectConfigs.find(currentSection) != m_subjectConfigs.end()) {
                SubjectFullConfig& config = m_subjectConfigs[currentSection];

                // 只支持新的简化格式
                if (key == "duration") {
                    try {
                        config.subjectInfo.durationMinutes = std::stoi(value);
                    } catch (...) {
                        // 解析失败，保持默认值
                    }
                } else {
                    // 解析为指令模板，key是时间偏移，value是指令信息
                    InstructionTemplate instruction;
                    if (parseInstructionLine(key, value, instruction)) {
                        config.instructions.push_back(instruction);
                    }
                }
            }
        }
    }

    return !m_subjectConfigs.empty();
}

std::string ConfigManager::getDefaultConfigPath() const {
    // 获取可执行文件所在目录
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);

    // 获取可执行文件所在目录
    std::filesystem::path exeDir = std::filesystem::path(exePath).parent_path();
    return (exeDir / "config" / "默认配置.ini").string();
}

std::vector<SubjectConfig> ConfigManager::getSubjects() const {
    std::vector<SubjectConfig> subjects;
    for (const auto& pair : m_subjectConfigs) {
        subjects.push_back(pair.second.subjectInfo);
    }
    return subjects;
}

SubjectConfig ConfigManager::getSubjectConfig(const std::string& subjectName) const {
    auto it = m_subjectConfigs.find(subjectName);
    if (it != m_subjectConfigs.end()) {
        return it->second.subjectInfo;
    }

    // 返回空配置，调用者需要检查
    return {};
}

std::vector<InstructionTemplate> ConfigManager::getInstructionTemplates(const std::string& subjectName) const {
    auto it = m_subjectConfigs.find(subjectName);
    if (it != m_subjectConfigs.end()) {
        // 返回该科目的指令列表，并确保按时间排序
        auto instructions = it->second.instructions;
        std::sort(instructions.begin(), instructions.end(),
                  [](const InstructionTemplate& a, const InstructionTemplate& b) {
                      return a.offsetSeconds < b.offsetSeconds;
                  });
        return instructions;
    }

    // 如果没找到，返回空列表
    return {};
}

std::vector<std::string> ConfigManager::getSubjectNames() const {
    std::vector<std::string> names;
    for (const auto& pair : m_subjectConfigs) {
        names.push_back(pair.first);
    }
    return names;
}

bool ConfigManager::loadDefaultConfig() {
    return loadConfig(getDefaultConfigPath());
}

bool ConfigManager::parseConfigLine(const std::string& line, std::string& key, std::string& value) {
    size_t pos = line.find('=');
    if (pos == std::string::npos) {
        return false;
    }

    key = trim(line.substr(0, pos));
    value = trim(line.substr(pos + 1));
    return !key.empty() && !value.empty();
}


bool ConfigManager::parseInstructionLine(const std::string& timeKey, const std::string& config,
                                        InstructionTemplate& instruction) {
    size_t pipePos1 = config.find('|');
    if (pipePos1 == std::string::npos) {
        return false;
    }

    std::string name = trim(config.substr(0, pipePos1));
    std::string audioFile = trim(config.substr(pipePos1 + 1));

    if (name.empty() || audioFile.empty()) {
        return false;
    }

    instruction.name = name;
    instruction.audioFile = audioFile;

    // 解析时间偏移（直接使用秒数）
    try {
        instruction.offsetSeconds = std::stoi(timeKey);
    } catch (...) {
        return false;
    }

    return true;
}

std::string ConfigManager::trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        return "";
    }

    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}