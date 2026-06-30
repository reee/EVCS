#include "PathUtil.h"
#include "StringUtil.h"
#include <windows.h>

namespace {
// audio 子目录用宽字符字面量，避免与宽字符 path 拼接时触发 locale 依赖转换。
constexpr const wchar_t* AUDIO_DIR = L"audio";
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
    // filename 为 UTF-8 字节串（来自 INI），必须先显式转宽字符再拼接，
    // 否则 path 的 operator/ 接受 std::string 时会走 locale 依赖转换，
    // 导致中文文件名错码（不变量 §5 + 中文路径一等公民）。
    return getAppDir() / AUDIO_DIR / StringUtil::utf8ToWide(filename);
}

std::filesystem::path PathUtil::getConfigPath(const std::wstring& filename) {
    return getAppDir() / CONFIG_DIR / filename;
}
