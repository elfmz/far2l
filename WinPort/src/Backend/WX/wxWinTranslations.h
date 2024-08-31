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
