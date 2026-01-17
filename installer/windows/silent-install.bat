@echo off
rem ============================================================================
rem IME Pinyin Silent Installation Script
rem 
rem This script performs a silent installation of IME Pinyin.
rem Useful for enterprise deployment and automated installations.
rem 
rem Usage:
rem   silent-install.bat [installer_path]
rem 
rem Options (passed to installer):
rem   /S        - Silent mode (no UI)
rem   /D=path   - Installation directory (must be last parameter)
rem 
rem Examples:
rem   silent-install.bat
rem   silent-install.bat ime-pinyin-1.0.0-installer.exe
rem   silent-install.bat ime-pinyin-1.0.0-installer.exe /D=C:\IME
rem ============================================================================

setlocal enabledelayedexpansion

echo ============================================
echo IME Pinyin Silent Installation
echo ============================================
echo.

rem Get installer path
set INSTALLER=%1
if "%INSTALLER%"=="" (
    rem Find latest installer in current directory
    for %%f in (ime-pinyin-*-installer.exe) do set INSTALLER=%%f
)

if "%INSTALLER%"=="" (
    echo ERROR: No installer found!
    echo Please specify the installer path or run from the directory containing the installer.
    exit /b 1
)

if not exist "%INSTALLER%" (
    echo ERROR: Installer not found: %INSTALLER%
    exit /b 1
)

echo Installer: %INSTALLER%
echo.

rem Get additional parameters
set EXTRA_PARAMS=
shift
:parse_params
if "%1"=="" goto end_params
set EXTRA_PARAMS=%EXTRA_PARAMS% %1
shift
goto parse_params
:end_params

echo Running silent installation...
echo Command: "%INSTALLER%" /S %EXTRA_PARAMS%
echo.

"%INSTALLER%" /S %EXTRA_PARAMS%

if errorlevel 1 (
    echo.
    echo ERROR: Installation failed with error code %errorlevel%
    exit /b %errorlevel%
)

echo.
echo ============================================
echo Installation completed successfully!
echo ============================================

rem Verify installation
echo.
echo Verifying installation...

reg query "HKLM\Software\Microsoft\Windows\CurrentVersion\Uninstall\IMEPinyin" >nul 2>&1
if errorlevel 1 (
    echo WARNING: Installation registry key not found
) else (
    echo Installation verified in registry
)

exit /b 0
