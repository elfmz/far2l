#pragma once
#include "WinPort.h"
#include <wx/wx.h>
#include <map>
#include <vector>
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
	// Visibility belongs to the image ID, not a decoded bitmap: QuickView can
	// delete and replace its bitmap from a worker while a dialog is open.
	struct Visibility
	{
		bool visible{true};
		std::vector<SMALL_RECT> exclusions;
	};
	std::map<std::string, Visibility> _visibility;

public:
	void Paint(wxDC &dc, const wxRect &rc, unsigned int font_width, unsigned int font_height);

	bool Set(const char *id, DWORD64 flags, const SMALL_RECT *area, DWORD width, DWORD height, const void *buffer, unsigned int font_height);
	bool Transform(const char *id, const SMALL_RECT *area, uint16_t tf);
	bool Delete(const char *id);
	bool SetVisibility(const char *id, bool visible, const SMALL_RECT *exclusions, size_t count);
};
