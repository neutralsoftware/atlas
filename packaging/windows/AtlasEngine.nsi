Unicode true
!include "MUI2.nsh"

Name "Atlas Engine"
OutFile "${OUTPUT_FILE}"
InstallDir "$PROGRAMFILES64\Atlas Engine"
RequestExecutionLevel admin
SetCompressor /SOLID lzma
BrandingText "Atlas Engine"

!define MUI_ICON "${SOURCE_DIR}\atlas-star.ico"
!define MUI_UNICON "${SOURCE_DIR}\atlas-star.ico"
!define MUI_ABORTWARNING
!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_RIGHT
!define MUI_HEADERIMAGE_BITMAP "${__FILEDIR__}\assets\header.bmp"
!define MUI_WELCOMEFINISHPAGE_BITMAP "${__FILEDIR__}\assets\welcome.bmp"
!define MUI_UNWELCOMEFINISHPAGE_BITMAP "${__FILEDIR__}\assets\welcome.bmp"
!define MUI_WELCOMEPAGE_TITLE "Welcome to Atlas Engine"
!define MUI_WELCOMEPAGE_TEXT "Build worlds. Bring ideas to life.$\r$\n$\r$\nInstall the Atlas editor and its bundled tools to start creating your next project.$\r$\n$\r$\nClick Next to get started."
!define MUI_FINISHPAGE_TITLE "Atlas Engine is ready"
!define MUI_FINISHPAGE_TEXT "Your creative workspace is installed.$\r$\n$\r$\nOpen Atlas Engine from your desktop or Start menu to create or open a project."

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH
!insertmacro MUI_LANGUAGE "English"

Section
    SetOutPath "$INSTDIR"
    File /r "${SOURCE_DIR}\*"
    CreateDirectory "$SMPROGRAMS\Atlas Engine"
    CreateShortcut "$SMPROGRAMS\Atlas Engine\Atlas Engine.lnk" "$INSTDIR\AtlasEditor.exe"
    CreateShortcut "$DESKTOP\Atlas Engine.lnk" "$INSTDIR\AtlasEditor.exe"
    WriteUninstaller "$INSTDIR\Uninstall.exe"
SectionEnd

Section "Uninstall"
    Delete "$DESKTOP\Atlas Engine.lnk"
    Delete "$SMPROGRAMS\Atlas Engine\Atlas Engine.lnk"
    RMDir "$SMPROGRAMS\Atlas Engine"
    RMDir /r "$INSTDIR"
SectionEnd
