#pragma once
#include <string>
#include <windows.h>

class AudioPlayer {
public:
    static bool initialize();
    static void cleanup();
    static void playAudioFile(const std::string& filename);
    static int getSystemVolume();
    
private:
    static bool s_initialized;
    static bool playMP3WithBass(const std::string& filePath);
    static bool playWAVWithPlaySound(const std::string& filePath);
};
