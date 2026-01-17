; IME Pinyin Installation Script
; Based on Weasel (小狼毫) NSIS script
; 
; This script creates a Windows installer for the cross-platform IME
; Requirements: NSIS 3.08+

!include FileFunc.nsh
!include LogicLib.nsh
!include MUI2.nsh
!include x64.nsh
!include winVer.nsh

Unicode true

;--------------------------------
; Version Configuration

!ifndef IME_VERSION
!define IME_VERSION 1.0.0
!endif

!ifndef IME_BUILD
!define IME_BUILD 0
!endif

!ifndef PRODUCT_VERSION
!define PRODUCT_VERSION ${IME_VERSION}.${IME_BUILD}
!endif

;--------------------------------
; General Settings

!define IME_NAME "拼音输入法"
!define IME_NAME_EN "IME Pinyin"
!define IME_PUBLISHER "IME Pinyin Team"
!define IME_URL "https://github.com/example/ime-pinyin"
!define IME_ROOT $INSTDIR\ime-pinyin-${IME_VERSION}
!define REG_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\IMEPinyin"
!define REG_IME_KEY "Software\IMEPinyin"

; Installer name
Name "${IME_NAME} ${IME_VERSION}"

; Output file
OutFile "..\..\output\windows\ime-pinyin-${PRODUCT_VERSION}-installer.exe"

; Version info
VIProductVersion "${IME_VERSION}.${IME_BUILD}"
VIAddVersionKey /LANG=2052 "ProductName" "${IME_NAME}"
VIAddVersionKey /LANG=2052 "Comments" "基于 RIME 引擎的跨平台中文输入法"
VIAddVersionKey /LANG=2052 "CompanyName" "${IME_PUBLISHER}"
VIAddVersionKey /LANG=2052 "LegalCopyright" "Copyright (C) ${IME_PUBLISHER}"
VIAddVersionKey /LANG=2052 "FileDescription" "${IME_NAME}"
VIAddVersionKey /LANG=2052 "FileVersion" "${IME_VERSION}"

; Icon
!define MUI_ICON "resources\ime.ico"
!define MUI_UNICON "resources\ime.ico"

; Compression
SetCompressor /SOLID lzma

; Request admin privileges
RequestExecutionLevel admin

;--------------------------------
; Interface Settings

!define MUI_ABORTWARNING
!define MUI_WELCOMEFINISHPAGE_BITMAP "resources\welcome.bmp"
!define MUI_UNWELCOMEFINISHPAGE_BITMAP "resources\welcome.bmp"

;--------------------------------
; Pages

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "LICENSE.txt"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

;--------------------------------
; Languages

!insertmacro MUI_LANGUAGE "SimpChinese"
LangString DISPLAYNAME ${LANG_SIMPCHINESE} "拼音输入法"
LangString LNKFORSETTING ${LANG_SIMPCHINESE} "【拼音输入法】设置"
LangString LNKFORDEPLOY ${LANG_SIMPCHINESE} "【拼音输入法】重新部署"
LangString LNKFORSERVER ${LANG_SIMPCHINESE} "拼音输入法服务"
LangString LNKFORUSERFOLDER ${LANG_SIMPCHINESE} "【拼音输入法】用户文件夹"
LangString LNKFORAPPFOLDER ${LANG_SIMPCHINESE} "【拼音输入法】程序文件夹"
LangString LNKFORUNINSTALL ${LANG_SIMPCHINESE} "卸载拼音输入法"
LangString CONFIRMATION ${LANG_SIMPCHINESE} '安装前，请先卸载旧版本。$\n$\n点击 "确定" 移除旧版本，或点击 "取消" 放弃本次安装。'
LangString SYSTEMVERSIONNOTOK ${LANG_SIMPCHINESE} "您的系统不被支持，最低系统要求: Windows 8.1!"

