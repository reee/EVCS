#include "ConfigManager.h"
#include "Subject.h"
#include "PathUtil.h"
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <cstdio>
#include <windows.h>

namespace {
// 防御性上限（不变量 §4）：实际配置远小于这些值，超出视为异常输入并拒绝。
constexpr DWORD kMaxConfigFileSize = 1 * 1024 * 1024;      // 单文件 ≤ 1MB
constexpr int kMaxConfigLineCount = 10000;                  // 行数 ≤ 10000
constexpr int kMaxInstructionsPerSubject = 500;             // 单科目指令 ≤ 500
constexpr int kMaxInstructionsTotal = 5000;                 // 全局指令 ≤ 5000
constexpr size_t kMaxAudioFilenameLength = 260;             // 音频文件名长度 ≤ 260

// audioFile 路径穿越防护（不变量 §4/§5）。
// 允许裸文件名与子目录（如 english/tl.mp3）；禁止绝对路径、盘符、.. 上跳。
bool isSafeAudioFilename(const std::string& name) {
    if (name.empty() || name.size() > kMaxAudioFilenameLength) {
        return false;
    }
    // 任何盘符 / 冒号都拒绝
    if (name.find(':') != std::string::npos) {
        return false;
    }
    // 绝对路径拒绝
    if (name.front() == '/' || name.front() == '\\') {
        return false;
    }
    // 检查每个路径段，拒绝 ".." 段（按 / 与 \ 切分）
    size_t start = 0;
    while (start <= name.size()) {
        size_t end = name.find_first_of("/\\", start);
        std::string segment = (end == std::string::npos)
                                  ? name.substr(start)
                                  : name.substr(start, end - start);
        if (segment == "..") {
            return false;
        }
        if (end == std::string::npos) break;
        start = end + 1;
    }
    return true;
}

void logConfigWarning(const char* msg) {
    char buf[256];
    std::snprintf(buf, sizeof(buf), "[ConfigManager] %s\n", msg);
    OutputDebugStringA(buf);
}
}  // namespace

ConfigManager& ConfigManager::getInstance() {
    static ConfigManager instance;
    return instance;
}

bool ConfigManager::loadConfig(const std::wstring& filePath) {
    m_currentConfigPath = filePath;

    // 清空现有配置
    m_subjectConfigs.clear();

    // 使用 Windows API 打开文件 - 方案1
    HANDLE hFile = CreateFileW(filePath.c_str(), GENERIC_READ, FILE_SHARE_READ,
                              NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile == INVALID_HANDLE_VALUE) {
        return false;
    }

    // 获取文件大小
    DWORD fileSize = GetFileSize(hFile, NULL);
    if (fileSize == INVALID_FILE_SIZE || fileSize == 0) {
        CloseHandle(hFile);
        return false;
    }
    if (fileSize > kMaxConfigFileSize) {
        CloseHandle(hFile);
        logConfigWarning("config file exceeds size limit, rejected");
        return false;
    }

    // 读取文件内容
    std::string fileContent(fileSize, '\0');
    DWORD bytesRead;
    if (!ReadFile(hFile, &fileContent[0], fileSize, &bytesRead, NULL)) {
        CloseHandle(hFile);
        return false;
    }

    CloseHandle(hFile);

    // 使用字符串流解析内容
    std::istringstream file(fileContent);
    std::string line;
    std::string currentSection;
    int lineNum = 0;
    int totalInstructionCount = 0;

    while (std::getline(file, line)) {
        lineNum++;
        if (lineNum > kMaxConfigLineCount) {
            logConfigWarning("config line count exceeds limit, rejected");
            return false;
        }

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
                config.subjectInfo.durationMinutes = Subject::DEFAULT_DURATION_MINUTES;
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
                        config.subjectInfo.durationMinutes = Subject::DEFAULT_DURATION_MINUTES;
                    }
                } else {
                    // 解析为指令模板，key是时间偏移，value是指令信息
                    InstructionTemplate instruction;
                    if (parseInstructionLine(key, value, instruction)) {
                        // 防御性上限（不变量 §4）：超限视为配置异常，整体拒绝
                        // （而非静默截断——静默漏指令在考试场景下更危险）。
                        if (config.instructions.size() >= kMaxInstructionsPerSubject) {
                            logConfigWarning("instruction count per subject exceeds limit, rejected");
                            return false;
                        }
                        if (totalInstructionCount >= kMaxInstructionsTotal) {
                            logConfigWarning("total instruction count exceeds limit, rejected");
                            return false;
                        }
                        config.instructions.push_back(instruction);
                        totalInstructionCount++;
                    }
                }
            }
        }
    }

    return !m_subjectConfigs.empty();
}

std::wstring ConfigManager::getDefaultConfigPath() const {
    return PathUtil::getConfigPath(L"default.ini").wstring();
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

    // 路径穿越防护（不变量 §4/§5）：禁止绝对路径/盘符/..上跳
    if (!isSafeAudioFilename(audioFile)) {
        logConfigWarning("audioFile rejected: path traversal or invalid");
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