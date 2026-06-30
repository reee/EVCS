#include "PathUtil.h"
#include <windows.h>

namespace {
constexpr const char* AUDIO_DIR = "audio";
constexpr const wchar_t* CONFIG_DIR = L"config";
}  // namespace

std::filesystem::path PathUtil::getAppDir() {
    wchar_t exePath[MAX_PATH];
    DWORD len = GetModuleFileNameW(NULL, exePath, MAX_PATH);
    if (len == 0 || len >= MAX_PATH) {
        // 失败或路径被截断
        return std::filesystem::path();
    }
    return std::filesystem::path(exePath).parent_path();
}

std::filesystem::path PathUtil::getAudioPath(const std::string& filename) {
    return getAppDir() / AUDIO_DIR / filename;
}

std::filesystem::path PathUtil::getConfigPath(const std::wstring& filename) {
    return getAppDir() / CONFIG_DIR / filename;
}
