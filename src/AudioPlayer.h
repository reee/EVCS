#pragma once
#include <string>
#include <windows.h>

class AudioPlayer {
public:
    static bool initialize();
    static void cleanup();
    static void playAudioFile(const std::string& filename);
    static double getAudioDuration(const std::string& filename);  // 获取音频文件时长（秒）
    static int getSystemVolume();

private:
    static bool s_initialized;
    static bool playWithBass(const std::wstring& filePath);  // 统一使用BASS播放，接受宽字符串路径
};
