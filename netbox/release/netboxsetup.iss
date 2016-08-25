#define YEAR 2013
#define STATUS "release"
#ifndef ROOT_DIR
#define ROOT_DIR ".."
#endif
#define BUILD_DIR ROOT_DIR + "/build/NetBox/
#ifndef SOURCE_DIR
#define SOURCE_DIR ROOT_DIR + "/src/NetBox"
#endif
#ifndef OUTPUT_DIR
#define OUTPUT_DIR ROOT_DIR + "/build"
#endif
#ifndef BINARIES_DIR_FAR2
#define BINARIES_DIR_FAR2 BUILD_DIR + "/Far2"
#endif
#ifndef BINARIES_DIR_FAR3
#define BINARIES_DIR_FAR3 BUILD_DIR + "/Far3"
#endif

#ifndef PUTTY_SOURCE_DIR
#define PUTTY_SOURCE_DIR "C:/Program Files/Putty"
#endif

#define FileSourceMain_Far2x86 BINARIES_DIR_FAR2 + "/x86/NetBox.dll"
#define FileSourceMain_Far2x64 BINARIES_DIR_FAR2 + "/x64/NetBox.dll"
#define FileSourceMain_Far3x86 BINARIES_DIR_FAR3 + "/x86/NetBox.dll"
#define FileSourceMain_Far3x64 BINARIES_DIR_FAR3 + "/x64/NetBox.dll"
#define FileSourceEng SOURCE_DIR + "/NetBoxEng.lng"
#define FileSourceRus SOURCE_DIR + "/NetBoxRus.lng"
#define FileSourceChangeLog ROOT_DIR + "/ChangeLog"
#define FileSourceReadmeEng ROOT_DIR + "/README.md"
#define FileSourceReadmeRu ROOT_DIR + "/README.RU.md"
#define FileSourceLicense ROOT_DIR + "/LICENSE.txt"
#define PluginSubDirName "NetBox"

#define Major
#define Minor
#define Rev
#define Build
#expr ParseVersion(FileSourceMain_Far2x86, Major, Minor, Rev, Build)
#define Version Str(Major) + "." + Str(Minor) + (Rev > 0 ? "." + Str(Rev) : "") + \
  (STATUS != "" ? " " + STATUS : "")

