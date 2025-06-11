@echo off
REM EVCS项目清理脚本
REM 作者: GitHub Copilot
REM 创建日期: 2025-06-11

echo ==================================================
echo               EVCS 项目清理脚本
echo ==================================================

REM 获取脚本所在目录，然后定位到项目根目录
set "SCRIPT_DIR=%~dp0"
set "PROJECT_DIR=%SCRIPT_DIR%.."
cd /d "%PROJECT_DIR%"

echo 项目根目录: %PROJECT_DIR%

REM 设置构建目录
set "BUILD_DIR=%PROJECT_DIR%\build"

if exist "%BUILD_DIR%" (
    echo 正在清理构建目录: %BUILD_DIR%
    rmdir /s /q "%BUILD_DIR%"
    if errorlevel 1 (
        echo 警告: 无法完全清理构建目录，可能有文件正在使用中
    ) else (
        echo 构建目录已清理完成
    )
) else (
    echo 构建目录不存在，无需清理
)

REM 清理临时文件
echo.
echo 正在清理临时文件...
if exist "*.tmp" del /q "*.tmp"
if exist "*.log" del /q "*.log"

echo.
echo ==================================================
echo               清理完成！
echo ==================================================
echo.
pause
