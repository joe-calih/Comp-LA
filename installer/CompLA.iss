; Inno Setup script - builds CompLA-Setup.exe (installs the VST3 plugin and the standalone app)
#define MyAppName "Comp LA"
#define MyAppVersion "2.0.0"
#define MyAppPublisher "Loops"
#define MyAppURL "https://loops.co.ke"

[Setup]
AppId={{B1D3A6B2-5C0A-4B7E-9A58-6F3C4A2D7E11}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}/contact
AppUpdatesURL={#MyAppURL}
VersionInfoCompany={#MyAppPublisher}
DefaultDirName={autopf}\Loops\Comp LA
DefaultGroupName=Loops
DisableProgramGroupPage=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
OutputDir=..\dist-installer
OutputBaseFilename=CompLA-Setup-{#MyAppVersion}
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UninstallDisplayName={#MyAppName}

[Types]
Name: "full"; Description: "Plugin + standalone app"
Name: "plugin"; Description: "VST3 plugin only (FL Studio, etc.)"

[Components]
Name: "vst3"; Description: "VST3 plugin (installs to Common Files\VST3)"; Types: full plugin; Flags: fixed
Name: "standalone"; Description: "Standalone app (Comp LA.exe)"; Types: full

[Files]
Source: "..\dist\Comp LA.vst3\*"; DestDir: "{commoncf64}\VST3\Comp LA.vst3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\dist\Comp LA.exe"; DestDir: "{app}"; Components: standalone; Flags: ignoreversion

[Icons]
Name: "{autoprograms}\Loops\Comp LA"; Filename: "{app}\Comp LA.exe"; Components: standalone

[Run]
Filename: "{#MyAppURL}"; Description: "Visit loops.co.ke"; Flags: postinstall shellexec skipifsilent unchecked
