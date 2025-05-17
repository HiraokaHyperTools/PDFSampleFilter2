; example2.nsi
;
; This script is based on example1.nsi, but it remember the directory, 
; has uninstall support and (optionally) installs start menu shortcuts.
;
; It will install example2.nsi into a directory that the user selects,

;--------------------------------

Unicode true

!define APP "PDFSampleFilter2"
!define BINDIR32 "Release"
!define BINDIR64 "x64\Release"

!define COM "HIRAOKA HYPERS TOOLS, Inc."

!system 'DefineAsmVer.exe "${BINDIR32}\${APP}.dll" "!define VER ""[ProductVersion]"" " "!define F4VER ""[F4VER]"" " "!define LegalCopyright ""[LegalCopyright]"" " "!define FileDescription ""[FileDescription]"" " "!define FileVersion ""[FileVersion]"" " > Appver.tmp'
!include "Appver.tmp"
!searchreplace APV "${VER}" "." "_"

!system 'MySign "${BINDIR32}\${APP}.dll" "${BINDIR64}\${APP}.dll"'
!finalize 'MySign "%1"'

; The name of the installer
Name "${APP} ver ${VER}"

; The file to write
OutFile "Setup_${APP}.exe"

; The default installation directory
InstallDir "$PROGRAMFILES\${APP}"

; Registry key to check for directory (so if you install again, it will 
; overwrite the old one automatically)
InstallDirRegKey HKLM "Software\${COM}\${APP}" "Install_Dir"

; Request application privileges for Windows Vista
RequestExecutionLevel admin

XPStyle on

LoadLanguageFile "${NSISDIR}\Contrib\Language files\Japanese.nlf"

!include "LogicLib.nsh"
!include "x64.nsh"

;--------------------------------

; Pages

Page license
Page directory
Page components
Page instfiles

LicenseData "..\SetupLicense.rtf"

UninstPage uninstConfirm
UninstPage instfiles

;--------------------------------

; The stuff to install
Section ""

  SectionIn RO
  
  ; Set output path to the installation directory.
  SetOutPath $INSTDIR
  
  ; Put file there

  ; Write the installation path into the registry
  WriteRegStr HKLM "Software\${COM}\${APP}" "Install_Dir" "$INSTDIR"
  
  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP}" "DisplayName" "${APP}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP}" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP}" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP}" "NoRepair" 1
  WriteUninstaller "$INSTDIR\uninstall.exe"
  
SectionEnd

Section "${APP} (x86)" x86
  ; Set output path to the installation directory.
  SetOutPath $INSTDIR\x86

  ; Put file there
  File "${BINDIR32}\PDFSampleFilter2.dll"
  File "..\pdfium-win-x86\bin\*"
  
  ExecWait 'regsvr32.exe /s "$OUTDIR\${APP}.dll"' $0
  DetailPrint "ExitCode: $0"
  
  ${If} $0 != 0
    Abort "regsvr32.exe (for x86) failed with ExitCode: $0"
  ${EndIf}

SectionEnd

Section "${APP} (x64)" x64
  ; Set output path to the installation directory.
  SetOutPath $INSTDIR\x64

  ; Put file there
  File "${BINDIR64}\PDFSampleFilter2.dll"
  File "..\pdfium-win-x64\bin\*"
  
  ExecWait 'regsvr32.exe /s "$OUTDIR\${APP}.dll"' $0
  DetailPrint "ExitCode: $0"
  
  ${If} $0 != 0
    Abort "regsvr32.exe (for x64) failed with ExitCode: $0"
  ${EndIf}

SectionEnd

Function .onInit
  ${IfNot} ${RunningX64}
    SectionSetFlags ${x64} 0
  ${EndIf}
FunctionEnd

;--------------------------------

; Uninstaller

Section "Uninstall"
  
  ; Remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP}"
  DeleteRegKey HKLM "Software\${COM}\${APP}"

  ExecWait 'regsvr32.exe /s /u "$INSTDIR\x86\${APP}.dll"' $0
  DetailPrint "ExitCode: $0"

  ${If} ${RunningX64}
    ExecWait 'regsvr32.exe /s /u "$INSTDIR\x64\${APP}.dll"' $0
    DetailPrint "ExitCode: $0"
  ${EndIf}

  ; Remove files and uninstaller
  Delete "$INSTDIR\x86\pdfium.dll"
  Delete "$INSTDIR\x86\PDFSampleFilter2.dll"
  RMDir  "$INSTDIR\x86"
  Delete "$INSTDIR\x64\pdfium.dll"
  Delete "$INSTDIR\x64\PDFSampleFilter2.dll"
  RMDir  "$INSTDIR\x64"
  Delete "$INSTDIR\uninstall.exe"

  ; Remove shortcuts, if any

  ; Remove directories used
  RMDir "$INSTDIR"

SectionEnd