!insertmacro MUI_LANGUAGE "English"
LangString DISPLAYNAME ${LANG_ENGLISH} "IME Pinyin"
LangString LNKFORSETTING ${LANG_ENGLISH} "IME Pinyin Settings"
LangString LNKFORDEPLOY ${LANG_ENGLISH} "IME Pinyin Deploy"
LangString LNKFORSERVER ${LANG_ENGLISH} "IME Pinyin Server"
LangString LNKFORUSERFOLDER ${LANG_ENGLISH} "IME Pinyin User Folder"
LangString LNKFORAPPFOLDER ${LANG_ENGLISH} "IME Pinyin App Folder"
LangString LNKFORUNINSTALL ${LANG_ENGLISH} "Uninstall IME Pinyin"
LangString CONFIRMATION ${LANG_ENGLISH} "Before installation, please uninstall the old version.$\n$\nPress 'OK' to remove the old version, or 'Cancel' to abort installation."
LangString SYSTEMVERSIONNOTOK ${LANG_ENGLISH} "Your system is not supported. Minimum system required: Windows 8.1!"

;--------------------------------
; Installer Functions

Function .onInit
  ; Check Windows version (minimum Windows 8.1)
  ${IfNot} ${AtLeastWin8.1}
    IfSilent toquit
    MessageBox MB_OK '$(SYSTEMVERSIONNOTOK)'
toquit:
    Quit
  ${EndIf}

  ; Check for existing installation
  ReadRegStr $R0 HKLM "${REG_IME_KEY}" "InstallDir"
  StrCmp $R0 "" 0 skip
  
  ; Set default installation directory based on architecture
  ${If} ${AtLeastWin11}
    ${If} ${IsNativeARM64}
      StrCpy $INSTDIR "$PROGRAMFILES64\IMEPinyin"
    ${ElseIf} ${IsNativeAMD64}
      StrCpy $INSTDIR "$PROGRAMFILES64\IMEPinyin"
    ${Else}
      StrCpy $INSTDIR "$PROGRAMFILES\IMEPinyin"
    ${Endif}
  ${Else}
    ${If} ${IsNativeAMD64}
      StrCpy $INSTDIR "$PROGRAMFILES64\IMEPinyin"
    ${Else}
      StrCpy $INSTDIR "$PROGRAMFILES\IMEPinyin"
    ${Endif}
  ${Endif}
skip:

  ; Check for previous installation
  ReadRegStr $R0 HKLM "${REG_UNINST_KEY}" "UninstallString"
  StrCmp $R0 "" done

  StrCpy $0 "Upgrade"
  IfSilent uninst 0
  MessageBox MB_OKCANCEL|MB_ICONINFORMATION "$(CONFIRMATION)" IDOK uninst
  Abort

uninst:
  ; Backup user data
  ReadRegStr $R1 HKLM "${REG_IME_KEY}" "InstallDir"
  StrCmp $R1 "" call_uninstaller
  IfFileExists $R1\data\*.* 0 call_uninstaller
  CreateDirectory $TEMP\ime-pinyin-backup
  CopyFiles $R1\data\*.* $TEMP\ime-pinyin-backup

call_uninstaller:
  ; Stop the IME server
  ExecWait '"$R1\IMEServer.exe" /quit'
  ExecWait '"$R1\IMESetup.exe" /u'
  
  ; Remove registry keys
  DeleteRegKey HKLM "${REG_IME_KEY}"
  DeleteRegKey HKLM "${REG_UNINST_KEY}"
  
  ; Remove autorun
  ${If} ${IsNativeARM64}
    SetRegView 64
  ${ElseIf} ${IsNativeAMD64}
    SetRegView 64
  ${Endif}
  DeleteRegValue HKLM "Software\Microsoft\Windows\CurrentVersion\Run" "IMEServer"
  SetRegView 32
  
  ; Remove files
  Delete "$R1\data\opencc\*.*"
  Delete "$R1\data\*.*"
  Delete "$R1\*.*"
  RMDir "$R1\data\opencc"
  RMDir "$R1\data"
  RMDir "$R1"
  
  ; Remove shortcuts
  SetShellVarContext all
  Delete "$SMPROGRAMS\$(DISPLAYNAME)\*.*"
  RMDir "$SMPROGRAMS\$(DISPLAYNAME)"
  
  SetRebootFlag true
  Sleep 800

done:
FunctionEnd

; Registry key for installation directory
InstallDirRegKey HKLM "${REG_IME_KEY}" "InstallDir"

;--------------------------------
; Main Installation Section

