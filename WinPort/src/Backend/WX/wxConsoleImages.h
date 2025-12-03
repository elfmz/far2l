#pragma once
#include "WinPort.h"
#include <wx/wx.h>
#include <map>
#include <string>

class wxConsoleImages
{
	struct wxConsoleImage
	{
		wxBitmap bitmap;
		wxBitmap scaled_bitmap;
		SMALL_RECT area{-1, -1, -1, -1};
		bool pixel_offset{false};
	};

	struct Images : std::map<std::string, wxConsoleImage> {} _images;

public:
	void Paint(wxPaintDC &dc, const wxRect &rc, unsigned int font_width, unsigned int font_height);

	bool Set(const char *id, DWORD64 flags, const SMALL_RECT *area, DWORD width, DWORD height, const void *buffer, unsigned int font_height);
	bool Transform(const char *id, const SMALL_RECT *area, uint16_t tf);
	bool Delete(const char *id);
};
