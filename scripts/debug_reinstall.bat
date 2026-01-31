@echo off
setlocal DisableDelayedExpansion
chcp 65001 >nul
echo ========================================
echo SuYan IME Debug ^& Update Script
echo ========================================
echo.

set ROOT_DIR=%~dp0..
set BUILD_X64=%ROOT_DIR%\build-win
set BUILD_X86=%ROOT_DIR%\build-win-x86
set DLL64=%BUILD_X64%\bin\Release\SuYan.dll
set DLL32=%BUILD_X64%\bin\Release\SuYan32.dll
set SERVER=%BUILD_X64%\bin\Release\SuYanServer.exe
set CLSID={A1B2C3D4-E5F6-7890-ABCD-EF1234567890}

if "%QT_PREFIX%"=="" set QT_PREFIX=C:\Qt\6.7.2\msvc2019_64
if "%VCPKG_PREFIX%"=="" set VCPKG_PREFIX=C:\vcpkg\installed\x64-windows

:: 1. 停止 Server 并注销 DLL
echo [Step 1] Stopping server and unregistering DLLs...
taskkill /F /IM SuYanServer.exe >nul 2>&1
if exist "%DLL64%" regsvr32 /u /s "%DLL64%"
if exist "%DLL32%" regsvr32 /u /s "%DLL32%"
timeout /t 1 /nobreak >nul

:: 2. 构建
echo.
echo [Step 2] Building...
cmake --build "%BUILD_X64%" --config Release --target SuYanTSF SuYanServer -j
if errorlevel 1 (
    echo Build failed!
    pause
    exit /b 1
)

if exist "%BUILD_X86%" (
    cmake --build "%BUILD_X86%" --config Release --target SuYanTSF -j
    if exist "%BUILD_X86%\src\platform\windows\tsf_dll\Release\SuYan32.dll" (
        copy /Y "%BUILD_X86%\src\platform\windows\tsf_dll\Release\SuYan32.dll" "%BUILD_X64%\bin\Release\" >nul
    )
)

:: 3. 写入调试用注册表
echo.
echo [Step 3] Setting up registry...
reg add "HKLM\Software\Microsoft\Windows\CurrentVersion\Uninstall\SuYan" /v InstallLocation /t REG_SZ /d "%BUILD_X64%\bin\Release" /f /reg:64 >nul

:: 4. 注册 DLL
echo.
echo [Step 4] Registering DLLs...
if exist "%DLL64%" regsvr32 /s "%DLL64%" & echo   Registered SuYan.dll
if exist "%DLL32%" regsvr32 /s "%DLL32%" & echo   Registered SuYan32.dll

:: 5. 启动 Server
echo.
echo [Step 5] Starting server...
start "" "%SERVER%"

echo.
echo ========================================
echo Done! Switch to another IME and back to test.
echo ========================================
pause
