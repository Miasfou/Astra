param (
    [switch]$Clean = $false,
    [switch]$NoLaunch = $false
)

$ErrorActionPreference = "Stop"
$Root = $PSScriptRoot
$StartDir = Get-Location

# --- 1. BUILD SECTION ---
Write-Host "==========================================" -ForegroundColor Cyan
Write-Host "      Astra Chess - RELEASE BUILD" -ForegroundColor Cyan
Write-Host "==========================================" -ForegroundColor Cyan

try {
    # Optional Clean
    if ($Clean -and (Test-Path "$Root\build")) { 
        Write-Host "Cleaning build directory..." -ForegroundColor Yellow
        Remove-Item "$Root\build" -Recurse -Force 
    }

    # Create Build Dir
    if (!(Test-Path "$Root\build")) { New-Item -ItemType Directory -Force -Path "$Root\build" | Out-Null }
    Set-Location "$Root\build"

    # Configure (Force Release type)
    Write-Host "Configuring CMake (Release)..." -ForegroundColor Gray
    cmake .. -DCMAKE_BUILD_TYPE=Release

    # Build
    Write-Host "Compiling..." -ForegroundColor Yellow
    cmake --build . --config Release

    # Copy Executables to Root
    $Apps = @("AstraChess.exe")
    foreach ($app in $Apps) {
        $src = "$Root\build\Release\$app"
        if (!(Test-Path $src)) { $src = "$Root\build\$app" }
        
        if (Test-Path $src) { 
            Copy-Item $src -Destination "$Root\$app" -Force 
            Write-Host "Updated $app" -ForegroundColor Green
        }
    }

    Write-Host "Build Successful!" -ForegroundColor Green
}
catch {
    Write-Error "Build failed: $_"
    Set-Location $StartDir
    exit 1
}
finally {
    Set-Location $StartDir
}

# --- 2. LAUNCH ---
if (-not $NoLaunch) {
    if (Test-Path "$Root\AstraChess.exe") {
        Write-Host "`nLaunching Astra Chess..." -ForegroundColor Cyan
        & "$Root\AstraChess.exe"
    }
}