@echo off
setlocal enabledelayedexpansion

rem ============================================================================
rem IME Pinyin Windows Full Build Script
rem 
rem This script builds the complete Windows installer including:
rem   1. librime core library
rem   2. Weasel frontend components
rem   3. Dictionary data
rem   4. NSIS installer package
rem 
rem Prerequisites:
rem   - Visual Studio 2019/2022 with C++ workload
rem   - CMake 3.16+
rem   - NSIS 3.08+
rem   - Boost libraries
rem   - Git
rem 
rem Usage:
rem   build-windows-installer.bat [options]
rem 
rem Options:
rem   all       - Build everything (default)
rem   deps      - Build dependencies only
rem   weasel    - Build Weasel only
rem   installer - Build installer only
rem   clean     - Clean build artifacts
rem 
rem Environment Variables:
rem   BOOST_ROOT    - Path to Boost libraries
rem   IME_VERSION   - Version number (default: 1.0.0)
rem   IME_BUILD     - Build number (default: 0)
rem ============================================================================

echo ============================================
echo IME Pinyin Windows Full Build
echo ============================================
echo.

rem Get script directory
set SCRIPT_DIR=%~dp0
set PROJECT_ROOT=%SCRIPT_DIR%..

rem Set default version
if not defined IME_VERSION set IME_VERSION=1.0.0
if not defined IME_BUILD set IME_BUILD=0
set PRODUCT_VERSION=%IME_VERSION%.%IME_BUILD%

echo Project Root: %PROJECT_ROOT%
echo Version: %PRODUCT_VERSION%
echo.

rem Parse command line
set BUILD_DEPS=0
set BUILD_WEASEL=0
set BUILD_INSTALLER=0
set BUILD_CLEAN=0

if "%1"=="" (
    set BUILD_DEPS=1
    set BUILD_WEASEL=1
    set BUILD_INSTALLER=1
)

:parse_args
if "%1"=="" goto end_parse
if "%1"=="all" (
    set BUILD_DEPS=1
    set BUILD_WEASEL=1
    set BUILD_INSTALLER=1
)
if "%1"=="deps" set BUILD_DEPS=1
if "%1"=="weasel" set BUILD_WEASEL=1
if "%1"=="installer" set BUILD_INSTALLER=1
if "%1"=="clean" set BUILD_CLEAN=1
shift
goto parse_args
:end_parse

rem Clean if requested
if %BUILD_CLEAN%==1 (
    echo Cleaning build artifacts...
    if exist "%PROJECT_ROOT%\output\windows" rmdir /s /q "%PROJECT_ROOT%\output\windows"
    if exist "%PROJECT_ROOT%\deps\weasel\build" rmdir /s /q "%PROJECT_ROOT%\deps\weasel\build"
    echo Clean completed.
    if %BUILD_DEPS%==0 if %BUILD_WEASEL%==0 if %BUILD_INSTALLER%==0 exit /b 0
)

rem Check for Visual Studio
where cl >nul 2>&1
if errorlevel 1 (
    echo ERROR: Visual Studio C++ compiler not found!
    echo Please run this script from a Developer Command Prompt.
    exit /b 1
)

rem Check for NSIS
set NSIS_PATH=
if exist "%ProgramFiles(x86)%\NSIS\Bin\makensis.exe" (
    set "NSIS_PATH=%ProgramFiles(x86)%\NSIS\Bin\makensis.exe"
) else if exist "%ProgramFiles%\NSIS\Bin\makensis.exe" (
    set "NSIS_PATH=%ProgramFiles%\NSIS\Bin\makensis.exe"
)

if "%NSIS_PATH%"=="" (
    if %BUILD_INSTALLER%==1 (
        echo ERROR: NSIS not found! Required for building installer.
        echo Please install NSIS from https://nsis.sourceforge.io/
        exit /b 1
    )
)

rem Create output directory
if not exist "%PROJECT_ROOT%\output\windows" mkdir "%PROJECT_ROOT%\output\windows"

rem Build dependencies (librime)
if %BUILD_DEPS%==1 (
    echo.
    echo ============================================
    echo Building Dependencies (librime)
    echo ============================================
    echo.
    
    cd /d "%PROJECT_ROOT%\deps\weasel"
    
    rem Check for env.bat
    if not exist env.bat (
        if exist env.bat.template (
            copy env.bat.template env.bat
            echo Created env.bat from template. Please configure BOOST_ROOT.
        )
    )
    
    call build.bat boost
    if errorlevel 1 (
        echo ERROR: Failed to build Boost!
        exit /b 1
    )
    
    call build.bat rime
    if errorlevel 1 (
        echo ERROR: Failed to build librime!
        exit /b 1
    )
    
    call build.bat data
    if errorlevel 1 (
        echo ERROR: Failed to build data!
        exit /b 1
    )
    
    call build.bat opencc
    if errorlevel 1 (
        echo ERROR: Failed to build OpenCC!
        exit /b 1
    )
    
    echo Dependencies built successfully.
)

