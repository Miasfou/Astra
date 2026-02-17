# run.ps1
$ErrorActionPreference = "Stop"
$Root = $PSScriptRoot

# 1. Build
if (!(Test-Path "$Root\build")) { New-Item -ItemType Directory -Force -Path "$Root\build" | Out-Null }
Set-Location "$Root\build"
try {
    if (!(Test-Path "CMakeCache.txt")) { cmake .. }
    cmake --build . --config Release
} catch {
    Write-Host "Build failed." -ForegroundColor Red
    exit
}

# 2. Find Exe
$ExePath = "$Root\build\Release\AstraChess.exe"
if (!(Test-Path $ExePath)) { $ExePath = "$Root\build\AstraChess.exe" }

if (!(Test-Path $ExePath)) {
    Write-Host "Could not find AstraChess.exe" -ForegroundColor Red
    exit
}

# 3. Run from Root
Write-Host "Running from: $Root" -ForegroundColor Green
Set-Location $Root
& $ExePath