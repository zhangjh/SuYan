; Installation Script - reads brand from command line defines

!include FileFunc.nsh
!include LogicLib.nsh
!include MUI2.nsh
!include x64.nsh
!include winVer.nsh

Unicode true

!ifndef IME_NAME
!define IME_NAME "SuYan"
!endif

!ifndef IME_NAME_CN
!define IME_NAME_CN "素言"
!endif

!ifndef IME_VERSION
!define IME_VERSION "1.0.0"
!endif

!ifndef IME_BUILD
!define IME_BUILD "0"
!endif

!ifndef PRODUCT_VERSION
!define PRODUCT_VERSION "${IME_VERSION}.${IME_BUILD}"
!endif

!ifndef IME_COMPANY
!define IME_COMPANY "Unknown"
!endif

!define IME_ROOT "$INSTDIR\${IME_NAME}-${IME_VERSION}"
!define REG_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${IME_NAME}"
!define REG_IME_KEY "Software\${IME_NAME}"

Name "${IME_NAME_CN} ${IME_VERSION}"
OutFile "..\..\output\windows\${IME_NAME}-${PRODUCT_VERSION}-x64-installer.exe"

VIProductVersion "${IME_VERSION}.${IME_BUILD}"
VIAddVersionKey /LANG=2052 "ProductName" "${IME_NAME_CN}"
VIAddVersionKey /LANG=2052 "CompanyName" "${IME_COMPANY}"
VIAddVersionKey /LANG=2052 "FileDescription" "${IME_NAME_CN}"
VIAddVersionKey /LANG=2052 "FileVersion" "${IME_VERSION}"
VIAddVersionKey /LANG=2052 "LegalCopyright" "Copyright (C) ${IME_COMPANY}"

!if /FileExists "..\..\resources\icons\app-icon.ico"
!define MUI_ICON "..\..\resources\icons\app-icon.ico"
!define MUI_UNICON "..\..\resources\icons\app-icon.ico"
!endif

SetCompressor /SOLID lzma
RequestExecutionLevel admin

!define MUI_ABORTWARNING

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "LICENSE.txt"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "SimpChinese"
LangString DISPLAYNAME ${LANG_SIMPCHINESE} "${IME_NAME_CN}"
LangString LNKFORSETTING ${LANG_SIMPCHINESE} "【${IME_NAME_CN}】设置"
LangString LNKFORDEPLOY ${LANG_SIMPCHINESE} "【${IME_NAME_CN}】重新部署"
LangString LNKFORUNINSTALL ${LANG_SIMPCHINESE} "卸载${IME_NAME_CN}"
LangString CONFIRMATION ${LANG_SIMPCHINESE} '检测到旧版本，点击"确定"移除旧版本继续安装。'
LangString ARCH_NOT_SUPPORTED ${LANG_SIMPCHINESE} "此安装程序仅支持 64 位 Windows 系统。"

!insertmacro MUI_LANGUAGE "English"
LangString DISPLAYNAME ${LANG_ENGLISH} "${IME_NAME}"
LangString LNKFORSETTING ${LANG_ENGLISH} "${IME_NAME} Settings"
LangString LNKFORDEPLOY ${LANG_ENGLISH} "${IME_NAME} Deploy"
LangString LNKFORUNINSTALL ${LANG_ENGLISH} "Uninstall ${IME_NAME}"
LangString CONFIRMATION ${LANG_ENGLISH} "Old version detected. Press 'OK' to remove and continue."
LangString ARCH_NOT_SUPPORTED ${LANG_ENGLISH} "This installer only supports 64-bit Windows."

Function .onInit
  ${IfNot} ${RunningX64}
    MessageBox MB_OK '$(ARCH_NOT_SUPPORTED)'
    Quit
  ${EndIf}

  ${IfNot} ${AtLeastWin8.1}
    MessageBox MB_OK "Windows 8.1 or later required."
    Quit
  ${EndIf}

  SetRegView 64
  StrCpy $INSTDIR "$PROGRAMFILES64\${IME_NAME}"

  ReadRegStr $R0 HKLM "${REG_UNINST_KEY}" "UninstallString"
  StrCmp $R0 "" done

  StrCpy $0 "Upgrade"
  IfSilent uninst 0
  MessageBox MB_OKCANCEL|MB_ICONINFORMATION "$(CONFIRMATION)" IDOK uninst
  Abort

uninst:
  ReadRegStr $R1 HKLM "${REG_IME_KEY}" "InstallDir"
  StrCmp $R1 "" done
  ExecWait '"$R1\WeaselServer.exe" /quit'
  ExecWait '"$R1\WeaselSetup.exe" /u'
  DeleteRegKey HKLM "${REG_IME_KEY}"
  DeleteRegKey HKLM "${REG_UNINST_KEY}"
  DeleteRegValue HKLM "Software\Microsoft\Windows\CurrentVersion\Run" "WeaselServer"
  RMDir /r "$R1"
  SetShellVarContext all
  RMDir /r "$SMPROGRAMS\$(DISPLAYNAME)"
  Sleep 500

