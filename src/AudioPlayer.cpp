#include "AudioPlayer.h"
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <filesystem>
#include <algorithm>
#include <cctype>

// 包含Bass Audio Library
#include "../third_party/bass/bass.h"

// 根据平台链接对应的Bass库
#ifdef _WIN64
#pragma comment(lib, "../third_party/bass/x64/bass.lib")
#else
#pragma comment(lib, "../third_party/bass/bass.lib")
#endif

// 为了兼容性也包含MCI作为备用方案
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

bool AudioPlayer::s_initialized = false;

bool AudioPlayer::initialize() {
    if (s_initialized) {
        return true;
    }
    
    // 初始化Bass库
    // 参数: device (-1 = default), frequency (44100 Hz), flags (0), window handle (NULL), class identifier (NULL)
    if (BASS_Init(-1, 44100, 0, NULL, NULL)) {
        s_initialized = true;
        OutputDebugStringA("Bass Audio Library initialized successfully\n");
        return true;
    } else {
        DWORD error = BASS_ErrorGetCode();
        char errorMsg[256];
        sprintf_s(errorMsg, "Bass initialization failed with error code: %d\n", error);
        OutputDebugStringA(errorMsg);
        
        // 作为备用方案，我们仍然可以使用MCI和PlaySound
        s_initialized = true;
        OutputDebugStringA("Falling back to MCI/PlaySound for audio playback\n");
        return true;
    }
}

void AudioPlayer::cleanup() {
    if (s_initialized) {
        // 清理Bass库
        BASS_Free();
        s_initialized = false;
        OutputDebugStringA("Bass Audio Library cleanup completed\n");
    }
}

void AudioPlayer::playAudioFile(const std::string& filename) {
    if (!s_initialized) {
        initialize();
    }
    
    std::filesystem::path audioPath = std::filesystem::current_path() / "audio" / filename;
    if (!std::filesystem::exists(audioPath)) {
        OutputDebugStringA("Audio file not found: ");
        OutputDebugStringA(filename.c_str());
        OutputDebugStringA("\n");
        return;
    }
    
    // 检查文件扩展名
    std::string extension = audioPath.extension().string();
    for (auto& c : extension) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    
    if (extension == ".mp3") {
        // MP3文件使用Bass库播放（当前使用MCI作为替代）
        if (playMP3WithBass(audioPath.string())) {
            OutputDebugStringA("MP3 played successfully: ");
            OutputDebugStringA(filename.c_str());
            OutputDebugStringA("\n");
        } else {
            OutputDebugStringA("Failed to play MP3: ");
            OutputDebugStringA(filename.c_str());
            OutputDebugStringA("\n");
        }
    } else {
        // WAV文件使用PlaySound播放
        if (playWAVWithPlaySound(audioPath.string())) {
            OutputDebugStringA("WAV played successfully: ");
            OutputDebugStringA(filename.c_str());
            OutputDebugStringA("\n");
        } else {
            OutputDebugStringA("Failed to play WAV: ");
            OutputDebugStringA(filename.c_str());
            OutputDebugStringA("\n");
        }
    }
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

bool AudioPlayer::playMP3WithBass(const std::string& filePath) {
    // 使用Bass Audio Library播放MP3
    
    // 将路径转换为宽字符串以支持Unicode文件名
    std::wstring wideFilePath;
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, filePath.c_str(), -1, NULL, 0);
    if (size_needed > 0) {
        wideFilePath.resize(size_needed - 1);
        MultiByteToWideChar(CP_UTF8, 0, filePath.c_str(), -1, &wideFilePath[0], size_needed);
    }
    
    // 创建音频流
    HSTREAM stream = BASS_StreamCreateFile(FALSE, wideFilePath.c_str(), 0, 0, 
                                         BASS_STREAM_AUTOFREE | BASS_UNICODE);
    
    if (stream) {
        // 播放音频流
        BOOL result = BASS_ChannelPlay(stream, FALSE);
        if (result) {
            OutputDebugStringA("MP3 playback started using Bass Audio Library\n");
            return true;
        } else {
            DWORD error = BASS_ErrorGetCode();
            char errorMsg[256];
            sprintf_s(errorMsg, "Bass play failed with error code: %d\n", error);
            OutputDebugStringA(errorMsg);
            
            BASS_StreamFree(stream);
        }
    } else {
        DWORD error = BASS_ErrorGetCode();
        char errorMsg[256];
        sprintf_s(errorMsg, "Bass stream creation failed with error code: %d\n", error);
        OutputDebugStringA(errorMsg);
    }
    
    // 如果Bass播放失败，回退到MCI
    OutputDebugStringA("Falling back to MCI for MP3 playback\n");
    
    // 停止之前可能正在播放的音频
    mciSendStringW(L"stop mp3", NULL, 0, NULL);
    mciSendStringW(L"close mp3", NULL, 0, NULL);
    
    // 构建MCI命令
    std::wstring command = L"open \"" + wideFilePath + L"\" type mpegvideo alias mp3";
    MCIERROR error = mciSendStringW(command.c_str(), NULL, 0, NULL);
    
    if (error != 0) {
        // 尝试使用waveaudio类型
        command = L"open \"" + wideFilePath + L"\" alias mp3";
        error = mciSendStringW(command.c_str(), NULL, 0, NULL);
        
        if (error != 0) {
            wchar_t errorText[256];
            mciGetErrorStringW(error, errorText, 256);
            OutputDebugStringW(L"MCI open error: ");
            OutputDebugStringW(errorText);
            OutputDebugStringW(L"\n");
            return false;
        }
    }
    
    // 播放音频
    error = mciSendStringW(L"play mp3", NULL, 0, NULL);
    if (error != 0) {
        wchar_t errorText[256];
        mciGetErrorStringW(error, errorText, 256);
        OutputDebugStringW(L"MCI play error: ");
        OutputDebugStringW(errorText);
        OutputDebugStringW(L"\n");
        
        // 清理资源
        mciSendStringW(L"close mp3", NULL, 0, NULL);
        return false;
    }
    
    OutputDebugStringA("MP3 playback started using MCI fallback\n");
    return true;
}

bool AudioPlayer::playWAVWithPlaySound(const std::string& filePath) {
    BOOL result = PlaySoundA(filePath.c_str(), NULL, SND_FILENAME | SND_ASYNC);
    return result != FALSE;
}
