
#include "Plugin.h"

#include <windows.h>

#include <string>
#include <algorithm>
#include <vector>

extern "C"
{

SHAREDSYMBOL int WINAPI GetMinFarVersionW()
{
    return Plugin::getInstance().getVersion();
}

SHAREDSYMBOL void WINAPI SetStartupInfoW(const struct PluginStartupInfo * psi)
{
    Plugin::getInstance().setStartupInfo(psi);
}

SHAREDSYMBOL void WINAPI ExitFARW()
{
    Plugin::getInstance().exitFar();
}

SHAREDSYMBOL void WINAPI GetPluginInfoW(PluginInfo * pi)
{
    Plugin::getInstance().getPluginInfo(pi);
}

SHAREDSYMBOL int WINAPI ConfigureW(int item)
{
    return Plugin::getInstance().configure(item);
}

SHAREDSYMBOL HANDLE WINAPI OpenPluginW(int openFrom, intptr_t item)
{
    return Plugin::getInstance().openPlugin(openFrom, item);
}

SHAREDSYMBOL void WINAPI ClosePluginW(HANDLE Plugin)
{
    Plugin::getInstance().closePlugin(Plugin);
}

SHAREDSYMBOL void WINAPI GetOpenPluginInfoW(HANDLE Plugin, OpenPluginInfo * pluginInfo)
{
    Plugin::getInstance().getOpenPluginInfo(Plugin, pluginInfo);
}

SHAREDSYMBOL int WINAPI GetFindDataW(HANDLE Plugin, PluginPanelItem ** PanelItem, int * itemsNumber, int OpMode)
{
    return Plugin::getInstance().getFindData(Plugin, PanelItem, itemsNumber, OpMode);
}

SHAREDSYMBOL void WINAPI FreeFindDataW(HANDLE Plugin, PluginPanelItem * PanelItem, int itemsNumber)
{
    Plugin::getInstance().freeFindData(Plugin, PanelItem, itemsNumber);
}

SHAREDSYMBOL int WINAPI ProcessHostFileW(HANDLE Plugin,
                            struct PluginPanelItem * PanelItem, int ItemsNumber, int OpMode)
{
    return Plugin::getInstance().processHostFile(Plugin, PanelItem, ItemsNumber, OpMode);
}

SHAREDSYMBOL int WINAPI ProcessKeyW(HANDLE Plugin, int key, unsigned int controlState)
{
    return Plugin::getInstance().processKey(Plugin, key, controlState);
}

SHAREDSYMBOL int WINAPI ProcessEventW(HANDLE Plugin, int Event, void * Param)
{
    return Plugin::getInstance().processEvent(Plugin, Event, Param);
}

SHAREDSYMBOL int WINAPI SetDirectoryW(HANDLE Plugin, const wchar_t * Dir, int OpMode)
{
    return Plugin::getInstance().setDirectory(Plugin, Dir, OpMode);
}

SHAREDSYMBOL int WINAPI MakeDirectoryW(HANDLE Plugin, const wchar_t ** Name, int OpMode)
{
    return Plugin::getInstance().makeDirectory(Plugin, Name, OpMode);
}

SHAREDSYMBOL int WINAPI DeleteFilesW(HANDLE Plugin, PluginPanelItem * PanelItem, int itemsNumber, int OpMode)
{
    return Plugin::getInstance().deleteFiles(Plugin, PanelItem, itemsNumber, OpMode);
}

SHAREDSYMBOL int WINAPI GetFilesW(HANDLE Plugin, PluginPanelItem * PanelItem, int itemsNumber,
                     int Move, const wchar_t ** destPath, int OpMode)
{
    return Plugin::getInstance().getFiles(Plugin, PanelItem, itemsNumber,
                                          Move, destPath, OpMode);
}

SHAREDSYMBOL int WINAPI PutFilesW(HANDLE Plugin, PluginPanelItem * PanelItem, int itemsNumber, int Move, const wchar_t * SrcPath, int OpMode)
{
    return Plugin::getInstance().putFiles(Plugin, PanelItem, itemsNumber,
                                          Move, SrcPath, OpMode);
}

SHAREDSYMBOL int WINAPI ProcessEditorEventW(int Event, void * Param)
{
    return Plugin::getInstance().processEditorEvent(Event, Param);
}

SHAREDSYMBOL int WINAPI ProcessEditorInputW(const INPUT_RECORD * Rec)
{
    return Plugin::getInstance().processEditorInput(Rec);
}

SHAREDSYMBOL HANDLE WINAPI OpenFilePluginW(const wchar_t * fileName, const uint8_t * fileHeader, int fileHeaderSize, int /*OpMode*/)
{
    if (fileName == nullptr)
    {
        return INVALID_HANDLE_VALUE;
    }
    std::wstring name(fileName);
    std::wstring ext(L".gvfsmounts");
    if(!std::equal(name.rbegin(), name.rbegin()+ext.size(), ext.rbegin(), ext.rend()))
    {
        return INVALID_HANDLE_VALUE;
    }
    std::vector<uint8_t> fHdr(fileHeader, fileHeader+fileHeaderSize);
    std::vector<uint8_t> xmlHdr{'<', '?', 'x', 'm', 'l'};
    if (!std::equal(xmlHdr.begin(), xmlHdr.end(), fHdr.begin(), fHdr.begin() + xmlHdr.size()))
    {
        return INVALID_HANDLE_VALUE;
    }
    return Plugin::getInstance().openPlugin(OPEN_ANALYSE,
                                            reinterpret_cast<intptr_t>(fileName));
}

__attribute__((constructor)) void so_init(void)
{
    //  FarPlugin = CreateFarPlugin(0);
}

} // extern "C"
