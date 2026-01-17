@echo off
REM Windows 构建脚本
REM 用法: scripts\build-windows.bat [Debug|Release]

setlocal enabledelayedexpansion

set BUILD_TYPE=%1
if "%BUILD_TYPE%"=="" set BUILD_TYPE=Debug

set SCRIPT_DIR=%~dp0
set PROJECT_ROOT=%SCRIPT_DIR%..
set BUILD_DIR=%PROJECT_ROOT%\build\windows-%BUILD_TYPE%

echo === CrossPlatformIME Windows 构建 ===
echo 构建类型: %BUILD_TYPE%
echo 构建目录: %BUILD_DIR%
echo.

REM 检查 CMake
where cmake >nul 2>nul
if %ERRORLEVEL% neq 0 (
    echo 错误: 未找到 cmake
    echo 请安装 CMake 并添加到 PATH
    exit /b 1
)

REM 检查 Visual Studio
set VS_FOUND=0
if exist "%ProgramFiles%\Microsoft Visual Studio\2022" set VS_FOUND=1
if exist "%ProgramFiles%\Microsoft Visual Studio\2019" set VS_FOUND=1
if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2022" set VS_FOUND=1
if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2019" set VS_FOUND=1

if %VS_FOUND%==0 (
    echo 错误: 未找到 Visual Studio 2019 或 2022
    echo 请安装 Visual Studio 并包含 C++ 开发工具
    exit /b 1
)

echo √ 所有依赖已就绪
echo.

REM 检查 submodules
if not exist "%PROJECT_ROOT%\deps\librime\CMakeLists.txt" (
    echo 警告: librime submodule 未初始化
    echo 正在初始化 submodules...
    cd /d "%PROJECT_ROOT%"
    git submodule update --init --recursive
)

REM 创建构建目录
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
cd /d "%BUILD_DIR%"

REM 配置 CMake (使用 Visual Studio 生成器)
echo 配置 CMake...
cmake "%PROJECT_ROOT%" ^
    -G "Visual Studio 17 2022" ^
    -A x64 ^
    -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
    -DBUILD_TESTS=ON ^
    -DBUILD_WINDOWS_FRONTEND=OFF ^
    -DENABLE_CLOUD_SYNC=ON

if %ERRORLEVEL% neq 0 (
    echo CMake 配置失败，尝试使用 Visual Studio 2019...
    cmake "%PROJECT_ROOT%" ^
        -G "Visual Studio 16 2019" ^
        -A x64 ^
        -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
        -DBUILD_TESTS=ON ^
        -DBUILD_WINDOWS_FRONTEND=OFF ^
        -DENABLE_CLOUD_SYNC=ON
)

if %ERRORLEVEL% neq 0 (
    echo 错误: CMake 配置失败
    exit /b 1
)

REM 构建
echo.
echo 开始构建...
cmake --build . --config %BUILD_TYPE% --parallel

if %ERRORLEVEL% neq 0 (
    echo 错误: 构建失败
    exit /b 1
)

echo.
echo === 构建完成 ===
echo 输出目录: %BUILD_DIR%\bin\%BUILD_TYPE%
echo.

REM 运行测试 (可选)
if "%2"=="--test" (
    echo 运行测试...
    ctest -C %BUILD_TYPE% --output-on-failure
)

endlocal
