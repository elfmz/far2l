#pragma once
#include <wx/graphics.h>
#include "Paint.h"

namespace WXCustomDrawChar
{
	typedef void (*Draw_T)(ConsolePaintContext *context, wxPaintDC &dc, unsigned int start_y, unsigned int cx);

	Draw_T Get(const wchar_t c);
}
