# Bass Audio Library 集成说明

## 下载和安装 Bass Audio Library

1. 访问 Bass Audio Library 官网: https://www.un4seen.com/bass.html
2. 下载 Windows 版本的 Bass Audio Library
3. 将以下文件复制到对应目录：
   - `bass.h` -> `third_party/bass/bass.h`
   - `bass.lib` -> `third_party/bass/bass.lib` (x86版本)
   - `x64/bass.lib` -> `third_party/bass/x64/bass.lib` (x64版本)
   - `bass.dll` -> 与 `EVCS.exe` 同一目录

## 启用真正的Bass支持

在 `AudioPlayer.cpp` 中的 `playMP3WithBass` 函数里：

1. 取消注释 Bass 实现代码块（方案1）
2. 注释掉 MCI 实现代码块（方案2）
3. 确保项目链接到正确的 bass.lib

## 当前实现

当前使用 MCI (Media Control Interface) 作为临时解决方案来播放MP3文件。
这是Windows内置的功能，不需要额外的库文件，但功能相对有限。

一旦部署了Bass库，可以获得更好的音频播放性能和更多功能。
