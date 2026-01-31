@echo off
REM ============================================
REM SuYan Input Method - Windows 一键构建脚本
REM ============================================
REM
REM 用法:
REM   scripts\build_pkg.bat [选项]
REM
REM 选项:
REM   --skip-build    跳过构建步骤，直接打包
REM   --help          显示帮助信息
REM
REM 前置条件:
REM   - CMake 3.20+
REM   - Visual Studio 2022
REM   - Qt 6.x
REM   - NSIS
REM
REM 输出:
REM   - installer\SuYan-<version>-Setup.exe

setlocal enabledelayedexpansion

set "SCRIPT_DIR=%~dp0"
set "PROJECT_DIR=%SCRIPT_DIR%.."
set "BUILD_X64=%PROJECT_DIR%\build-win"
set "BUILD_X86=%PROJECT_DIR%\build-win32"
set "INSTALLER_DIR=%PROJECT_DIR%\installer"

REM 默认参数
set "SKIP_BUILD=0"
set "VERSION=1.0.0"

REM 解析参数
:parse_args
if "%~1"=="" goto :check_env
if /i "%~1"=="--skip-build" (
    set "SKIP_BUILD=1"
    shift
    goto :parse_args
)
if /i "%~1"=="--help" goto :show_help
echo 错误: 未知选项 %~1
goto :show_help

:show_help
echo.
echo SuYan Input Method - Windows 一键构建脚本
echo.
echo 用法: %~nx0 [选项]
echo.
echo 选项:
echo   --skip-build    跳过构建步骤，直接打包
echo   --help          显示帮助信息
echo.
echo 示例:
echo   %~nx0                  完整构建并打包
echo   %~nx0 --skip-build     跳过构建，直接打包
echo.
exit /b 0

:check_env
echo.
echo ============================================
echo   SuYan Input Method - Windows 构建脚本
echo ============================================
echo.

REM 检查 Qt
if "%QT_PREFIX%"=="" set "QT_PREFIX=C:\Qt\6.7.2\msvc2019_64"
if not exist "%QT_PREFIX%\bin\Qt6Core.dll" (
    echo [错误] Qt6 未找到: %QT_PREFIX%
    echo 请设置 QT_PREFIX 环境变量指向 Qt 安装目录
    exit /b 1
)
echo [OK] Qt6: %QT_PREFIX%

REM 检查 vcpkg
if "%VCPKG_PREFIX%"=="" set "VCPKG_PREFIX=C:\vcpkg\installed\x64-windows"
if not exist "%VCPKG_PREFIX%\bin\yaml-cpp.dll" (
    echo [错误] vcpkg 依赖未找到: %VCPKG_PREFIX%
    echo 请运行: vcpkg install yaml-cpp sqlite3
    exit /b 1
)
echo [OK] vcpkg: %VCPKG_PREFIX%

REM 检查 NSIS
set "NSIS_PATH="
where makensis >nul 2>&1
if errorlevel 1 (
    if exist "C:\Program Files (x86)\NSIS\makensis.exe" (
        set "NSIS_PATH=C:\Program Files (x86)\NSIS\"
    ) else if exist "C:\Program Files\NSIS\makensis.exe" (
        set "NSIS_PATH=C:\Program Files\NSIS\"
    ) else (
        echo [错误] NSIS 未找到
        echo 请从 https://nsis.sourceforge.io/ 安装 NSIS
        exit /b 1
    )
)
echo [OK] NSIS 已安装

REM 检查 librime
if not exist "%PROJECT_DIR%\deps\librime\lib\x64\rime.dll" (
    echo [错误] librime 未找到
    echo 请运行: scripts\get-rime-windows.ps1
    exit /b 1
)
echo [OK] librime 已就绪
echo.

if "%SKIP_BUILD%"=="1" goto :build_installer

REM ============================================
REM 构建 64 位
REM ============================================
echo ============================================
echo [1/4] 构建 64 位版本...
echo ============================================
if not exist "%BUILD_X64%" mkdir "%BUILD_X64%"
cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="%QT_PREFIX%;%VCPKG_PREFIX%" -S "%PROJECT_DIR%" -B "%BUILD_X64%"
if errorlevel 1 (
    echo [错误] CMake 配置失败
    exit /b 1
)
cmake --build "%BUILD_X64%" --config Release
if errorlevel 1 (
    echo [错误] 64 位构建失败
    exit /b 1
)
echo [OK] 64 位构建完成
echo.

REM ============================================
REM 构建 32 位
REM ============================================
echo ============================================
echo [2/4] 构建 32 位 TSF DLL...
echo ============================================
if not exist "%BUILD_X86%" mkdir "%BUILD_X86%"
cmake -G "Visual Studio 17 2022" -A Win32 -S "%PROJECT_DIR%" -B "%BUILD_X86%"
if errorlevel 1 (
    echo [错误] 32 位 CMake 配置失败
    exit /b 1
)
cmake --build "%BUILD_X86%" --config Release --target SuYan32
if errorlevel 1 (
    echo [错误] 32 位构建失败
    exit /b 1
)
copy /Y "%BUILD_X86%\src\platform\windows\tsf_dll\Release\SuYan32.dll" "%BUILD_X64%\bin\Release\" >nul
echo [OK] 32 位构建完成
echo.

REM ============================================
REM 部署依赖
REM ============================================
echo ============================================
echo [3/4] 部署依赖...
echo ============================================
"%QT_PREFIX%\bin\windeployqt" --release --no-translations "%BUILD_X64%\bin\Release\SuYanServer.exe"
copy /Y "%VCPKG_PREFIX%\bin\yaml-cpp.dll" "%BUILD_X64%\bin\Release\" >nul
copy /Y "%VCPKG_PREFIX%\bin\sqlite3.dll" "%BUILD_X64%\bin\Release\" >nul
copy /Y "%PROJECT_DIR%\deps\librime\lib\x64\rime.dll" "%BUILD_X64%\bin\Release\" >nul
echo [OK] 依赖部署完成
echo.

:build_installer
REM ============================================
REM 创建安装包
REM ============================================
echo ============================================
echo [4/4] 创建安装包...
echo ============================================

REM 验证构建产物
if not exist "%BUILD_X64%\bin\Release\SuYanServer.exe" (
    echo [错误] SuYanServer.exe 未找到，请先构建
    exit /b 1
)
if not exist "%BUILD_X64%\bin\Release\SuYan.dll" (
    echo [错误] SuYan.dll 未找到
    exit /b 1
)
if not exist "%BUILD_X64%\bin\Release\SuYan32.dll" (
    echo [错误] SuYan32.dll 未找到
    exit /b 1
)

cd /d "%INSTALLER_DIR%"
"%NSIS_PATH%makensis" suyan.nsi
if errorlevel 1 (
    echo [错误] 安装包创建失败
    exit /b 1
)
cd /d "%PROJECT_DIR%"

echo.
echo ============================================
echo   构建完成！
echo ============================================
echo.
echo   安装包: %INSTALLER_DIR%\SuYan-%VERSION%-Setup.exe
echo.
echo   安装方法:
echo     双击运行安装程序，或使用命令行:
echo     SuYan-%VERSION%-Setup.exe /S
echo.

exit /b 0
