# EVCS项目清理脚本 (PowerShell版本)
# 作者: GitHub Copilot
# 创建日期: 2025-06-11

param(
    [switch]$Force,
    [switch]$Help
)

function Show-Help {
    Write-Host "EVCS项目清理脚本" -ForegroundColor Green
    Write-Host "用法: .\clean.ps1 [参数]" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "参数:" -ForegroundColor Yellow
    Write-Host "  -Force    强制清理，不询问确认"
    Write-Host "  -Help     显示此帮助信息"
}

if ($Help) {
    Show-Help
    exit 0
}

Write-Host "==================================================" -ForegroundColor Green
Write-Host "               EVCS 项目清理脚本" -ForegroundColor Green
Write-Host "==================================================" -ForegroundColor Green

# 获取脚本所在目录，然后定位到项目根目录
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectDir = Split-Path -Parent $ScriptDir
$BuildDir = Join-Path $ProjectDir "build"

Write-Host "脚本目录: $ScriptDir" -ForegroundColor Cyan

Write-Host "项目目录: $ProjectDir" -ForegroundColor Cyan

# 检查构建目录是否存在
if (Test-Path $BuildDir) {
    Write-Host "发现构建目录: $BuildDir" -ForegroundColor Yellow
    
    if (-not $Force) {
        $confirmation = Read-Host "确定要删除构建目录吗？ (y/N)"
        if ($confirmation -ne 'y' -and $confirmation -ne 'Y') {
            Write-Host "取消清理操作" -ForegroundColor Yellow
            exit 0
        }
    }
    
    try {
        Write-Host "正在清理构建目录..." -ForegroundColor Yellow
        Remove-Item $BuildDir -Recurse -Force
        Write-Host "✓ 构建目录已清理完成" -ForegroundColor Green
    } catch {
        Write-Host "⚠ 警告: 无法完全清理构建目录: $_" -ForegroundColor Yellow
        Write-Host "  可能有文件正在使用中，请关闭相关程序后重试" -ForegroundColor Yellow
    }
} else {
    Write-Host "构建目录不存在，无需清理" -ForegroundColor Cyan
}

# 清理临时文件
Write-Host ""
Write-Host "正在清理临时文件..." -ForegroundColor Yellow

$TempFiles = @("*.tmp", "*.log", "*.bak", "*~")
$CleanedCount = 0

foreach ($Pattern in $TempFiles) {
    $Files = Get-ChildItem -Path $ProjectDir -Filter $Pattern -Recurse -File -ErrorAction SilentlyContinue
    foreach ($File in $Files) {
        try {
            Remove-Item $File.FullName -Force
            Write-Host "  删除: $($File.Name)" -ForegroundColor Gray
            $CleanedCount++
        } catch {
            Write-Host "  无法删除: $($File.Name)" -ForegroundColor Yellow
        }
    }
}

if ($CleanedCount -gt 0) {
    Write-Host "✓ 清理了 $CleanedCount 个临时文件" -ForegroundColor Green
} else {
    Write-Host "✓ 未发现需要清理的临时文件" -ForegroundColor Green
}

Write-Host ""
Write-Host "==================================================" -ForegroundColor Green
Write-Host "               清理完成！" -ForegroundColor Green
Write-Host "==================================================" -ForegroundColor Green
