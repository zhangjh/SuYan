@echo off
rem ============================================================================
rem IME Pinyin Silent Uninstallation Script
rem 
rem This script performs a silent uninstallation of IME Pinyin.
rem Useful for enterprise deployment and automated uninstallations.
rem 
rem Usage:
rem   silent-uninstall.bat
rem 
rem The script will:
rem   1. Find the installation directory from registry
rem   2. Run the uninstaller in silent mode
rem   3. Clean up any remaining files
rem ============================================================================

setlocal enabledelayedexpansion

echo ============================================
echo IME Pinyin Silent Uninstallation
echo ============================================
echo.

rem Check for admin privileges
net session >nul 2>&1
if errorlevel 1 (
    echo ERROR: This script requires administrator privileges.
    echo Please run as administrator.
    exit /b 1
)

rem Find installation directory from registry
set INSTALL_DIR=
for /f "tokens=2*" %%a in ('reg query "HKLM\Software\IMEPinyin" /v InstallDir 2^>nul ^| find "InstallDir"') do (
    set INSTALL_DIR=%%b
)

if "%INSTALL_DIR%"=="" (
    echo IME Pinyin is not installed or installation directory not found.
    exit /b 0
)

echo Installation directory: %INSTALL_DIR%
echo.

rem Check for uninstaller
set UNINSTALLER=%INSTALL_DIR%\uninstall.exe
if not exist "%UNINSTALLER%" (
    echo ERROR: Uninstaller not found: %UNINSTALLER%
    echo Attempting manual cleanup...
    goto manual_cleanup
)

echo Running silent uninstallation...
echo.

"%UNINSTALLER%" /S

if errorlevel 1 (
    echo WARNING: Uninstaller returned error code %errorlevel%
    echo Attempting manual cleanup...
    goto manual_cleanup
)

echo.
echo Waiting for uninstallation to complete...
timeout /t 5 /nobreak >nul

goto verify_uninstall

:manual_cleanup
echo.
echo Performing manual cleanup...

rem Stop IME server
taskkill /f /im IMEServer.exe >nul 2>&1

rem Remove registry keys
reg delete "HKLM\Software\IMEPinyin" /f >nul 2>&1
reg delete "HKLM\Software\Microsoft\Windows\CurrentVersion\Uninstall\IMEPinyin" /f >nul 2>&1
reg delete "HKLM\Software\Microsoft\Windows\CurrentVersion\Run" /v IMEServer /f >nul 2>&1

rem Try 64-bit registry as well
reg delete "HKLM\Software\IMEPinyin" /f /reg:64 >nul 2>&1
reg delete "HKLM\Software\Microsoft\Windows\CurrentVersion\Run" /v IMEServer /f /reg:64 >nul 2>&1

rem Remove files
if exist "%INSTALL_DIR%" (
    rmdir /s /q "%INSTALL_DIR%" >nul 2>&1
)

rem Remove start menu shortcuts
set STARTMENU=%ProgramData%\Microsoft\Windows\Start Menu\Programs
if exist "%STARTMENU%\拼音输入法" (
    rmdir /s /q "%STARTMENU%\拼音输入法" >nul 2>&1
)
if exist "%STARTMENU%\IME Pinyin" (
    rmdir /s /q "%STARTMENU%\IME Pinyin" >nul 2>&1
)

:verify_uninstall
echo.
echo Verifying uninstallation...

reg query "HKLM\Software\Microsoft\Windows\CurrentVersion\Uninstall\IMEPinyin" >nul 2>&1
if errorlevel 1 (
    echo Uninstallation verified - registry key removed
) else (
    echo WARNING: Registry key still exists
)

if exist "%INSTALL_DIR%" (
    echo WARNING: Installation directory still exists: %INSTALL_DIR%
) else (
    echo Installation directory removed
)

echo.
echo ============================================
echo Uninstallation completed!
echo A system restart may be required.
echo ============================================

exit /b 0
