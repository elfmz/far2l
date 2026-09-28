#include <wx/wx.h>
#include <wx/display.h>
#ifdef TEST_X11
// Exercise the non-Mac translation paths with recorded X11 keycodes.
// wx event objects are real; this is not an X server integration test.
#undef __WXMAC__
#define __WXGTK__ 1
#endif
#include "wxWinTranslations.cpp"
#include <cassert>
#include <cstdio>
bool g_broadway = false;
bool g_remote = false;
static BOOL option = FALSE;
extern "C" BOOL WinPortGetUseRightAltAsAltGr() { return option; }
void Touchbar_SetAlternate(bool) {}
Threaded::~Threaded() = default;
wxKeyboardLedsState::~wxKeyboardLedsState() = default;
void *wxKeyboardLedsState::ThreadProc() { return nullptr; }
unsigned int wxKeyboardLedsState::Current(bool) { return 0; }
wxKeyboardLedsState g_wx_keyboard_leds_state;
void wxConsoleInputShim::Enqueue(const INPUT_RECORD *, DWORD) {}
static wxKeyEvent key(int code, unsigned raw, int unicode = 0, bool alt = false, bool ctrl = false) {
    wxKeyEvent e(wxEVT_KEY_DOWN);
    e.m_keyCode = code; e.m_rawCode = raw; e.m_uniChar = unicode;
    e.SetAltDown(alt); e.SetControlDown(ctrl);
    return e;
}
int main() {
#ifdef TEST_X11
    for (int enabled : {0, 1}) for (int code : {0, int(WXK_ALT)}) {
        option = enabled;
        KeyTracker t;
        auto altgr = key(code, 0xfe03, 0, true, true);
        t.OnKeyDown(altgr, 1);
        assert(t.RightAlt());
        assert(t.Composing() == bool(enabled));
        auto ir = wx2INPUT_RECORD(TRUE, altgr, t);
        assert(ir.Event.KeyEvent.wVirtualKeyCode == VK_MENU);
        assert(ir.Event.KeyEvent.dwControlKeyState == ENHANCED_KEY);
        auto a = key('A', 'a', 0x105, true, true);
        t.OnKeyDown(a, 2);
        ir = wx2INPUT_RECORD(TRUE, a, t);
        assert(ir.Event.KeyEvent.uChar.UnicodeChar == 0x105);
        assert(t.RightAlt()); // bypasses the OnChar uppercase workaround
        assert((ir.Event.KeyEvent.dwControlKeyState & (RIGHT_ALT_PRESSED | LEFT_CTRL_PRESSED)) == (RIGHT_ALT_PRESSED | LEFT_CTRL_PRESSED));
        t.OnKeyUp(a);
        assert(t.RightAlt() && t.Composing() == bool(enabled));
        // A code=0 IME/character release must not release ISO_Level3_Shift.
        auto ime = key(0, 'a', 0x105);
        t.OnKeyUp(ime);
        assert(t.RightAlt() && t.Composing() == bool(enabled));
        auto leftAlt = key(WXK_ALT, 0xffe9);
        t.OnKeyUp(leftAlt);
        assert(t.RightAlt() && t.Composing() == bool(enabled));
        t.OnKeyUp(altgr);
        assert(!t.RightAlt() && !t.Composing());
        ir = wx2INPUT_RECORD(FALSE, altgr, t);
        assert(ir.Event.KeyEvent.dwControlKeyState == ENHANCED_KEY);
        t.OnKeyDown(altgr, 3);
        t.ForceAllUp();
        assert(!t.RightAlt() && !t.Composing());
    }
    for (int enabled : {0, 1}) {
        option = enabled;
        KeyTracker t;
        auto rightAlt = key(WXK_ALT, 0xffea, 0, true);
        t.OnKeyDown(rightAlt, 1);
        assert(!t.RightAlt() && t.LeftAlt());
        assert(t.Composing() == bool(enabled));
        auto ir = wx2INPUT_RECORD(TRUE, rightAlt, t);
        assert((ir.Event.KeyEvent.dwControlKeyState & (RIGHT_ALT_PRESSED | LEFT_CTRL_PRESSED)) == 0);
        assert(ir.Event.KeyEvent.dwControlKeyState & LEFT_ALT_PRESSED);
        t.OnKeyUp(rightAlt);
        assert(!t.Composing());
    }
#else
    for (int enabled : {0, 1}) {
        option = enabled;
        KeyTracker t;
        auto rightOption = key(WXK_ALT, 0x3d, 0, true);
        t.OnKeyDown(rightOption, 1);
        assert(t.Composing() == bool(enabled));
        auto leftOption = key(WXK_ALT, 0x3a);
        t.OnKeyUp(leftOption);
        assert(t.Composing() == bool(enabled));
        t.OnKeyUp(rightOption);
        assert(!t.Composing());
        t.OnKeyDown(rightOption, 2);
        t.ForceAllUp();
        assert(!t.Composing());
    }
#endif
    puts("AltGr event regression checks passed");
}
extern "C" UINT WINPORT(MapVirtualKey)(UINT code, UINT) { return code; }