[Setup]
AppId=netbox
AppMutex=NetBox
AppName=NetBox plugin for Far2/Far3
AppPublisher=Michael Lukashov
AppPublisherURL=https://github.com/michaellukashov/Far-NetBox
AppSupportURL=http://forum.farmanager.com/viewtopic.php?f=39&t=6638
AppUpdatesURL=http://plugring.farmanager.com/plugin.php?pid=859&l=en
VersionInfoCompany=Michael Lukashov
VersionInfoDescription=Setup for NetBox plugin for Far2/Far3 {#Version}
VersionInfoVersion={#Major}.{#Minor}.{#Rev}.{#Build}
VersionInfoTextVersion={#Version}
VersionInfoCopyright=(c) 2011-{#YEAR} Michael Lukashov
DefaultDirName={pf}\Far Manager\Plugins\{#PluginSubDirName}
UsePreviousAppDir=false
DisableProgramGroupPage=true
LicenseFile=licence.setup
; UninstallDisplayIcon={app}\winscp.ico
OutputDir={#OUTPUT_DIR}
DisableStartupPrompt=yes
AppVersion={#Version}
AppVerName=NetBox plugin for Far2/Far3 {#Version}
OutputBaseFilename=FarNetBox-{#Major}.{#Minor}.{#Rev}_Far2_Far3_x86_x64
Compression=lzma2/ultra
SolidCompression=yes
PrivilegesRequired=none
Uninstallable=no
MinVersion=5.1
DisableDirPage=yes
; AlwaysShowDirOnReadyPage=yes

ArchitecturesInstallIn64BitMode=x64

[Types]
Name: full; Description: "Full installation"
; Name: compact; Description: "Compact installation"
Name: custom; Description: "Custom installation"; Flags: iscustom
; Languages: en ru

[Components]
Name: main_far2_x86; Description: "NetBox for Far2/x86"; Types: full custom; check: IsFar2X86Installed
Name: main_far2_x64; Description: "NetBox for Far2/x64"; Types: full custom; check: IsWin64 and IsFar3X64Installed
Name: main_far3_x86; Description: "NetBox for Far3/x86"; Types: full custom; check: IsFar3X86Installed
Name: main_far3_x64; Description: "NetBox for Far3/x64"; Types: full custom; check: IsWin64 and IsFar3X64Installed
; Name: pageant; Description: "Pageant (SSH authentication agent)"; Types: full
; Name: puttygen; Description: "PuTTYgen (key generator)"; Types: full

[Files]
Source: "{#FileSourceMain_Far2x86}"; DestName: "NetBox.dll"; DestDir: "{code:GetPlugin2X86Dir}"; Components: main_far2_x86; Flags: ignoreversion
Source: "{#FileSourceMain_Far2x64}"; DestName: "NetBox.dll"; DestDir: "{code:GetPlugin2X64Dir}"; Components: main_far2_x64; Flags: ignoreversion
Source: "{#FileSourceMain_Far3x86}"; DestName: "NetBox.dll"; DestDir: "{code:GetPlugin3X86Dir}"; Components: main_far3_x86; Flags: ignoreversion
Source: "{#FileSourceMain_Far3x64}"; DestName: "NetBox.dll"; DestDir: "{code:GetPlugin3X64Dir}"; Components: main_far3_x64; Flags: ignoreversion
Source: "{#FileSourceEng}"; DestName: "NetBoxEng.lng"; DestDir: "{code:GetPlugin2X86Dir}"; Components: main_far2_x86; Flags: ignoreversion
Source: "{#FileSourceEng}"; DestName: "NetBoxEng.lng"; DestDir: "{code:GetPlugin2X64Dir}"; Components: main_far2_x64; Flags: ignoreversion
Source: "{#FileSourceEng}"; DestName: "NetBoxEng.lng"; DestDir: "{code:GetPlugin3X86Dir}"; Components: main_far3_x86; Flags: ignoreversion
Source: "{#FileSourceEng}"; DestName: "NetBoxEng.lng"; DestDir: "{code:GetPlugin3X64Dir}"; Components: main_far3_x64; Flags: ignoreversion
Source: "{#FileSourceRus}"; DestName: "NetBoxRus.lng"; DestDir: "{code:GetPlugin2X86Dir}"; Components: main_far2_x86; Flags: ignoreversion
Source: "{#FileSourceRus}"; DestName: "NetBoxRus.lng"; DestDir: "{code:GetPlugin2X64Dir}"; Components: main_far2_x64; Flags: ignoreversion
Source: "{#FileSourceRus}"; DestName: "NetBoxRus.lng"; DestDir: "{code:GetPlugin3X86Dir}"; Components: main_far3_x86; Flags: ignoreversion
Source: "{#FileSourceRus}"; DestName: "NetBoxRus.lng"; DestDir: "{code:GetPlugin3X64Dir}"; Components: main_far3_x64; Flags: ignoreversion
Source: "{#FileSourceChangeLog}"; DestName: "ChangeLog"; DestDir: "{code:GetPlugin2X86Dir}"; Components: main_far2_x86; Flags: ignoreversion
Source: "{#FileSourceChangeLog}"; DestName: "ChangeLog"; DestDir: "{code:GetPlugin2X64Dir}"; Components: main_far2_x64; Flags: ignoreversion
Source: "{#FileSourceChangeLog}"; DestName: "ChangeLog"; DestDir: "{code:GetPlugin3X86Dir}"; Components: main_far3_x86; Flags: ignoreversion
Source: "{#FileSourceChangeLog}"; DestName: "ChangeLog"; DestDir: "{code:GetPlugin3X64Dir}"; Components: main_far3_x64; Flags: ignoreversion
Source: "{#FileSourceReadmeEng}"; DestName: "README.md"; DestDir: "{code:GetPlugin2X86Dir}"; Components: main_far2_x86; Flags: ignoreversion
Source: "{#FileSourceReadmeEng}"; DestName: "README.md"; DestDir: "{code:GetPlugin2X64Dir}"; Components: main_far2_x64; Flags: ignoreversion
Source: "{#FileSourceReadmeEng}"; DestName: "README.md"; DestDir: "{code:GetPlugin3X86Dir}"; Components: main_far3_x86; Flags: ignoreversion
Source: "{#FileSourceReadmeEng}"; DestName: "README.md"; DestDir: "{code:GetPlugin3X64Dir}"; Components: main_far3_x64; Flags: ignoreversion
Source: "{#FileSourceReadmeRu}"; DestName: "README.RU.md"; DestDir: "{code:GetPlugin2X86Dir}"; Components: main_far2_x86; Flags: ignoreversion
Source: "{#FileSourceReadmeRu}"; DestName: "README.RU.md"; DestDir: "{code:GetPlugin2X64Dir}"; Components: main_far2_x64; Flags: ignoreversion
Source: "{#FileSourceReadmeRu}"; DestName: "README.RU.md"; DestDir: "{code:GetPlugin3X86Dir}"; Components: main_far3_x86; Flags: ignoreversion
Source: "{#FileSourceReadmeRu}"; DestName: "README.RU.md"; DestDir: "{code:GetPlugin3X64Dir}"; Components: main_far3_x64; Flags: ignoreversion
Source: "{#FileSourceLicense}"; DestName: "LICENSE.txt"; DestDir: "{code:GetPlugin2X86Dir}"; Components: main_far2_x86; Flags: ignoreversion
Source: "{#FileSourceLicense}"; DestName: "LICENSE.txt"; DestDir: "{code:GetPlugin2X64Dir}"; Components: main_far2_x64; Flags: ignoreversion
Source: "{#FileSourceLicense}"; DestName: "LICENSE.txt"; DestDir: "{code:GetPlugin3X86Dir}"; Components: main_far3_x86; Flags: ignoreversion
Source: "{#FileSourceLicense}"; DestName: "LICENSE.txt"; DestDir: "{code:GetPlugin3X64Dir}"; Components: main_far3_x64; Flags: ignoreversion
; Source: "{#PUTTY_SOURCE_DIR}\LICENCE"; DestDir: "{code:GetPluginX86Dir}\PuTTY"; Components: main_x86 pageant puttygen; Flags: ignoreversion
; Source: "{#PUTTY_SOURCE_DIR}\LICENCE"; DestDir: "{code:GetPluginX64Dir}\PuTTY"; Components: main_x64 pageant puttygen; Flags: ignoreversion
; Source: "{#PUTTY_SOURCE_DIR}\putty.hlp"; DestDir: "{code:GetPluginX86Dir}\PuTTY"; Components: main_x86 pageant puttygen; Flags: ignoreversion
; Source: "{#PUTTY_SOURCE_DIR}\putty.hlp"; DestDir: "{code:GetPluginX64Dir}\PuTTY"; Components: main_x64 pageant puttygen; Flags: ignoreversion
; Source: "{#PUTTY_SOURCE_DIR}\pageant.exe"; DestDir: "{code:GetPluginX86Dir}\PuTTY"; Components: main_x86 pageant; Flags: ignoreversion
; Source: "{#PUTTY_SOURCE_DIR}\pageant.exe"; DestDir: "{code:GetPluginX64Dir}\PuTTY"; Components: main_x64 pageant; Flags: ignoreversion
; Source: "{#PUTTY_SOURCE_DIR}\puttygen.exe"; DestDir: "{code:GetPluginX86Dir}\PuTTY"; Components: main_x86 puttygen; Flags: ignoreversion
; Source: "{#PUTTY_SOURCE_DIR}\puttygen.exe"; DestDir: "{code:GetPluginX64Dir}\PuTTY"; Components: main_x64 puttygen; Flags: ignoreversion

[InstallDelete]

[Code]

var
  InputDirsPage: TInputDirWizardPage;

function GetFar2X86InstallDir(): String;
var
  InstallDir: String;
begin
  if RegQueryStringValue(HKLM, 'Software\Far2', 'InstallDir', InstallDir) or
     RegQueryStringValue(HKCU, 'Software\Far2', 'InstallDir', InstallDir) then
  begin
    Result := InstallDir;
  end;
end;

function GetFar2X64InstallDir(): String;
var
  InstallDir: String;
begin
  if RegQueryStringValue(HKCU, 'Software\Far2', 'InstallDir_x64', InstallDir) or
     RegQueryStringValue(HKLM, 'Software\Far2', 'InstallDir_x64', InstallDir) then
  begin
    Result := InstallDir;
  end;
end;

function IsFar2X86Installed(): Boolean;
begin
  Result := GetFar2X86InstallDir() <> '';
end;

function IsFar2X64Installed(): Boolean;
begin
  Result := GetFar2X64InstallDir() <> '';
end;

function GetDefaultFar2X86Dir(): String;
var
  InstallDir: String;
begin
  InstallDir := GetFar2X86InstallDir();
  if InstallDir <> '' then
  begin
    Result := AddBackslash(InstallDir) + 'Plugins\{#PluginSubDirName}';
  end
  else
  begin
    Result := ExpandConstant('{pf}\Far2\Plugins\{#PluginSubDirName}');
  end;
end;

function GetDefaultFar2X64Dir(): String;
var
  InstallDir: String;
begin
  InstallDir := GetFar2X64InstallDir();
  if InstallDir <> '' then
  begin
    Result := AddBackslash(InstallDir) + 'Plugins\{#PluginSubDirName}';
  end
  else
  begin
    Result := ExpandConstant('{pf}\Far2\Plugins\{#PluginSubDirName}');
  end;
end;

function GetPlugin2X86Dir(Param: String): String;
begin
  Result := InputDirsPage.Values[0];
end;

function GetPlugin2X64Dir(Param: String): String;
begin
  Result := InputDirsPage.Values[1];
end;

function GetFar3X86InstallDir(): String;
var
  InstallDir: String;
begin
  if RegQueryStringValue(HKLM, 'Software\Far Manager', 'InstallDir', InstallDir) or
     RegQueryStringValue(HKCU, 'Software\Far Manager', 'InstallDir', InstallDir) then
  begin
    Result := InstallDir;
  end;
end;

function GetFar3X64InstallDir(): String;
var
  InstallDir: String;
begin
  if RegQueryStringValue(HKLM, 'Software\Far Manager', 'InstallDir_x64', InstallDir) or
     RegQueryStringValue(HKCU, 'Software\Far Manager', 'InstallDir_x64', InstallDir) then
  begin
    Result := InstallDir;
  end;
end;

function IsFar3X86Installed(): Boolean;
begin
  Result := GetFar3X86InstallDir() <> '';
end;

function IsFar3X64Installed(): Boolean;
begin
  Result := GetFar3X64InstallDir() <> '';
end;

function GetDefaultFar3X86Dir(): String;
var
  InstallDir: String;
begin
  InstallDir := GetFar3X86InstallDir();
  if InstallDir <> '' then
  begin
    Result := AddBackslash(InstallDir) + 'Plugins\{#PluginSubDirName}';
  end
  else
  begin
    Result := ExpandConstant('{pf}\Far Manager\Plugins\{#PluginSubDirName}');
  end;
end;

function GetDefaultFar3X64Dir(): String;
var
  InstallDir: String;
begin
  InstallDir := GetFar3X64InstallDir();
  if InstallDir <> '' then
  begin
    Result := AddBackslash(InstallDir) + 'Plugins\{#PluginSubDirName}';
  end
  else
  begin
    Result := ExpandConstant('{pf}\Far Manager\Plugins\{#PluginSubDirName}');
  end;
end;

function GetPlugin3X86Dir(Param: String): String;
begin
  Result := InputDirsPage.Values[2];
end;

function GetPlugin3X64Dir(Param: String): String;
begin
  Result := InputDirsPage.Values[3];
end;

procedure CreateTheWizardPage;
begin
  // Input dirs
  InputDirsPage := CreateInputDirPage(wpSelectComponents,
  'Select plugin location', 'Where plugin should be installed?',
  'Plugin will be installed in the following folder(s).'#13#10#13#10 +
  'To continue, click Next. If you would like to select a different folder, click Browse.',
  False, 'Plugin folder');
  InputDirsPage.Add('Far2/x86 plugin location:');
  InputDirsPage.Values[0] := GetDefaultFar2X86Dir();
  InputDirsPage.Add('Far2/x64 plugin location:');
  InputDirsPage.Values[1] := GetDefaultFar2X64Dir();
  InputDirsPage.Add('Far3/x86 plugin location:');
  InputDirsPage.Values[2] := GetDefaultFar3X86Dir();
  InputDirsPage.Add('Far3/x64 plugin location:');
  InputDirsPage.Values[3] := GetDefaultFar3X64Dir();
end;

procedure SetupInputDirs();
begin
  InputDirsPage.Edits[0].Enabled := IsComponentSelected('main_far2_x86');
  InputDirsPage.Buttons[0].Enabled := IsComponentSelected('main_far2_x86');
  InputDirsPage.PromptLabels[0].Enabled := IsComponentSelected('main_far2_x86');

  // InputDirsPage.Edits[1].Visible := IsWin64();
  // InputDirsPage.Buttons[1].Visible := IsWin64();
  // InputDirsPage.PromptLabels[1].Visible := IsWin64();

  InputDirsPage.Edits[1].Enabled := IsComponentSelected('main_far2_x64');
  InputDirsPage.Buttons[1].Enabled := IsComponentSelected('main_far2_x64');
  InputDirsPage.PromptLabels[1].Enabled := IsComponentSelected('main_far2_x64');

  InputDirsPage.Edits[2].Enabled := IsComponentSelected('main_far3_x86');
  InputDirsPage.Buttons[2].Enabled := IsComponentSelected('main_far3_x86');
  InputDirsPage.PromptLabels[2].Enabled := IsComponentSelected('main_far3_x86');

  // InputDirsPage.Edits[3].Visible := IsWin64();
  // InputDirsPage.Buttons[3].Visible := IsWin64();
  // InputDirsPage.PromptLabels[3].Visible := IsWin64();

  InputDirsPage.Edits[3].Enabled := IsComponentSelected('main_far3_x64');
  InputDirsPage.Buttons[3].Enabled := IsComponentSelected('main_far3_x64');
  InputDirsPage.PromptLabels[3].Enabled := IsComponentSelected('main_far3_x64');
end;

function NextButtonClick(CurPageID: Integer): Boolean;
begin
  if CurPageID = wpWelcome then
  begin
    // SetupComponents();
  end
  else
  if CurPageID = wpSelectComponents then
  begin
    SetupInputDirs();
  end
  else
  if CurPageID = InputDirsPage.ID then
  begin
    WizardForm.DirEdit.Text := InputDirsPage.Values[0];
  end;
  Result := True;
end;

function BackButtonClick(CurPageID: Integer): Boolean;
begin
  // MsgBox('CurPageID: ' + IntToStr(CurPageID), mbInformation, mb_Ok);
  if CurPageID = InputDirsPage.ID then
  begin
    // SetupComponents();
  end;
  if CurPageID = wpReady then
  begin
    SetupInputDirs();
  end;
  Result := True;
end;

procedure InitializeWizard();
begin
  // Custom wizard page
  CreateTheWizardPage;

  WizardForm.LicenseAcceptedRadio.Checked := True;
end;
