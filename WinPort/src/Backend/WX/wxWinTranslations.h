#pragma once

#include "WinCompat.h"
#include "../WinPortRGB.h"

#include <set>

#include <wx/wx.h>
#include <wx/display.h>


class KeyTracker
{
	std::set<int> _pressed_keys;
	bool _composing = false;
#ifndef __WXMAC__
	bool _right_control = false;
#endif
	wxKeyEvent _last_keydown;
	DWORD _last_keydown_ticks = 0;

	bool CheckForSuddenModifierUp(wxKeyCode keycode);

public:
	void OnKeyDown(wxKeyEvent& event, DWORD ticks);
	bool OnKeyUp(wxKeyEvent& event);

	bool CheckForSuddenModifiersUp();
	void ForceAllUp();

	bool Alt() const;
	bool Shift() const;
	bool LeftControl() const;
	bool RightControl() const;

	bool Composing() const { return _composing; }

	const wxKeyEvent& LastKeydown() const;
	DWORD LastKeydownTicks() const;
};

///////////////

struct wx2INPUT_RECORD : INPUT_RECORD
{
	wx2INPUT_RECORD(BOOL KeyDown, const wxKeyEvent& event, const KeyTracker &key_tracker);
};

extern WinPortPalette g_wx_palette;
extern bool g_wx_norgb;

WinPortRGB WxConsoleForeground2RGB(DWORD64 attributes);
WinPortRGB WxConsoleBackground2RGB(DWORD64 attributes);

DWORD WxKeyboardLedsState();

void WinPortWxAssertHandler(const wxString& file, int line, const wxString& func, const wxString& cond, const wxString& msg);

