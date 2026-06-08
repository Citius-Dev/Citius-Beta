# Build script for Citius compiler on Windows
param(
    [string]$BuildType = "Release",
    [string]$VSInstallPath = "F:\dev\VSBT"
)

$ProjectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$BuildDir = Join-Path $ProjectRoot "build"

# Create build directory
New-Item -ItemType Directory -Path $BuildDir -Force | Out-Null

# Find MSVC compiler
$MSPath = Join-Path $VSInstallPath "MSVC"
if (Test-Path $MSPath) {
    $MSVCDirs = Get-ChildItem -Path $MSPath -Directory | Sort-Object Name -Descending
    if ($MSVCDirs.Count -gt 0) {
        $VCToolsPath = Join-Path $MSVCDirs[0].FullName "bin\Hostx64\x64"
        $Env:Path = "$VCToolsPath;$Env:Path"
    }
}

# Find CMake
$CmakePath = "C:\Program Files\CMake\bin\cmake.exe"
if (-not (Test-Path $CmakePath)) {
    Write-Error "CMake not found. Please install CMake."
    exit 1
}

Write-Host "=== Citius Compiler Build ===" -ForegroundColor Cyan
Write-Host "Build type: $BuildType"
Write-Host "Build dir:  $BuildDir"

# Configure
& $CmakePath -S $ProjectRoot -B $BuildDir -G "Ninja" `
    -DCMAKE_BUILD_TYPE=$BuildType `
    -DLLVM_DIR="C:\Program Files\LLVM\lib\cmake\llvm"

if ($LASTEXITCODE -ne 0) {
    # Try with Visual Studio generator
    & $CmakePath -S $ProjectRoot -B $BuildDir `
        -DCMAKE_BUILD_TYPE=$BuildType `
        -DLLVM_DIR="C:\Program Files\LLVM\lib\cmake\llvm"
}

if ($LASTEXITCODE -ne 0) {
    Write-Error "CMake configuration failed"
    exit 1
}

# Build
& $CmakePath --build $BuildDir --config $BuildType

if ($LASTEXITCODE -ne 0) {
    Write-Error "Build failed"
    exit 1
}

Write-Host "=== Build successful! ===" -ForegroundColor Green
Write-Host "Compiler: $BuildDir\bin\citius.exe"
