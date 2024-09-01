#pragma once
#include "../WinCompat.h"

struct WinPortRGB
{
	unsigned char r;
	unsigned char g;
	unsigned char b;
	unsigned char a;

	inline WinPortRGB(DWORD rgb = 0) : r(rgb & 0xff), g((rgb >> 8) & 0xff), b((rgb >> 16) & 0xff), a(0) {}
	inline WinPortRGB(unsigned char r_, unsigned char g_, unsigned char b_) : r(r_), g(g_), b(b_), a(0) {}

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

	inline uint32_t AsRGB() const
	{
		return uint32_t(r) | (uint32_t(g) << 8) | (uint32_t(b) << 16);
	}

	inline uint32_t AsBGR() const
	{
		return uint32_t(b) | (uint32_t(g) << 8) | (uint32_t(r) << 16);
	}
};

#define BASE_PALETTE_SIZE 16

struct WinPortPalette
{
	WinPortRGB background[BASE_PALETTE_SIZE];
	WinPortRGB foreground[BASE_PALETTE_SIZE];
};

extern WinPortPalette g_winport_palette;

void InitPalette();

inline WinPortRGB ConsoleForeground2RGB(const WinPortPalette &palette, DWORD64 attributes)
{
	if ((attributes & FOREGROUND_TRUECOLOR) != 0) {
		return GET_RGB_FORE(attributes);
	}

	return palette.foreground[(attributes & 0x0f)];
}

inline WinPortRGB ConsoleBackground2RGB(const WinPortPalette &palette, DWORD64 attributes)
{
	if ((attributes & BACKGROUND_TRUECOLOR) != 0) {
		return GET_RGB_BACK(attributes);
	}

	return palette.background[(attributes & 0xf0) >> 4];
}
