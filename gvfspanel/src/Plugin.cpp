
#include "Plugin.h"

#include "LngStringIDs.h"

Plugin& Plugin::getInstance()
{
    static Plugin instance;
    return instance;
}

Plugin::Plugin()
{

}

int Plugin::getVersion()
{

}

void Plugin::setStartupInfo(const PluginStartupInfo *psi)
{

}

void Plugin::getPluginInfo(PluginInfo *pi)
{
    pi->StructSize=sizeof(PluginInfo);
    pi->Flags=PF_PRELOAD;
    static const wchar_t *DiskMenuStrings[1];
    DiskMenuStrings[0]=pi->GetMsg(pi->ModuleNumber, MDiskMenuString);
    Info->DiskMenuStrings=DiskMenuStrings;
  #ifndef UNICODE
    static int DiskMenuNumbers[1];
    DiskMenuNumbers[0]=FSF.atoi(Opt.DisksMenuDigit);
    Info->DiskMenuNumbers=DiskMenuNumbers;
  #endif
    Info->DiskMenuStringsNumber=Opt.AddToDisksMenu?ARRAYSIZE(DiskMenuStrings):0;
    static const TCHAR *PluginMenuStrings[1];
    PluginMenuStrings[0]=GetMsg(MTempPanel);
    Info->PluginMenuStrings=Opt.AddToPluginsMenu?PluginMenuStrings:NULL;
    Info->PluginMenuStringsNumber=Opt.AddToPluginsMenu?ARRAYSIZE(PluginMenuStrings):0;
    static const TCHAR *PluginCfgStrings[1];
    PluginCfgStrings[0]=GetMsg(MTempPanel);
    Info->PluginConfigStrings=PluginCfgStrings;
    Info->PluginConfigStringsNumber=ARRAYSIZE(PluginCfgStrings);
    Info->CommandPrefix=Opt.Prefix;
}

HANDLE Plugin::openPlugin(int openFrom, intptr_t item)
{

}

void Plugin::closePlugin(HANDLE Plugin)
{

}

int Plugin::getOpenPluginInfo(HANDLE Plugin, OpenPluginInfo *pluginInfo)
{

}

int Plugin::getFindData(HANDLE Plugin, PluginPanelItem **PanelItem, int *itemsNumber, int OpMode)
{

}

int Plugin::freeFindData(HANDLE Plugin, PluginPanelItem *PanelItem, int itemsNumber)
{

}

int Plugin::processHostFile(HANDLE Plugin, PluginPanelItem *PanelItem, int ItemsNumber, int OpMode)
{

}

int Plugin::processKey(HANDLE Plugin, int key, unsigned int controlState)
{

}

int Plugin::rocessEvent(HANDLE Plugin, int Event, void *Param)
{

}

int Plugin::setDirectory(HANDLE Plugin, const wchar_t *Dir, int OpMode)
{

}

int Plugin::makeDirectory(HANDLE Plugin, const wchar_t **Name, int OpMode)
{

}

int Plugin::deleteFiles(HANDLE Plugin, PluginPanelItem *PanelItem, int itemsNumber, int OpMode)
{

}

int Plugin::getFiles(HANDLE Plugin, PluginPanelItem *PanelItem, int itemsNumber, int Move, const wchar_t **destPath, int OpMode)
{

}

int Plugin::putFiles(HANDLE Plugin, PluginPanelItem *PanelItem, int itemsNumber, int Move, const wchar_t *SrcPath, int OpMode)
{

}

int Plugin::processEditorEvent(int Event, void *Param)
{

}

int Plugin::processEditorInput(const INPUT_RECORD *Rec)
{

}
