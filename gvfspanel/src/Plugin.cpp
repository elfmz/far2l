
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

    static const wchar_t *PluginMenuStrings[1];
    PluginMenuStrings[0] = m_pPsi.GetMsg(m_pPsi.ModuleNumber, MGvfsPanel);
    pi->PluginMenuStrings = Opt.AddToPluginsMenu?PluginMenuStrings:NULL;
    pi->PluginMenuStringsNumber = Opt.AddToPluginsMenu ? ARRAYSIZE(PluginMenuStrings) : 0;

    static const wchar_t *PluginCfgStrings[1];
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
    switch(key)
    {
    case VK_F3:
    case VK_F5:
    case VK_F6:
        // block keys
        return 1;
    case VK_F4:
    {
        struct PanelInfo pInfo;
        m_pPsi.Control(Plugin, FCTL_GETPANELINFO, 0, (LONG_PTR)&pInfo);
        auto currentItem = pInfo.CurrentItem;
        PluginPanelItem item = m_items[currentItem];
        std::wstring name = item.FindData.lpwszFileName;
        auto it = m_mountPoints.find(name);
        if(it != m_mountPoints.end())
        {
            if(GetLoginData(m_pPsi, it->second))
            {
                MountPoint changedMountPt = it->second;
                m_mountPoints.erase(it);
                m_mountPoints.insert(
                            std::pair<std::wstring, MountPoint>
                            (changedMountPt.m_resPath, changedMountPt));
                m_pPsi.Control(Plugin, FCTL_UPDATEPANEL, 0, nullptr);

            }
        }
        return 1; // return 1: far should not handel this key
    }
    default:
        return 0; // all other keys left to far
    }
}

int Plugin::processEvent(HANDLE Plugin, int Event, void *Param)
{
    return 0;
}

int Plugin::setDirectory(HANDLE Plugin, const wchar_t *Dir, int OpMode)
{
    if(OpMode == 0)
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
    }

    return 0;
}

int Plugin::makeDirectory(HANDLE Plugin, const wchar_t **Name, int OpMode)
{
    MountPoint mountPoint;
    if(GetLoginData(this->m_pPsi, mountPoint))
    {
        m_mountPoints.insert(std::pair<std::wstring, MountPoint>(mountPoint.m_resPath, mountPoint));
        return 1;
    }
    return 0;
}

int Plugin::deleteFiles(HANDLE Plugin, PluginPanelItem *PanelItem, int itemsNumber, int OpMode)
{
    const unsigned N = 2;
    const wchar_t *msgItems[N] =
    {
        L"Delete selected mounts",
        L"Do you really want to delete mount point?"
    };

    auto msgCode = m_pPsi.Message(m_pPsi.ModuleNumber, FMSG_WARNING | FMSG_MB_YESNO, NULL, msgItems, N, 0);
    // if no or canceled msg box, do nothing
    if(msgCode == -1 || msgCode == 1)
    {
        return -1;
    }

    for(int i = 0; i < itemsNumber; ++i)
    {
        auto name = PanelItem[i].FindData.lpwszFileName;
        auto it = m_mountPoints.find(name);
        if(it != m_mountPoints.end())
        {
            m_mountPoints.erase(it);
        }
    }
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

