#include "headers.hpp"
#include <sys/utsname.h>
#include <KeyFileHelper.h>
#include "plugins.hpp"
#include "interf.hpp"
#include "clipboard.hpp"
#include "pathmix.hpp"
#include "vmenu.hpp"
#include "config.hpp"
#include "farversion.h"
#include "vtshell.h"

void FarAbout(PluginManager &Plugins)
{
	static bool b_hide_empty = true;
	int npl;
	FARString fs, fs2, fs2copy;
	MenuItemEx mi, mis;
	mi.Flags = b_hide_empty ? LIF_HIDDEN : 0;
	mis.Flags = LIF_SEPARATOR;

	VMenu ListAbout(b_hide_empty ? L"far:about *" : L"far:about", nullptr, 0, ScrY-4);
	ListAbout.SetFlags(VMENU_SHOWAMPERSAND | VMENU_IGNORE_SINGLECLICK);
	ListAbout.ClearFlags(VMENU_MOUSEREACTION);
	//ListAbout.SetFlags(VMENU_WRAPMODE);
	ListAbout.SetHelp(L"SpecCmd");//L"FarAbout");
	ListAbout.SetBottomTitle(L"ESC or F10 to close, Ctrl-C or Ctrl-Ins - copy all, Ctrl-H - (un)hide empty, Ctrl-Alt-F - filtering");

	fs.Format(L"          FAR2L Version: %s", FAR_BUILD);
	ListAbout.AddItem(fs); fs2copy = fs;
	fs =      L"               Compiler: ";
#if defined (__clang__)
	fs.AppendFormat(L"Clang, version %d.%d.%d", __clang_major__, __clang_minor__, __clang_patchlevel__);
#elif defined (__INTEL_COMPILER)
	fs.AppendFormat(L"Intel C/C++, version %d (build date %d)", __INTEL_COMPILER, __INTEL_COMPILER_BUILD_DATE);
#elif defined (__GNUC__)
	fs.AppendFormat(L"GCC, version %d.%d.%d", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
#else
	fs.Append(L"Unknown");
#endif
	ListAbout.AddItem(fs); fs2copy += "\n" + fs;

	fs.Format(L"               Platform: %s", FAR_PLATFORM);
	ListAbout.AddItem(fs); fs2copy += "\n" + fs;
	fs.Format(L"                Backend: %s", WinPortBackendInfo(-1));
	ListAbout.AddItem(fs); fs2copy += "\n" + fs;
	fs.Format(L"    ConsoleColorPalette: %u", WINPORT(GetConsoleColorPalette)(NULL) );
	ListAbout.AddItem(fs); fs2copy += "\n" + fs;
	fs.Format(L"                  Admin: %ls", Opt.IsUserAdmin ? Msg::FarTitleAddonsAdmin : L"-");
	ListAbout.AddItem(fs); fs2copy += "\n" + fs;
	//apiGetEnvironmentVariable("FARPID", fs2);
	//fs = L"           PID: " + fs2;
	fs.Format(L"                    PID: %lu", (unsigned long)getpid());
	ListAbout.AddItem(fs); fs2copy += "\n" + fs;

	//apiGetEnvironmentVariable("FARLANG", fs2);
	fs =      L"  Main | Help languages: " + Opt.strLanguage + L" | " + Opt.strHelpLanguage;
	ListAbout.AddItem(fs); fs2copy += "\n" + fs;

	fs.Format(L"   OEM | ANSI codepages: %u | %u", WINPORT(GetOEMCP)(), WINPORT(GetACP)() );
	ListAbout.AddItem(fs); fs2copy += "\n" + fs;

	//apiGetEnvironmentVariable("FARHOME", fs2);
	fs =      L"Far directory (FARHOME): \"" + g_strFarPath.GetMB() + L"\"";
	ListAbout.AddItem(fs); fs2copy += "\n" + fs;

	fs.Format(L"       Config directory: \"%s\"", InMyConfig("",FALSE).c_str() );
	ListAbout.AddItem(fs); fs2copy += "\n" + fs;

	fs.Format(L"        Cache directory: \"%s\"", InMyCache("",FALSE).c_str() );
	ListAbout.AddItem(fs); fs2copy += "\n" + fs;

	fs.Format(L"         Temp directory: \"%s\"", InMyTemp("").c_str() );
	ListAbout.AddItem(fs); fs2copy += "\n" + fs;

	if (Opt.CmdLine.UseShell)
		fs.Format(L"   Command shell (User): \"%ls\"", Opt.CmdLine.strShell.CPtr() );
	else
		fs.Format(L" Command shell (System): \"%s\"", GetSystemShell() );
	ListAbout.AddItem(fs); fs2copy += "\n" + fs;

	ListAbout.AddItem(L""); fs2copy += "\n";
	struct utsname un;
	fs = L"                  uname: ";
	if (uname(&un)==0) {
		fs.AppendFormat(L"%s %s %s %s", un.sysname, un.release, un.version, un.machine);
		ListAbout.AddItem(fs);
	}
	else {
		mi.strName = fs;
		ListAbout.AddItem(&mi);
	}
	fs2copy += "\n" + fs;

	for (unsigned int i = 0; ; i++) {
		const char *info = WinPortBackendInfo(i);
		if (!info) {
			break;
		}
		fs.Format(L"%23s: %s", "System component", info);
		ListAbout.AddItem(fs);
		fs2copy += "\n" + fs;
	}

	static const char * const env_vars[] = {
		"HOSTNAME", "USER",
		"FARSETTINGS", "FAR2L_ARGS",
		"XDG_SESSION_TYPE",
		"TERM", "COLORTERM",
		"GDK_BACKEND", "DESKTOP_SESSION",
		"WSL_DISTRO_NAME", "WSL2_GUI_APPS_ENABLED",
		"DISPLAY", "WAYLAND_DISPLAY" };
	for (unsigned int i = 0; i < ARRAYSIZE(env_vars); i++) {
		fs.Format(L"%23s: ", env_vars[i]);
		if (apiGetEnvironmentVariable(env_vars[i], fs2)) {
			fs += fs2.CPtr();
			ListAbout.AddItem(fs);
		}
		else {
			mi.strName = fs;
			ListAbout.AddItem(&mi);
		}
		fs2copy += "\n" + fs;
	}

	ListAbout.AddItem(L""); fs2copy += "\n";

	npl = Plugins.GetPluginsCount();
	fs.Format(L"      Number of plugins: %d", npl);
	ListAbout.AddItem(fs); fs2copy += "\n" + fs;

	mi.Flags = 0;

	for(int i = 0; i < npl; i++)
	{
		fs.Format(L"Plugin#%02d ",  i+1);
		mis.strName = fs;
		mi.PrefixLen = fs.GetLength()-1;

		Plugin *pPlugin = Plugins.GetPlugin(i);
		if(pPlugin == nullptr) {
			ListAbout.AddItem(&mis); fs2copy += "\n--- " + mis.strName + " ---";
			mi.strName = fs + L"!!! ERROR get plugin";
			ListAbout.AddItem(&mi); fs2copy += "\n" + mi.strName;
			continue;
		}
		mis.strName = fs + PointToName(pPlugin->GetModuleName());
		ListAbout.AddItem(&mis); fs2copy += "\n--- " + mis.strName + " ---";

		mi.strName = fs + pPlugin->GetModuleName();
		ListAbout.AddItem(&mi); fs2copy += "\n" + mi.strName;

		mi.strName = fs + L"Settings Name: " + pPlugin->GetSettingsName();
		ListAbout.AddItem(&mi); fs2copy += "\n" + mi.strName;

		int iFlags;
		int j;
		PluginInfo pInfo{};
		KeyFileReadHelper kfh(PluginsIni());
		FARString fsCommandPrefix = L"";
		std::vector<FARString> fsDiskMenuStrings;
		std::vector<FARString> fsPluginMenuStrings;
		std::vector<FARString> fsPluginConfigStrings;

		if (pPlugin->CheckWorkFlags(PIWF_CACHED)) {
			iFlags = kfh.GetUInt(pPlugin->GetSettingsName(), "Flags", 0);
			fsCommandPrefix = kfh.GetString(pPlugin->GetSettingsName(), "CommandPrefix", L"");
			for (j = 0; ; j++) {
				const auto &key_name = StrPrintf("DiskMenuString%d", j);
				/*if (!kfh.HasKey(key_name))
					break;*/
				fs2 = kfh.GetString(pPlugin->GetSettingsName(), key_name, "");
				if( fs2.IsEmpty() )
					break;
				fsDiskMenuStrings.emplace_back(fs2);
			}
			for (j = 0; ; j++) {
				const auto &key_name = StrPrintf("PluginMenuString%d", j);
				/*if (!kfh.HasKey(key_name))
					break;*/
				fs2 = kfh.GetString(pPlugin->GetSettingsName(), key_name, "");
				if( fs2.IsEmpty() )
					break;
				fsPluginMenuStrings.emplace_back(fs2);
			}
			for (j = 0; ; j++) {
				const auto &key_name = StrPrintf("PluginConfigString%d", j);
				/*if (!kfh.HasKey(key_name))
					break;*/
				fs2 = kfh.GetString(pPlugin->GetSettingsName(), key_name, "");
				if( fs2.IsEmpty() )
					break;
				fsPluginConfigStrings.emplace_back(fs2);
			}
		}
		else {
			if (pPlugin->GetPluginInfo(&pInfo)) {
				iFlags = pInfo.Flags;
				fsCommandPrefix = pInfo.CommandPrefix;
				for (j = 0; j < pInfo.DiskMenuStringsNumber; j++)
					fsDiskMenuStrings.emplace_back(pInfo.DiskMenuStrings[j]);
				for (j = 0; j < pInfo.PluginMenuStringsNumber; j++)
					fsPluginMenuStrings.emplace_back(pInfo.PluginMenuStrings[j]);
				for (j = 0; j < pInfo.PluginConfigStringsNumber; j++)
					fsPluginConfigStrings.emplace_back(pInfo.PluginConfigStrings[j]);
			}
			else
				iFlags = -1;
		}

		mi.strName.Format(L"%ls     %s Cached  %s Loaded ",
			fs.CPtr(),
			pPlugin->CheckWorkFlags(PIWF_CACHED) ? "[x]" : "[ ]",
			pPlugin->GetFuncFlags() & PICFF_LOADED ? "[x]" : "[ ]");
		ListAbout.AddItem(&mi); fs2copy += "\n" + mi.strName;

		if (iFlags >= 0) {
			mi.strName.Format(L"%lsF11: %s Panel   %s Dialog  %s Viewer  %s Editor ",
				fs.CPtr(),
				iFlags & PF_DISABLEPANELS ? "[ ]" : "[x]",
				iFlags & PF_DIALOG ? "[x]" : "[ ]",
				iFlags & PF_VIEWER ? "[x]" : "[ ]",
				iFlags & PF_EDITOR ? "[x]" : "[ ]");
			ListAbout.AddItem(&mi); fs2copy += "\n" + mi.strName;
		}

		mi.strName.Format(L"%ls     %s EditorInput ", fs.CPtr(), pPlugin->HasProcessEditorInput() ? "[x]" : "[ ]");
		ListAbout.AddItem(&mi); fs2copy += "\n" + mi.strName;

		j = 0;
		for (auto& fs_vec : fsDiskMenuStrings) {
			j++;
			mi.strName.Format(L"%ls     DiskMenuString: %2d=\"%ls\"", fs.CPtr(), j, fs_vec.CPtr());
			ListAbout.AddItem(&mi); fs2copy += "\n" + mi.strName;
		}
		j = 0;
		for (auto& fs_vec : fsPluginMenuStrings) {
			j++;
			mi.strName.Format(L"%ls   PluginMenuString: %2d=\"%ls\"", fs.CPtr(), j, fs_vec.CPtr());
			ListAbout.AddItem(&mi); fs2copy += "\n" + mi.strName;
		}
		j = 0;
		for (auto& fs_vec : fsPluginConfigStrings) {
			j++;
			mi.strName.Format(L"%ls PluginConfigString: %2d=\"%ls\"", fs.CPtr(), j, fs_vec.CPtr());
			ListAbout.AddItem(&mi); fs2copy += "\n" + mi.strName;
		}
		if ( !fsCommandPrefix.IsEmpty() ) {
			mi.strName.Format(L"%ls      CommandPrefix: \"%ls\"", fs.CPtr(), fsCommandPrefix.CPtr());
			ListAbout.AddItem(&mi); fs2copy += "\n" + mi.strName;
		}
		
	}

	ListAbout.SetPosition(-1, -1, 0, 0);
	/*int iListExitCode = 0;
	do {
		ListAbout.Process();
		iListExitCode = ListAbout.GetExitCode();
		if (iListExitCode>=0)
			ListAbout.ClearDone(); // no close after select item by ENTER or mouse click
	} while(iListExitCode>=0);*/
	ListAbout.Show();
	do {
		while (!ListAbout.Done()) {
			FarKey Key = ListAbout.ReadInput();
			switch (Key) {
				case KEY_CTRLC:
				case KEY_CTRLINS:
				case KEY_CTRLNUMPAD0:
					CopyToClipboard(fs2copy.CPtr());
					break;
				case KEY_CTRLH: {
						struct MenuItemEx *mip;
						if (b_hide_empty) {
							b_hide_empty = false;
							for (int i = 0; i < ListAbout.GetItemCount(); i++) {
								mip = ListAbout.GetItemPtr(i);
								mip->Flags &= ~LIF_HIDDEN;
							}
							ListAbout.SetTitle(L"far:about");
						}
						else {
							b_hide_empty = true;
							for (int i = 0; i < ListAbout.GetItemCount(); i++) {
								mip = ListAbout.GetItemPtr(i);
								if (mip->strName.Ends(L": "))
									mip->Flags |= LIF_HIDDEN;
							}
							ListAbout.SetTitle(L"far:about *");
						}
					}
					ListAbout.Show();
					break;
				default:
					ListAbout.ProcessInput();
					continue;
			}
		}
		if (ListAbout.GetExitCode() < 0) // exit from loop only by ESC or F10 or click outside vmenu
			break;
		ListAbout.ClearDone(); // no close after select item by ENTER or mouse click
	} while(1);
}