done:
FunctionEnd

InstallDirRegKey HKLM "${REG_IME_KEY}" "InstallDir"

Section "Main" SecMain
  SectionIn RO
  SetRegView 64

  WriteRegStr HKLM "${REG_IME_KEY}" "InstallDir" "$INSTDIR"
  StrCpy $INSTDIR "${IME_ROOT}"

  IfFileExists "$INSTDIR\WeaselServer.exe" 0 +2
  ExecWait '"$INSTDIR\WeaselServer.exe" /quit'

  SetOverwrite on
  SetOutPath $INSTDIR

  File "LICENSE.txt"
  File "README.txt"
  File "WeaselServer.exe"
  File "WeaselDeployer.exe"
  File "WeaselSetup.exe"
  File "rime.dll"
  File "weaselx64.dll"
  File "weaselx64.ime"
  File /nonfatal "WinSparkle.dll"

  SetOutPath $INSTDIR\data
  File /nonfatal "data\*.yaml"
  File /nonfatal "data\*.txt"

  SetOutPath $INSTDIR\data\cn_dicts
  File /nonfatal "data\cn_dicts\*.*"

  SetOutPath $INSTDIR\data\en_dicts
  File /nonfatal "data\en_dicts\*.*"

  SetOutPath $INSTDIR\data\lua
  File /nonfatal "data\lua\*.*"

  SetOutPath $INSTDIR\data\opencc
  File /nonfatal "data\opencc\*.*"

  SetOutPath $INSTDIR

  StrCpy $R2 "/i"
  ${GetParameters} $R0
  ClearErrors
  ${GetOptions} $R0 "/S" $R1
  IfErrors +2 0
  StrCpy $R2 "/s"

  ExecWait '"$INSTDIR\WeaselSetup.exe" $R2'

  WriteRegStr HKLM "${REG_UNINST_KEY}" "DisplayName" "$(DISPLAYNAME)"
  WriteRegStr HKLM "${REG_UNINST_KEY}" "DisplayIcon" '"$INSTDIR\WeaselServer.exe"'
  WriteRegStr HKLM "${REG_UNINST_KEY}" "DisplayVersion" "${IME_VERSION}.${IME_BUILD}"
  WriteRegStr HKLM "${REG_UNINST_KEY}" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegStr HKLM "${REG_UNINST_KEY}" "Publisher" "${IME_COMPANY}"
  WriteRegDWORD HKLM "${REG_UNINST_KEY}" "NoModify" 1
  WriteRegDWORD HKLM "${REG_UNINST_KEY}" "NoRepair" 1
  WriteUninstaller "$INSTDIR\uninstall.exe"

  IfSilent deploy_silently
  ExecWait "$INSTDIR\WeaselDeployer.exe /install"
  GoTo deploy_done
deploy_silently:
  ExecWait "$INSTDIR\WeaselDeployer.exe /deploy"
deploy_done:

  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Run" "WeaselServer" "$INSTDIR\WeaselServer.exe"
  Exec "$INSTDIR\WeaselServer.exe"

  StrCmp $0 "Upgrade" 0 +2
  SetRebootFlag true
SectionEnd

Section "Start Menu" SecShortcuts
  SetShellVarContext all
  CreateDirectory "$SMPROGRAMS\$(DISPLAYNAME)"
  CreateShortCut "$SMPROGRAMS\$(DISPLAYNAME)\$(LNKFORSETTING).lnk" "$INSTDIR\WeaselDeployer.exe"
  CreateShortCut "$SMPROGRAMS\$(DISPLAYNAME)\$(LNKFORDEPLOY).lnk" "$INSTDIR\WeaselDeployer.exe" "/deploy"
  CreateShortCut "$SMPROGRAMS\$(DISPLAYNAME)\$(LNKFORUNINSTALL).lnk" "$INSTDIR\uninstall.exe"
SectionEnd

Section "Uninstall"
  SetRegView 64
  ExecWait '"$INSTDIR\WeaselServer.exe" /quit'
  ExecWait '"$INSTDIR\WeaselSetup.exe" /u'

  DeleteRegKey HKLM "${REG_IME_KEY}"
  DeleteRegKey HKLM "${REG_UNINST_KEY}"
  DeleteRegValue HKLM "Software\Microsoft\Windows\CurrentVersion\Run" "WeaselServer"

  RMDir /r "$INSTDIR"

  SetShellVarContext all
  RMDir /r "$SMPROGRAMS\$(DISPLAYNAME)"

  SetRebootFlag true
SectionEnd
