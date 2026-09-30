Unicode true
Name "Atlas Engine"
OutFile "${OUTPUT_FILE}"
InstallDir "$PROGRAMFILES64\Atlas Engine"
RequestExecutionLevel admin

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
