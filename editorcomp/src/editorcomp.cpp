#include "editorcomp.h"
#include "Editor.h"
#include "Editors.h"

#define BAD_MODIFIERS (RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED | RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED | SHIFT_PRESSED)


using namespace std;

Editors *editors = nullptr;

SHAREDSYMBOL void WINPORT_DllStartup(const char *path) {
    // No operations.
}

const wchar_t *getMsg(PluginStartupInfo &info, int msgId) {
    return info.GetMsg(info.ModuleNumber, msgId);
}

const wchar_t *title[1];

SHAREDSYMBOL void WINAPI SetStartupInfoW(const struct PluginStartupInfo *Info) {
    editors = new Editors(*Info, *Info->FSF);
    title[0] = getMsg(editors->getInfo(), 0);
}

SHAREDSYMBOL void WINAPI GetPluginInfoW(struct PluginInfo *Info) {
    memset(Info, 0, sizeof(*Info));
    Info->StructSize = sizeof(*Info);
    Info->Flags = PF_EDITOR | PF_DISABLEPANELS;

    Info->PluginConfigStringsNumber = 1;
    Info->PluginConfigStrings = title;
    Info->PluginMenuStringsNumber = 1;
    Info->PluginMenuStrings = title;
};

SHAREDSYMBOL int WINAPI ConfigureW(int ItemNumber) {
    PluginStartupInfo &info = editors->getInfo();

    int w = 50;
    int h = 10;

    struct FarDialogItem fdi[] = {
            {DI_DOUBLEBOX, 1,  1, w - 2, h - 2, 0,     {}, 0, 0,    getMsg(info, 0), 0},
            {DI_TEXT,      3,  2, 0,     h - 1, FALSE, {}, 0, 0,    getMsg(info, 1), 0},
            {DI_CHECKBOX,  3,  4, 0,     0,     TRUE,  {}, 0, 0,    getMsg(info, 2), 0},
            {DI_EDIT,      3,  5, w - 4, 0,     0,  {},  0, 0, nullptr, 0},
            {DI_SINGLEBOX, 2,  6, 0,     0,     FALSE, {}, 0, 0,    L"", 0},
            {DI_BUTTON,    11, 7, 0,     0,     FALSE, {}, 0, TRUE, getMsg(info, 3), 0},
            {DI_BUTTON,    26, 7, 0,     0,     FALSE, {}, 0, 0,    getMsg(info, 4), 0}
    };

    unsigned int size = sizeof(fdi) / sizeof(fdi[0]);
    fdi[2].Param.Selected = editors->getAutoEnabling();
    fdi[3].PtrData = (const TCHAR*)editors->getFileMasks().c_str();

    HANDLE hDlg = info.DialogInit(info.ModuleNumber, -1, -1, w, h, L"config", fdi, size, 0, 0, nullptr, 0);

    int runResult = info.DialogRun(hDlg);
    if (runResult == int(size) - 2) {
        editors->setAutoEnabling(info.SendDlgMessage(hDlg, DM_GETCHECK, 2, 0) == BSTATE_CHECKED);
        editors->setFileMasks(((const TCHAR *)info.SendDlgMessage(hDlg,DM_GETCONSTTEXTPTR, 3, 0)));
    }

    info.DialogFree(hDlg);
    return 1;
}

SHAREDSYMBOL HANDLE WINAPI OpenPluginW(int OpenFrom, INT_PTR Item) {
    Editor *editor = editors->getEditor();
    if (!editor)
        return INVALID_HANDLE_VALUE;

    PluginStartupInfo &info = editors->getInfo();

    int w = 50;
    int h = 8;

    struct FarDialogItem fdi[] = {
            {DI_DOUBLEBOX, 1,  1, w - 2, h - 2, 0,     {}, 0, 0,    getMsg(info, 0), 0},
            {DI_TEXT,      3,  2, 0,     h - 1, FALSE, {}, 0, 0,    getMsg(info, 1), 0},
            {DI_CHECKBOX,  3,  4, 0,     0,     TRUE,  {}, 0, 0,    getMsg(info, 5), 0},
            {DI_SINGLEBOX, 2,  5, 0,     0,     FALSE, {}, 0, 0,    L"", 0},
            {DI_BUTTON,    11, 6, 0,     0,     FALSE, {}, 0, TRUE, getMsg(info, 3), 0},
            {DI_BUTTON,    26, 6, 0,     0,     FALSE, {}, 0, 0,    getMsg(info, 4), 0}
    };

    bool active = editor->getEnabled();
    unsigned int size = sizeof(fdi) / sizeof(fdi[0]);
    fdi[2].Param.Selected = active;

    HANDLE hDlg = info.DialogInit(info.ModuleNumber, -1, -1, w, h, L"config", fdi, size, 0, 0, nullptr, 0);

    int runResult = info.DialogRun(hDlg);
    if (runResult == int(size) - 2) {
        bool checked = (info.SendDlgMessage(hDlg, DM_GETCHECK, 2, 0) == BSTATE_CHECKED);
        if (checked != active) {
            editor->setEnabled(checked);
        }
    }

    info.DialogFree(hDlg);
    return INVALID_HANDLE_VALUE;
};

