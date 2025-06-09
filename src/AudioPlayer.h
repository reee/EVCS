#pragma once
#include <string>
#include <windows.h>

class AudioPlayer {
public:
    static void playAudioFile(const std::string& filename);
    static int getSystemVolume();
};
