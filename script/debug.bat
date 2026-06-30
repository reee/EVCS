@echo off
setlocal enabledelayedexpansion

echo =====================================
echo EVCS Debug Package Script (Debug x64)
echo =====================================
echo.

set "SCRIPT_DIR=%~dp0"
set "PROJECT_ROOT=%SCRIPT_DIR%.."
set "PROJECT_NAME=EVCS"
set "EXE_NAME=%PROJECT_NAME%.exe"
set "BASS_DLL=bass.dll"
set "BUILD_DIR=build"
set "DEBUG_DIR=debug"

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

REM ---- Configure and build Debug ----
echo [2/3] Configuring and building Debug...
cd /d "%PROJECT_ROOT%\%BUILD_DIR%"
"%CMAKE_CMD%" -G "%CMAKE_GENERATOR%" -A x64 "%PROJECT_ROOT%"
if errorlevel 1 (
    echo [ERROR] CMake configuration failed.
    cd /d "%PROJECT_ROOT%"
    pause
    exit /b 1
)
"%CMAKE_CMD%" --build . --config Debug
if errorlevel 1 (
    echo [ERROR] Build failed.
    cd /d "%PROJECT_ROOT%"
    pause
    exit /b 1
)
cd /d "%PROJECT_ROOT%"

REM ---- Deploy files to debug directory ----
echo [3/3] Deploying debug files...
call "%SCRIPT_DIR%_deploy.bat" "%DEBUG_DIR%" Debug DEBUG_INFO.txt
if errorlevel 1 (
    echo [ERROR] File deployment failed.
    cd /d "%PROJECT_ROOT%"
    pause
    exit /b 1
)

REM ---- Append debug-specific info ----
(
echo.
echo Debug Features:
echo - Debug symbols enabled
echo - Enhanced error logging to Visual Studio Output window
echo - Detailed BASS audio library error reporting
echo - Configuration loading diagnostics
echo.
echo Debug Instructions:
echo 1. Open %BUILD_DIR%\%PROJECT_NAME%.sln in Visual Studio
echo 2. Select Debug configuration
echo 3. Run with debugging (F5)
echo 4. Check the Output window for debug messages
) >> "%PROJECT_ROOT%\%DEBUG_DIR%\DEBUG_INFO.txt"

echo.
echo =====================================
echo Debug package complete!
echo =====================================
echo Output dir: %PROJECT_ROOT%\%DEBUG_DIR%
echo.
echo Contents:
dir /b "%PROJECT_ROOT%\%DEBUG_DIR%"
echo.
echo To debug:
echo   1. Open %PROJECT_ROOT%\%BUILD_DIR%\%PROJECT_NAME%.sln
echo   2. Set Debug configuration
echo   3. Run with debugging (F5)
echo   4. Check the Output window for debug messages
echo.
pause
