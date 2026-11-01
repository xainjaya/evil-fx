; Evil FX installer for Windows (Inno Setup 6)
;
; Build after compiling Release:
;   iscc /DAppVersion=1.2.0 /DArtefacts="..\..\build\EvilFX_artefacts\Release" packaging\windows\EvilFX.iss
; The installer lands in packaging\windows\Output\.

#ifndef AppVersion
  #define AppVersion "1.2.0"
#endif
#ifndef Artefacts
  #define Artefacts "..\..\build\EvilFX_artefacts\Release"
#endif

[Setup]
AppId={{6E7A1C3B-5F0D-4C8E-9B2A-3E51F0C0FFEE}
AppName=Evil FX
AppVersion={#AppVersion}
AppVerName=Evil FX {#AppVersion}
AppPublisher=Xain Jaya
AppPublisherURL=https://xainjaya.com
AppSupportURL=https://xainjaya.com
DefaultDirName={autopf}\Evil FX
DefaultGroupName=Evil FX
DisableProgramGroupPage=yes
LicenseFile=..\..\LICENSE
OutputBaseFilename=EvilFX-{#AppVersion}-Windows-Setup
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
UninstallDisplayName=Evil FX {#AppVersion}
#ifdef SetupIcon
SetupIconFile={#SetupIcon}
#endif

[Types]
Name: "full";   Description: "Everything (VST3, CLAP and the standalone app)"
Name: "custom"; Description: "Pick formats"; Flags: iscustom

[Components]
Name: "vst3";       Description: "VST3 plugin  (Common Files\VST3)";  Types: full custom
Name: "clap";       Description: "CLAP plugin  (Common Files\CLAP)";  Types: full custom
Name: "standalone"; Description: "Standalone app";                    Types: full custom

[Files]
Source: "{#Artefacts}\VST3\Evil FX.vst3\*"; DestDir: "{commoncf64}\VST3\Evil FX.vst3"; Flags: recursesubdirs createallsubdirs ignoreversion; Components: vst3
Source: "{#Artefacts}\CLAP\Evil FX.clap";   DestDir: "{commoncf64}\CLAP";             Flags: ignoreversion skipifsourcedoesntexist; Components: clap
Source: "{#Artefacts}\Standalone\Evil FX.exe"; DestDir: "{app}";                      Flags: ignoreversion; Components: standalone
Source: "..\..\README.md";                  DestDir: "{app}";                         Flags: ignoreversion isreadme
Source: "..\..\LICENSE";                    DestDir: "{app}";                         Flags: ignoreversion

[Icons]
Name: "{autoprograms}\Evil FX"; Filename: "{app}\Evil FX.exe"; Components: standalone
Name: "{autodesktop}\Evil FX";  Filename: "{app}\Evil FX.exe"; Components: standalone; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "Desktop shortcut for the standalone app"; Components: standalone; Flags: unchecked

[Run]
Filename: "https://xainjaya.com"; Description: "Visit xainjaya.com"; Flags: postinstall shellexec skipifsilent unchecked