SHAREDSYMBOL int WINAPI ProcessEditorEventW(int Event, void *Param) {
    if (Event == EE_CLOSE) {
        Editor *editor = editors->getEditor(Param);
        editors->remove(editor);
    } else {
        Editor *editor = editors->getEditor();

        if (editor->getEnabled()) {
            if (Event == EE_READ) {
                //
            } else {
                if (Event == EE_REDRAW) {
                    editor->updateWords();
                    editor->processSuggestion();
                } else if (Event == EE_SAVE) {
                    //?
                }
            }
        }
    }

    return 0;
}

SHAREDSYMBOL int WINAPI ProcessEditorInputW(const INPUT_RECORD *ir) {
    if (ir->EventType == KEY_EVENT && (ir->Event.KeyEvent.dwControlKeyState & BAD_MODIFIERS) == 0
        && ir->Event.KeyEvent.wVirtualScanCode == 0
        && !ir->Event.KeyEvent.bKeyDown
        && ir->Event.KeyEvent.wVirtualKeyCode == 0)
        return 0;

    Editor *editor = editors->getEditor();
    if (!editor->getEnabled())
        return 0;

    // Is regular key event?
    if (ir->EventType == KEY_EVENT && (ir->Event.KeyEvent.dwControlKeyState & BAD_MODIFIERS) == 0
        && ir->Event.KeyEvent.wVirtualScanCode == 0 && ir->Event.KeyEvent.bKeyDown) {

        // Tab ?
        if (editor->getState() == DO_ACTION && ir->Event.KeyEvent.wVirtualKeyCode == VK_TAB) {
            editor->confirmSuggestion();
            return 1;
        }

        // Escape ?
        if (editor->getState() == DO_ACTION &&
            (ir->Event.KeyEvent.wVirtualKeyCode == 27 || ir->Event.KeyEvent.wVirtualKeyCode == 46)) {
            bool has_suggestion = editor->getSuggestionLength() > 0;
            editor->declineSuggestion();
            return has_suggestion;
        }

    }

    if (ir->EventType == KEY_EVENT && ir->Event.KeyEvent.bKeyDown
        && ir->Event.KeyEvent.wVirtualKeyCode != VK_CONTROL
        && ir->Event.KeyEvent.wVirtualKeyCode != VK_SHIFT
        && ir->Event.KeyEvent.wVirtualKeyCode != VK_MENU)
        editor->declineSuggestion();


    if (ir->EventType == MOUSE_EVENT && ir->Event.MouseEvent.dwButtonState)
        editor->declineSuggestion();

    // Is regular key event?
    if (ir->EventType == KEY_EVENT && (ir->Event.KeyEvent.dwControlKeyState & BAD_MODIFIERS) == 0
        && ir->Event.KeyEvent.wVirtualScanCode == 0 && !ir->Event.KeyEvent.bKeyDown
        && ir->Event.KeyEvent.wVirtualKeyCode != 9 && ir->Event.KeyEvent.wVirtualKeyCode != 8
        && ir->Event.KeyEvent.wVirtualKeyCode != 27 && ir->Event.KeyEvent.wVirtualKeyCode != 46)
        editors->getEditor()->on();

    return 0;
}

SHAREDSYMBOL void WINAPI ExitFARW() {
    if (editors != nullptr) {
        delete editors;
        editors = nullptr;
    }
}
