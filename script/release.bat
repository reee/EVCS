@echo off
setlocal enabledelayedexpansion

echo =====================================
echo EVCS Release Build Script
echo =====================================
echo.

REM 获取脚本所在目录的父目录（项目根目录）
set "SCRIPT_DIR=%~dp0"
set "PROJECT_ROOT=%SCRIPT_DIR%.."

REM 设置变量
set "PROJECT_NAME=EVCS"
set "BUILD_DIR=build"
set "RELEASE_DIR=release"
set "EXE_NAME=%PROJECT_NAME%.exe"
set "BASS_DLL=bass.dll"

REM 检查是否存在必要的第三方文件
echo [1/6] Checking dependencies...
if not exist "%PROJECT_ROOT%\third_party\bass\x64\bass.dll" (
    echo ERROR: bass.dll not found in %PROJECT_ROOT%\third_party\bass\x64\
    echo Please download BASS Audio Library and place bass.dll in the correct location.
    pause
    exit /b 1
)

REM 清理旧的构建目录
echo [2/6] Cleaning old build files...
if exist "%PROJECT_ROOT%\%BUILD_DIR%" (
    echo Cleaning %BUILD_DIR% directory...
    rmdir /s /q "%PROJECT_ROOT%\%BUILD_DIR%"
)

REM 清理旧的发布目录
echo [3/6] Cleaning old release files...
if exist "%PROJECT_ROOT%\%RELEASE_DIR%" (
    echo Cleaning %RELEASE_DIR% directory...
    rmdir /s /q "%PROJECT_ROOT%\%RELEASE_DIR%"
)

REM 创建发布目录结构
echo [4/6] Creating release directory structure...
mkdir "%PROJECT_ROOT%\%RELEASE_DIR%"
mkdir "%PROJECT_ROOT%\%RELEASE_DIR%\config"
mkdir "%PROJECT_ROOT%\%RELEASE_DIR%\audio"

REM 构建项目
echo [5/6] Building project...
mkdir "%PROJECT_ROOT%\%BUILD_DIR%"
cd "%PROJECT_ROOT%\%BUILD_DIR%"

REM 检查CMake是否可用
cmake --version >nul 2>&1
if errorlevel 1 (
    echo ERROR: CMake not found. Please install CMake and add it to PATH.
    cd ..
    pause
    exit /b 1
)

REM 生成Visual Studio项目文件
echo Generating Visual Studio project files...
cmake -G "Visual Studio 17 2022" -A x64 "%PROJECT_ROOT%" > nul 2>&1
if errorlevel 1 (
    echo ERROR: CMake configuration failed.
    cd "%PROJECT_ROOT%"
    pause
    exit /b 1
)

REM 编译Release版本
echo Building Release configuration...
cmake --build . --config Release > nul 2>&1
if errorlevel 1 (
    echo ERROR: Build failed.
    cd "%PROJECT_ROOT%"
    pause
    exit /b 1
)

cd "%PROJECT_ROOT%"

REM 复制文件到发布目录
echo [6/6] Copying files to release directory...

REM 复制可执行文件
echo Copying executable...
if exist "%PROJECT_ROOT%\%BUILD_DIR%\Release\%EXE_NAME%" (
    copy "%PROJECT_ROOT%\%BUILD_DIR%\Release\%EXE_NAME%" "%PROJECT_ROOT%\%RELEASE_DIR%\" > nul
    echo   - %EXE_NAME% [OK]
) else (
    echo ERROR: %EXE_NAME% not found in build output
    pause
    exit /b 1
)

REM 复制BASS DLL
echo Copying BASS audio library...
copy "%PROJECT_ROOT%\third_party\bass\x64\bass.dll" "%PROJECT_ROOT%\%RELEASE_DIR%\" > nul
echo   - bass.dll [OK]

REM 复制README文件
if exist "%PROJECT_ROOT%\README.txt" (
    echo Copying documentation...
    copy "%PROJECT_ROOT%\README.txt" "%PROJECT_ROOT%\%RELEASE_DIR%\" > nul
    echo   - README.txt [OK]
) else (
    echo WARNING: README.txt not found
)

REM 复制配置文件
if exist "%PROJECT_ROOT%\config\*" (
    echo Copying configuration files...
    xcopy "%PROJECT_ROOT%\config\*" "%PROJECT_ROOT%\%RELEASE_DIR%\config\" /y > nul
    echo   - Config files [OK]
) else (
    echo WARNING: config directory not found or empty
)

REM 复制音频文件目录（如果存在）
if exist "%PROJECT_ROOT%\audio\*" (
    echo Copying audio files...
    xcopy "%PROJECT_ROOT%\audio\*" "%PROJECT_ROOT%\%RELEASE_DIR%\audio\" /e /y > nul
    echo   - Audio files [OK]
) else (
    echo NOTE: audio directory not found - user will need to create it and add audio files
)

REM 创建版本信息文件
echo Creating version info...
(
echo Release Information:
echo ===================
echo Project: %PROJECT_NAME%
echo Build Date: %date% %time%
echo Build Type: Release x64
echo.
echo Required Components:
echo - %EXE_NAME% ^(main executable^)
echo - bass.dll ^(audio library^)
echo - config\ ^(configuration files^)
echo - audio\ ^(audio files directory - user provided^)
echo - README.txt ^(user documentation^)
echo.
echo Audio Files Required ^(user provided^):
echo - 1kq12.mp3, 2kq10.mp3, 3kq5.mp3
echo - 4ksks.mp3, 5jsq15.mp3, 6ksjs.mp3
echo - sy.mp3, tl.mp3 ^(for English subject^)
) > "%PROJECT_ROOT%\%RELEASE_DIR%\RELEASE_INFO.txt"

echo.
echo =====================================
echo Build completed successfully!
echo =====================================
echo.
echo Release directory: %PROJECT_ROOT%\%RELEASE_DIR%
echo.
echo Contents:
dir /b "%PROJECT_ROOT%\%RELEASE_DIR%"
echo.
echo To distribute:
echo 1. The entire "%RELEASE_DIR%" folder is ready for distribution
echo 2. Users will need to add their own audio files to the "audio" subfolder
echo 3. See README.txt for detailed instructions
echo.
echo Release package created at: %PROJECT_ROOT%\%RELEASE_DIR%
echo.

pause