#include "FARPlugin.h"
#include "MTPPlugin.h"
#include "MTPLog.h"
#include "lng.h"

#include <algorithm>
#include <cstring>
#include <WideMB.h>

extern "C" {

SHAREDSYMBOL int WINAPI GetMinFarVersionW(void)
{
    return MAKEFARVERSION(2, 0);
}

SHAREDSYMBOL void WINAPI SetStartupInfoW(const PluginStartupInfo* Info)
{
    if (!Info) {
        return;
    }

    MTPLog_Init(Info->ModuleName);
    memcpy(&g_Info, Info, std::min((size_t)Info->StructSize, sizeof(PluginStartupInfo)));
    if (Info->FSF) {
        g_FSF = *(Info->FSF);
        g_Info.FSF = &g_FSF;
    }
}

SHAREDSYMBOL void WINAPI GetPluginInfoW(PluginInfo* Info)
{
    if (!Info) {
        return;
    }

    Info->StructSize = sizeof(*Info);
    Info->SysID = 0x4D545050;  // 'MTPP'
    Info->Flags = PF_FULLCMDLINE;

    // Disk-menu only (Alt+F1/F2); no F9 entry since plugin has no settings.
    static const wchar_t* s_disk_menu_strings[] = {L"MTP"};
    Info->DiskMenuStrings = s_disk_menu_strings;
    Info->DiskMenuStringsNumber = 1;

    static const wchar_t* s_menu_strings[1];
    s_menu_strings[0] = Lng(MPluginTitle);
    static const wchar_t* s_command_prefix = L"mtp";

    Info->PluginMenuStrings = s_menu_strings;
    Info->PluginMenuStringsNumber = 1;
    Info->PluginConfigStrings = nullptr;
    Info->PluginConfigStringsNumber = 0;
    Info->CommandPrefix = s_command_prefix;
}

SHAREDSYMBOL HANDLE WINAPI OpenPluginW(int, INT_PTR)
{
    DBG("OpenPluginW build_marker=mtp_dbg_v2\n");
    try {
        return reinterpret_cast<HANDLE>(new MTPPlugin());
    } catch (...) {
        return INVALID_HANDLE_VALUE;
    }
}

SHAREDSYMBOL void WINAPI ClosePluginW(HANDLE hPlugin)
{
    DBG("ClosePluginW hPlugin=%p\n", hPlugin);
    if (!hPlugin || hPlugin == INVALID_HANDLE_VALUE) {
        return;
    }
    delete reinterpret_cast<MTPPlugin*>(hPlugin);
}

SHAREDSYMBOL int WINAPI GetFindDataW(HANDLE hPlugin, PluginPanelItem** pPanelItem, int* pItemsNumber, int OpMode)
{
    DBG("GetFindDataW hPlugin=%p opmode=0x%x\n", hPlugin, OpMode);
    if (!hPlugin || hPlugin == INVALID_HANDLE_VALUE) {
        return FALSE;
    }
    return reinterpret_cast<MTPPlugin*>(hPlugin)->GetFindData(pPanelItem, pItemsNumber, OpMode);
}

SHAREDSYMBOL void WINAPI FreeFindDataW(HANDLE hPlugin, PluginPanelItem* PanelItem, int ItemsNumber)
{
    if (!hPlugin || hPlugin == INVALID_HANDLE_VALUE) {
        return;
    }
    reinterpret_cast<MTPPlugin*>(hPlugin)->FreeFindData(PanelItem, ItemsNumber);
}

SHAREDSYMBOL void WINAPI GetOpenPluginInfoW(HANDLE hPlugin, OpenPluginInfo* Info)
{
    if (!hPlugin || hPlugin == INVALID_HANDLE_VALUE) {
        return;
    }
    reinterpret_cast<MTPPlugin*>(hPlugin)->GetOpenPluginInfo(Info);
}

SHAREDSYMBOL int WINAPI ProcessKeyW(HANDLE hPlugin, int Key, unsigned int ControlState)
{
    DBG("ProcessKeyW hPlugin=%p key=%d ctrl=0x%x\n", hPlugin, Key, ControlState);
    if (!hPlugin || hPlugin == INVALID_HANDLE_VALUE) {
        return FALSE;
    }
    return reinterpret_cast<MTPPlugin*>(hPlugin)->ProcessKey(Key, ControlState);
}

SHAREDSYMBOL int WINAPI ProcessEventW(HANDLE hPlugin, int Event, void* Param)
{
    if (!hPlugin || hPlugin == INVALID_HANDLE_VALUE) {
        return FALSE;
    }
    return reinterpret_cast<MTPPlugin*>(hPlugin)->ProcessEvent(Event, Param);
}

SHAREDSYMBOL int WINAPI SetDirectoryW(HANDLE hPlugin, const wchar_t* Dir, int OpMode)
{
    DBG("SetDirectoryW hPlugin=%p dir=%s opmode=0x%x\n", hPlugin, Dir ? StrWide2MB(Dir).c_str() : "(null)", OpMode);
    if (!hPlugin || hPlugin == INVALID_HANDLE_VALUE) {
        return FALSE;
    }
    return reinterpret_cast<MTPPlugin*>(hPlugin)->SetDirectory(Dir, OpMode);
}

SHAREDSYMBOL int WINAPI MakeDirectoryW(HANDLE hPlugin, const wchar_t** Name, int OpMode)
{
    if (!hPlugin || hPlugin == INVALID_HANDLE_VALUE) {
        return FALSE;
    }
    return reinterpret_cast<MTPPlugin*>(hPlugin)->MakeDirectory(Name, OpMode);
}

SHAREDSYMBOL int WINAPI DeleteFilesW(HANDLE hPlugin, PluginPanelItem* PanelItem, int ItemsNumber, int OpMode)
{
    if (!hPlugin || hPlugin == INVALID_HANDLE_VALUE) {
        return FALSE;
    }
    return reinterpret_cast<MTPPlugin*>(hPlugin)->DeleteFiles(PanelItem, ItemsNumber, OpMode);
}

SHAREDSYMBOL HANDLE WINAPI OpenFilePluginW(const wchar_t*, const unsigned char*, int, int)
{
    return INVALID_HANDLE_VALUE;
}

SHAREDSYMBOL int WINAPI GetLinkTargetW(HANDLE, PluginPanelItem*, wchar_t*, size_t, int)
{
    return FALSE;
}

SHAREDSYMBOL int WINAPI GetFilesW(HANDLE hPlugin, PluginPanelItem* PanelItem, int ItemsNumber, int Move, const wchar_t** DestPath, int OpMode)
{
    if (!hPlugin || hPlugin == INVALID_HANDLE_VALUE) {
        return FALSE;
    }
    return reinterpret_cast<MTPPlugin*>(hPlugin)->GetFiles(PanelItem, ItemsNumber, Move, DestPath, OpMode);
}

SHAREDSYMBOL int WINAPI PutFilesW(HANDLE hPlugin, PluginPanelItem* PanelItem, int ItemsNumber, int Move, const wchar_t* SrcPath, int OpMode)
{
    if (!hPlugin || hPlugin == INVALID_HANDLE_VALUE) {
        return FALSE;
    }
    return reinterpret_cast<MTPPlugin*>(hPlugin)->PutFiles(PanelItem, ItemsNumber, Move, SrcPath, OpMode);
}

SHAREDSYMBOL int WINAPI ExecuteW(HANDLE hPlugin, PluginPanelItem* PanelItem, int ItemsNumber, int OpMode)
{
    if (!hPlugin || hPlugin == INVALID_HANDLE_VALUE) {
        return FALSE;
    }
    return reinterpret_cast<MTPPlugin*>(hPlugin)->Execute(PanelItem, ItemsNumber, OpMode);
}

SHAREDSYMBOL int WINAPI ConfigureW(int)
{
    return FALSE;
}

SHAREDSYMBOL void WINAPI ExitFARW(void)
{
}

SHAREDSYMBOL int WINAPI MayExitFARW(void)
{
    return TRUE;
}

} // extern "C"
