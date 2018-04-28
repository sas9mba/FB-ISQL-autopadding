;  Initial Developer's Public License.
;  The contents of this file are subject to the  Initial Developer's Public
;  License Version 1.0 (the "License"). You may not use this file except
;  in compliance with the License. You may obtain a copy of the License at
;    http://www.ibphoenix.com?a=ibphoenix&page=ibp_idpl
;  Software distributed under the License is distributed on an "AS IS" basis,
;  WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
;  for the specific language governing rights and limitations under the
;  License.
;
;  The Original Code is copyright 2001-20016 Paul Reeves for IBPhoenix.
;
;  The Initial Developer of the Original Code is Paul Reeves for IBPhoenix.
;
;  All Rights Reserved.
;
;   Contributor(s):
;     Tilo Muetze, Theo ? and Michael Rimov for improved detection
;     of an existing install directory.
;     Simon Carter for the WinSock2 detection.
;     Philippe Makowski for internationalization and french translation
;     Sergey Nikitin for migrating to ISS v5.

;   Usage Notes:
;
;   This script has been designed to work with Inno Setup v5.5.8 (unicode
;   version). It is available as a quick start pack from here:
;     http://www.jrsoftware.org/isdl.php#qsp
;
;
;   Known bugs and problems etc etc.
;
;   o The uninstaller can only uninstall what it installed.
;     For instance, if Firebird is installed as a service, but is actually
;     running as an application when the uninstaller is run then the
;     application will not be stopped and the uninstall will not complete
;     cleanly.
;
;   o The uninstaller does not know how to stop multiple instances of a classic
;     server. They must be stopped manually.
;
;
;
#define MyAppPublisher "Firebird Project"
#define MyAppURL "http://www.firebirdsql.org/"
#define MyAppName "Firebird"
#define MyAppId "FBDBServer"

#define FirebirdURL MyAppURL
#define UninstallBinary "{app}\firebird.exe"

;Hard code some defaults to aid debugging and running script standalone.
;In practice, these values are set in the environment and we use the env vars.
#define MajorVer "4"
#define MinorVer "0"
#define PointRelease "0"
#define BuildNumber "0"
#define PackageNumber "0"
#define FilenameSuffix ""


;-------Start of Innosetup script debug flags section

; if iss_release is undefined then iss_debug is set
; Setting iss_release implies that the defines for files,
; examples and compression are set. If debug is set then this
; section controls the settings of files, examples
; and compression.

; A dynamically generated sed script sets the appropriate define
; See BuildExecutableInstall.bat for more details.


;#define iss_debug

#ifndef iss_debug
#define iss_release
#endif

;;;;;;;;;;;;;;;;;;;;;;;;;
#ifdef iss_release
#define files
#define examples
#define compression
#else
#define iss_debug
#endif
;--------------------

;--------------------
#ifdef iss_debug
;Be more verbose
#pragma option -v+
#pragma verboselevel 9

;Useful for cases where engine is built without examples.
#undef examples
;Useful when testing structure of script - adding files takes time.
#undef files
;We speed up compilation (and hence testing) by not compressing contents.
#undef compression

;Default to x64 for testing
#define PlatformTarget "x64"
#endif
;-------#ifdef iss_debug

;-------end of Innosetup script debug flags section


;-------Start of Innosetup script


;---- These three defines need a bit of tidying up in the near future,
;     but for now they must stay, as the BuildExecutableInstall.bat
;     uses them.
#define release
#define no_pdb
#define i18n


;------If necessary we can turn off i18n by uncommenting this undefine
;------In general this is a good idea for alpha and beta releases. 
#undef  i18n

;----- If we are debugging the script (and not executed from command prompt)
;----- there is no guarantee that the environment variable exists. However an
;----- expression such as #define FB_MAJOR_VER GetEnv("FB_MAJOR_VER") will
;----- 'define' the macro anyway so we need to test for a valid env var before
;----- we define our macro.
#if Len(GetEnv("FB_MAJOR_VER")) > 0
#define FB_MAJOR_VER GetEnv("FB_MAJOR_VER")
#endif
#ifdef FB_MAJOR_VER
#define MajorVer FB_MAJOR_VER
#endif

#if Len(GetEnv("FB_MINOR_VER")) > 0
#define FB_MINOR_VER GetEnv("FB_MINOR_VER")
#endif
#ifdef FB_MINOR_VER
#define MinorVer FB_MINOR_VER
#endif

#if Len(GetEnv("FB_REV_NO")) > 0
#define FB_REV_NO GetEnv("FB_REV_NO")
#endif
#ifdef FB_REV_NO
#define PointRelease FB_REV_NO
#endif

#if Len(GetEnv("FB_BUILD_NO")) > 0
#define FB_BUILD_NO GetEnv("FB_BUILD_NO")
#endif
#ifdef FB_BUILD_NO
#define BuildNumber FB_BUILD_NO
#endif

#if Len(GetEnv("FBBUILD_PACKAGE_NUMBER")) > 0
#define FBBUILD_PACKAGE_NUMBER GetEnv("FBBUILD_PACKAGE_NUMBER")
#endif
#ifdef FBBUILD_PACKAGE_NUMBER
#define PackageNumber FBBUILD_PACKAGE_NUMBER
#endif

#if Len(GetEnv("FBBUILD_FILENAME_SUFFIX")) > 0
#define FBBUILD_FILENAME_SUFFIX GetEnv("FBBUILD_FILENAME_SUFFIX")
#endif
#ifdef FBBUILD_FILENAME_SUFFIX
#define FilenameSuffix FBBUILD_FILENAME_SUFFIX
#if pos('_',FilenameSuffix) == 0
#define FilenameSuffix "_" + FilenameSuffix
#endif
#endif


#if BuildNumber == "0"
#define MyAppVerString MajorVer + "." + MinorVer + "." + PointRelease
#else
#define MyAppVerString MajorVer + "." + MinorVer + "." + PointRelease + "." + BuildNumber
#endif
#define MyAppVerName MyAppName + " " + MyAppVerString

;---- If we haven't already set PlatformTarget then pick it up from the environment.
#ifndef PlatformTarget
#define PlatformTarget GetEnv("FB_TARGET_PLATFORM")
#endif
#if PlatformTarget == ""
#define PlatformTarget "win32"
#endif

;If we are still under development we can ignore some missing files.
#if GetEnv("FBBUILD_PROD_STATUS") == "DEV"
#define SkipFileIfDevStatus " skipifsourcedoesntexist "
#else 
#define SkipFileIfDevStatus " "
#endif

;This location is relative to SourceDir (declared below)
#define FilesDir="output_" + PlatformTarget
#if PlatformTarget == "x64"
#define WOW64Dir="output_win32"
#endif
#define msvc_version 12

;BaseVer should be used for all MajorVer.MinorVer installs.
;This allows us to upgrade silently from MajorVer.MinorVer.m to MajorVer.MinorVer.n
#define BaseVer MajorVer + "_" + MinorVer
#define AppVer MajorVer + "_" + MinorVer
#define GroupnameVer MajorVer + "." + MinorVer

;These variables are set in BuildExecutableInstall
#define FB15_cur_ver GetEnv("FBBUILD_FB15_CUR_VER")
#define FB20_cur_ver GetEnv("FBBUILD_FB20_CUR_VER")
#define FB21_cur_ver GetEnv("FBBUILD_FB21_CUR_VER")
#define FB25_cur_ver GetEnv("FBBUILD_FB25_CUR_VER")
#define FB30_cur_ver GetEnv("FBBUILD_FB30_CUR_VER")
#define FB40_cur_ver GetEnv("FBBUILD_FB40_CUR_VER")
#define FB_cur_ver FB40_cur_ver
#define FB_last_ver FB30_cur_ver

; We can save space by shipping a pdb package that just includes
; the pdb files. It would then upgrade an existing installation,
; however, it wouldn't work on its own. This practice would go
; against previous policy, which was to make the pdb package a
; complete package. OTOH, now we have 32-bit and 64-bit platforms
; to support we would benefit from slimming down the pdb packages.
;;Uncomment the following lines to build the pdb package
;;with just the pdb files.
;#ifdef ship_pdb
;#undef files
;#undef examples
;#endif