static int wxKeyCode2WinKeyCode(int code)
{
	switch (code) {
	case WXK_BACK: return VK_BACK;
	case WXK_TAB: return VK_TAB;
	case WXK_RETURN: return VK_RETURN;
	case WXK_ESCAPE: return VK_ESCAPE;
	case WXK_SPACE: return VK_SPACE;
	case WXK_DELETE: return VK_DELETE;
	case WXK_START: return 0;
	case WXK_LBUTTON: return VK_LBUTTON;
	case WXK_RBUTTON: return VK_RBUTTON;
	case WXK_CANCEL: return VK_CANCEL;
	case WXK_MBUTTON: return VK_MBUTTON;
	case WXK_CLEAR: return VK_CLEAR;
	case WXK_SHIFT: return VK_SHIFT;
	case WXK_ALT: return VK_MENU;
	case WXK_CONTROL: return VK_CONTROL;
#ifdef __WXMAC__
	case WXK_RAW_CONTROL: return VK_RCONTROL;
#endif
	case WXK_MENU: return VK_APPS;
	case WXK_PAUSE: return VK_PAUSE;
	case WXK_CAPITAL: return VK_CAPITAL;
	case WXK_END: return VK_END;
	case WXK_HOME: return VK_HOME;
	case WXK_LEFT: return VK_LEFT;
	case WXK_UP: return VK_UP;
	case WXK_RIGHT: return VK_RIGHT;
	case WXK_DOWN: return VK_DOWN;
	case WXK_SELECT: return VK_SELECT;
	case WXK_PRINT: return VK_PRINT;
	case WXK_EXECUTE: return VK_EXECUTE;
	case WXK_SNAPSHOT: return VK_SNAPSHOT;
#ifdef __WXOSX__
	case WXK_HELP: return VK_INSERT; //Or its hackintosh bug? If so - remove this ifdef.
#else
	case WXK_HELP: return VK_HELP;
#endif
	case WXK_INSERT: return VK_INSERT;
	case WXK_NUMPAD0: return VK_NUMPAD0;
	case WXK_NUMPAD1: return VK_NUMPAD1;
	case WXK_NUMPAD2: return VK_NUMPAD2;
	case WXK_NUMPAD3: return VK_NUMPAD3;
	case WXK_NUMPAD4: return VK_NUMPAD4;
	case WXK_NUMPAD5: return VK_NUMPAD5;
	case WXK_NUMPAD6: return VK_NUMPAD6;
	case WXK_NUMPAD7: return VK_NUMPAD7;
	case WXK_NUMPAD8: return VK_NUMPAD8;
	case WXK_NUMPAD9: return VK_NUMPAD9;
	case WXK_MULTIPLY: return VK_MULTIPLY;
	case WXK_ADD: return VK_ADD;
	case WXK_SEPARATOR: return VK_SEPARATOR;
	case WXK_SUBTRACT: return VK_SUBTRACT;
	case WXK_DECIMAL: return VK_DECIMAL;
	case WXK_DIVIDE: return VK_DIVIDE;
	case WXK_F1: return VK_F1;
	case WXK_F2: return VK_F2;
	case WXK_F3: return VK_F3;
	case WXK_F4: return VK_F4;
	case WXK_F5: return VK_F5;
	case WXK_F6: return VK_F6;
	case WXK_F7: return VK_F7;
	case WXK_F8: return VK_F8;
	case WXK_F9: return VK_F9;
	case WXK_F10: return VK_F10;
	case WXK_F11: return VK_F11;
	case WXK_F12: return VK_F12;
	case WXK_F13: return VK_F13;
	case WXK_F14: return VK_F14;
	case WXK_F15: return VK_F15;
	case WXK_F16: return VK_F16;
	case WXK_F17: return VK_F17;
	case WXK_F18: return VK_F18;
	case WXK_F19: return VK_F19;
	case WXK_F20: return VK_F20;
	case WXK_F21: return VK_F21;
	case WXK_F22: return VK_F22;
	case WXK_F23: return VK_F23;
	case WXK_F24: return VK_F24;
	case WXK_NUMLOCK: return VK_NUMLOCK;
	case WXK_SCROLL: return VK_SCROLL;
	case WXK_PAGEUP: return VK_PRIOR;
	case WXK_PAGEDOWN: return VK_NEXT;
	case WXK_NUMPAD_SPACE: return VK_SPACE;
	case WXK_NUMPAD_TAB: return VK_TAB;
	case WXK_NUMPAD_ENTER: return VK_RETURN;
	case WXK_NUMPAD_F1: return VK_F1;
	case WXK_NUMPAD_F2: return VK_F2;
	case WXK_NUMPAD_F3: return VK_F3;
	case WXK_NUMPAD_F4: return VK_F4;
	case WXK_NUMPAD_HOME: return VK_HOME;
	case WXK_NUMPAD_LEFT: return VK_LEFT;
	case WXK_NUMPAD_UP: return VK_UP;
	case WXK_NUMPAD_RIGHT: return VK_RIGHT;
	case WXK_NUMPAD_DOWN: return VK_DOWN;
	case WXK_NUMPAD_PAGEUP: return VK_PRIOR;
	case WXK_NUMPAD_PAGEDOWN: return VK_NEXT;
	case WXK_NUMPAD_END: return VK_END;
	case WXK_NUMPAD_BEGIN: return VK_CLEAR;
	case WXK_NUMPAD_INSERT: return VK_INSERT;
	case WXK_NUMPAD_DELETE: return VK_DELETE;
	case WXK_NUMPAD_EQUAL: return VK_OEM_NEC_EQUAL;
	case WXK_NUMPAD_MULTIPLY: return VK_MULTIPLY;
	case WXK_NUMPAD_ADD: return VK_ADD;
	case WXK_NUMPAD_SEPARATOR: return VK_SEPARATOR;
	case WXK_NUMPAD_SUBTRACT: return VK_SUBTRACT;
	case WXK_NUMPAD_DECIMAL: return VK_DECIMAL;
	case WXK_NUMPAD_DIVIDE: return VK_DIVIDE;

	case WXK_WINDOWS_LEFT: return VK_LWIN;
	case WXK_WINDOWS_RIGHT: return VK_RWIN;
	case WXK_WINDOWS_MENU: return VK_MENU;
	case L'.': return VK_OEM_PERIOD;
	case L',': return VK_OEM_COMMA;
	case L'_': case L'-': return VK_OEM_MINUS;
	case L'+': return VK_OEM_PLUS;
	case L';': case L':': return VK_OEM_1;
	case L'/': case L'?': return VK_OEM_2;
	case L'~': case L'`': return VK_OEM_3;
	case L'[': case L'{': return VK_OEM_4;
	case L'\\': case L'|': return VK_OEM_5;
	case L']': case '}': return VK_OEM_6;
	case L'\'': case '\"': return VK_OEM_7;
	case L'!': return '1';
	case L'@': return '2';
	case L'#': return '3';
	case L'$': return '4';
	case L'%': return '5';
	case L'^': return '6';
	case L'&': return '7';
	case L'*': return '8';
	case L'(': return '9';
	case L')': return '0';
	}
	//fprintf(stderr, "not translated %u %lc", code, code);
	return code;
}
