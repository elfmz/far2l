#include <windows.h>
#define _FAR_NO_NAMELESS_UNIONS
#include "plugin.hpp"
#include "../../etc/plugs.h"
#include "FileLng.hpp"
#include "FileCase.hpp"

#include <utils.h>
#include <KeyFileHelper.h>

#define INI_LOCATION	InMyConfig("plugins/autowrap/settings.ini")
#define INI_SECTION		"Settings"

#include "FileMix.icpp"
#include "filecvt.icpp"
#include "ProcessName.icpp"


SHAREDSYMBOL int WINAPI EXP_NAME(GetMinFarVersion)()
{
  return FARMANAGERVERSION;
}


SHAREDSYMBOL void WINAPI EXP_NAME(SetStartupInfo)(const struct PluginStartupInfo *Info)
{
	::Info=*Info;
	::FSF=*Info->FSF;
	::Info.FSF=&::FSF;

	KeyFileReadHelper kfh(INI_LOCATION);
	Opt.ConvertMode=kfh.GetInt(INI_SECTION,("ConvertMode"),0);
	Opt.ConvertModeExt=kfh.GetInt(INI_SECTION,("ConvertModeExt"),0);
	Opt.SkipMixedCase=kfh.GetInt(INI_SECTION,("SkipMixedCase"),1);
	Opt.ProcessSubDir=kfh.GetInt(INI_SECTION,("ProcessSubDir"),0);
	Opt.ProcessDir=kfh.GetInt(INI_SECTION,("ProcessDir"),0);
	kfh.GetChars(Opt.WordDiv,ARRAYSIZE(Opt.WordDiv),INI_SECTION,("WordDiv"),_T(" _"));
	Opt.WordDivLen=lstrlen(Opt.WordDiv);
}


SHAREDSYMBOL HANDLE WINAPI EXP_NAME(OpenPlugin)(int OpenFrom,INT_PTR Item)
{
  CaseConvertion();
  return(INVALID_HANDLE_VALUE);
}


SHAREDSYMBOL void WINAPI EXP_NAME(GetPluginInfo)(struct PluginInfo *Info)
{
	Info->StructSize=sizeof(*Info);
	static TCHAR *PluginMenuStrings[1];
	PluginMenuStrings[0]=(TCHAR*)GetMsg(MFileCase);
	Info->PluginMenuStrings=PluginMenuStrings;
	Info->PluginMenuStringsNumber=ARRAYSIZE(PluginMenuStrings);
}