Section "IME Pinyin" SecMain
  SectionIn RO

  ; Write installation path to registry
  WriteRegStr HKLM "${REG_IME_KEY}" "InstallDir" "$INSTDIR"

  ; Reset INSTDIR for new version
  StrCpy $INSTDIR "${IME_ROOT}"

  ; Stop existing server if running
  IfFileExists "$INSTDIR\IMEServer.exe" 0 +2
  ExecWait '"$INSTDIR\IMEServer.exe" /quit'

  SetOverwrite try
  SetOutPath $INSTDIR

  ; Restore backup data if exists
  IfFileExists $TEMP\ime-pinyin-backup\*.* 0 program_files
  CreateDirectory $INSTDIR\data
  CopyFiles $TEMP\ime-pinyin-backup\*.* $INSTDIR\data
  RMDir /r $TEMP\ime-pinyin-backup

program_files:
  ; Core files
  File "LICENSE.txt"
  File "README.txt"
  
  ; IME DLLs (architecture-specific)
  File "ime_pinyin.dll"
  ${If} ${RunningX64}
    File "ime_pinyinx64.dll"
  ${EndIf}
  ${If} ${IsNativeARM64}
    File /nonfatal "ime_pinyinARM.dll"
    File /nonfatal "ime_pinyinARM64.dll"
  ${EndIf}
  
  ; IME files
  File "ime_pinyin.ime"
  ${If} ${RunningX64}
    File "ime_pinyinx64.ime"
  ${EndIf}
  ${If} ${IsNativeARM64}
    File /nonfatal "ime_pinyinARM.ime"
    File /nonfatal "ime_pinyinARM64.ime"
  ${EndIf}
  
  ; Main executables (architecture-specific)
  ${If} ${AtLeastWin11}
    ${If} ${IsNativeARM64}
      File "IMEDeployer.exe"
      File "IMEServer.exe"
      File "rime.dll"
    ${ElseIf} ${IsNativeAMD64}
      File "IMEDeployer.exe"
      File "IMEServer.exe"
      File "rime.dll"
    ${Else}
      File "Win32\IMEDeployer.exe"
      File "Win32\IMEServer.exe"
      File "Win32\rime.dll"
    ${Endif}
  ${Else}
    ${If} ${IsNativeAMD64}
      File "IMEDeployer.exe"
      File "IMEServer.exe"
      File "rime.dll"
    ${Else}
      File "Win32\IMEDeployer.exe"
      File "Win32\IMEServer.exe"
      File "Win32\rime.dll"
    ${Endif}
  ${Endif}

  File "IMESetup.exe"
  
  ; Data files
  SetOutPath $INSTDIR\data
  File "data\*.yaml"
  File /nonfatal "data\*.txt"
  File /nonfatal "data\*.gram"
  
  ; OpenCC data files
  SetOutPath $INSTDIR\data\opencc
  File "data\opencc\*.json"
  File "data\opencc\*.ocd*"

  SetOutPath $INSTDIR

  ; Run setup (register IME)
  StrCpy $R2 "/i"
  ${GetParameters} $R0
  ClearErrors
  ${GetOptions} $R0 "/S" $R1
  IfErrors +2 0
  StrCpy $R2 "/s"

  ExecWait '"$INSTDIR\IMESetup.exe" $R2'

  ; Write uninstall information
  WriteRegStr HKLM "${REG_UNINST_KEY}" "DisplayName" "$(DISPLAYNAME)"
  WriteRegStr HKLM "${REG_UNINST_KEY}" "DisplayIcon" '"$INSTDIR\IMEServer.exe"'
  WriteRegStr HKLM "${REG_UNINST_KEY}" "DisplayVersion" "${IME_VERSION}.${IME_BUILD}"
  WriteRegStr HKLM "${REG_UNINST_KEY}" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegStr HKLM "${REG_UNINST_KEY}" "Publisher" "${IME_PUBLISHER}"
  WriteRegStr HKLM "${REG_UNINST_KEY}" "URLInfoAbout" "${IME_URL}"
  WriteRegStr HKLM "${REG_UNINST_KEY}" "HelpLink" "${IME_URL}"
  WriteRegDWORD HKLM "${REG_UNINST_KEY}" "NoModify" 1
  WriteRegDWORD HKLM "${REG_UNINST_KEY}" "NoRepair" 1
  WriteUninstaller "$INSTDIR\uninstall.exe"

  ; Deploy (run as user)
  IfSilent deploy_silently
  ExecWait "$INSTDIR\IMEDeployer.exe /install"
  GoTo deploy_done

