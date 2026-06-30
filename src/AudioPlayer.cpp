#include "AudioPlayer.h"
#include "StringUtil.h"
#include "PathUtil.h"
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <filesystem>
#include <algorithm>
#include <cctype>
#include <windows.h>
#include <mmsystem.h>
#include <cstdio>

// 包含 Bass Audio Library
#include "../third_party/bass/bass.h"

// 根据平台链接对应的 Bass 库
#ifdef _WIN64
#pragma comment(lib, "../third_party/bass/x64/bass.lib")
#else
#pragma comment(lib, "../third_party/bass/bass.lib")
#endif

#pragma comment(lib, "winmm.lib")

bool AudioPlayer::s_initialized = false;
DWORD AudioPlayer::s_currentStream = 0;

namespace {
void logBassError(const char* context) {
    int code = BASS_ErrorGetCode();
    char buf[128];
    std::snprintf(buf, sizeof(buf), "[BASS] %s failed, error=%d\n", context, code);
    OutputDebugStringA(buf);
}
}  // namespace

bool AudioPlayer::initialize() {
    if (s_initialized) {
        return true;
    }

    // 初始化 Bass 库（-1 表示使用默认输出设备）
    if (BASS_Init(-1, 44100, 0, NULL, NULL)) {
        s_initialized = true;
        return true;
    }

    logBassError("BASS_Init");
    return false;
}

void AudioPlayer::cleanup() {
    if (s_initialized) {
        stop();
        BASS_Free();
        s_initialized = false;
    }
}

bool AudioPlayer::playAudioFile(const std::string& filename) {
    if (!s_initialized) {
        if (!initialize()) {
            return false;
        }
    }

    std::filesystem::path audioPath = PathUtil::getAudioPath(filename);
    if (!std::filesystem::exists(audioPath)) {
        return false;
    }

    // 停止上一次播放（如有）
    stop();

    // 不使用 BASS_STREAM_AUTOFREE：保留句柄以便查询活跃状态
    // 播放结束/出错时由 stop()/cleanup() 显式释放
    std::wstring widePath = audioPath.wstring();
    HSTREAM stream = BASS_StreamCreateFile(FALSE, widePath.c_str(), 0, 0, BASS_UNICODE);
    if (!stream) {
        logBassError("BASS_StreamCreateFile");
        return false;
    }

    if (!BASS_ChannelPlay(stream, FALSE)) {
        logBassError("BASS_ChannelPlay");
        BASS_StreamFree(stream);
        return false;
    }

    s_currentStream = stream;
    return true;
}

bool AudioPlayer::isPlaying() {
    if (!s_initialized || !s_currentStream) {
        return false;
    }
    // BASS_ACTIVE_PLAYING / BASS_ACTIVE_STALLED 都视为"占用中"
    DWORD active = BASS_ChannelIsActive(s_currentStream);
    return active == BASS_ACTIVE_PLAYING || active == BASS_ACTIVE_STALLED;
}

void AudioPlayer::stop() {
    if (s_currentStream) {
        BASS_ChannelStop(s_currentStream);
        BASS_StreamFree(s_currentStream);
        s_currentStream = 0;
    }
}

double AudioPlayer::getCurrentStreamDuration() {
    if (!s_initialized || !s_currentStream) {
        return 0.0;
    }
    // 直接查询当前播放通道长度，无需另开流
    QWORD lengthBytes = BASS_ChannelGetLength(s_currentStream, BASS_POS_BYTE);
    if (lengthBytes == (QWORD)-1) {
        return 0.0;
    }
    return BASS_ChannelBytes2Seconds(s_currentStream, lengthBytes);
}

int AudioPlayer::getSystemVolume() {
    HRESULT hr;
    IMMDeviceEnumerator* deviceEnumerator = NULL;
    IMMDevice* defaultDevice = NULL;
    IAudioEndpointVolume* endpointVolume = NULL;
    float currentVolume = 0;

    hr = CoCreateInstance(
        __uuidof(MMDeviceEnumerator),
        NULL,
        CLSCTX_ALL,
        __uuidof(IMMDeviceEnumerator),
        (void**)&deviceEnumerator);
    if (FAILED(hr)) return 0;

    hr = deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &defaultDevice);
    if (FAILED(hr)) {
        deviceEnumerator->Release();
        return 0;
    }

    hr = defaultDevice->Activate(
        __uuidof(IAudioEndpointVolume),
        CLSCTX_ALL,
        NULL,
        (void**)&endpointVolume);
    if (FAILED(hr)) {
        defaultDevice->Release();
        deviceEnumerator->Release();
        return 0;
    }

    hr = endpointVolume->GetMasterVolumeLevelScalar(&currentVolume);

    endpointVolume->Release();
    defaultDevice->Release();
    deviceEnumerator->Release();

    if (FAILED(hr)) return 0;
    return static_cast<int>(currentVolume * 100);
}

double AudioPlayer::getAudioDuration(const std::string& filename) {
    if (!s_initialized) {
        if (!initialize()) {
            return 0.0;
        }
    }

    std::filesystem::path audioPath = PathUtil::getAudioPath(filename);
    if (!std::filesystem::exists(audioPath)) {
        return 0.0;
    }

    std::wstring widePath = audioPath.wstring();
    HSTREAM stream = BASS_StreamCreateFile(FALSE, widePath.c_str(), 0, 0, BASS_UNICODE);
    if (!stream) {
        return 0.0;
    }

    double lengthSeconds = 0.0;
    QWORD lengthBytes = BASS_ChannelGetLength(stream, BASS_POS_BYTE);
    if (lengthBytes != (QWORD)-1) {
        lengthSeconds = BASS_ChannelBytes2Seconds(stream, lengthBytes);
    }
    BASS_StreamFree(stream);
    return lengthSeconds;
}
