#pragma once

#include "WinCompat.h"

#include <wx/wx.h>
#include <wx/display.h>

int wxKeyCode2WinKeyCode(int code);

struct wx2INPUT_RECORD : INPUT_RECORD
{
	wx2INPUT_RECORD(wxKeyEvent& event, BOOL KeyDown);
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
