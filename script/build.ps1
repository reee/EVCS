# EVCS项目编译脚本 (PowerShell版本)
# 作者: GitHub Copilot
# 创建日期: 2025-06-11

param(
    [string]$Config = "Release",
    [string]$Generator = "Visual Studio 17 2022",
    [switch]$Clean,
    [switch]$Help
)

function Show-Help {
    Write-Host "EVCS项目编译脚本" -ForegroundColor Green
    Write-Host "用法: .\build.ps1 [参数]" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "参数:" -ForegroundColor Yellow
    Write-Host "  -Config <配置>     编译配置 (Debug/Release，默认: Release)"
    Write-Host "  -Generator <生成器> CMake生成器 (默认: Visual Studio 17 2022)"
    Write-Host "  -Clean             清理构建目录后重新编译"
    Write-Host "  -Help              显示此帮助信息"
    Write-Host ""
    Write-Host "示例:" -ForegroundColor Yellow
    Write-Host "  .\build.ps1                    # 使用默认设置编译"
    Write-Host "  .\build.ps1 -Config Debug      # 编译Debug版本"
    Write-Host "  .\build.ps1 -Clean             # 清理后编译"
}

if ($Help) {
    Show-Help
    exit 0
}

Write-Host "==================================================" -ForegroundColor Green
Write-Host "               EVCS 项目编译脚本" -ForegroundColor Green
Write-Host "==================================================" -ForegroundColor Green

# 检查CMake是否安装
try {
    $cmakeVersion = cmake --version 2>$null
    if ($LASTEXITCODE -ne 0) {
        throw "CMake not found"
    }
    Write-Host "✓ CMake已安装: $($cmakeVersion[0])" -ForegroundColor Green
} catch {
    Write-Host "✗ 错误: 未找到CMake，请确保已安装CMake并添加到PATH环境变量" -ForegroundColor Red
    exit 1
}

# 获取脚本所在目录，然后定位到项目根目录
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectDir = Split-Path -Parent $ScriptDir
$BuildDir = Join-Path $ProjectDir "build"

Write-Host "脚本目录: $ScriptDir" -ForegroundColor Cyan
Write-Host "项目目录: $ProjectDir" -ForegroundColor Cyan
Write-Host "构建目录: $BuildDir" -ForegroundColor Cyan
Write-Host "编译配置: $Config" -ForegroundColor Cyan

# 清理构建目录（如果需要）
if ($Clean -and (Test-Path $BuildDir)) {
    Write-Host "正在清理构建目录..." -ForegroundColor Yellow
    Remove-Item $BuildDir -Recurse -Force
    Write-Host "✓ 构建目录已清理" -ForegroundColor Green
}

# 创建构建目录
if (-not (Test-Path $BuildDir)) {
    Write-Host "创建构建目录: $BuildDir" -ForegroundColor Yellow
    New-Item -ItemType Directory -Path $BuildDir | Out-Null
}

# 进入构建目录
Set-Location $BuildDir

try {
    Write-Host ""
    Write-Host "正在生成项目文件..." -ForegroundColor Yellow
      # 生成项目文件
    $cmakeArgs = @("-G", $Generator, "-A", "x64", $ProjectDir)
    & cmake @cmakeArgs
    
    if ($LASTEXITCODE -ne 0) {
        throw "CMake配置失败"
    }
    
    Write-Host "✓ 项目文件生成成功" -ForegroundColor Green
    
    Write-Host ""
    Write-Host "正在编译项目..." -ForegroundColor Yellow
    
    # 编译项目
    & cmake --build . --config $Config
    
    if ($LASTEXITCODE -ne 0) {
        throw "编译失败"
    }
    
    Write-Host "✓ 编译成功" -ForegroundColor Green
    
    Write-Host ""
    Write-Host "==================================================" -ForegroundColor Green
    Write-Host "               编译完成！" -ForegroundColor Green
    Write-Host "==================================================" -ForegroundColor Green
    
    $ExePath = Join-Path $BuildDir "$Config\EVCS.exe"
    if (Test-Path $ExePath) {
        Write-Host "✓ 可执行文件位置: $ExePath" -ForegroundColor Green
        $FileInfo = Get-Item $ExePath
        Write-Host "  文件大小: $([math]::Round($FileInfo.Length / 1KB, 2)) KB" -ForegroundColor Cyan
        Write-Host "  修改时间: $($FileInfo.LastWriteTime)" -ForegroundColor Cyan
    } else {
        Write-Host "⚠ 警告: 未找到可执行文件，请检查编译输出" -ForegroundColor Yellow
    }
    
} catch {
    Write-Host "✗ 错误: $_" -ForegroundColor Red
    Set-Location $ProjectDir
    exit 1
} finally {
    # 回到项目根目录
    Set-Location $ProjectDir
}

Write-Host ""
Write-Host "编译脚本执行完成！" -ForegroundColor Green
