
#include "gvfsdlg.h"
#include "MountOptions.h"
#include "LngStringIDs.h"

#include "plugin.hpp"

#include <string>
#include <vector>
#include <iostream>

std::vector<FarDialogItem> InitDialogItems(PluginStartupInfo &info,
        const std::vector<InitDialogItem> &Init)
{
    std::vector<FarDialogItem> dlgItems;
    for (const auto &item : Init)
    {
        FarDialogItem farDlgItem;
        farDlgItem.Type = item.Type;
        farDlgItem.X1 = item.X1;
        farDlgItem.Y1 = item.Y1;
        farDlgItem.X2 = item.X2;
        farDlgItem.Y2 = item.Y2;
        farDlgItem.Focus = item.Focus;
        farDlgItem.Reserved = item.Selected;
        farDlgItem.Flags = item.Flags;
        farDlgItem.DefaultButton = item.DefaultButton;

        // TODO: refactor
        if(item.lngIdx >= 0 && item.lngIdx < __LAST_LNG_ENTRY__)
        {
            farDlgItem.PtrData = info.GetMsg(info.ModuleNumber, (unsigned int)item.lngIdx);
        }
        else
        {
            farDlgItem.PtrData = item.text.c_str();
        }
        dlgItems.emplace_back(farDlgItem);
    }
    return dlgItems;
}

bool GetLoginData(PluginStartupInfo &info, MountPoint& mountPoint)
{
    const int DIALOG_WIDTH = 78;
    const int DIALOG_HEIGHT = 13;

    std::vector<InitDialogItem> initItems = {
        {DI_DOUBLEBOX,3,1,DIALOG_WIDTH-3,DIALOG_HEIGHT-2,0,0,0,0,
                MConfigTitle, L""},
        {DI_TEXT,5,2,0,2,0,0,0,0,
                -1, L"Resource path (smb://, scp://, webdav:// ..."},
        {DI_FIXEDIT,7,3,DIALOG_WIDTH-6,3,1,0,0,0,
                -1,                     mountPoint.m_resPath.c_str()},

        {DI_TEXT,5,4,0,4,0,0,0,0,
                -1, L"login"},
        {DI_FIXEDIT,7,5,DIALOG_WIDTH-6,5,1,0,0,0,
                -1,                     mountPoint.m_user.c_str()},

        {DI_TEXT,5,6,0,6,0,0,0,0,
                -1, L"password"},
        {DI_FIXEDIT,7,7,DIALOG_WIDTH-6,7,1,0,0,0,
                -1,                     mountPoint.m_password.c_str()},


        {DI_CHECKBOX,5,8,0,8,0,0,0,0,
                -1,    L"Add this mount point to disk menu"},

        {DI_BUTTON,0,10,0,10,0,0,DIF_CENTERGROUP,1,
                MOk,                   L""},
        {DI_BUTTON,0,10,0,10,0,0,DIF_CENTERGROUP,0,
                MCancel,               L""}
    };
    auto dialogItems = InitDialogItems(info, initItems);

    HANDLE hDlg=info.DialogInit(info.ModuleNumber,-1,-1,DIALOG_WIDTH,DIALOG_HEIGHT,
                        L"Config",dialogItems.data(), dialogItems.size(),0,0,NULL,0);

    int ret = info.DialogRun(hDlg);
    if(ret == -1 || ret == 9)
    {
        return false;
    }
    mountPoint.m_resPath = reinterpret_cast<const wchar_t*>(info.SendDlgMessage(hDlg,DM_GETCONSTTEXTPTR,2,0));
    mountPoint.m_user = reinterpret_cast<const wchar_t*>(info.SendDlgMessage(hDlg,DM_GETCONSTTEXTPTR,4,0));
    mountPoint.m_password = reinterpret_cast<const wchar_t*>(info.SendDlgMessage(hDlg,DM_GETCONSTTEXTPTR,6,0));

    return true;
}
