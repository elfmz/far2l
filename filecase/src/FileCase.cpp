#define _FAR_NO_NAMELESS_UNIONS
#include <farplug-wide.h>
#include "FileLng.hpp"
#include "FileCase.hpp"

#include <utils.h>
#include <KeyFileHelper.h>

#define INI_LOCATION InMyConfig("plugins/filecase/config.ini")
#define INI_SECTION  "Settings"

#include "FileMix.icpp"
#include "filecvt.icpp"
#include "ProcessName.icpp"

SHAREDSYMBOL int WINAPI EXP_NAME(GetMinFarVersion)()
{
	return FARMANAGERVERSION;
}

SHAREDSYMBOL void WINAPI EXP_NAME(SetStartupInfo)(const struct PluginStartupInfo *Info)
{
	::Info = *Info;
	::FSF = *Info->FSF;
	::Info.FSF = &::FSF;

	KeyFileReadSection kfh(INI_LOCATION, INI_SECTION);
	Opt.ConvertMode = kfh.GetInt(("ConvertMode"), 0);
	Opt.ConvertModeExt = kfh.GetInt(("ConvertModeExt"), 0);
	Opt.SkipMixedCase = kfh.GetInt(("SkipMixedCase"), 1);
	Opt.ProcessSubDir = kfh.GetInt(("ProcessSubDir"), 0);
	Opt.ProcessDir = kfh.GetInt(("ProcessDir"), 0);
	kfh.GetChars(Opt.WordDiv, ARRAYSIZE(Opt.WordDiv), ("WordDiv"), L" _");
	Opt.WordDivLen = lstrlen(Opt.WordDiv);
}

SHAREDSYMBOL HANDLE WINAPI EXP_NAME(OpenPlugin)(int OpenFrom, INT_PTR Item)
{
	CaseConversion();
	return (INVALID_HANDLE_VALUE);
}

SHAREDSYMBOL void WINAPI EXP_NAME(GetPluginInfo)(struct PluginInfo *Info)
{
	Info->StructSize = sizeof(*Info);
	static TCHAR *PluginMenuStrings[1];
	PluginMenuStrings[0] = (TCHAR *)GetMsg(MFileCase);
	Info->PluginMenuStrings = PluginMenuStrings;
	Info->PluginMenuStringsNumber = ARRAYSIZE(PluginMenuStrings);
}
