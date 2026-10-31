; Inno Setup script: installs the VST3 plugin and the standalone app.
#define AppName "Noir Guitar"
#define AppVersion "1.0.0"

[Setup]
AppId={{6B0B7C1E-5B7A-4C57-9E5B-2D6B9C0E4A11}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher=Noir Audio
DefaultDirName={autopf}\Noir Guitar
DefaultGroupName={#AppName}
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
OutputDir=..\dist
OutputBaseFilename=NoirGuitar-Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
DisableProgramGroupPage=yes
UninstallDisplayName={#AppName}

[Types]
Name: "full"; Description: "VST3 plugin + standalone app"
Name: "vst"; Description: "VST3 plugin only"
Name: "custom"; Description: "Custom"; Flags: iscustom

[Components]
Name: "vst3"; Description: "VST3 plugin (for your DAW)"; Types: full vst custom; Flags: fixed
Name: "standalone"; Description: "Standalone app (play without a DAW)"; Types: full custom

[Files]
; VST3 bundle goes to the standard system folder that every DAW scans.
Source: "..\stage\Noir Guitar.vst3\*"; DestDir: "{commoncf64}\VST3\Noir Guitar.vst3"; Components: vst3; Flags: recursesubdirs ignoreversion
Source: "..\stage\Noir Guitar.exe"; DestDir: "{app}"; Components: standalone; Flags: ignoreversion

[Icons]
Name: "{autoprograms}\{#AppName}"; Filename: "{app}\Noir Guitar.exe"; Components: standalone
Name: "{autodesktop}\{#AppName}"; Filename: "{app}\Noir Guitar.exe"; Components: standalone; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; Components: standalone; Flags: unchecked

[Run]
Filename: "{app}\Noir Guitar.exe"; Description: "Launch Noir Guitar"; Flags: nowait postinstall skipifsilent; Components: standalone
