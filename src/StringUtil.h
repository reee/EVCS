#pragma once

#include <string>
#include <locale>
#include <codecvt>

/**
 * Unicode编码转换工具类
 * 提供UTF-8和宽字符之间的转换功能
 */
class StringUtil {
public:
    /**
     * 将UTF-8字符串转换为宽字符串
     * @param utf8Str UTF-8编码的字符串
     * @return 宽字符串
     */
    static std::wstring utf8ToWide(const std::string& utf8Str);

    /**
     * 将宽字符串转换为UTF-8字符串
     * @param wideStr 宽字符串
     * @return UTF-8编码的字符串
     */
    static std::string wideToUtf8(const std::wstring& wideStr);

    /**
     * 将UTF-8字符串转换为宽字符串（C字符串版本）
     * @param utf8Str UTF-8编码的C字符串
     * @return 宽字符串
     */
    static std::wstring utf8ToWide(const char* utf8Str);

    /**
     * 将宽字符串转换为UTF-8字符串（C字符串版本）
     * @param wideStr 宽C字符串
     * @return UTF-8编码的字符串
     */
    static std::string wideToUtf8(const wchar_t* wideStr);

private:
    // 使用Windows API进行转换的内部函数
    static std::wstring utf8ToWideWindows(const std::string& utf8Str);
    static std::string wideToUtf8Windows(const std::wstring& wideStr);
};