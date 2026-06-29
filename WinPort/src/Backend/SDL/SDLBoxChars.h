#pragma once

#include <SDL.h>
#include <cstdint>

#include "WinPortRGB.h"

namespace SDLBoxChar
{
	struct Painter
	{
		int fw{0};
		int fh{0};
		int thickness{1};
		uint32_t wc{0};
		SDL_Renderer *renderer{nullptr};
		SDL_Color color{255, 255, 255, 255};
		int origin_x{0};
		int origin_y{0};

		bool prev_space;
		int text_accent {0};

		WinPortRGB _clr_text;
		WinPortRGB _clr_back;
		WinPortRGB _clr_accent_back;
		WinPortRGB _clr_accent_text;

		bool MayDrawFadedEdges() const;
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

		void FillRectangle(int left, int top, int right, int bottom);
		void FillPixel(int left, int top);

		void DrawEllipticArcE(int left, int top, int width, int height, double start, double end, int thickness);
		void DrawEllipticArcS(int left, int top, int width, int height, double start, double end, int thickness);
		void FillEllipticPieE(int left, int top, int width, int height, double start, double end);
		void FillEllipticPieS(int left, int top, int width, int height, double start, double end);
		void DrawLine(int X1, int Y1, int X2, int Y2, int thickness);

		inline void DrawEllipticArc(int left, int top, int width, int height, double start, double end, int thickness) {
			DrawEllipticArcS(left, top, width, height, start, end, thickness);
		}

		inline void FillEllipticPie(int left, int top, int width, int height, double start, double end) {
			FillEllipticPieS(left, top, width, height, start, end);
		}

		void SaveBrush();
		void RestoreBrush();

		void SetFillColor(const WinPortRGB& c);
	};

	using DrawT = void (*)(Painter &p, unsigned int start_y, unsigned int cx);

	DrawT Get(uint32_t c);
}
