@echo off
setlocal enabledelayedexpansion

rem ============================================================================
rem IME Pinyin Windows Installer Build Script
rem 
rem This script builds the Windows installer using NSIS.
rem 
rem Prerequisites:
rem   - NSIS 3.08+ installed (https://nsis.sourceforge.io/)
rem   - Visual Studio 2019/2022 with C++ workload
rem   - All build artifacts in output directory
rem 
rem Usage:
rem   build-installer.bat [version] [build]
rem   
rem   version: Version number (default: 1.0.0)
rem   build:   Build number (default: 0)
rem 
rem Examples:
rem   build-installer.bat
rem   build-installer.bat 1.0.0 1
rem   build-installer.bat 1.1.0 0
rem ============================================================================

echo ============================================
echo IME Pinyin Windows Installer Builder
echo ============================================
echo.

rem Parse command line arguments
set IME_VERSION=%1
set IME_BUILD=%2

if "%IME_VERSION%"=="" set IME_VERSION=1.0.0
if "%IME_BUILD%"=="" set IME_BUILD=0

set PRODUCT_VERSION=%IME_VERSION%.%IME_BUILD%

echo Version: %IME_VERSION%
echo Build:   %IME_BUILD%
echo Product: %PRODUCT_VERSION%
echo.

rem Get script directory
set SCRIPT_DIR=%~dp0
set PROJECT_ROOT=%SCRIPT_DIR%..\..

rem Check for NSIS
set NSIS_PATH=
if exist "%ProgramFiles(x86)%\NSIS\Bin\makensis.exe" (
    set "NSIS_PATH=%ProgramFiles(x86)%\NSIS\Bin\makensis.exe"
) else if exist "%ProgramFiles%\NSIS\Bin\makensis.exe" (
    set "NSIS_PATH=%ProgramFiles%\NSIS\Bin\makensis.exe"
) else (
    echo ERROR: NSIS not found!
    echo Please install NSIS from https://nsis.sourceforge.io/
    exit /b 1
)

echo Found NSIS: %NSIS_PATH%
echo.

rem Create output directory
if not exist "%PROJECT_ROOT%\output\windows" (
    mkdir "%PROJECT_ROOT%\output\windows"
)

rem Check for required files
echo Checking required files...

set MISSING_FILES=0

if not exist "%SCRIPT_DIR%install.nsi" (
    echo ERROR: install.nsi not found!
    set MISSING_FILES=1
)

if not exist "%SCRIPT_DIR%LICENSE.txt" (
    echo WARNING: LICENSE.txt not found, creating placeholder...
    echo IME Pinyin License > "%SCRIPT_DIR%LICENSE.txt"
)

if not exist "%SCRIPT_DIR%README.txt" (
    echo WARNING: README.txt not found, creating placeholder...
    echo IME Pinyin - Cross-platform Chinese Input Method > "%SCRIPT_DIR%README.txt"
)

rem Create resources directory if not exists
if not exist "%SCRIPT_DIR%resources" (
    mkdir "%SCRIPT_DIR%resources"
)

rem Check for icon (create placeholder if missing)
if not exist "%SCRIPT_DIR%resources\ime.ico" (
    echo WARNING: ime.ico not found in resources directory
    echo Please add an icon file before final release
)

if %MISSING_FILES%==1 (
    echo.
    echo ERROR: Missing required files. Please ensure all files are in place.
    exit /b 1
)

echo All required files found.
echo.

rem Build the installer
echo Building installer...
echo.

"%NSIS_PATH%" ^
    /DIME_VERSION=%IME_VERSION% ^
    /DIME_BUILD=%IME_BUILD% ^
    /DPRODUCT_VERSION=%PRODUCT_VERSION% ^
    "%SCRIPT_DIR%install.nsi"

if errorlevel 1 (
    echo.
    echo ERROR: Failed to build installer!
    exit /b 1
)

echo.
echo ============================================
echo Installer built successfully!
echo Output: %PROJECT_ROOT%\output\windows\ime-pinyin-%PRODUCT_VERSION%-installer.exe
echo ============================================

exit /b 0
