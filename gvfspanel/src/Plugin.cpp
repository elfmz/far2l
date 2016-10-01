
#include "Plugin.h"
#include "gvfsdlg.h"
#include "LngStringIDs.h"

Plugin& Plugin::getInstance()
{
    static Plugin instance;
    return instance;
}

Plugin::Plugin()
{
    Opt.AddToDisksMenu = true;
}

int Plugin::getVersion()
{
    return 1;
}

void Plugin::setStartupInfo(const PluginStartupInfo *psi)
{
    this->m_pPsi = *psi;
}

void Plugin::getPluginInfo(PluginInfo *pi)
{

    pi->StructSize=sizeof(PluginStartupInfo);
    pi->Flags=PF_PRELOAD;

    static const wchar_t *DiskMenuStrings[1];
    DiskMenuStrings[0] = m_pPsi.GetMsg(m_pPsi.ModuleNumber, MDiskMenuString);
    pi->DiskMenuStrings = DiskMenuStrings;
    pi->DiskMenuStringsNumber = Opt.AddToDisksMenu ? ARRAYSIZE(DiskMenuStrings) : 0;

    static const TCHAR *PluginMenuStrings[1];
    PluginMenuStrings[0] = m_pPsi.GetMsg(m_pPsi.ModuleNumber, MGvfsPanel);
    pi->PluginMenuStrings = Opt.AddToPluginsMenu?PluginMenuStrings:NULL;
    pi->PluginMenuStringsNumber = Opt.AddToPluginsMenu ? ARRAYSIZE(PluginMenuStrings) : 0;

    static const TCHAR *PluginCfgStrings[1];
    PluginCfgStrings[0] = m_pPsi.GetMsg(m_pPsi.ModuleNumber, MGvfsPanel);
    pi->PluginConfigStrings = PluginCfgStrings;
    pi->PluginConfigStringsNumber = ARRAYSIZE(PluginCfgStrings);
    pi->CommandPrefix=Opt.Prefix;
}

HANDLE Plugin::openPlugin(int openFrom, intptr_t item)
{
    return static_cast<HANDLE>(this);
}

void Plugin::closePlugin(HANDLE Plugin)
{

}

void Plugin::getOpenPluginInfo(HANDLE Plugin, OpenPluginInfo *pluginInfo)
{

}

int Plugin::getFindData(HANDLE Plugin, PluginPanelItem **PanelItem, int *itemsNumber, int OpMode)
{
    updatePanelItems();
    *PanelItem = &(m_items[0]);
    *itemsNumber = m_items.size();
    return 1;
}

void Plugin::freeFindData(HANDLE Plugin, PluginPanelItem *PanelItem, int itemsNumber)
{

}

int Plugin::processHostFile(HANDLE Plugin, PluginPanelItem *PanelItem, int ItemsNumber, int OpMode)
{
    return 0;
}

int Plugin::processKey(HANDLE Plugin, int key, unsigned int controlState)
{
    return 0;
}

int Plugin::processEvent(HANDLE Plugin, int Event, void *Param)
{
    return 0;
}

int Plugin::setDirectory(HANDLE Plugin, const wchar_t *Dir, int OpMode)
{
    auto it = m_mountPoints.find(std::wstring(Dir));
    if(it != m_mountPoints.end())
    {
        auto mountPoint = it->second;
        if(!mountPoint.isMounted())
        {
            // switchdirectory to:
            if(! mountPoint.mount())
                return 0;

        }
        // change directory to:
        mountPoint.getFsPath();
    }

    return 0;
}

int Plugin::makeDirectory(HANDLE Plugin, const wchar_t **Name, int OpMode)
{
    auto mountPoint = GetLoginData(this->m_pPsi);
    *Name = mountPoint.m_resPath.c_str();
    m_mountPoints.insert(std::pair<std::wstring, MountPoint>(mountPoint.m_resPath, mountPoint));
    return 1;
}

int Plugin::deleteFiles(HANDLE Plugin, PluginPanelItem *PanelItem, int itemsNumber, int OpMode)
{
    return 0;
}

int Plugin::getFiles(HANDLE Plugin, PluginPanelItem *PanelItem, int itemsNumber, int Move, const wchar_t **destPath, int OpMode)
{
    return 0;
}

int Plugin::putFiles(HANDLE Plugin, PluginPanelItem *PanelItem, int itemsNumber, int Move, const wchar_t *SrcPath, int OpMode)
{
    return 0;
}

int Plugin::processEditorEvent(int Event, void *Param)
{
    return 0;
}

int Plugin::processEditorInput(const INPUT_RECORD *Rec)
{
    return 0;
}

void Plugin::updatePanelItems()
{
    m_items.clear();
    for(const auto& mountPt : m_mountPoints)
    {
        PluginPanelItem item { };
        // dangerous stuff. if m_resPAth or mountPt changes (reallocation) it will crash far
        // but for test purposes enough
        item.FindData.lpwszFileName = mountPt.second.m_resPath.c_str();
        item.FindData.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_ARCHIVE;
        item.CustomColumnNumber = 0;
        m_items.push_back(item);
    }
}

