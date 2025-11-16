#include "StringUtil.h"
#include <windows.h>

std::wstring StringUtil::utf8ToWide(const std::string& utf8Str) {
    return utf8ToWideWindows(utf8Str);
}

std::string StringUtil::wideToUtf8(const std::wstring& wideStr) {
    return wideToUtf8Windows(wideStr);
}

std::wstring StringUtil::utf8ToWide(const char* utf8Str) {
    if (!utf8Str) return std::wstring();
    return utf8ToWide(std::string(utf8Str));
}

std::string StringUtil::wideToUtf8(const wchar_t* wideStr) {
    if (!wideStr) return std::string();
    return wideToUtf8(std::wstring(wideStr));
}

std::wstring StringUtil::utf8ToWideWindows(const std::string& utf8Str) {
    if (utf8Str.empty()) {
        return std::wstring();
    }

    int size_needed = MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, NULL, 0);
    if (size_needed <= 0) {
        return std::wstring();
    }

    std::wstring result(size_needed - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, &result[0], size_needed);
    return result;
}

std::string StringUtil::wideToUtf8Windows(const std::wstring& wideStr) {
    if (wideStr.empty()) {
        return std::string();
    }

    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, NULL, 0, NULL, NULL);
    if (size_needed <= 1) {
        return std::string();  // 只有null终止符或转换失败
    }

    std::string result(size_needed - 1, 0);  // 减1去掉null终止符
    WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, &result[0], size_needed, NULL, NULL);
    return result;
}