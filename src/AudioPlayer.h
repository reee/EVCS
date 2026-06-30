#pragma once
#include <string>
#include <windows.h>

class AudioPlayer {
public:
    // 初始化 BASS 库。失败时通过 OutputDebugString 输出 BASS 错误码
    static bool initialize();
    static void cleanup();

    // 播放音频文件（位于 audio 子目录）。返回是否成功开始播放
    static bool playAudioFile(const std::string& filename);

    // 当前是否有音频正在播放（基于 BASS 通道活跃状态）
    static bool isPlaying();

    // 停止当前播放（如有）
    static void stop();

    // 获取当前正在播放流的时长（秒）。无活跃流或失败返回 0.0
    static double getCurrentStreamDuration();

    // 获取音频文件时长（秒）。失败返回 0.0
    static double getAudioDuration(const std::string& filename);

    // 获取系统主音量百分比 [0,100]，失败返回 0
    static int getSystemVolume();

private:
    static bool s_initialized;
    // BASS 流句柄（HSTREAM 即 DWORD）。0 表示无流。
    // 用 DWORD 而非 HSTREAM，避免头文件依赖 bass.h
    static DWORD s_currentStream;
};
