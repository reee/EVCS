@echo off
setlocal enabledelayedexpansion

echo ==================================================
echo               EVCS Build Script (Release x64)
echo ==================================================

set "SCRIPT_DIR=%~dp0"
set "PROJECT_DIR=%SCRIPT_DIR%.."
set "BUILD_DIR=%PROJECT_DIR%\build"

echo Project root: %PROJECT_DIR%
echo.

REM ---- Toolchain detection (sets CMAKE_CMD / CMAKE_GENERATOR) ----
call "%SCRIPT_DIR%_detect.bat"
if errorlevel 1 (
    echo.
    echo [FAILED] Toolchain detection failed, aborting.
    pause
    exit /b 1
)
echo.

REM ---- Create build directory ----
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
cd /d "%BUILD_DIR%"

REM ---- Generate project files ----
echo Generating project files...
"%CMAKE_CMD%" -G "%CMAKE_GENERATOR%" -A x64 "%PROJECT_DIR%"
if errorlevel 1 (
    echo [ERROR] CMake configuration failed.
    cd /d "%PROJECT_DIR%"
    pause
    exit /b 1
)

REM ---- Build Release ----
echo.
echo Building Release...
"%CMAKE_CMD%" --build . --config Release
if errorlevel 1 (
    echo [ERROR] Build failed.
    cd /d "%PROJECT_DIR%"
    pause
    exit /b 1
)

cd /d "%PROJECT_DIR%"

echo.
echo ==================================================
echo               Build complete!
echo ==================================================
if exist "%BUILD_DIR%\Release\EVCS.exe" (
    echo Executable: %BUILD_DIR%\Release\EVCS.exe
) else (
    echo [WARN] EVCS.exe not found at expected location
)
echo.
pause