rem Build Weasel
if %BUILD_WEASEL%==1 (
    echo.
    echo ============================================
    echo Building Weasel
    echo ============================================
    echo.
    
    cd /d "%PROJECT_ROOT%\deps\weasel"
    
    call xbuild.bat weasel
    if errorlevel 1 (
        echo ERROR: Failed to build Weasel!
        exit /b 1
    )
    
    echo Weasel built successfully.
    
    rem Copy build artifacts to output
    echo.
    echo Copying build artifacts...
    
    set OUTPUT_DIR=%PROJECT_ROOT%\output\windows\staging
    if not exist "%OUTPUT_DIR%" mkdir "%OUTPUT_DIR%"
    if not exist "%OUTPUT_DIR%\data" mkdir "%OUTPUT_DIR%\data"
    if not exist "%OUTPUT_DIR%\data\opencc" mkdir "%OUTPUT_DIR%\data\opencc"
    if not exist "%OUTPUT_DIR%\Win32" mkdir "%OUTPUT_DIR%\Win32"
    
    rem Copy main executables
    copy /y output\WeaselServer.exe "%OUTPUT_DIR%\IMEServer.exe" >nul
    copy /y output\WeaselDeployer.exe "%OUTPUT_DIR%\IMEDeployer.exe" >nul
    copy /y output\WeaselSetup.exe "%OUTPUT_DIR%\IMESetup.exe" >nul
    copy /y output\rime.dll "%OUTPUT_DIR%\" >nul
    
    rem Copy Win32 versions
    if exist output\Win32\WeaselServer.exe (
        copy /y output\Win32\WeaselServer.exe "%OUTPUT_DIR%\Win32\IMEServer.exe" >nul
        copy /y output\Win32\WeaselDeployer.exe "%OUTPUT_DIR%\Win32\IMEDeployer.exe" >nul
        copy /y output\Win32\rime.dll "%OUTPUT_DIR%\Win32\" >nul
    )
    
    rem Copy IME DLLs
    copy /y output\weasel.dll "%OUTPUT_DIR%\ime_pinyin.dll" >nul
    copy /y output\weasel.ime "%OUTPUT_DIR%\ime_pinyin.ime" >nul
    if exist output\weaselx64.dll (
        copy /y output\weaselx64.dll "%OUTPUT_DIR%\ime_pinyinx64.dll" >nul
        copy /y output\weaselx64.ime "%OUTPUT_DIR%\ime_pinyinx64.ime" >nul
    )
    
    rem Copy data files
    copy /y output\data\*.yaml "%OUTPUT_DIR%\data\" >nul 2>&1
    copy /y output\data\*.txt "%OUTPUT_DIR%\data\" >nul 2>&1
    copy /y output\data\opencc\*.json "%OUTPUT_DIR%\data\opencc\" >nul 2>&1
    copy /y output\data\opencc\*.ocd* "%OUTPUT_DIR%\data\opencc\" >nul 2>&1
    
    rem Copy license and readme
    copy /y "%PROJECT_ROOT%\installer\windows\LICENSE.txt" "%OUTPUT_DIR%\" >nul
    copy /y "%PROJECT_ROOT%\installer\windows\README.txt" "%OUTPUT_DIR%\" >nul
    
    echo Build artifacts copied to %OUTPUT_DIR%
)

rem Build installer
if %BUILD_INSTALLER%==1 (
    echo.
    echo ============================================
    echo Building Installer
    echo ============================================
    echo.
    
    cd /d "%PROJECT_ROOT%\installer\windows"
    
    rem Copy staging files to installer directory
    set STAGING_DIR=%PROJECT_ROOT%\output\windows\staging
    if exist "%STAGING_DIR%" (
        xcopy /y /e "%STAGING_DIR%\*" . >nul 2>&1
    )
    
    "%NSIS_PATH%" ^
        /DIME_VERSION=%IME_VERSION% ^
        /DIME_BUILD=%IME_BUILD% ^
        /DPRODUCT_VERSION=%PRODUCT_VERSION% ^
        install.nsi
    
    if errorlevel 1 (
        echo ERROR: Failed to build installer!
        exit /b 1
    )
    
    echo.
    echo Installer built successfully!
    echo Output: %PROJECT_ROOT%\output\windows\ime-pinyin-%PRODUCT_VERSION%-installer.exe
)

echo.
echo ============================================
echo Build completed successfully!
echo ============================================

cd /d "%PROJECT_ROOT%"
exit /b 0