;Some more strings to distinguish the name of final executable
#ifdef ship_pdb
#define pdb_str="_pdb"
#else
#define pdb_str=""
#endif
#ifdef debug
#define debug_str="_debug"
#else
#define debug_str=""
#endif


[Setup]
AppName={#MyAppName}
;The following is important - all ISS install packages should
;duplicate this. See the InnoSetup help for details.
#if PlatformTarget == "x64"
AppID={#MyAppId}_{#BaseVer}_{#PlatformTarget}
#else
AppID={#MyAppId}_{#BaseVer}
#endif
AppVerName={#MyAppVerName} ({#PlatformTarget})
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
AppVersion={#MyAppVerString}
VersionInfoVersion={#MyAppVerString}

SourceDir=..\..\..\..\
OutputBaseFilename={#MyAppName}-{#MyAppVerString}_{#PackageNumber}_{#PlatformTarget}{#debug_str}{#pdb_str}{#FilenameSuffix}
;OutputManifestFile={#MyAppName}-{#MyAppVerString}_{#PackageNumber}_{#PlatformTarget}{#debug_str}{#pdb_str}{#FilenameSuffix}_Setup-Manifest.txt
OutputDir=builds\install_images
;!!! These directories are as seen from SourceDir !!!
#define ScriptsDir "builds\install\arch-specific\win32"
#define LicensesDir "builds\install\misc"
#define GenDir "gen\readmes"
LicenseFile={#LicensesDir}\IPLicense.txt

WizardImageFile={#ScriptsDir}\firebird_install_logo1.bmp
WizardSmallImageFile={#ScriptsDir}\firebird_install_logo1.bmp

DefaultDirName={code:ChooseInstallDir|{pf}\Firebird\Firebird_{#AppVer}}
DefaultGroupName=Firebird {#GroupnameVer} ({#PlatformTarget})

UninstallDisplayIcon={code:ChooseUninstallIcon|{#UninstallBinary}}
#ifndef compression
Compression=none
#else
SolidCompression=yes
#endif

ShowUndisplayableLanguages={#defined iss_debug}
AllowNoIcons=true
AlwaysShowComponentsList=true
PrivilegesRequired=admin
SetupMutex={#MyAppName}

#if PlatformTarget == "x64"
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
#endif

;This feature is incomplete, as more thought is required.
#define setuplogging
#ifdef setuplogging
;New with IS 5.2 -let's use it by default until we figure out how and whether it should be used.
SetupLogging=yes
#endif

[Languages]
Name: en; MessagesFile: compiler:Default.isl; InfoBeforeFile: {#GenDir}\installation_readme.txt; InfoAfterFile: {#GenDir}\readme.txt;
#ifdef i18n
Name: ba; MessagesFile: compiler:Languages\Bosnian.isl; InfoBeforeFile: {#GenDir}\ba\Instalacija_ProcitajMe.txt; InfoAfterFile: {#GenDir}\ba\ProcitajMe.txt;
Name: cz; MessagesFile: compiler:Languages\Czech.isl; InfoBeforeFile: {#GenDir}\cz\instalace_ctime.txt; InfoAfterFile: {#GenDir}\cz\ctime.txt;
Name: fr; MessagesFile: compiler:Languages\French.isl; InfoBeforeFile: {#GenDir}\fr\installation_lisezmoi.txt; InfoAfterFile: {#GenDir}\fr\lisezmoi.txt;
;Name: de; MessagesFile: compiler:Languages\German.isl; InfoBeforeFile: {#GenDir}\de\installation_liesmich.txt; InfoAfterFile: {#GenDir}\de\liesmich.txt;
;Name: es; MessagesFile: compiler:Languages\Spanish.isl; InfoBeforeFile: {#GenDir}\es\leame_instalacion.txt; InfoAfterFile: {#GenDir}\es\leame.txt;
;Name: hu; MessagesFile: compiler:Languages\Hungarian.isl; InfoBeforeFile: {#GenDir}\hu\telepitesi_segedlet.txt; InfoAfterFile: {#GenDir}\hu\olvass_el.txt;
;Name: it; MessagesFile: compiler:Languages\Italian.isl; InfoBeforeFile: {#GenDir}\it\leggimi_installazione.txt; InfoAfterFile: {#GenDir}\it\leggimi.txt
;Name: pl; MessagesFile: compiler:Languages\Polish.isl; InfoBeforeFile: {#GenDir}\pl\instalacja_czytajto.txt; InfoAfterFile: {#GenDir}\pl\czytajto.txt;
;Name: pt; MessagesFile: compiler:Languages\Portuguese.isl; InfoBeforeFile: {#GenDir}\pt\instalacao_leia-me.txt; InfoAfterFile: {#GenDir}\pt\leia-me.txt
Name: ru; MessagesFile: compiler:Languages\Russian.isl; InfoBeforeFile: {#GenDir}\ru\installation_readme.txt; InfoAfterFile: {#GenDir}\ru\readme.txt;
;Name: si; MessagesFile: compiler:Languages\Slovenian.isl; InfoBeforeFile: {#GenDir}\si\instalacija_precitajMe.txt; InfoAfterFile: {#GenDir}\readme.txt;
#endif

[Messages]
;en.BeveledLabel=English
#ifdef i18n
;ba.BeveledLabel=Bosanski
;cz.BeveledLabel=Czech
;fr.BeveledLabel=Français
;de.BeveledLabel=Deutsch
;es.BeveledLabel=Español
;hu.BeveledLabel=Magyar
;it.BeveledLabel=Italiano
;pl.BeveledLabel=Polski
;ru.BeveledLabel=
;pt.BeveledLabel=Português
;si.BeveledLabel=Slovenski
#endif

[CustomMessages]
#include "custom_messages.inc"
#ifdef i18n
#include "ba\custom_messages_ba.inc"
#include "cz\custom_messages_cz.inc"
#include "fr\custom_messages_fr.inc"
;#include "de\custom_messages_de.inc"
;#include "es\custom_messages_es.inc"
;#include "hu\custom_messages_hu.inc"
;#include "it\custom_messages_it.inc"
;#include "pl\custom_messages_pl.inc"
;#include "pt\custom_messages_pt.inc"
#include "ru\custom_messages_ru.inc"
;#include "si\custom_messages_si.inc"
#endif

#ifdef iss_debug
; *** Note - this comment section needs revision or deletion. 
; It is only applicable to the ansi installer, which is no longer
; supported for Firebird 3
; By default, the languages available at runtime depend on the user's
; code page. A user with the Western European code page set will not
; even see that we support installation with the czech language
; for example.
; It can be useful when debugging to force the display of all available
; languages by setting LanguageCodePage to 0. Of course, if the langauge
; is not supported by the user's current code page it will be unusable.
[LangOptions]
LanguageCodePage=0
#endif

[Types]
Name: ServerInstall; Description: {cm:ServerInstall}
Name: DeveloperInstall; Description: {cm:DeveloperInstall}
Name: ClientInstall; Description: {cm:ClientInstall}
Name: CustomInstall; Description: {cm:CustomInstall}; Flags: iscustom;

[Components]
Name: ServerComponent; Description: {cm:ServerComponent}; Types: ServerInstall;
Name: DevAdminComponent; Description: {cm:DevAdminComponent}; Types: ServerInstall DeveloperInstall;
Name: ClientComponent; Description: {cm:ClientComponent}; Types: ServerInstall DeveloperInstall ClientInstall CustomInstall; Flags: fixed disablenouninstallwarning;

[Tasks]
;Server tasks
Name: UseClassicServerTask; Description: {cm:RunCS}; GroupDescription: {cm:ServerTaskDescription}; Components: ServerComponent; MinVersion: 4.0,4.0; Flags: exclusive unchecked; Check: ConfigureFirebird;
Name: UseSuperClassicTask; Description: {cm:RunSC}; GroupDescription: {cm:ServerTaskDescription}; Components: ServerComponent; MinVersion: 4.0,4.0; Flags: exclusive unchecked; Check: ConfigureFirebird;
Name: UseSuperClassicTask\UseGuardianTask; Description: {cm:UseGuardianTask}; Components: ServerComponent; MinVersion: 4.0,4.0; Flags: unchecked dontinheritcheck; Check: ConfigureFirebird;
Name: UseSuperServerTask; Description: {cm:RunSS}; GroupDescription: {cm:ServerTaskDescription}; Components: ServerComponent; MinVersion: 4.0,4.0; Flags: exclusive; Check: ConfigureFirebird;
Name: UseSuperServerTask\UseGuardianTask; Description: {cm:UseGuardianTask}; Components: ServerComponent; MinVersion: 4.0,4.0; Flags: unchecked dontinheritcheck; Check: ConfigureFirebird;
Name: UseApplicationTask; Description: {cm:UseApplicationTaskMsg}; GroupDescription: {cm:TaskGroupDescription}; Components: ServerComponent; MinVersion: 4,4; Flags: exclusive unchecked; Check: ConfigureFirebird;
Name: UseServiceTask; Description: {cm:UseServiceTask}; GroupDescription: {cm:TaskGroupDescription}; Components: ServerComponent; MinVersion: 0,4; Flags: exclusive; Check: ConfigureFirebird;
Name: AutoStartTask; Description: {cm:AutoStartTask}; Components: ServerComponent; MinVersion: 4,4; Check: ConfigureFirebird;
;Name: MenuGroupTask; Description: Create a Menu &Group; Components: DevAdminComponent; MinVersion: 4,4
;Copying of client libs to <sys>
Name: CopyFbClientToSysTask; Description: {cm:CopyFbClientToSysTask}; Components: ClientComponent; MinVersion: 4,4; Check: ShowCopyFbClientLibTask;
Name: CopyFbClientAsGds32Task; Description: {cm:CopyFbClientAsGds32Task}; Components: ClientComponent; MinVersion: 4,4; Flags: Unchecked; Check: ShowCopyGds32Task;
Name: EnableLegacyClientAuth; Description: {cm:EnableLegacyClientAuth}; Components: ClientComponent; MinVersion: 4,4; Flags: Unchecked; Check: ConfigureAuthentication;


[Run]
#if msvc_version >= 10
Filename: msiexec.exe; Parameters: "/qn /i ""{tmp}\vccrt{#msvc_version}_Win32.msi"" /L*v ""{tmp}\vccrt{#msvc_version}_Win32.log"" "; StatusMsg: "Installing MSVC 32-bit runtime libraries to system directory"; Check: HasWI30; Components: ClientComponent;
#if PlatformTarget == "x64"
Filename: msiexec.exe; Parameters: "/qn /i ""{tmp}\vccrt{#msvc_version}_x64.msi"" /L*v ""{tmp}\vccrt{#msvc_version}_x64.log"" ";  StatusMsg: "Installing MSVC 64-bit runtime libraries to system directory"; Check: HasWI30; Components: ClientComponent;
#endif
#endif

;Only register Firebird if we are installing AND configuring
Filename: {app}\instreg.exe; Parameters: "install "; StatusMsg: {cm:instreg}; MinVersion: 4.0,4.0; Components: ClientComponent; Flags: runminimized; Check: ConfigureFirebird;

Filename: {app}\instclient.exe; Parameters: "install fbclient"; StatusMsg: {cm:instclientCopyFbClient}; MinVersion: 4.0,4.0; Components: ClientComponent; Flags: runminimized; Check: CopyFBClientLib;
Filename: {app}\instclient.exe; Parameters: "install gds32"; StatusMsg: {cm:instclientGenGds32}; MinVersion: 4.0,4.0; Components: ClientComponent; Flags: runminimized; Check: CopyGds32
#if PlatformTarget == "x64"
Filename: {app}\WOW64\instclient.exe; Parameters: "install fbclient"; StatusMsg: {cm:instclientCopyFbClient}; MinVersion: 4.0,4.0; Components: ClientComponent; Flags: runminimized 32bit; Check: CopyFBClientLib;
Filename: {app}\WOW64\instclient.exe; Parameters: "install gds32"; StatusMsg: {cm:instclientGenGds32}; MinVersion: 4.0,4.0; Components: ClientComponent; Flags: runminimized 32bit; Check: CopyGds32
#endif

;If 'Install and start service' requested
;First, if installing service we must try and remove remnants of old service. Otherwise the new install will fail and when we start the service the old service will be started.
Filename: {app}\instsvc.exe; Parameters: "remove "; StatusMsg: {cm:instsvcSetup}; MinVersion: 0,4.0; Components: ServerComponent; Flags: runminimized; Tasks: UseServiceTask; Check: ConfigureFirebird;
Filename: {app}\instsvc.exe; Parameters: "install {code:ServiceStartFlags} "; StatusMsg: {cm:instsvcSetup}; MinVersion: 0,4.0; Components: ServerComponent; Flags: runminimized; Tasks: UseServiceTask; Check: ConfigureFirebird;
Filename: {app}\instsvc.exe; Description: {cm:instsvcStartQuestion}; Parameters: "start {code:ServiceName} "; StatusMsg: {cm:instsvcStartMsg}; MinVersion: 0,4.0; Components: ServerComponent; Flags: runminimized postinstall runascurrentuser; Tasks: UseServiceTask; Check: StartEngine
;If 'start as application' requested
Filename: {code:StartApp|{app}\firebird.exe}; Description: {cm:instappStartQuestion}; Parameters: {code:StartAppParams|' -a '}; StatusMsg: {cm:instappStartMsg}; MinVersion: 0,4.0; Components: ServerComponent; Flags: nowait postinstall; Tasks: UseApplicationTask; Check: StartEngine

;This is a preliminary test of jumping to a landing page. In practice, we are going to need to know the users language and the version number they have installed.
Filename: "{#MyAppURL}/afterinstall"; Description: "After installation - What Next?"; Flags: postinstall shellexec skipifsilent; Components: ServerComponent DevAdminComponent;

[Registry]
;If user has chosen to start as App they may well want to start automatically. That is handled by a function below.
;Unless we set a marker here the uninstall will leave some annoying debris.
Root: HKLM; Subkey: SOFTWARE\Microsoft\Windows\CurrentVersion\Run; ValueType: string; ValueName: Firebird; ValueData: ; Flags: uninsdeletevalue; Tasks: UseApplicationTask; Check: ConfigureFirebird;

;This doesn't seem to get cleared automatically by instreg on uninstall, so lets make sure of it
Root: HKLM; Subkey: "SOFTWARE\Firebird Project"; Flags: uninsdeletekeyifempty; Components: ClientComponent DevAdminComponent ServerComponent

;Clean up Invalid registry entries from previous installs.
Root: HKLM; Subkey: "SOFTWARE\FirebirdSQL"; ValueType: none; Flags: deletekey;

[Icons]
Name: {group}\Firebird Server; Filename: {app}\firebird.exe; Parameters: {code:StartAppParams}; Flags: runminimized; MinVersion: 4.0,4.0;  Check: InstallServerIcon; IconIndex: 0; Components: ServerComponent; Comment: Run Firebird Server (without guardian)
Name: {group}\Firebird Guardian; Filename: {app}\fbguard.exe; Parameters: {code:StartAppParams}; Flags: runminimized; MinVersion: 4.0,4.0;  Check: InstallGuardianIcon; IconIndex: 1; Components: ServerComponent; Comment: Run Firebird Server (with guardian); 
Name: {group}\Firebird ISQL Tool; Filename: {app}\isql.exe; Parameters: -z; WorkingDir: {app}; MinVersion: 4.0,4.0;  Comment: {cm:RunISQL}
Name: {group}\Firebird {#FB_cur_ver} Release Notes; Filename: {app}\doc\Firebird_v{#FB_cur_ver}.ReleaseNotes.pdf; MinVersion: 4.0,4.0; Comment: {#MyAppName} {cm:ReleaseNotes}
;Name: {group}\Firebird {#GroupnameVer} Quick Start Guide; Filename: {app}\doc\Firebird-{#MajorVer}-QuickStart.pdf; MinVersion: 4.0,4.0; Comment: {#MyAppName} {#FB_cur_ver}
Name: "{group}\After Installation"; Filename: "{app}\doc\After_Installation.url"; Comment: "New User? Here's a quick guide to what you should do next."
Name: "{group}\Firebird Web-site"; Filename: "{app}\doc\firebirdsql.org.url"
;Always install the original english version
Name: {group}\{cm:IconReadme,{#FB_cur_ver}}; Filename: {app}\readme.txt; MinVersion: 4.0,4.0;
#ifdef i18n
;And install translated readme.txt if non-default language is chosen.
Name: {group}\{cm:IconReadme,{#FB_cur_ver}}; Filename: {app}\{cm:ReadMeFile}; MinVersion: 4.0,4.0; Components: DevAdminComponent; Check: NonDefaultLanguage;
#endif
Name: {group}\{cm:Uninstall,{#FB_cur_ver}}; Filename: {uninstallexe}; Comment: Uninstall Firebird

[Files]
#ifdef files
Source: {#LicensesDir}\IPLicense.txt; DestDir: {app}; Components: ClientComponent; Flags: sharedfile ignoreversion;
Source: {#LicensesDir}\IDPLicense.txt; DestDir: {app}; Components: ClientComponent; Flags: sharedfile ignoreversion
Source: {#ScriptsDir}\After_Installation.url; DestDir: {app}\doc; Components: ServerComponent DevAdminComponent; Flags: sharedfile ignoreversion
Source: {#ScriptsDir}\firebirdsql.org.url; DestDir: {app}\doc; Components: ServerComponent DevAdminComponent; Flags: sharedfile ignoreversion
;Always install the original english version
Source: {#GenDir}\readme.txt; DestDir: {app}; Components: DevAdminComponent; Flags: ignoreversion;
#ifdef i18n
;Translated files
Source: {#GenDir}\ba\*.txt; DestDir: {app}\doc; Components: DevAdminComponent; Flags: ignoreversion; Languages: ba;
Source: {#GenDir}\cz\*.txt; DestDir: {app}\doc; Components: DevAdminComponent; Flags: ignoreversion; Languages: cz;
Source: {#GenDir}\fr\*.txt; DestDir: {app}\doc; Components: DevAdminComponent; Flags: ignoreversion; Languages: fr;
;Source: {#GenDir}\de\*.txt; DestDir: {app}\doc; Components: DevAdminComponent; Flags: ignoreversion; Languages: de;
;Source: {#GenDir}\es\*.txt; DestDir: {app}\doc; Components: DevAdminComponent; Flags: ignoreversion; Languages: es;
;Source: {#GenDir}\hu\*.txt; DestDir: {app}\doc; Components: DevAdminComponent; Flags: ignoreversion; Languages: hu;
;Source: {#GenDir}\it\*.txt; DestDir: {app}\doc; Components: DevAdminComponent; Flags: ignoreversion; Languages: it;
;Source: {#GenDir}\pl\*.txt; DestDir: {app}\doc; Components: DevAdminComponent; Flags: ignoreversion; Languages: pl;
;Source: {#GenDir}\pt\*.txt; DestDir: {app}\doc; Components: DevAdminComponent; Flags: ignoreversion; Languages: pt;
Source: {#GenDir}\ru\*.txt; DestDir: {app}\doc; Components: DevAdminComponent; Flags: ignoreversion; Languages: ru;
;Source: {#GenDir}\si\*.txt; DestDir: {app}\doc; Components: DevAdminComponent; Flags: ignoreversion; Languages: si;
#endif
Source: {#FilesDir}\firebird.conf; DestDir: {app}; DestName: firebird.conf.default; Components: ServerComponent;
Source: {#FilesDir}\firebird.conf; DestDir: {app}; DestName: firebird.conf; Components: ServerComponent; Flags: uninsneveruninstall; check: NoFirebirdConfExists
Source: {#FilesDir}\fbtrace.conf; DestDir: {app}; DestName: fbtrace.conf.default; Components: ServerComponent;
Source: {#FilesDir}\fbtrace.conf; DestDir: {app}; DestName: fbtrace.conf; Components: ServerComponent; Flags: uninsneveruninstall onlyifdoesntexist; check: NofbtraceConfExists;
Source: {#FilesDir}\databases.conf; DestDir: {app}; Components: ClientComponent; Flags: uninsneveruninstall onlyifdoesntexist
Source: {#FilesDir}\security4.fdb; DestDir: {app}; Destname: security4.fdb.empty; Components: ServerComponent;
Source: {#FilesDir}\security4.fdb; DestDir: {app}; Components: ServerComponent; Flags: uninsneveruninstall onlyifdoesntexist
Source: {#FilesDir}\firebird.msg; DestDir: {app}; Components: ClientComponent; Flags: sharedfile ignoreversion
Source: {#FilesDir}\firebird.log; DestDir: {app}; Components: ServerComponent; Flags: uninsneveruninstall skipifsourcedoesntexist external dontcopy

Source: {#FilesDir}\gbak.exe; DestDir: {app}; Components: DevAdminComponent; Flags: sharedfile ignoreversion
Source: {#FilesDir}\gfix.exe; DestDir: {app}; Components: DevAdminComponent; Flags: sharedfile ignoreversion
Source: {#FilesDir}\gpre.exe; DestDir: {app}; Components: DevAdminComponent; Flags: ignoreversion
Source: {#FilesDir}\gsec.exe; DestDir: {app}; Components: DevAdminComponent; Flags: sharedfile ignoreversion
Source: {#FilesDir}\gsplit.exe; DestDir: {app}; Components: DevAdminComponent; Flags: sharedfile ignoreversion
Source: {#FilesDir}\gstat.exe; DestDir: {app}; Components: ServerComponent; Flags: sharedfile ignoreversion
Source: {#FilesDir}\fbguard.exe; DestDir: {app}; Components: ServerComponent; Flags: sharedfile ignoreversion; Check: InstallGuardian;
Source: {#FilesDir}\firebird.exe; DestDir: {app}; Components: ServerComponent; Flags: sharedfile ignoreversion
Source: {#FilesDir}\fb_lock_print.exe; DestDir: {app}; Components: ServerComponent; Flags: sharedfile ignoreversion
Source: {#FilesDir}\ib_util.dll; DestDir: {app}; Components: ServerComponent; Flags: sharedfile ignoreversion
Source: {#FilesDir}\instclient.exe; DestDir: {app}; Components: ClientComponent; Flags: sharedfile ignoreversion
Source: {#FilesDir}\instreg.exe; DestDir: {app}; Components: ClientComponent; Flags: sharedfile ignoreversion
Source: {#FilesDir}\instsvc.exe; DestDir: {app}; Components: ServerComponent; MinVersion: 0,4.0; Flags: sharedfile ignoreversion
Source: {#FilesDir}\isql.exe; DestDir: {app}; Components: DevAdminComponent; Flags: ignoreversion
Source: {#FilesDir}\nbackup.exe; DestDir: {app}; Components: DevAdminComponent; Flags: ignoreversion
Source: {#FilesDir}\qli.exe; DestDir: {app}; Components: DevAdminComponent; Flags: ignoreversion
Source: {#FilesDir}\fbsvcmgr.exe; DestDir: {app}; Components: DevAdminComponent; Flags: ignoreversion
Source: {#FilesDir}\fbtracemgr.exe; DestDir: {app}; Components: DevAdminComponent; Flags: ignoreversion
Source: {#FilesDir}\fbclient.dll; DestDir: {app}; Components: ClientComponent; Flags: overwritereadonly sharedfile promptifolder
#if PlatformTarget == "x64"
Source: {#WOW64Dir}\fbclient.dll; DestDir: {app}\WOW64; Components: ClientComponent; Flags: overwritereadonly sharedfile promptifolder
Source: {#WOW64Dir}\instclient.exe; DestDir: {app}\WOW64; Components: ClientComponent; Flags: sharedfile ignoreversion
#endif
Source: {#FilesDir}\icuuc??.dll; DestDir: {app}; Components: ServerComponent; Flags: sharedfile ignoreversion
Source: {#FilesDir}\icuin??.dll; DestDir: {app}; Components: ServerComponent; Flags: sharedfile ignoreversion
Source: {#FilesDir}\icudt??.dll; DestDir: {app}; Components: ServerComponent; Flags: sharedfile ignoreversion
Source: {#FilesDir}\icudt*.dat;  DestDir: {app}; Components: ServerComponent; Flags: sharedfile ignoreversion
#if PlatformTarget =="Win32"
Source: {#FilesDir}\fbrmclib.dll; DestDir: {app}; Components: ServerComponent; Flags: sharedfile ignoreversion
#endif
Source: {#FilesDir}\zlib1.dll; DestDir: {app}; Components: ServerComponent; Flags: sharedfile ignoreversion

;Rules for installation of MS runtimes are simplified with MSVC10
;We just install the runtimes into the install dir.

#if msvc_version >= 10
Source: {#FilesDir}\msvcr{#msvc_version}?.dll; DestDir: {app}; Components: ClientComponent; Flags: sharedfile;
Source: {#FilesDir}\msvcp{#msvc_version}?.dll; DestDir: {app}; Components: ClientComponent; Flags: sharedfile;
#if PlatformTarget == "x64"
;If we are installing on x64 we need some 32-bit libraries for compatibility with 32-bit applications
Source: {#WOW64Dir}\msvcr{#msvc_version}?.dll; DestDir: {app}\WOW64; Components: ClientComponent; Flags: sharedfile;
Source: {#WOW64Dir}\msvcp{#msvc_version}?.dll; DestDir: {app}\WOW64; Components: ClientComponent; Flags: sharedfile;
#endif
#endif  /* if msvc_version >= 10 */

#if msvc_version >= 10
;Try to install CRT libraries to <sys> via msi, _IF_ msvc_version is 10 or later.
#if PlatformTarget == "x64"
;MinVersion 0,5.0 means no version of Win9x and at least Win2k if NT O/S
;In addition, O/S must have Windows Installer 3.0.
Source: {#FilesDir}\system32\vccrt{#msvc_version}_x64.msi; DestDir: {tmp};  Check: HasWI30; MinVersion: 0,5.0; Components: ClientComponent;
Source: {#WOW64Dir}\system32\vccrt{#msvc_version}_Win32.msi; DestDir: {tmp}; Check: HasWI30; MinVersion: 0,5.0; Components: ClientComponent;
#else
Source: {#FilesDir}\system32\vccrt{#msvc_version}_Win32.msi; DestDir: {tmp}; Check: HasWI30; MinVersion: 0,5.0; Components: ClientComponent;
#endif
#endif

;Docs
Source: {#ScriptsDir}\installation_scripted.txt; DestDir: {app}\doc; Components: DevAdminComponent; Flags: skipifsourcedoesntexist  ignoreversion
Source: {#ScriptsDir}\installation_scripted.txt; DestDir: {tmp}; Flags: DontCopy;
Source: {#FilesDir}\doc\*.*; DestDir: {app}\doc; Components: DevAdminComponent; Flags: skipifsourcedoesntexist  ignoreversion
Source: {#FilesDir}\doc\sql.extensions\*.*; DestDir: {app}\doc\sql.extensions; Components: DevAdminComponent; Flags: skipifsourcedoesntexist ignoreversion

;Other stuff
Source: {#FilesDir}\help\*.*; DestDir: {app}\help; Components: DevAdminComponent; Flags: ignoreversion;
Source: {#FilesDir}\include\*.*; DestDir: {app}\include; Components: DevAdminComponent; Flags: ignoreversion recursesubdirs createallsubdirs;
Source: {#FilesDir}\intl\fbintl.dll; DestDir: {app}\intl; Components: ServerComponent; Flags: sharedfile ignoreversion;
Source: {#FilesDir}\intl\fbintl.conf; DestDir: {app}\intl; Components: ServerComponent; Flags: onlyifdoesntexist
Source: {#FilesDir}\lib\*.*; DestDir: {app}\lib; Components: DevAdminComponent; Flags: ignoreversion;
#if PlatformTarget == "x64"
Source: {#WOW64Dir}\lib\*.lib; DestDir: {app}\WOW64\lib; Components: DevAdminComponent; Flags: ignoreversion
#endif
Source: {#FilesDir}\UDF\ib_udf.dll; DestDir: {app}\UDF; Components: ServerComponent; Flags: sharedfile ignoreversion;
Source: {#FilesDir}\UDF\fbudf.dll; DestDir: {app}\UDF; Components: ServerComponent; Flags: sharedfile ignoreversion;
Source: {#FilesDir}\UDF\*.sql; DestDir: {app}\UDF; Components: ServerComponent; Flags: ignoreversion;
Source: {#FilesDir}\UDF\*.txt; DestDir: {app}\UDF; Components: ServerComponent; Flags: ignoreversion;

Source: {#FilesDir}\plugins.conf; DestDir: {app}; Components: ServerComponent; Flags: ignoreversion;
Source: {#FilesDir}\plugins\*.dll; DestDir: {app}\plugins; Components: ServerComponent; Flags: ignoreversion;
Source: {#FilesDir}\plugins\*.conf; DestDir: {app}\plugins; Components: ServerComponent; Flags: ignoreversion;

Source: {#FilesDir}\misc\*.*; DestDir: {app}\misc; Components: ServerComponent; Flags: ignoreversion;
Source: {#FilesDir}\misc\upgrade\security\*.*; DestDir: {app}\misc\upgrade\security; Components: ServerComponent; Flags: ignoreversion;

;Source: {#FilesDir}\system32\Firebird2Control.cpl; DestDir: {sys}; Components: ServerComponent; MinVersion: 0,4.0; Flags: sharedfile ignoreversion promptifolder restartreplace uninsrestartdelete; Check: InstallCPLApplet
#endif /* files */

#ifdef examples
Source: {#FilesDir}\examples\*.*; DestDir: {app}\examples; Components: DevAdminComponent;  Flags: ignoreversion {#SkipFileIfDevStatus};
Source: {#FilesDir}\examples\api\*.*; DestDir: {app}\examples\api; Components: DevAdminComponent;  Flags: ignoreversion {#SkipFileIfDevStatus};
Source: {#FilesDir}\examples\build_win32\*.*; DestDir: {app}\examples\build_win32; Components: DevAdminComponent;  Flags: ignoreversion {#SkipFileIfDevStatus};
;Source: {#FilesDir}\examples\dyn\*.*; DestDir: {app}\examples\dyn; Components: DevAdminComponent;  Flags: ignoreversion {#SkipFileIfDevStatus};
Source: {#FilesDir}\examples\dbcrypt\*.*; DestDir: {app}\examples\dbcrypt; Components: DevAdminComponent;  Flags: ignoreversion {#SkipFileIfDevStatus};
Source: {#FilesDir}\examples\empbuild\*.*; DestDir: {app}\examples\empbuild; Components: DevAdminComponent;  Flags: ignoreversion {#SkipFileIfDevStatus};
Source: {#FilesDir}\examples\include\*.*; DestDir: {app}\examples\include; Components: DevAdminComponent;  Flags: ignoreversion {#SkipFileIfDevStatus};
Source: {#FilesDir}\examples\interfaces\*.*; DestDir: {app}\examples\interfaces; Components: DevAdminComponent;  Flags: ignoreversion {#SkipFileIfDevStatus};
Source: {#FilesDir}\examples\package\*.*; DestDir: {app}\examples\package; Components: DevAdminComponent;  Flags: ignoreversion {#SkipFileIfDevStatus};
Source: {#FilesDir}\examples\stat\*.*; DestDir: {app}\examples\stat; Components: DevAdminComponent;  Flags: ignoreversion {#SkipFileIfDevStatus};
Source: {#FilesDir}\examples\udf\*.*; DestDir: {app}\examples\udf; Components: DevAdminComponent;  Flags: ignoreversion {#SkipFileIfDevStatus};
Source: {#FilesDir}\examples\udr\*.*; DestDir: {app}\examples\udr; Components: DevAdminComponent;  Flags: ignoreversion {#SkipFileIfDevStatus};
#endif

#ifdef ship_pdb
Source: {#FilesDir}\fbclient.pdb; DestDir: {app}; Components: ClientComponent;
Source: {#FilesDir}\firebird.pdb; DestDir: {app}; Components: ServerComponent;
;Source: {#FilesDir}\fbembed.pdb; DestDir: {app}; Components: ClientComponent;
#if PlatformTarget == "x64"
Source: {#WOW64Dir}\fbclient.pdb; DestDir: {app}\WOW64; Components: ClientComponent;
#endif
#endif

[UninstallRun]
Filename: {app}\instsvc.exe; Parameters: "stop {code:ServiceName} "; StatusMsg: {cm:instsvcStopMsg}; MinVersion: 0,4.0; Components: ServerComponent; Flags: runminimized; Tasks: UseServiceTask; RunOnceId: StopService
Filename: {app}\instsvc.exe; Parameters: "remove {code:ServiceName} "; StatusMsg: {cm:instsvcRemove}; MinVersion: 0,4.0; Components: ServerComponent; Flags: runminimized; Tasks: UseServiceTask; RunOnceId: RemoveService
Filename: {app}\instclient.exe; Parameters: " remove gds32"; StatusMsg: {cm:instclientDecLibCountGds32}; MinVersion: 4.0,4.0; Flags: runminimized;
Filename: {app}\instclient.exe; Parameters: " remove fbclient"; StatusMsg: {cm:instclientDecLibCountFbClient}; MinVersion: 4.0,4.0; Flags: runminimized;
#if PlatformTarget == "x64"
Filename: {app}\wow64\instclient.exe; Parameters: " remove gds32"; StatusMsg: {cm:instclientDecLibCountGds32}; MinVersion: 4.0,4.0; Flags: runminimized 32bit;
Filename: {app}\wow64\instclient.exe; Parameters: " remove fbclient"; StatusMsg: {cm:instclientDecLibCountFbClient}; MinVersion: 4.0,4.0; Flags: runminimized 32bit;
#endif
Filename: {app}\instreg.exe; Parameters: " remove"; StatusMsg: {cm:instreg}; MinVersion: 4.0,4.0; Flags: runminimized; RunOnceId: RemoveRegistryEntry

[UninstallDelete]
Type: files; Name: "{app}\*.lck"
Type: files; Name: "{app}\*.evn"
Type: dirifempty; Name: "{app}"

[_ISTool]
EnableISX=true

[Code]
program Setup;


// Some global variables are also in FirebirdInstallEnvironmentChecks.inc
// This is not ideal, but then this scripting environment is not ideal, either.
// The basic point of the include files is to isolate chunks of code that are
// a) Form a module or have common functionality
// b) Debugged.
// This hopefully keeps the main script simpler to follow.

Var
  InstallRootDir: String;
  FirebirdConfSaved: String;
  ForceInstall: Boolean;        // If /force set on command-line we install _and_
                                // configure. Default is to install and configure only if
                                // no other working installation is found (unless we are installing
                                // over the same version)

  // Options for scripted uninstall.
  CleanUninstall: Boolean;      // If /clean is passed to the uninstaller it will delete
                                // user config files - firebird.conf, firebird.log,
                                // databases.conf, fbtrace.conf and the security database.

  SYSDBAPassword: String;       // SYSDBA password

#ifdef setuplogging
// Not yet implemented - leave log in %TEMP%
//  OkToCopyLog : Boolean;        // Set when installation is complete.
#endif

#include "FirebirdInstallSupportFunctions.inc"

#include "FirebirdInstallEnvironmentChecks.inc"

#include "FirebirdInstallGUIFunctions.inc"


var
  AdminUserPage: TInputQueryWizardPage;
  initWizardHeight: Integer; //In prev. version - the wizard form was resized to new size every time when go back button pressed

procedure InitializeWizard;
begin
  initWizardHeight := wizardform.height;

  { Create a page to grab the new SYSDBA password }
  AdminUserPage := CreateInputQueryPage(wpSelectTasks,
      ExpandConstant( '{cm:CreateSYSDBAPassword}' )
    , ExpandConstant( '{cm:ClickThroughPWCreation}' ) + #13#10 +
      ExpandConstant( '{cm:PasswordNote}' ) , '' );
  AdminUserPage.Add( ExpandConstant( '{cm:SYSDBAPassword}' ), True);
  AdminUserPage.Add( ExpandConstant( '{cm:RetypeSYSDBAPassword}' ), True);

  AdminUserPage.Values[0] := SYSDBAPassword;
  AdminUserPage.Values[1] := SYSDBAPassword;

end;


function InitializeSetup(): Boolean;
var
  i: Integer;
  CommandLine: String;
  cmdParams: TStringList;
begin

  result := true;

  CommandLine:=GetCmdTail;

  if ((pos('HELP',Uppercase(CommandLine)) > 0) or
    (pos('--',Uppercase(CommandLine)) > 0) )
//	or
//    (pos('/?',Uppercase(CommandLine)) > 0) or		// InnoSetup displays its own help if these switches are passed.
//    (pos('/H',Uppercase(CommandLine)) > 0) ) 		// Note also that our help scren only appears after the Choose Language dialogue :-(
	then begin
    ShowHelpDlg;
    result := False;
    Exit;

  end;

  if pos('FORCE',Uppercase(CommandLine)) > 0 then
    ForceInstall:=True;


    cmdParams := TStringList.create;
    for i:=0 to ParamCount do begin
      cmdParams.add(ParamStr(i));
      if pos('SYSDBAPASSWORD', Uppercase(ParamStr(i)) ) > 0 then
        SYSDBAPassword := Copy(ParamStr(i),Length('/SYSDBAPASSWORD=')+1,Length(ParamStr(i))-Length('/SYSDBAPASSWORD=') );
    end;
#ifdef iss_debug
    ShowDebugDlg(cmdParams.text,'');
#endif

  // Check if a server is running - we cannot continue if it is.
  if FirebirdDefaultServerRunning then begin
    result := false;
    exit;
  end;

  if not CheckWinsock2 then begin
    result := False;
    exit;
  end;

  //By default we want to install and confugure,
  //unless subsequent analysis suggests otherwise.
  InstallAndConfigure := Install + Configure;

  InstallRootDir := '';

  InitExistingInstallRecords;
  AnalyzeEnvironment;
  result := AnalysisAssessment;

end;


procedure DeInitializeSetup();
var
  ErrCode: Integer;
begin
  // Did the install fail because winsock 2 was not installed?
  if Winsock2Failure then
    // Ask user if they want to visit the Winsock2 update web page.
    if MsgBox(ExpandConstant('{cm:Winsock2Web1}')+#13#13+ExpandConstant('{cm:Winsock2Web2}'), mbInformation, MB_YESNO) = idYes then
      // User wants to visit the web page
      ShellExec('open', sMSWinsock2Update, '', '', SW_SHOWNORMAL, ewNoWait, ErrCode);

  if RunningServerVerString <> '' then
        MsgBox( #13+Format(ExpandConstant('{cm:FbRunning1}'), [RunningServerVerString])
      +#13
      +#13+ExpandConstant('{cm:FbRunning2}')
      +#13+ExpandConstant('{cm:FbRunning3}')
      +#13, mbError, MB_OK);

#ifdef setuplogging
// Not yet implemented - leave log in %TEMP%
//  if OkToCopyLog then
//    FileCopy (ExpandConstant ('{log}'), ExpandConstant ('{app}\InstallationLogFile.log'), FALSE);

//  RestartReplace (ExpandConstant ('{log}'), '');
#endif /* setuplogging */

end;

//This function tries to find an existing install of Firebird M.N
//If it succeeds it suggests that directory for the install
//Otherwise it suggests the default for Fb M.N
function ChooseInstallDir(Default: String): String;
var
  InterBaseRootDir,
  FirebirdRootDir: String;
begin
  //The installer likes to call this FOUR times, which makes debugging a pain,
  //so let's test for a previous call.
  if (InstallRootDir = '') then begin

    // Try to find the value of "RootDirectory" in the Firebird
    // registry settings. This is either where Fb 1.0 exists or Fb 1.5
    InterBaseRootDir := GetInterBaseDir;
    FirebirdRootDir := GetFirebirdDir;

    if (FirebirdRootDir <> '') and ( FirebirdRootDir = InterBaseRootDir ) then  //Fb 1.0 must be installed so don't overwrite it.
      InstallRootDir := Default;

    if (( InstallRootDir = '' ) and
        ( FirebirdRootDir = Default )) then // Fb 2.0 is already installed,
      InstallRootDir := Default;             // so we offer to install over it

    if (( InstallRootDir = '') and
        ( FirebirdVer[0] = 1 ) and ( FirebirdVer[1] = 5 ) ) then   // Firebird 1.5 is installed
      InstallRootDir := Default;                                   // but the user has changed the default

    if (( InstallRootDir = '') and
        ( FirebirdVer[0] = {#MajorVer} ) and ( FirebirdVer[1] = {#MinorVer} ) ) then   // Firebird 2.n is installed
      InstallRootDir := FirebirdRootDir;                            // but the user has changed the default

    // if we haven't found anything then try the FIREBIRD env var
    // User may have preferred location for Firebird, but has possibly
    // uninstalled previous version
    if (InstallRootDir = '') then
      InstallRootDir:=getenv('FIREBIRD');

    //if no existing locations found make sure we default to the default.
    if (InstallRootDir = '') then
      InstallRootDir := Default;

  end; // if InstallRootDir = '' then begin

  Result := ExpandConstant(InstallRootDir);

end;


function ServiceName(Default: String): String;
begin
    Result := ' -n DefaultInstance' ;
end;


function ServiceStartFlags(Default: String): String;
var
  ServerType: String;
  SvcParams: String;
  InstanceName: String;
begin
  ServerType := '';
  SvcParams := '';

  if IsComponentSelected('ServerComponent') and IsTaskSelected('AutoStartTask') then
    SvcParams := ' -auto '
  else
    SvcParams := ' -demand ';

  SvcParams := ServerType + SvcParams;

  if IsComponentSelected('ServerComponent') and ( IsTaskSelected('UseSuperServerTask\UseGuardianTask')
      or ( IsTaskSelected('UseSuperClassicTask\UseGuardianTask') ) ) then
    SvcParams := ServerType + SvcParams +  ' -guardian ';
  
  InstanceName := ServiceName('We currently do not support or test for a different instance name');

  SvcParams := SvcParams + InstanceName;

  Result := SvcParams;
end;


function GetAdminUserName: String;
begin
  Result := 'SYSDBA';
end;


function GetAdminUserPassword: String;
begin
    Result := AdminUserPage.Values[0];
    if Result = '' then
      Result := 'masterkey';
end;




function InitSecurityDB: Boolean;
var
  AStringList: TStringList;
  TempDir: String;
	ResultCode: Integer;
	CmdStr: string;
begin
	TempDir := ExpandConstant( '{tmp}' );
	CmdStr := ExpandConstant( '{app}\isql.exe' );
	AStringList := TStringList.create;
	with AStringList do begin
		Add( 'create user ' + GetAdminUserName + ' password ''' + GetAdminUserPassword + ''' using plugin Srp;' );
    if IsTaskSelected('EnableLegacyClientAuth') then
      if ( ( uppercase( GetAdminUserName ) <> 'SYSDBA' ) or ( GetAdminUserPassword <> 'masterkey' ) ) then
        Add( 'create or alter user ' + GetAdminUserName + ' password ''' + GetAdminUserPassword + ''' using plugin Legacy_UserManager;' );
		Add( 'commit;' );  //Technically exit implies a commit so this not necessary. OTOH, explicitly committing makes for more readable code.
		Add( 'exit;' );
		SaveToFile( Tempdir +'\temp.sql' );
	end;
	Result := Exec( CmdStr , ' -m -m2 -user SYSDBA -i ' + TempDir + '\temp.sql -o ' + TempDir + '\temp.sql.txt employee ' , TempDir, SW_HIDE, ewWaitUntilTerminated, ResultCode );
	DeleteFile( TempDir + +'\temp.sql ');
end;


function InstallGuardianIcon(): Boolean;
begin
  result := false;
  if IsTaskSelected('UseApplicationTask') and
    IsComponentSelected('ServerComponent') and (
    IsTaskSelected('UseSuperServerTask\UseGuardianTask') or
    IsTaskSelected('UseSuperClassicTask\UseGuardianTask') ) then
      result := true;
end;

function InstallServerIcon(): Boolean;
begin
  result := false;
  if IsTaskSelected('UseApplicationTask') and 
    IsComponentSelected('ServerComponent') and 
      not ( IsTaskSelected('UseSuperServerTask\UseGuardianTask') or
            IsTaskSelected('UseSuperClassicTask\UseGuardianTask') ) then
      result := true;
end;

function StartApp(Default: String): String;
begin
  if IsComponentSelected('ServerComponent') and ( 
      IsTaskSelected('UseSuperServerTask\UseGuardianTask') or
      IsTaskSelected('UseSuperClassicTask\UseGuardianTask') ) then
    Result := GetAppPath+'\fbguard.exe'
  else
    Result := GetAppPath+'\firebird.exe' ;
end;

function StartAppParams(Default: String): String;
begin
  Result := ' -a ';
end;

function IsNotAutoStartApp: boolean;
//Support function to help remove unwanted registry entry.
begin
  result := true;
  if ( IsComponentSelected('ServerComponent') and IsTaskSelected('AutoStartTask') ) and
    ( IsComponentSelected('ServerComponent') and IsTaskSelected('UseApplicationTask') ) then
  result := false;
end;


procedure RemoveSavedConfIfNoDiff;
//Compare firebird.conf with the one we just saved.
//If they match then we can remove the saved one.
var
  FirebirdConfStr: AnsiString;
  SavedConfStr: AnsiString;
begin
  LoadStringFromFile( GetAppPath+'\firebird.conf', FirebirdConfStr );
  LoadStringFromFile( FirebirdConfSaved, SavedConfStr );

  if CompareStr( FirebirdConfStr, SavedConfStr ) = 0 then
    DeleteFile( FirebirdConfSaved );
end;


procedure UpdateFirebirdConf;
// Update firebird conf. 
// If user has deselected the guardian we should update firebird.conf accordingly. 
// We also test if user has asked for classic or super server
// If EnableLegacyClientAuth has ben selected we update the file.......
// Otherwise we leave the file unchanged.
begin
  //There is no simple, foolproof and futureproof way to check whether
  //we are doing a server install, so the easiest way is to see if a
  //firebird.conf exists. If it doesn't then we don't care.
  if FileExists(GetAppPath+'\firebird.conf') then begin

    if (IsComponentSelected('ServerComponent') ) then begin 

// Setting GuardianOption to 0 makes no sense. If the user deploys the guardian 
// there is an expectation that it will restart the server in the event of a crash. 
// Otherwise, why start firebird with the guardian?
//      if not ( IsTaskSelected('UseSuperServerTask\UseGuardianTask') or  IsTaskSelected('UseSuperClassicTask\UseGuardianTask') ) then
//				ReplaceLine(GetAppPath+'\firebird.conf','GuardianOption','GuardianOption = 0','#');

      // These attempts to modify firebird.conf may not survice repeated installs.  

			if IsTaskSelected('UseClassicServerTask') then
				ReplaceLine(GetAppPath+'\firebird.conf','ServerMode = ','ServerMode = Classic','#');

      if IsTaskSelected('UseSuperClassicTask') then
				ReplaceLine(GetAppPath+'\firebird.conf','ServerMode = ','ServerMode = SuperClassic','#');

			if IsTaskSelected('UseSuperServerTask')  then
				ReplaceLine(GetAppPath+'\firebird.conf','ServerMode = ','ServerMode = Super','#');

      if IsTaskSelected('EnableLegacyClientAuth') then begin
				ReplaceLine(GetAppPath+'\firebird.conf','AuthServer = ','AuthServer = Legacy_Auth, Srp, Win_Sspi','#');
				ReplaceLine(GetAppPath+'\firebird.conf','AuthClient = ','AuthClient = Legacy_Auth, Srp, Win_Sspi','#');
				ReplaceLine(GetAppPath+'\firebird.conf','UserManager = ','UserManager = Legacy_UserManager, Srp','#');
				ReplaceLine(GetAppPath+'\firebird.conf','WireCrypt = ','WireCrypt = enabled','#');
      end;

		end;	
			
  end;
end;


function NonDefaultLanguage: boolean;
//return true if language other than default is chosen
begin
  result := (ActiveLanguage <> 'en');
end;

//Make installation form a little taller and wider.
const
  HEIGHT_INCREASE = 175;
  WIDTH_INCREASE = 40;

procedure ResizeWizardForm(Increase: Boolean);
var
  AWidth,AHeight: Integer;
begin
  //Do resize only once!
  if wizardform.height = initWizardHeight then begin
  AHeight := HEIGHT_INCREASE;
  AWidth := WIDTH_INCREASE;

  if not Increase then begin
    AHeight := (AHeight * (-1));
    AWidth := (AWidth * (-1));
  end;

  SetupWizardFormComponentsArrays;
  ResizeWizardFormHeight(AHeight);
//  ResizeWizardFormWidth(AWidth);
  end;
end;

procedure CurPageChanged(CurPage: Integer);
// These are the predefined page IDs of the Wizard form as of InnoSetup 5.5.6
// wpWelcome, wpLicense, wpPassword, wpInfoBefore, wpUserInfo, 
// wpSelectDir, wpSelectComponents, wpSelectProgramGroup, wpSelectTasks, 
// wpReady, wpPreparing, wpInstalling, wpInfoAfter, wpFinished 
begin
  case CurPage of
    wpWelcome:      ResizeWizardForm(True); //There was a bug: every time when "go back" pressed the form was resized!
    wpInfoBefore:   WizardForm.INFOBEFOREMEMO.font.name:='Courier New';
    wpInfoAfter:    WizardForm.INFOAFTERMEMO.font.name:='Courier New';
  end;
end;


procedure CurStepChanged(CurStep: TSetupStep);
// currently just three steps - ssInstall, ssPostInstall, ssDone 
var
  AppStr: String;
  ReadMeFileStr: String;
begin
   case CurStep of
    ssInstall: begin
//      RenamePreFB3RC1Files;
      SetupSharedFilesArray;
      GetSharedLibCountBeforeCopy;
      end;

    ssPostInstall: begin
      //Manually set the sharedfile count of these files.
      IncrementSharedCount(Is64BitInstallMode, GetAppPath+'\firebird.conf', false);
      IncrementSharedCount(Is64BitInstallMode, GetAppPath+'\firebird.log', false);
      IncrementSharedCount(Is64BitInstallMode, GetAppPath+'\databases.conf', false);
      IncrementSharedCount(Is64BitInstallMode, GetAppPath+'\fbtrace.conf', false);
      IncrementSharedCount(Is64BitInstallMode, GetAppPath+'\security4.fdb', false);

			InitSecurityDB;

      //Fix up conf file
      UpdateFirebirdConf;
      RemoveSavedConfIfNoDiff;

				
      end;

    ssDone: begin
      //If user has chosen to install an app and run it automatically set up the registry accordingly
      //so that the server or guardian starts evertime they login.
      if (IsComponentSelected('ServerComponent') and IsTaskSelected('AutoStartTask') ) and
              ( IsComponentSelected('ServerComponent') and IsTaskSelected('UseApplicationTask') ) then begin
        AppStr := StartApp('')+StartAppParams('');
        RegWriteStringValue (HKLM, 'SOFTWARE\Microsoft\Windows\CurrentVersion\Run', 'Firebird', AppStr);
      end;

      //Reset shared library count if necessary
      CheckSharedLibCountAtEnd;

      //Move lang specific readme from doc dir to root of install.
      if NonDefaultLanguage then begin
        ReadMeFileStr := ExpandConstant('{cm:ReadMeFile}');
        if FileCopy(GetAppPath+'\doc\'+ReadMeFileStr, GetAppPath+'\'+ReadMeFileStr, false) then
          DeleteFile(GetAppPath+'\doc\'+ReadMeFileStr);
      end;

#ifdef setuplogging
//      OkToCopyLog := True;
#endif

    end;
  end;
end;

function StartEngine: boolean;
begin
  if ConfigureFirebird then
    result := not FirebirdDefaultServerRunning;
end;

// # FIXME - we can probably remove this function
function ChooseUninstallIcon(Default: String): String;
begin
	result := GetAppPath+'\firebird.exe';
end;

//InnoSetup has a Check Parameter that allows installation if the function returns true.
//For firebird.conf we want to do two things:
// o if firebird.conf already exists then install firebird.conf.default
// o if firebird.conf does not exist then install firebird.conf
//
// This double test is also needed because the uninstallation rules are different for each file.
// We never uninstall firebird.conf. We always uninstall firebird.conf.default.

function FirebirdConfExists: boolean;
begin
  Result := fileexists(GetAppPath+'\firebird.conf');
end;

function NoFirebirdConfExists: boolean;
begin
  Result := not fileexists(GetAppPath+'\firebird.conf');
end;

function fbtraceConfExists: boolean;
begin
  Result := fileexists(GetAppPath+'\fbtrace.conf');
end;

function NofbtraceConfExists: boolean;
begin
  Result := not fileexists(GetAppPath+'\fbtrace.conf');
end;

function InitializeUninstall: Boolean;
var
  CommandLine: String;
begin
  CommandLine:=GetCmdTail;
  if pos('CLEAN',Uppercase(CommandLine))>0 then
    CleanUninstall:=True
  else
    CleanUninstall:=False;

  //MUST return a result of true, otherwise uninstall will abort!
  result := true;
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
var
  res: boolean;
  i,count: integer;
  aStringList: TStringList;
  appPath: string;
begin

  case CurUninstallStep of

//    usAppMutexCheck :

//    usUninstall :

    usPostUninstall : begin
        if CleanUninstall then begin

          aStringList := TStringList.create;
          appPath := GetAppPath;
          aStringList.add(appPath+'\firebird.conf');
          aStringList.add(appPath+'\firebird.log');
          aStringList.add(appPath+'\databases.conf');
          aStringList.add(appPath+'\fbtrace.conf');
          aStringList.add(appPath+'\security4.fdb');
          aStringList.add(appPath+'\security4.fdb.old');

          for count := 0 to aStringList.count - 1 do begin
      // We are manually handling the share count of these files, so we must
      // a) Decrement shared count of each one and
      // b) If Decrement reaches 0 (ie, function returns true) then we
            //    delete the file.
            // c) We arbitrarily break after 100 loops. Typically the shared count should only be 
            //    in single digits anyway but we don't want to risk the test entering an endless loop.
            i := 0;
            while not DecrementSharedCount(Is64BitInstallMode, aStringList[ count ] ) 
            do 
              if i = 100 then break else inc(i);

            res := DeleteFile( aStringList[ count ] );

          end;

      end;
    end;

    usDone :  res := RemoveDir(GetAppPath);

  end;

end;


function ShouldSkipPage(PageID: Integer): Boolean;
begin
  if ( PageID = AdminUserPage.ID ) then
    { If we are not configuring Firebird then don't prompt for SYSDBA pw. }
    if not ConfigureFirebird then
      Result := True
    else if not ConfigureAuthentication then
      Result := True
    else 
      Result := False;
end;


function NextButtonClick(CurPageID: Integer): Boolean;
var
	i: integer;
begin
  Result := True;
  case CurPageID of
    AdminUserPage.ID : begin
	  { check user has entered new sysdba password correctly. }
      i := CompareStr(AdminUserPage.Values[0],AdminUserPage.Values[1]);
      If  not (i = 0) then begin
        Result := False;
        AdminUserPage.Values[0] :='';
        AdminUserPage.Values[1] :='';
        MsgBox(ExpandConstant('{cm:SYSDBAPasswordMismatch}'), mbError, MB_OK);
      end;
    end;
  end;
end;
	
begin
end.
