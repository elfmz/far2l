
#include "Plugin.h"

#include <windows.h>

#include <string>
#include <algorithm>
#include <vector>

extern "C"
{

int WINAPI GetMinFarVersionW()
{
    return Plugin::getInstance().getVersion();
}

void WINAPI SetStartupInfoW(const struct PluginStartupInfo * psi)
{
    Plugin::getInstance().setStartupInfo(psi);
}

void WINAPI ExitFARW()
{
    Plugin::getInstance().exitFar();
}

void WINAPI GetPluginInfoW(PluginInfo * pi)
{
    Plugin::getInstance().getPluginInfo(pi);
}

int WINAPI ConfigureW(int item)
{
    return Plugin::getInstance().configure(item);
}

HANDLE WINAPI OpenPluginW(int openFrom, intptr_t item)
{
    return Plugin::getInstance().openPlugin(openFrom, item);
}

void WINAPI ClosePluginW(HANDLE Plugin)
{
    Plugin::getInstance().closePlugin(Plugin);
}

void WINAPI GetOpenPluginInfoW(HANDLE Plugin, OpenPluginInfo * pluginInfo)
{
    Plugin::getInstance().getOpenPluginInfo(Plugin, pluginInfo);
}

int WINAPI GetFindDataW(HANDLE Plugin, PluginPanelItem ** PanelItem, int * itemsNumber, int OpMode)
{
    return Plugin::getInstance().getFindData(Plugin, PanelItem, itemsNumber, OpMode);
}

void WINAPI FreeFindDataW(HANDLE Plugin, PluginPanelItem * PanelItem, int itemsNumber)
{
    Plugin::getInstance().freeFindData(Plugin, PanelItem, itemsNumber);
}

int WINAPI ProcessHostFileW(HANDLE Plugin,
                            struct PluginPanelItem * PanelItem, int ItemsNumber, int OpMode)
{
    return Plugin::getInstance().processHostFile(Plugin, PanelItem, ItemsNumber, OpMode);
}

int WINAPI ProcessKeyW(HANDLE Plugin, int key, unsigned int controlState)
{
    return Plugin::getInstance().processKey(Plugin, key, controlState);
}

int WINAPI ProcessEventW(HANDLE Plugin, int Event, void * Param)
{
    return Plugin::getInstance().rocessEvent(Plugin, Event, Param);
}

int WINAPI SetDirectoryW(HANDLE Plugin, const wchar_t * Dir, int OpMode)
{
    return Plugin::getInstance().setDirectory(Plugin, Dir, OpMode);
}

int WINAPI MakeDirectoryW(HANDLE Plugin, const wchar_t ** Name, int OpMode)
{
    return Plugin::getInstance().makeDirectory(Plugin, Name, OpMode);
}

int WINAPI DeleteFilesW(HANDLE Plugin, PluginPanelItem * PanelItem, int itemsNumber, int OpMode)
{
    return Plugin::getInstance().deleteFiles(Plugin, PanelItem, itemsNumber, OpMode);
}

int WINAPI GetFilesW(HANDLE Plugin, PluginPanelItem * PanelItem, int itemsNumber,
                     int Move, const wchar_t ** destPath, int OpMode)
{
    return Plugin::getInstance().getFiles(Plugin, PanelItem, itemsNumber,
                                          Move, destPath, OpMode);
}

int WINAPI PutFilesW(HANDLE Plugin, PluginPanelItem * PanelItem, int itemsNumber, int Move, const wchar_t * SrcPath, int OpMode)
{
    return Plugin::getInstance().putFiles(Plugin, PanelItem, itemsNumber,
                                          Move, SrcPath, OpMode);
}

int WINAPI ProcessEditorEventW(int Event, void * Param)
{
    return Plugin::getInstance().processEditorEvent(Event, Param);
}

int WINAPI ProcessEditorInputW(const INPUT_RECORD * Rec)
{
    return Plugin::getInstance().processEditorInput(Rec);
}

HANDLE WINAPI OpenFilePluginW(const wchar_t * fileName, const uint8_t * fileHeader, int fileHeaderSize, int /*OpMode*/)
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
