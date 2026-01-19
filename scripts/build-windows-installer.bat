@echo off
setlocal enabledelayedexpansion

set SCRIPT_DIR=%~dp0
set PROJECT_ROOT=%SCRIPT_DIR%..

rem Read brand.conf
for /f "usebackq tokens=1,* delims==" %%a in ("%PROJECT_ROOT%\brand.conf") do (
    set "line=%%a"
    if not "!line:~0,1!"=="#" if not "!line!"=="" (
        set "key=%%a"
        set "val=%%b"
        set "val=!val:"=!"
        if not "!val!"=="" set "!key!=!val!"
    )
)

if not defined IME_BRAND_NAME set IME_BRAND_NAME=SuYan
if not defined IME_BRAND_NAME_CN set IME_BRAND_NAME_CN=%IME_BRAND_NAME%
if not defined IME_VERSION set IME_VERSION=1.0.0
if not defined IME_BUILD set IME_BUILD=0
if not defined IME_COMPANY_NAME set IME_COMPANY_NAME=Unknown
set PRODUCT_VERSION=%IME_VERSION%.%IME_BUILD%

echo ============================================
echo %IME_BRAND_NAME% Windows Build (x64)
echo ============================================
echo Version: %PRODUCT_VERSION%

set BUILD_DEPS=0
set BUILD_WEASEL=0
set BUILD_INSTALLER=0

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
shift
goto parse_args
:end_parse

where cl >nul 2>&1
if errorlevel 1 (
    echo ERROR: Run from Developer Command Prompt
    exit /b 1
)

set NSIS_PATH=
if exist "%ProgramFiles(x86)%\NSIS\Bin\makensis.exe" (
    set "NSIS_PATH=%ProgramFiles(x86)%\NSIS\Bin\makensis.exe"
) else if exist "%ProgramFiles%\NSIS\Bin\makensis.exe" (
    set "NSIS_PATH=%ProgramFiles%\NSIS\Bin\makensis.exe"
)

if "%NSIS_PATH%"=="" if %BUILD_INSTALLER%==1 (
    echo ERROR: NSIS not found
    exit /b 1
)

if not exist "%PROJECT_ROOT%\output\windows" mkdir "%PROJECT_ROOT%\output\windows"

cd /d "%PROJECT_ROOT%\deps\weasel"
if not exist env.bat copy env.bat.template env.bat

if %BUILD_DEPS%==1 (
    echo Building dependencies...
    call xbuild.bat boost rime data opencc
    if errorlevel 1 exit /b 1
)

if %BUILD_WEASEL%==1 (
    echo Building Weasel (x64)...
    call xbuild.bat weasel
    if errorlevel 1 exit /b 1

    echo Copying files...
    set DST=%PROJECT_ROOT%\installer\windows
    copy /y output\WeaselServer.exe "!DST!\" >nul
    copy /y output\WeaselDeployer.exe "!DST!\" >nul
    copy /y output\WeaselSetup.exe "!DST!\" >nul
    copy /y output\rime.dll "!DST!\" >nul
    copy /y output\weaselx64.dll "!DST!\" >nul
    copy /y output\weaselx64.ime "!DST!\" >nul
    copy /y output\WinSparkle.dll "!DST!\" >nul 2>&1

    if not exist "!DST!\data" mkdir "!DST!\data"
    if not exist "!DST!\data\opencc" mkdir "!DST!\data\opencc"
    copy /y output\data\*.yaml "!DST!\data\" >nul 2>&1
    copy /y output\data\*.txt "!DST!\data\" >nul 2>&1
    copy /y output\data\opencc\*.* "!DST!\data\opencc\" >nul 2>&1
)

if %BUILD_INSTALLER%==1 (
    echo Copying rime-ice data...
    set DST=%PROJECT_ROOT%\installer\windows\data
    set SRC=%PROJECT_ROOT%\deps\rime-ice
    
    copy /y "!SRC!\default.yaml" "!DST!\" >nul
    copy /y "!SRC!\rime_ice.schema.yaml" "!DST!\" >nul
    copy /y "!SRC!\rime_ice.dict.yaml" "!DST!\" >nul
    copy /y "!SRC!\melt_eng.schema.yaml" "!DST!\" >nul
    copy /y "!SRC!\melt_eng.dict.yaml" "!DST!\" >nul
    copy /y "!SRC!\symbols_v.yaml" "!DST!\" >nul
    copy /y "!SRC!\symbols_caps_v.yaml" "!DST!\" >nul
    copy /y "%PROJECT_ROOT%\data\rime\default.custom.yaml" "!DST!\" >nul
    
    if not exist "!DST!\cn_dicts" mkdir "!DST!\cn_dicts"
    copy /y "!SRC!\cn_dicts\*.yaml" "!DST!\cn_dicts\" >nul
    
    if not exist "!DST!\en_dicts" mkdir "!DST!\en_dicts"
    copy /y "!SRC!\en_dicts\*.yaml" "!DST!\en_dicts\" >nul
    copy /y "!SRC!\en_dicts\*.txt" "!DST!\en_dicts\" >nul
    
    if not exist "!DST!\lua" mkdir "!DST!\lua"
    copy /y "!SRC!\lua\*.lua" "!DST!\lua\" >nul
    
    if not exist "!DST!\opencc" mkdir "!DST!\opencc"
    copy /y "!SRC!\opencc\*.*" "!DST!\opencc\" >nul

    echo Building installer...
    cd /d "%PROJECT_ROOT%\installer\windows"
    "%NSIS_PATH%" /DIME_NAME="%IME_BRAND_NAME%" /DIME_NAME_CN="%IME_BRAND_NAME_CN%" /DIME_VERSION=%IME_VERSION% /DIME_BUILD=%IME_BUILD% /DPRODUCT_VERSION=%PRODUCT_VERSION% /DIME_COMPANY="%IME_COMPANY_NAME%" install.nsi
    if errorlevel 1 exit /b 1
    echo Output: %PROJECT_ROOT%\output\windows\%IME_BRAND_NAME%-%PRODUCT_VERSION%-installer.exe
)

echo Build completed.
cd /d "%PROJECT_ROOT%"
