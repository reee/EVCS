#include "AudioPlayer.h"
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <filesystem>
#include <algorithm>
#include <cctype>
#include <windows.h>
#include <mmsystem.h>

// 包含Bass Audio Library
#include "../third_party/bass/bass.h"

// 根据平台链接对应的Bass库
#ifdef _WIN64
#pragma comment(lib, "../third_party/bass/x64/bass.lib")
#else
#pragma comment(lib, "../third_party/bass/bass.lib")
#endif

#pragma comment(lib, "winmm.lib")

bool AudioPlayer::s_initialized = false;

bool AudioPlayer::initialize() {
    if (s_initialized) {
        return true;
    }
    
    // 初始化Bass库
    if (BASS_Init(-1, 44100, 0, NULL, NULL)) {
        s_initialized = true;
        return true;
    } else {
        return false;
    }
}

void AudioPlayer::cleanup() {
    if (s_initialized) {
        BASS_Free();
        s_initialized = false;
    }
}

void AudioPlayer::playAudioFile(const std::string& filename) {
    if (!s_initialized) {
        initialize();
    }

    // 获取可执行文件所在目录
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    std::filesystem::path exeDir = std::filesystem::path(exePath).parent_path();
    std::filesystem::path audioPath = exeDir / "audio" / filename;

    if (!std::filesystem::exists(audioPath)) {
        return;
    }

    // 统一使用BASS播放所有音频文件
    playWithBass(audioPath.string());
}

int AudioPlayer::getSystemVolume() {
    HRESULT hr;
    IMMDeviceEnumerator* deviceEnumerator = NULL;
    IMMDevice* defaultDevice = NULL;
    IAudioEndpointVolume* endpointVolume = NULL;
    float currentVolume = 0;
    
    // 创建设备枚举器
    hr = CoCreateInstance(
        __uuidof(MMDeviceEnumerator),
        NULL,
        CLSCTX_ALL,
        __uuidof(IMMDeviceEnumerator),
        (void**)&deviceEnumerator
    );
    
    if (FAILED(hr)) return 0;
    
    // 获取默认音频终端
    hr = deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &defaultDevice);
    if (FAILED(hr)) {
        deviceEnumerator->Release();
        return 0;
    }
    
    // 获取音量控制接口
    hr = defaultDevice->Activate(
        __uuidof(IAudioEndpointVolume),
        CLSCTX_ALL,
        NULL,
        (void**)&endpointVolume
    );
    
    if (FAILED(hr)) {
        defaultDevice->Release();
        deviceEnumerator->Release();
        return 0;
    }
    
    // 获取主音量
    hr = endpointVolume->GetMasterVolumeLevelScalar(&currentVolume);
    
    // 释放COM接口
    endpointVolume->Release();
    defaultDevice->Release();
    deviceEnumerator->Release();
    
    if (FAILED(hr)) return 0;
    
    // 将浮点音量转换为百分比
    return static_cast<int>(currentVolume * 100);
}

bool AudioPlayer::playWithBass(const std::string& filePath) {
    // 将路径转换为宽字符串以支持Unicode文件名
    std::wstring wideFilePath;
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, filePath.c_str(), -1, NULL, 0);
    if (size_needed > 0) {
        wideFilePath.resize(size_needed - 1);
        MultiByteToWideChar(CP_UTF8, 0, filePath.c_str(), -1, &wideFilePath[0], size_needed);
    }
    
    // 创建音频流（支持WAV、MP3等多种格式）
    HSTREAM stream = BASS_StreamCreateFile(FALSE, wideFilePath.c_str(), 0, 0, 
                                         BASS_STREAM_AUTOFREE | BASS_UNICODE);
    
    if (stream) {
        // 播放音频流
        BOOL result = BASS_ChannelPlay(stream, FALSE);
        if (result) {
            return true;
        } else {
            BASS_StreamFree(stream);
            return false;
        }
    }
    
    return false;
}

double AudioPlayer::getAudioDuration(const std::string& filename) {
    if (!s_initialized) {
        initialize();
    }

    // 获取可执行文件所在目录
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    std::filesystem::path exeDir = std::filesystem::path(exePath).parent_path();
    std::filesystem::path audioPath = exeDir / "audio" / filename;

    if (!std::filesystem::exists(audioPath)) {
        return 0.0;
    }

    // 将路径转换为宽字符串
    std::wstring wideFilePath;
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, audioPath.string().c_str(), -1, NULL, 0);
    if (size_needed > 0) {
        wideFilePath.resize(size_needed - 1);
        MultiByteToWideChar(CP_UTF8, 0, audioPath.string().c_str(), -1, &wideFilePath[0], size_needed);
    }

    // 创建音频流但不播放
    HSTREAM stream = BASS_StreamCreateFile(FALSE, wideFilePath.c_str(), 0, 0, BASS_UNICODE);
    if (stream) {
        // 获取音频时长（以字节为单位）
        QWORD lengthBytes = BASS_ChannelGetLength(stream, BASS_POS_BYTE);
        if (lengthBytes != (QWORD)-1) {
            // 转换为秒
            double lengthSeconds = BASS_ChannelBytes2Seconds(stream, lengthBytes);
            BASS_StreamFree(stream);
            return lengthSeconds;
        }
        BASS_StreamFree(stream);
    }

    return 0.0;  // 获取时长失败
}
