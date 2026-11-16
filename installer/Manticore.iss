; Inno Setup script for Manticore (Fellowship of the String)
#define AppName "Manticore"
#define AppVersion "1.0.0"
#define Publisher "Fellowship of the String"
#define Build "..\build\Manticore_artefacts\Release"

[Setup]
AppId={{9C3D7E21-4B6A-4F0E-8D52-A17C6E0B3F95}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher={#Publisher}
DefaultDirName={autopf}\{#Publisher}\{#AppName}
DefaultGroupName={#Publisher}
DisableProgramGroupPage=yes
OutputDir=..\dist
OutputBaseFilename={#AppName}-{#AppVersion}-Setup
SetupIconFile=ManticoreSetup.ico
UninstallDisplayIcon={app}\Manticore.ico
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin

[Types]
Name: "full"; Description: "VST3 plug-in, standalone app and manual"
Name: "plugin"; Description: "VST3 plug-in and manual only"
Name: "custom"; Description: "Custom"; Flags: iscustom

[Components]
Name: "vst3"; Description: "Manticore VST3 plug-in (C:\Program Files\Common Files\VST3)"; Types: full plugin custom; Flags: fixed
Name: "standalone"; Description: "Manticore standalone app"; Types: full
Name: "manual"; Description: "User manual (PDF)"; Types: full plugin custom

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut for the standalone app"; Components: standalone

[Files]
Source: "{#Build}\VST3\Manticore.vst3\*"; DestDir: "{commoncf64}\VST3\Manticore.vst3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#Build}\Standalone\Manticore.exe"; DestDir: "{app}"; Components: standalone; Flags: ignoreversion
Source: "Manticore.ico"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\docs\Manticore_User_Manual.pdf"; DestDir: "{app}"; Components: manual; Flags: ignoreversion
Source: "..\Resources\Fonts\OFL-Cinzel.txt"; DestDir: "{app}\Licenses"; Flags: ignoreversion
Source: "..\Resources\Fonts\OFL-Rajdhani.txt"; DestDir: "{app}\Licenses"; Flags: ignoreversion

[Icons]
Name: "{group}\Manticore"; Filename: "{app}\Manticore.exe"; IconFilename: "{app}\Manticore.ico"; Components: standalone
Name: "{group}\Manticore User Manual"; Filename: "{app}\Manticore_User_Manual.pdf"; Components: manual
Name: "{commondesktop}\Manticore"; Filename: "{app}\Manticore.exe"; IconFilename: "{app}\Manticore.ico"; Tasks: desktopicon

[Run]
Filename: "{app}\Manticore_User_Manual.pdf"; Description: "Open the user manual"; Flags: postinstall shellexec skipifsilent unchecked; Components: manual
