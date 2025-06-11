@echo off
REM EVCS项目编译脚本 (Windows批处理版本)
REM 作者: GitHub Copilot
REM 创建日期: 2025-06-11

echo ==================================================
echo               EVCS 项目编译脚本
echo ==================================================

REM 设置错误时退出
setlocal enabledelayedexpansion

REM 检查CMake是否安装
cmake --version >nul 2>&1
if errorlevel 1 (
    echo 错误: 未找到CMake，请确保已安装CMake并添加到PATH环境变量
    pause
    exit /b 1
)

REM 获取脚本所在目录，然后定位到项目根目录
set "SCRIPT_DIR=%~dp0"
set "PROJECT_DIR=%SCRIPT_DIR%.."
cd /d "%PROJECT_DIR%"

echo 项目根目录: %PROJECT_DIR%

REM 设置构建目录
set "BUILD_DIR=%PROJECT_DIR%\build"

REM 创建构建目录
if not exist "%BUILD_DIR%" (
    echo 创建构建目录: %BUILD_DIR%
    mkdir "%BUILD_DIR%"
)

REM 进入构建目录
cd /d "%BUILD_DIR%"

echo.
echo 正在生成项目文件...
REM 生成Visual Studio项目文件 (注意这里使用 %PROJECT_DIR% 而不是 ..)
cmake -G "Visual Studio 17 2022" -A x64 "%PROJECT_DIR%"
if errorlevel 1 (
    echo 错误: CMake配置失败
    pause
    exit /b 1
)

echo.
echo 正在编译项目...
REM 编译项目
cmake --build . --config Release
if errorlevel 1 (
    echo 错误: 编译失败
    pause
    exit /b 1
)

echo.
echo ==================================================
echo               编译完成！
echo ==================================================
echo 可执行文件位置: %BUILD_DIR%\Release\EVCS.exe
echo.

REM 检查可执行文件是否存在
if exist "%BUILD_DIR%\Release\EVCS.exe" (
    echo 编译成功！您可以在 build\Release 目录下找到 EVCS.exe
    echo 完整路径: %BUILD_DIR%\Release\EVCS.exe
) else (
    echo 警告: 未找到可执行文件，请检查编译输出
)

echo.
pause
