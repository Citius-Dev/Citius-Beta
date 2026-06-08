@echo off
setlocal enabledelayedexpansion

SET VSDEVCMD=F:\dev\VSBT\Common7\Tools\VsDevCmd.bat
SET PROJECT_DIR=%~dp0
SET BUILD_DIR=%PROJECT_DIR%build
SET CMAKE=C:\Program Files\CMake\bin\cmake.exe

echo === Citius Compiler Build ===
echo Project: %PROJECT_DIR%

REM Find CMake
if not exist "%CMAKE%" (
    where cmake >nul 2>&1
    if errorlevel 1 (
        echo Error: CMake not found
        exit /b 1
    )
    set CMAKE=cmake
)

REM Setup VS environment
if exist "%VSDEVCMD%" (
    call "%VSDEVCMD%" -arch=x64 -host_arch=x64
) else (
    echo Warning: VsDevCmd.bat not found, trying to find MSVC directly
)

REM Create build directory
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

REM Configure with CMake
echo Configuring...
"%CMAKE%" -S "%PROJECT_DIR%" -B "%BUILD_DIR%" -G "Ninja" -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 (
    REM Try Visual Studio generator
    "%CMAKE%" -S "%PROJECT_DIR%" -B "%BUILD_DIR%" -DCMAKE_BUILD_TYPE=Release
)

if errorlevel 1 (
    echo Error: CMake configuration failed
    exit /b 1
)

REM Build
echo Building...
"%CMAKE%" --build "%BUILD_DIR%" --config Release

if errorlevel 1 (
    echo Error: Build failed
    exit /b 1
)

echo === Build successful! ===
echo Compiler: %BUILD_DIR%\bin\citius.exe
