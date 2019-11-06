#pragma once
#include <wx/dcclient.h>

namespace WXCustomDrawChar
{
	struct FontMetrics
	{
		wxCoord fw;
		wxCoord fh;
		wxCoord thickness;
	};

	typedef void (*Draw_T)(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx);

	Draw_T Get(const wchar_t c);
}
