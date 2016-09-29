
#include "gvfsdlg.h"
#include "MountOptions.h"
#include "LngStringIDs.h"

#include "plugin.hpp"

#include <string>
#include <vector>

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
        if(item.lngIdx > 0)
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

MountPoint GetLoginData(PluginStartupInfo &info)
{
    const int DIALOG_WIDTH = 78;
    const int DIALOG_HEIGHT = 22;

    std::vector<InitDialogItem> initItems = {
        {DI_DOUBLEBOX,3,1,72,8,0,0,0,0,MConfigTitle, L""},
        {DI_CHECKBOX,5,2,0,2,0,0,0,0,MConfigAddToDisksMenu, L""},
        {DI_FIXEDIT,7,3,7,3,1,0,0,0,0, L""},
        {DI_TEXT,9,3,0,3,0,0,0,0,MConfigDisksMenuDigit, L""},
        {DI_TEXT,5,4,0,4,0,0,DIF_BOXCOLOR|DIF_SEPARATOR,0,0, L""},
        {DI_CHECKBOX,5,5,0,5,0,0,0,0,MConfigCommonPanel, L""},
        {DI_TEXT,5,6,0,6,0,0,DIF_BOXCOLOR|DIF_SEPARATOR,0,0, L""},
        {DI_BUTTON,0,7,0,7,0,0,DIF_CENTERGROUP,1,MOk, L""},
        {DI_BUTTON,0,7,0,7,0,0,DIF_CENTERGROUP,0,MCancel, L""}
    };
    auto dialogItems = InitDialogItems(info, initItems);

    HANDLE hDlg=info.DialogInit(info.ModuleNumber,-1,-1,DIALOG_WIDTH,DIALOG_HEIGHT,
                        L"Config",dialogItems.data(),dialogItems.size(),0,0,NULL,0);

    int ret = info.DialogRun(hDlg);

    // TODO: fill mount based on user input
    return MountPoint();
}
