@echo off
REM ============================================
REM SuYan Input Method - Installer Build Script
REM ============================================
setlocal enabledelayedexpansion

set "PROJECT_DIR=%~dp0.."
set "BUILD_DIR=%PROJECT_DIR%\build-win"
set "INSTALLER_DIR=%PROJECT_DIR%\installer"

echo ============================================
echo SuYan Installer Build Script
echo ============================================
echo.

REM Check for NSIS
set "NSIS_PATH="
where makensis >nul 2>&1
if errorlevel 1 (
    if exist "C:\Program Files (x86)\NSIS\makensis.exe" (
        set "NSIS_PATH=C:\Program Files (x86)\NSIS\"
    ) else if exist "C:\Program Files\NSIS\makensis.exe" (
        set "NSIS_PATH=C:\Program Files\NSIS\"
    ) else (
        echo ERROR: NSIS not found in PATH or standard locations
        echo Please install NSIS from https://nsis.sourceforge.io/
        exit /b 1
    )
)

REM Check for build output
if not exist "%BUILD_DIR%\bin\Release\SuYanServer.exe" (
    echo ERROR: Build output not found
    echo Please run build_release.bat first
    exit /b 1
)

if not exist "%BUILD_DIR%\bin\Release\SuYan.dll" (
    echo ERROR: SuYan.dll not found
    exit /b 1
)

if not exist "%BUILD_DIR%\bin\Release\SuYan32.dll" (
    echo ERROR: SuYan32.dll not found
    exit /b 1
)

echo Building installer...
cd /d "%INSTALLER_DIR%"
"%NSIS_PATH%makensis" suyan.nsi
if errorlevel 1 (
    echo ERROR: Installer build failed
    exit /b 1
)

echo.
echo ============================================
echo Installer created successfully!
echo ============================================
echo.
echo Installer: %INSTALLER_DIR%\SuYan-1.0.0-Setup.exe
echo.
