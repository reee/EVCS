#include "AudioPlayer.h"
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <filesystem>

void AudioPlayer::playAudioFile(const std::string& filename) {
    std::filesystem::path audioPath = std::filesystem::current_path() / "audio" / filename;
    if (!std::filesystem::exists(audioPath)) {
        return;
    }
    
    // 使用PlaySound播放wav文件
    PlaySoundA(audioPath.string().c_str(), NULL, SND_FILENAME | SND_ASYNC);
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