deploy_silently:
  ExecWait "$INSTDIR\IMEDeployer.exe /deploy"
deploy_done:

  ; Set autorun
  ${If} ${IsNativeARM64}
    SetRegView 64
  ${ElseIf} ${IsNativeAMD64}
    SetRegView 64
  ${Endif}
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Run" "IMEServer" "$INSTDIR\IMEServer.exe"
  
  ; Start server
  Exec "$INSTDIR\IMEServer.exe"

  ; Prompt reboot if upgrade
  StrCmp $0 "Upgrade" 0 +2
  SetRebootFlag true

SectionEnd

;--------------------------------
; Start Menu Shortcuts Section

Section "Start Menu Shortcuts" SecShortcuts
  SetShellVarContext all
  CreateDirectory "$SMPROGRAMS\$(DISPLAYNAME)"
  CreateShortCut "$SMPROGRAMS\$(DISPLAYNAME)\$(LNKFORSETTING).lnk" "$INSTDIR\IMEDeployer.exe" "" "$SYSDIR\shell32.dll" 21
  CreateShortCut "$SMPROGRAMS\$(DISPLAYNAME)\$(LNKFORDEPLOY).lnk" "$INSTDIR\IMEDeployer.exe" "/deploy" "$SYSDIR\shell32.dll" 144
  CreateShortCut "$SMPROGRAMS\$(DISPLAYNAME)\$(LNKFORSERVER).lnk" "$INSTDIR\IMEServer.exe" "" "$INSTDIR\IMEServer.exe" 0
  CreateShortCut "$SMPROGRAMS\$(DISPLAYNAME)\$(LNKFORUSERFOLDER).lnk" "$INSTDIR\IMEServer.exe" "/userdir" "$SYSDIR\shell32.dll" 126
  CreateShortCut "$SMPROGRAMS\$(DISPLAYNAME)\$(LNKFORAPPFOLDER).lnk" "$INSTDIR\IMEServer.exe" "/appdir" "$SYSDIR\shell32.dll" 19
  CreateShortCut "$SMPROGRAMS\$(DISPLAYNAME)\$(LNKFORUNINSTALL).lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
SectionEnd

;--------------------------------
; Uninstaller Section

Section "Uninstall"
  ; Stop server
  ExecWait '"$INSTDIR\IMEServer.exe" /quit'

  ; Unregister IME
  ExecWait '"$INSTDIR\IMESetup.exe" /u'

  ; Remove registry keys
  DeleteRegKey HKLM "${REG_IME_KEY}"
  DeleteRegKey HKLM "${REG_UNINST_KEY}"
  
  ; Remove autorun
  ${If} ${IsNativeARM64}
    SetRegView 64
  ${ElseIf} ${IsNativeAMD64}
    SetRegView 64
  ${Endif}
  DeleteRegValue HKLM "Software\Microsoft\Windows\CurrentVersion\Run" "IMEServer"

  ; Remove files
  SetOutPath $TEMP
  Delete "$INSTDIR\data\opencc\*.*"
  Delete "$INSTDIR\data\*.*"
  Delete "$INSTDIR\*.*"
  RMDir "$INSTDIR\data\opencc"
  RMDir "$INSTDIR\data"
  RMDir "$INSTDIR"
  
  ; Remove shortcuts
  SetShellVarContext all
  Delete "$SMPROGRAMS\$(DISPLAYNAME)\*.*"
  RMDir "$SMPROGRAMS\$(DISPLAYNAME)"

  ; Prompt reboot
  SetRebootFlag true
SectionEnd

;--------------------------------
; Section Descriptions

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SecMain} "安装拼音输入法核心组件"
  !insertmacro MUI_DESCRIPTION_TEXT ${SecShortcuts} "创建开始菜单快捷方式"
!insertmacro MUI_FUNCTION_DESCRIPTION_END
