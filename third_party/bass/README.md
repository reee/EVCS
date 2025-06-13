# Bass Audio Library 集成说明

## Bass Audio Library集成步骤(项目已包含头文件和lib文件)

1. 访问 Bass Audio Library 官网: https://www.un4seen.com/bass.html
2. 下载 Windows 版本的 Bass Audio Library
3. 将以下文件复制到对应目录：
   - `bass.h` -> `third_party/bass/bass.h`
   - `bass.lib` -> `third_party/bass/bass.lib` (x86版本)
   - `x64/bass.lib` -> `third_party/bass/x64/bass.lib` (x64版本)
4. 编译完成后将对应版本的`bass.dll` 复制到 `EVCS.exe` 同一目录


