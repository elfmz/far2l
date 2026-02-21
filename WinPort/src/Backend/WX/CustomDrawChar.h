#pragma once
#include <wx/dcclient.h>

namespace WXCustomDrawChar
{
	struct Painter
	{
		wxCoord fw;
		wxCoord fh;
		wxCoord thickness;
		wchar_t wc;

		bool MayDrawFadedEdges();

		void SetColorFaded();
		void SetColorExtraFaded();
		int GetFontAscent();

		void FillRectangle(wxCoord left, wxCoord top, wxCoord right, wxCoord bottom);
		void FillPixel(wxCoord left, wxCoord top);
		void DrawEllipticArc(wxCoord left, wxCoord top, wxCoord width, wxCoord height, double start, double end, wxCoord thickness);
		void FillEllipticPie(wxCoord left, wxCoord top, wxCoord width, wxCoord height, double start, double end);
		void DrawLine(wxCoord X1, wxCoord Y1, wxCoord X2, wxCoord Y2, wxCoord thickness);
	};

	typedef void (*DrawT)(Painter &p, unsigned int start_y, unsigned int cx);

	DrawT Get(const wchar_t c);
}
