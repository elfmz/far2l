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
		bool prev_space;

		bool MayDrawFadedEdges();

		void SetColorFaded();
		void SetColorExtraFaded();
		void SetColorEmboss();
		void SetColorSoften();
		void SetColorRed();
		int GetFontAscent();

		void SetAccentBackground();
		void SetAccentForeground();
		void SetBackground();
		void SetForeground();

		void FillRectangle(wxCoord left, wxCoord top, wxCoord right, wxCoord bottom);
		void FillGradientRectangle(wxCoord left, wxCoord top, wxCoord right, wxCoord bottom);
		void FillPixel(wxCoord left, wxCoord top);
		void DrawEllipticArc(wxCoord left, wxCoord top, wxCoord width, wxCoord height, double start, double end, wxCoord thickness);
		void FillEllipticPie(wxCoord left, wxCoord top, wxCoord width, wxCoord height, double start, double end);
		void DrawLine(wxCoord X1, wxCoord Y1, wxCoord X2, wxCoord Y2, wxCoord thickness);

		void SaveBrush();
		void RestoreBrush();
	};

	typedef void (*DrawT)(Painter &p, unsigned int start_y, unsigned int cx);

	DrawT Get(const wchar_t c);
}
