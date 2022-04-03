#include "editorcomp.h"
#include "Editor.h"
#include "Editors.h"

#define RELEVANT_MODIFIERS (RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED | RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED | SHIFT_PRESSED)


using namespace std;

Editors *editors = nullptr;

SHAREDSYMBOL void PluginModuleOpen(const char *path)
{
    // No operations.
}

const wchar_t *title[1];

SHAREDSYMBOL void WINAPI SetStartupInfoW(const struct PluginStartupInfo *Info) {
    editors = new Editors(*Info, *Info->FSF);
    title[0] = wcsdup(editors->getSettings()->getMsg(M_TITLE));
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
    editors->getSettings()->configurationMenuDialog();
    return 1;
}

SHAREDSYMBOL HANDLE WINAPI OpenPluginW(int OpenFrom, INT_PTR Item) {
    Editor *editor = editors->getEditor();
    if (!editor)
        return INVALID_HANDLE_VALUE;

    bool enabled = editor->getEnabled();
    editors->getSettings()->editorMenuDialog(enabled);
    editor->setEnabled(enabled);
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
    if (ir->EventType == KEY_EVENT && (ir->Event.KeyEvent.dwControlKeyState & RELEVANT_MODIFIERS) == 0
        && ir->Event.KeyEvent.wVirtualScanCode == 0
        && !ir->Event.KeyEvent.bKeyDown
        && ir->Event.KeyEvent.wVirtualKeyCode == 0) {
        return 0;
    }

    if (ir->EventType == MOUSE_EVENT && !ir->Event.MouseEvent.dwButtonState) {
        return 0;
    }

    Editor *editor = editors->getEditor();
    if (!editor->getEnabled()) {
        return 0;
    }

    if (ir->EventType == MOUSE_EVENT) {
        if (ir->Event.MouseEvent.dwButtonState) {
            editor->declineSuggestion();
        }

    } else if (ir->EventType == KEY_EVENT) { // Is regular key event?
        DWORD relevant_modifiers = (ir->Event.KeyEvent.dwControlKeyState & RELEVANT_MODIFIERS);

        if (ir->Event.KeyEvent.bKeyDown && ir->Event.KeyEvent.wVirtualScanCode == 0) {
            // Tab or Shift+Tab ?
            if (ir->Event.KeyEvent.wVirtualKeyCode == VK_TAB && editor->getState() == DO_ACTION) {
                switch (relevant_modifiers) {
                    case 0:
                        editor->confirmSuggestion();
                        return 1;

                    case SHIFT_PRESSED:
                        editor->toggleSuggestion();
                        return 1;
                }
            }

            // Escape ?
            if (editor->getState() == DO_ACTION && relevant_modifiers == 0
                && (ir->Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE || ir->Event.KeyEvent.wVirtualKeyCode == VK_DELETE)) {

                bool has_suggestion = editor->getSuggestionLength() > 0;
                editor->declineSuggestion();
                return has_suggestion;
            }
        }

        if (ir->Event.KeyEvent.bKeyDown
            && ir->Event.KeyEvent.wVirtualKeyCode != VK_CONTROL
            && ir->Event.KeyEvent.wVirtualKeyCode != VK_SHIFT
            && ir->Event.KeyEvent.wVirtualKeyCode != VK_MENU) {

            editor->declineSuggestion();
        }

        // Is character-typing key event?
        if (!ir->Event.KeyEvent.bKeyDown && ir->Event.KeyEvent.wVirtualScanCode == 0
            && (relevant_modifiers == 0 || relevant_modifiers == SHIFT_PRESSED)
            && ir->Event.KeyEvent.wVirtualKeyCode != VK_TAB && ir->Event.KeyEvent.wVirtualKeyCode != VK_BACK
            && ir->Event.KeyEvent.wVirtualKeyCode != VK_ESCAPE && ir->Event.KeyEvent.wVirtualKeyCode != VK_DELETE
            && ir->Event.KeyEvent.wVirtualKeyCode != VK_UP && ir->Event.KeyEvent.wVirtualKeyCode != VK_DOWN
            && ir->Event.KeyEvent.wVirtualKeyCode != VK_LEFT && ir->Event.KeyEvent.wVirtualKeyCode != VK_RIGHT
            && ir->Event.KeyEvent.wVirtualKeyCode != VK_RETURN
            && ir->Event.KeyEvent.uChar.UnicodeChar != 0) {

            editors->getEditor()->on();
        }
    }


    return 0;
}

SHAREDSYMBOL void WINAPI ExitFARW() {
    if (editors != nullptr) {
        delete editors;
        editors = nullptr;
    }
    free((void*)title[0]);
}
