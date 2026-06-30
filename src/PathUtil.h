#pragma once

#include <string>
#include <filesystem>

// 路径解析工具：集中处理可执行文件目录与 audio 子目录的拼接
class PathUtil {
public:
    // 获取可执行文件所在目录（失败时返回空 path）
    static std::filesystem::path getAppDir();

    // 获取 audio 子目录下的完整路径
    static std::filesystem::path getAudioPath(const std::string& filename);

    // 获取 config 子目录下的完整路径
    static std::filesystem::path getConfigPath(const std::wstring& filename);
};
