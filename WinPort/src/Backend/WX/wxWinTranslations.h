#pragma once

#include "WinCompat.h"

#include <set>

#include <wx/wx.h>
#include <wx/display.h>


class KeyTracker
{
	std::set<int> _pressed_keys;
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

	const wxKeyEvent& LastKeydown() const;
	DWORD LastKeydownTicks() const;
};

///////////////

struct wx2INPUT_RECORD : INPUT_RECORD
{
	wx2INPUT_RECORD(BOOL KeyDown, const wxKeyEvent& event, const KeyTracker &key_tracker);
};

struct WinPortRGB
{
	unsigned char r;
	unsigned char g;
	unsigned char b;

	inline WinPortRGB(unsigned char r_ = 0, unsigned char g_ = 0, unsigned char b_ = 0) : r(r_), g(g_), b(b_) {}

	inline bool operator == (const WinPortRGB &rgb) const
	{
		return r == rgb.r && g == rgb.g && b == rgb.b;
	}

	inline bool operator != (const WinPortRGB &rgb) const
	{
		return r != rgb.r || g != rgb.g || b != rgb.b;
	}

	inline bool operator < (const WinPortRGB &rgb) const
	{
		if (r < rgb.r) return true;
		if (r > rgb.r) return false;
		if (g < rgb.g) return true;
		if (g > rgb.g) return false;
		if (b < rgb.b) return true;

		return false;
	}
};

WinPortRGB ConsoleForeground2RGB(USHORT attributes);
WinPortRGB ConsoleBackground2RGB(USHORT attributes);

bool InitPalettes();
