@echo off
setlocal enabledelayedexpansion

echo =====================================
echo EVCS Release Package Script (Release x64)
echo =====================================
echo.

set "SCRIPT_DIR=%~dp0"
set "PROJECT_ROOT=%SCRIPT_DIR%.."
set "PROJECT_NAME=EVCS"
set "EXE_NAME=%PROJECT_NAME%.exe"
set "BASS_DLL=bass.dll"
set "BUILD_DIR=build"
set "RELEASE_DIR=release"

REM ---- Toolchain detection (sets CMAKE_CMD / CMAKE_GENERATOR) ----
call "%SCRIPT_DIR%_detect.bat"
if errorlevel 1 (
    echo.
    echo [FAILED] Toolchain detection failed, aborting.
    pause
    exit /b 1
)
echo.

REM ---- Clean old build dir ----
echo [1/3] Cleaning old build directory...
if exist "%PROJECT_ROOT%\%BUILD_DIR%" rmdir /s /q "%PROJECT_ROOT%\%BUILD_DIR%"
mkdir "%PROJECT_ROOT%\%BUILD_DIR%"

REM ---- Configure and build Release ----
echo [2/3] Configuring and building Release...
cd /d "%PROJECT_ROOT%\%BUILD_DIR%"
"%CMAKE_CMD%" -G "%CMAKE_GENERATOR%" -A x64 "%PROJECT_ROOT%"
if errorlevel 1 (
    echo [ERROR] CMake configuration failed.
    cd /d "%PROJECT_ROOT%"
    pause
    exit /b 1
)
"%CMAKE_CMD%" --build . --config Release
if errorlevel 1 (
    echo [ERROR] Build failed.
    cd /d "%PROJECT_ROOT%"
    pause
    exit /b 1
)
cd /d "%PROJECT_ROOT%"

REM ---- Deploy files to release directory ----
echo [3/3] Deploying release files...
call "%SCRIPT_DIR%_deploy.bat" "%RELEASE_DIR%" Release RELEASE_INFO.txt
if errorlevel 1 (
    echo [ERROR] File deployment failed.
    cd /d "%PROJECT_ROOT%"
    pause
    exit /b 1
)

echo.
echo =====================================
echo Release package complete!
echo =====================================
echo Output dir: %PROJECT_ROOT%\%RELEASE_DIR%
echo.
echo Contents:
dir /b "%PROJECT_ROOT%\%RELEASE_DIR%"
echo.
echo Distribution notes:
echo   1. The "%RELEASE_DIR%" folder is ready to distribute as-is
echo   2. Users must add their own audio files to the audio\ subfolder
echo   3. See README.txt for details
echo.
pause
