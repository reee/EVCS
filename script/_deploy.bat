@echo off
REM ==================================================
REM EVCS file deployment library (_deploy.bat)
REM Called by release.bat / debug.bat
REM No setlocal: variables are passed back to caller
REM
REM Usage:
REM   call _deploy.bat <output_dir> <config> <info_file>
REM e.g.:
REM   call _deploy.bat release Release RELEASE_INFO.txt
REM   call _deploy.bat debug   Debug   DEBUG_INFO.txt
REM
REM Prerequisites (set by caller):
REM   PROJECT_ROOT, SCRIPT_DIR, BUILD_DIR, EXE_NAME, BASS_DLL, PROJECT_NAME
REM   Build must be complete: %BUILD_DIR%\<config>\%EXE_NAME%
REM
REM Exit code: 0=ok, 1=fail (error already printed)
REM ==================================================

set "DEPLOY_DIR_NAME=%~1"
set "DEPLOY_CONFIG=%~2"
set "DEPLOY_INFO_FILE=%~3"

if "%DEPLOY_DIR_NAME%"=="" (
    echo [ERROR] _deploy.bat missing argument: output_dir
    exit /b 1
)
if "%DEPLOY_CONFIG%"=="" (
    echo [ERROR] _deploy.bat missing argument: config ^(Release/Debug^)
    exit /b 1
)
if "%DEPLOY_INFO_FILE%"=="" (
    echo [ERROR] _deploy.bat missing argument: info_file
    exit /b 1
)

set "DEPLOY_DIR=%PROJECT_ROOT%\%DEPLOY_DIR_NAME%"

REM ---- Clean and recreate output dir ----
echo [deploy] Preparing output dir: %DEPLOY_DIR%
if exist "%DEPLOY_DIR%" rmdir /s /q "%DEPLOY_DIR%"
mkdir "%DEPLOY_DIR%" 2>nul
mkdir "%DEPLOY_DIR%\config" 2>nul
mkdir "%DEPLOY_DIR%\audio" 2>nul

REM ---- Copy executable ----
set "SRC_EXE=%PROJECT_ROOT%\%BUILD_DIR%\%DEPLOY_CONFIG%\%EXE_NAME%"
if not exist "%SRC_EXE%" (
    echo [ERROR] Build output not found: %SRC_EXE%
    exit /b 1
)
copy "%SRC_EXE%" "%DEPLOY_DIR%\" >nul
echo   - %EXE_NAME% [OK]

REM ---- Copy BASS DLL ----
copy "%PROJECT_ROOT%\third_party\bass\x64\bass.dll" "%DEPLOY_DIR%\" >nul
echo   - bass.dll [OK]

REM ---- Copy README ----
if exist "%PROJECT_ROOT%\README.txt" (
    copy "%PROJECT_ROOT%\README.txt" "%DEPLOY_DIR%\" >nul
    echo   - README.txt [OK]
) else (
    echo   - README.txt [skipped: not found]
)

REM ---- Copy config files ----
if exist "%PROJECT_ROOT%\config\*" (
    xcopy "%PROJECT_ROOT%\config\*" "%DEPLOY_DIR%\config\" /y >nul
    echo   - config\ [OK]
) else (
    echo   - config\ [skipped: not found or empty]
)

REM ---- Copy audio files (if any) ----
if exist "%PROJECT_ROOT%\audio\*" (
    xcopy "%PROJECT_ROOT%\audio\*" "%DEPLOY_DIR%\audio\" /e /y >nul
    echo   - audio\ [OK]
) else (
    echo   - audio\ [note: none; user must add their own]
)

REM ---- Generate info file (base; debug.bat may append extras) ----
(
echo Build Information:
echo ==================
echo Project: %PROJECT_NAME%
echo Build Date: %date% %time%
echo Build Type: %DEPLOY_CONFIG% x64
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
) > "%DEPLOY_DIR%\%DEPLOY_INFO_FILE%"
echo   - %DEPLOY_INFO_FILE% [OK]

exit /b 0
