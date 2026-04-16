// FAR Manager includes
#include "farplug-wide.h"

// Standard library includes
#include <algorithm>

// Local includes
#include "ADBPlugin.h"
#include "ADBLog.h"
#include "FARPlugin.h"

extern "C" {

SHAREDSYMBOL int WINAPI GetMinFarVersionW()
{
	DBG("GetMinFarVersionW called\n");
	return MAKEFARVERSION(2, 0);
}

SHAREDSYMBOL void WINAPI SetStartupInfoW(const PluginStartupInfo *Info)
{
	DBG("SetStartupInfoW called\n");
	// Copy the Info into global instance (like NetRocks)
	if (Info) {
		memcpy(&g_Info, Info, std::min((size_t)Info->StructSize, sizeof(PluginStartupInfo)));
		// Copy FSF structure to local copy and point g_Info.FSF to it
		if (Info->FSF) {
			g_FSF = *(Info->FSF);
			g_Info.FSF = &g_FSF;
		}
	}
}

SHAREDSYMBOL void WINAPI GetPluginInfoW(PluginInfo *Info)
{
	DBG("GetPluginInfoW called\n");
	if (!Info) {
		return;
	}
	Info->StructSize = sizeof(*Info);
	Info->SysID = 0x41444250; // 'ADBP'
	Info->Flags = PF_FULLCMDLINE;
	static const wchar_t *s_disk_menu_strings[] = {L"ADB"};
	Info->DiskMenuStrings = s_disk_menu_strings;
	Info->DiskMenuStringsNumber = 1;
	static const wchar_t *s_menu_strings[] = {L"ADB Plugin"};
	static const wchar_t *s_config_strings[] = {L"ADB Plugin"};
	Info->PluginMenuStrings = s_menu_strings;
	Info->PluginMenuStringsNumber = 1;
	Info->PluginConfigStrings = s_config_strings;
	Info->PluginConfigStringsNumber = 1;
	static const wchar_t *s_command_prefix = L"adb";
	Info->CommandPrefix = s_command_prefix;
}

SHAREDSYMBOL HANDLE WINAPI OpenPluginW(int OpenFrom, INT_PTR Item)
{
	DBG("OpenPluginW called: OpenFrom=%d, Item=%ld\n", OpenFrom, (long)Item);
	try {
		// Create new plugin instance
		ADBPlugin* plugin = new ADBPlugin();
		return (HANDLE)plugin;
	} catch (const std::exception& e) {
		return INVALID_HANDLE_VALUE;
	} catch (...) {
		return INVALID_HANDLE_VALUE;
	}
}

SHAREDSYMBOL void WINAPI ClosePluginW(HANDLE hPlugin)
{
	DBG("ClosePluginW called: hPlugin=%p\n", hPlugin);
	if (hPlugin && hPlugin != INVALID_HANDLE_VALUE) {
		ADBPlugin *plugin = (ADBPlugin*)hPlugin;
		delete plugin;
	}
}

SHAREDSYMBOL int WINAPI GetFindDataW(HANDLE hPlugin, PluginPanelItem **pPanelItem, int *pItemsNumber, int OpMode)
{
	DBG("GetFindDataW called: hPlugin=%p, OpMode=0x%x\n", hPlugin, OpMode);
	if (!hPlugin || hPlugin == INVALID_HANDLE_VALUE) {
		return 0;
	}
	ADBPlugin *plugin = (ADBPlugin*)hPlugin;
	int result = plugin->GetFindData(pPanelItem, pItemsNumber, OpMode);
	return result;
}

SHAREDSYMBOL void WINAPI FreeFindDataW(HANDLE hPlugin, PluginPanelItem *PanelItem, int ItemsNumber)
{
	DBG("FreeFindDataW called: hPlugin=%p, ItemsNumber=%d\n", hPlugin, ItemsNumber);
	if (hPlugin && hPlugin != INVALID_HANDLE_VALUE) {
		ADBPlugin *plugin = (ADBPlugin*)hPlugin;
		plugin->FreeFindData(PanelItem, ItemsNumber);
	}
}

SHAREDSYMBOL void WINAPI GetOpenPluginInfoW(HANDLE hPlugin, OpenPluginInfo *Info)
{
	DBG("GetOpenPluginInfoW called: hPlugin=%p\n", hPlugin);
	if (!hPlugin || hPlugin == INVALID_HANDLE_VALUE) {
		return;
	}
	ADBPlugin *plugin = (ADBPlugin*)hPlugin;
	plugin->GetOpenPluginInfo(Info);
}

SHAREDSYMBOL int WINAPI ProcessKeyW(HANDLE hPlugin, int Key, unsigned int ControlState)
{
	DBG("ProcessKeyW called: hPlugin=%p, Key=%d, ControlState=0x%x\n", hPlugin, Key, ControlState);
	if (!hPlugin || hPlugin == INVALID_HANDLE_VALUE) {
		return 0;
	}
	ADBPlugin *plugin = (ADBPlugin*)hPlugin;
	return plugin->ProcessKey(Key, ControlState);
}

SHAREDSYMBOL int WINAPI ProcessEventW(HANDLE hPlugin, int Event, void *Param)
{
	DBG("ProcessEventW called: hPlugin=%p, Event=%d, Param=%p\n", hPlugin, Event, Param);
	if (!hPlugin || hPlugin == INVALID_HANDLE_VALUE) {
		return 0;
	}
	switch (Event) {
		case FE_COMMAND:
			if (Param) {
				ADBPlugin *plugin = (ADBPlugin*)hPlugin;
				return plugin->ProcessEventCommand((const wchar_t *)Param, hPlugin);
			}
			break;
		default:
			;
	}
	return 0;
}

SHAREDSYMBOL int WINAPI SetDirectoryW(HANDLE hPlugin, const wchar_t *Dir, int OpMode)
{
	DBG("SetDirectoryW called: hPlugin=%p, Dir=%ls, OpMode=0x%x\n", hPlugin, Dir, OpMode);
	if (!hPlugin || hPlugin == INVALID_HANDLE_VALUE) {
		DBG("SetDirectoryW: Invalid handle\n");
		return 0;
	}
	ADBPlugin *plugin = (ADBPlugin*)hPlugin;
	int result = plugin->SetDirectory(Dir, OpMode);
	DBG("SetDirectoryW: plugin->SetDirectory returned %d\n", result);
	return result;
}

SHAREDSYMBOL int WINAPI MakeDirectoryW(HANDLE hPlugin, const wchar_t **Name, int OpMode)
{
	DBG("MakeDirectoryW called: hPlugin=%p, Name=%p, OpMode=0x%x\n", hPlugin, Name, OpMode);
	if (!hPlugin || hPlugin == INVALID_HANDLE_VALUE) {
		return 0;
	}
	ADBPlugin *plugin = (ADBPlugin*)hPlugin;
	return plugin->MakeDirectory(Name, OpMode);
}

SHAREDSYMBOL int WINAPI DeleteFilesW(HANDLE hPlugin, PluginPanelItem *PanelItem, int ItemsNumber, int OpMode)
{
	DBG("DeleteFilesW called: hPlugin=%p, ItemsNumber=%d, OpMode=0x%x\n", hPlugin, ItemsNumber, OpMode);
	if (!hPlugin) {
		return FALSE;
	}
	ADBPlugin *plugin = static_cast<ADBPlugin*>(hPlugin);
	return plugin->DeleteFiles(PanelItem, ItemsNumber, OpMode);
}

SHAREDSYMBOL int WINAPI GetFilesW(HANDLE hPlugin, PluginPanelItem *PanelItem, int ItemsNumber, int Move, const wchar_t **DestPath, int OpMode)
{
	DBG("GetFilesW called: hPlugin=%p, ItemsNumber=%d, Move=%d, DestPath=%p, OpMode=0x%x\n", hPlugin, ItemsNumber, Move, DestPath, OpMode);
	if (!hPlugin || hPlugin == INVALID_HANDLE_VALUE) {
		return 0;
	}
	ADBPlugin *plugin = (ADBPlugin*)hPlugin;
	int result = plugin->GetFiles(PanelItem, ItemsNumber, Move, DestPath, OpMode);
	return result;
}

SHAREDSYMBOL HANDLE WINAPI OpenFilePluginW(const wchar_t *Name, const unsigned char *Data, int DataSize, int OpMode)
{
	DBG("OpenFilePluginW called: Name=%ls, DataSize=%d, OpMode=0x%x\n", Name, DataSize, OpMode);
	return INVALID_HANDLE_VALUE;
}

SHAREDSYMBOL int WINAPI PutFilesW(HANDLE hPlugin, PluginPanelItem *PanelItem, int ItemsNumber, int Move, const wchar_t *SrcPath, int OpMode)
{
	DBG("PutFilesW called: hPlugin=%p, ItemsNumber=%d, Move=%d, SrcPath=%ls, OpMode=0x%x\n", hPlugin, ItemsNumber, Move, SrcPath, OpMode);
	if (!hPlugin || hPlugin == INVALID_HANDLE_VALUE) {
		return 0;
	}
	ADBPlugin *plugin = (ADBPlugin*)hPlugin;
	return plugin->PutFiles(PanelItem, ItemsNumber, Move, SrcPath, OpMode);
}

SHAREDSYMBOL int WINAPI ProcessHostFileW(HANDLE hPlugin, PluginPanelItem *PanelItem, int ItemsNumber, int OpMode)
{
	DBG("ProcessHostFileW called: hPlugin=%p, ItemsNumber=%d, OpMode=0x%x\n", hPlugin, ItemsNumber, OpMode);
	if (!hPlugin || hPlugin == INVALID_HANDLE_VALUE) {
		return 0;
	}
	ADBPlugin *plugin = (ADBPlugin*)hPlugin;
	return plugin->ProcessHostFile(PanelItem, ItemsNumber, OpMode);
}

SHAREDSYMBOL int WINAPI GetLinkTargetW(HANDLE hPlugin, PluginPanelItem *PanelItem, wchar_t *Target, size_t TargetSize, int OpMode)
{
	DBG("GetLinkTargetW called: hPlugin=%p, TargetSize=%zu, OpMode=0x%x\n", hPlugin, TargetSize, OpMode);
	return 0;
}

SHAREDSYMBOL int WINAPI ProcessEventCommandW(HANDLE hPlugin, const wchar_t *cmd)
{
	DBG("ProcessEventCommandW called: hPlugin=%p, cmd=%ls\n", hPlugin, cmd);
	if (!hPlugin || hPlugin == INVALID_HANDLE_VALUE) {
		return FALSE;
	}
	ADBPlugin *plugin = (ADBPlugin*)hPlugin;
	return plugin->ProcessEventCommand(cmd, hPlugin);
}

SHAREDSYMBOL int WINAPI ExecuteW(HANDLE hPlugin, PluginPanelItem *PanelItem, int ItemsNumber, int OpMode)
{
	DBG("ExecuteW called: hPlugin=%p, ItemsNumber=%d, OpMode=0x%x\n", hPlugin, ItemsNumber, OpMode);
	return 0;
}

SHAREDSYMBOL int WINAPI ConfigureW(int ItemNumber)
{
	DBG("ConfigureW called: ItemNumber=%d\n", ItemNumber);
	return 0;
}

SHAREDSYMBOL void WINAPI ExitFARW()
{
	DBG("ExitFARW called\n");
}

SHAREDSYMBOL int WINAPI MayExitFARW()
{
	DBG("MayExitFARW called\n");
	return 1;
}

} // extern "C"
