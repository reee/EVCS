@echo off
REM ==================================================
REM EVCS toolchain detection library (_detect.bat)
REM Called by build.bat / debug.bat / release.bat
REM No setlocal: variables are passed back to caller
REM
REM Output variables (for caller):
REM   CMAKE_CMD        - cmake command (absolute path or 'cmake')
REM   CMAKE_GENERATOR  - CMake generator name (Visual Studio 17 2022 / 16 2019)
REM   VSWHERE          - path to vswhere.exe (if located)
REM
REM Exit code: 0=ok, 1=fail (error already printed)
REM ==================================================

REM ---- project root (for dependency check); caller sets SCRIPT_DIR ----
if not defined SCRIPT_DIR set "SCRIPT_DIR=%~dp0"
set "PROJECT_ROOT=%SCRIPT_DIR%.."

REM ---- 1. Locate cmake.exe: PATH first, vswhere fallback ----
set "CMAKE_CMD="
set "VSWHERE="

REM 1a. Try cmake on PATH
cmake --version >nul 2>&1
if not errorlevel 1 (
    set "CMAKE_CMD=cmake"
    goto :cmake_found
)

REM 1b. No cmake on PATH, locate VS-bundled cmake via vswhere
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" set "VSWHERE=%ProgramW6432%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" set "VSWHERE=%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe"

if not exist "%VSWHERE%" (
    echo [ERROR] cmake not found, and Visual Studio Installer ^(vswhere^) not found.
    echo         Install one of:
    echo           - CMake 3.10+ and add it to PATH, OR
    echo           - Visual Studio 2019/2022 with the C++ workload and CMake component.
    exit /b 1
)

REM Find latest VS install with C++ tools
set "VS_INSTALL_PATH="
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
    set "VS_INSTALL_PATH=%%i"
)

if not defined VS_INSTALL_PATH (
    echo [ERROR] vswhere could not locate a Visual Studio install with the C++ toolchain.
    echo         In Visual Studio Installer, enable the "Desktop development with C++" workload.
    exit /b 1
)

set "BUNDLED_CMAKE=%VS_INSTALL_PATH%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
if not exist "%BUNDLED_CMAKE%" (
    echo [ERROR] Visual Studio is installed but the CMake component is missing:
    echo         %BUNDLED_CMAKE%
    echo         Enable "C++ CMake tools for Windows" in Visual Studio Installer,
    echo         or install CMake separately and add it to PATH.
    exit /b 1
)
set "CMAKE_CMD=%BUNDLED_CMAKE%"

:cmake_found

REM ---- 2. Pick CMake generator: VS2022 preferred, VS2019 fallback ----
if not defined VS_INSTALL_PATH (
    REM cmake from PATH: assume VS2022 generator (modern CMake auto-detects installed VS)
    set "CMAKE_GENERATOR=Visual Studio 17 2022"
    goto :generator_done
)

set "VS_VERSION="
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationVersion`) do (
    set "VS_VERSION=%%i"
)

set "VS_MAJOR="
for /f "tokens=1 delims=." %%i in ("%VS_VERSION%") do set "VS_MAJOR=%%i"

if "%VS_MAJOR%"=="17" (
    set "CMAKE_GENERATOR=Visual Studio 17 2022"
) else if "%VS_MAJOR%"=="16" (
    set "CMAKE_GENERATOR=Visual Studio 16 2019"
) else (
    echo [WARN] Detected VS major version %VS_MAJOR%; not mapped, trying VS2022 generator.
    set "CMAKE_GENERATOR=Visual Studio 17 2022"
)

:generator_done

REM ---- 3. Dependency check ----
if not exist "%PROJECT_ROOT%\third_party\bass\x64\bass.dll" (
    echo [ERROR] Missing dependency: third_party\bass\x64\bass.dll
    echo         Download BASS Audio Library and place bass.dll there.
    exit /b 1
)

echo [detect] CMake     : %CMAKE_CMD%
echo [detect] Generator : %CMAKE_GENERATOR%
exit /b 0
