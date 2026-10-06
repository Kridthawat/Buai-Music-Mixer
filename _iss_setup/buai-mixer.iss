; Buai Music Mixer - Inno Setup script
; Build with (from repository root, after the app is built and deployed):
;   iscc /DAppVersion=3.0.0 _iss_setup\buai-mixer.iss
; Expected layout:  deploy\x64\  (BuaiMusicMixer.exe + Qt/BASS dlls, see .github\workflows\build-windows.yml)

#ifndef AppVersion
  #define AppVersion "1.0.0"
#endif
#define MyAppName "Buai Music Mixer"
#define MyAppPublisher "Buai Music"
#define MyAppExeName "BuaiMusicMixer.exe"
#define DeployDir "..\deploy\x64"
#define DeployDir32 "..\deploy\x86"

[Setup]
AppId={{27743F76-5859-4759-95FD-D472EFC32A4B}
AppName={#MyAppName}
AppVersion={#AppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}\Buai Music Mixer
DefaultGroupName={#MyAppName}
AllowNoIcons=yes
LicenseFile=..\gpl-3.0.rtf
OutputDir=..\dist
OutputBaseFilename=BuaiMusicMixer-{#AppVersion}-setup
SetupIconFile=..\icon.ico
UninstallDisplayIcon={app}\{#MyAppExeName}
Compression=lzma
SolidCompression=yes
UsePreviousAppDir=yes
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon
; 32-bit edition: use this one when you need 32-bit VST plugins
Name: "{group}\{#MyAppName} (32-bit VST)"; Filename: "{app}\x86\{#MyAppExeName}"; WorkingDir: "{app}"

[Run]
Filename: "{tmp}\VC_redist.x86.exe"; Parameters: "/install /passive /norestart"; StatusMsg: "Installing Microsoft Visual C++ Runtime (x86)..."; Flags: waituntilterminated; Check: VC2017RedistX86NeedsInstall
Filename: "{tmp}\VC_redist.x64.exe"; Parameters: "/install /passive /norestart"; StatusMsg: "Installing Microsoft Visual C++ Runtime..."; Flags: waituntilterminated; Check: VC2017RedistNeedsInstall
Filename: "{app}\{#MyAppExeName}"; Flags: nowait postinstall skipifsilent; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"

[Dirs]
Name: "{app}\SoundFonts"; Flags: uninsneveruninstall
Name: "{app}\VST"; Flags: uninsneveruninstall

[Files]
Source: "{#DeployDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#DeployDir32}\*"; DestDir: "{app}\x86"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#DeployDir}\..\VC_redist.x86.exe"; DestDir: "{tmp}"; Flags: ignoreversion deleteafterinstall; Check: VC2017RedistX86NeedsInstall
Source: "{#DeployDir}\..\VC_redist.x64.exe"; DestDir: "{tmp}"; Flags: ignoreversion deleteafterinstall; Check: VC2017RedistNeedsInstall

[Code]
function VC2017RedistNeedsInstall: Boolean;
var
  Version: String;
begin
  if RegQueryStringValue(HKEY_LOCAL_MACHINE, 'SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64', 'Version', Version) then
  begin
    Log('VC Redist Version check : found ' + Version);
    Result := (CompareStr(Version, 'v14.23.27820.00') < 0);
  end
  else
    Result := True;
end;

function VC2017RedistX86NeedsInstall: Boolean;
var
  Version: String;
begin
  if RegQueryStringValue(HKLM32, 'SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x86', 'Version', Version) then
    Result := (CompareStr(Version, 'v14.23.27820.00') < 0)
  else
    Result := True;
end;
