#pragma once

#include "WinCompat.h"

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

struct WinPortRGB
{
	unsigned char r;
	unsigned char g;
	unsigned char b;

	inline WinPortRGB(DWORD rgb = 0) : r(rgb & 0xff), g((rgb >> 8) & 0xff), b((rgb >> 16) & 0xff) {}
	inline WinPortRGB(unsigned char r_, unsigned char g_, unsigned char b_) : r(r_), g(g_), b(b_) {}

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

WinPortRGB ConsoleForeground2RGB(DWORD64 attributes);
WinPortRGB ConsoleBackground2RGB(DWORD64 attributes);

bool InitPalettes();
