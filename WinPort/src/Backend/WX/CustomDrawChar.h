#pragma once
#include <wx/dcclient.h>

namespace WXCustomDrawChar
{
	struct Painter
	{
		wxCoord fw;
		wxCoord fh;
		wxCoord thickness;

		bool MayDrawFadedEdges();

		void SetColorFaded();
		void SetColorExtraFaded();

		void FillRectangle(wxCoord left, wxCoord top, wxCoord right, wxCoord bottom);
		void FillPixel(wxCoord left, wxCoord top);
	};

	typedef void (*DrawT)(Painter &p, unsigned int start_y, unsigned int cx);

	DrawT Get(const wchar_t c);
}
