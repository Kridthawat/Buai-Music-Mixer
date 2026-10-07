; Buai Music Mixer - Inno Setup script
; Build with (from repository root, after the app is built and deployed):
;   iscc /DAppVersion=3.0.0 _iss_setup\buai-mixer-x86.iss
; Expected layout:  deploy\x86\  (BuaiMusicMixer.exe + Qt/BASS dlls, see .github\workflows\build-windows.yml)

#ifndef AppVersion
  #define AppVersion "1.2.0"
#endif
#define MyAppName "Buai Music Mixer"
#define MyAppPublisher "Buai Music"
#define MyAppExeName "BuaiMusicMixer.exe"
#define DeployDir "..\deploy\x86"

[Setup]
AppId={{5B1E6C52-8F0A-4D3B-9A57-3C8E1D7F2B46}
AppName={#MyAppName}
AppVersion={#AppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}\Buai Music Mixer (32-bit)
DefaultGroupName={#MyAppName} (32-bit)
AllowNoIcons=yes
LicenseFile=..\gpl-3.0.rtf
OutputDir=..\dist
OutputBaseFilename=BuaiMusicMixer-{#AppVersion}-x86-setup
SetupIconFile=..\icon.ico
UninstallDisplayIcon={app}\{#MyAppExeName}
Compression=lzma
SolidCompression=yes
UsePreviousAppDir=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{tmp}\VC_redist.x86.exe"; Parameters: "/install /passive /norestart"; StatusMsg: "Installing Microsoft Visual C++ Runtime..."; Flags: waituntilterminated; Check: VC2017RedistNeedsInstall
Filename: "{app}\{#MyAppExeName}"; Flags: nowait postinstall skipifsilent; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"

[Dirs]
Name: "{app}\SoundFonts"; Flags: uninsneveruninstall
Name: "{app}\VST"; Flags: uninsneveruninstall

[Files]
Source: "{#DeployDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#DeployDir}\..\VC_redist.x86.exe"; DestDir: "{tmp}"; Flags: ignoreversion deleteafterinstall; Check: VC2017RedistNeedsInstall

[Code]
function VC2017RedistNeedsInstall: Boolean;
var
  Version: String;
begin
  if RegQueryStringValue(HKLM32, 'SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x86', 'Version', Version) then
  begin
    Log('VC Redist Version check : found ' + Version);
    Result := (CompareStr(Version, 'v14.23.27820.00') < 0);
  end
  else
    Result := True;
end;
