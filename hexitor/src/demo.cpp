#include "fardialogbuilder.h"

using namespace fardialogbuilder;

class Dialog1 : public Dialog {
public:
    Dialog1(PluginStartupInfo *psi_)
    : Dialog(psi_, L"Replace", L"replace", 0) {
        DlgTEXT t1(L"Replace:");
        DlgHLine t2;
        DlgBUTTON b1(L"&Replace", DIF_CENTERGROUP, 1);
        DlgBUTTON b2(L"&All", DIF_CENTERGROUP);
        DlgBUTTON b3(L"&Skip", DIF_CENTERGROUP);
        DlgBUTTON b4(L"&Cancel", DIF_CENTERGROUP, 0, 1);

        std::vector<Window*> hcontrols = { &b1, &b2, &b3, &b4 };
        DlgHSizer hsizer(hcontrols);

        std::vector<Window*> vcontrols = { &t1, &t2, &hsizer };
        DlgVSizer contents(vcontrols);

        buildFDI(&contents);
        show();
    }

    LONG_PTR WINAPI dlgproc(
        HANDLE   hDlg,
        int      Msg,
        int      Param1,
        LONG_PTR Param2
    ) {
        return (LONG_PTR)0;
    }
};

int main() {
    PluginStartupInfo *psi = NULL;

    Dialog1 dlg(psi);
    return 0;
}
