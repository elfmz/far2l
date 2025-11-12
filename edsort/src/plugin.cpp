#include "farapi.h"
#include "edsort.h"

PluginStartupInfo    _PSI;
FarStandardFunctions _FSF;

SHAREDSYMBOL int WINAPI EXP_NAME(GetMinFarVersion)()
{
	return FARMANAGERVERSION;
}

SHAREDSYMBOL void WINAPI EXP_NAME(SetStartupInfo)(const PluginStartupInfo* psi)
{
	_PSI = *psi;
	_FSF = *psi->FSF;
	_PSI.FSF = &_FSF;
}

SHAREDSYMBOL void WINAPI EXP_NAME(GetPluginInfo)(PluginInfo* info)
{
	static const wchar_t* menu_strings[1];
	menu_strings[0] = I18N(ps_title);

	info->StructSize = sizeof(*info);
	info->Flags = PF_EDITOR | PF_DISABLEPANELS;
	info->DiskMenuStringsNumber = 0;
	info->PluginConfigStringsNumber = 0;
	info->PluginMenuStrings = menu_strings;
	info->PluginMenuStringsNumber = sizeof(menu_strings) / sizeof(menu_strings[0]);
}

SHAREDSYMBOL HANDLE WINAPI EXP_NAME(OpenPlugin)(int openFrom, INT_PTR item)
{
	EdSort dlg;
	dlg.Run();
	return INVALID_HANDLE_VALUE;
}
