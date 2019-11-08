#pragma once
#include <wx/dcclient.h>

namespace WXCustomDrawChar
{
	struct ICharPaintContext
	{
		wxCoord fw;
		wxCoord fh;
		wxCoord thickness;

		virtual bool SetColorFaded() = 0;
		virtual bool SetColorExtraFaded() = 0;
	};

	typedef void (*DrawT)(wxPaintDC &dc, ICharPaintContext &ctx, unsigned int start_y, unsigned int cx);

	DrawT Get(const wchar_t c);
}
