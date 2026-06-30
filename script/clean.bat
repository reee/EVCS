@echo off
setlocal enabledelayedexpansion

echo ==================================================
echo               EVCS Clean Script
echo ==================================================

set "SCRIPT_DIR=%~dp0"
set "PROJECT_DIR=%SCRIPT_DIR%.."
cd /d "%PROJECT_DIR%"

echo Project root: %PROJECT_DIR%
echo.

REM Clean build directory
set "BUILD_DIR=%PROJECT_DIR%\build"
if exist "%BUILD_DIR%" (
    echo Cleaning build directory: %BUILD_DIR%
    rmdir /s /q "%BUILD_DIR%"
    if errorlevel 1 (
        echo   [WARN] Could not fully clean (files may be in use)
    ) else (
        echo   [OK]
    )
) else (
    echo Build directory not found, skipping.
)

REM Clean release directory
set "RELEASE_DIR=%PROJECT_DIR%\release"
if exist "%RELEASE_DIR%" (
    echo Cleaning release directory: %RELEASE_DIR%
    rmdir /s /q "%RELEASE_DIR%"
    if errorlevel 1 (
        echo   [WARN] Could not fully clean (files may be in use)
    ) else (
        echo   [OK]
    )
) else (
    echo Release directory not found, skipping.
)

REM Clean debug directory
set "DEBUG_DIR=%PROJECT_DIR%\debug"
if exist "%DEBUG_DIR%" (
    echo Cleaning debug directory: %DEBUG_DIR%
    rmdir /s /q "%DEBUG_DIR%"
    if errorlevel 1 (
        echo   [WARN] Could not fully clean (files may be in use)
    ) else (
        echo   [OK]
    )
) else (
    echo Debug directory not found, skipping.
)

REM Clean temp files
echo.
echo Cleaning temp files...
if exist "*.tmp" del /q "*.tmp"
if exist "*.log" del /q "*.log"

echo.
echo ==================================================
echo               Clean complete!
echo ==================================================
echo.
pause
