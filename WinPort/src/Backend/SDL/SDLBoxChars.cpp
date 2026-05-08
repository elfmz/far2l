#include <algorithm>
#include "SDLBoxChars.h"
#include "UtfDefines.h"

#include "SDLBackendUtils.h"

#include <Colorspace.h>

namespace SDLBoxChar
{

bool Painter::MayDrawFadedEdges() const
{
	return false;
}

void Painter::SetColorFaded()
{
}

void Painter::SetColorExtraFaded()
{
}

void Painter::FillRectangle(int left, int top, int right, int bottom)
{
	if (!renderer) {
		return;
	}
	if (right < left) std::swap(left, right);
	if (bottom < top) std::swap(top, bottom);
	const int width = right - left + 1;
	const int height = bottom - top + 1;
	if (width <= 0 || height <= 0) {
		return;
	}
	SDL_Rect rect{
		origin_x + left,
		origin_y + top,
		width,
		height
	};
	SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
	SDL_RenderFillRect(renderer, &rect);
}

void Painter::SetFillColor(const WinPortRGB& c) {
	color = ToSDLColor(c);
}

void Painter::SetAccentBackground() {
	ComputeAccents(_clr_text, _clr_back, _clr_accent_text, _clr_accent_back);
	SetFillColor(_clr_accent_back);
}

void Painter::SetBackground() {
	SetFillColor(_clr_back);
}

void Painter::SetAccentForeground() {
	ComputeAccents(_clr_text, _clr_back, _clr_accent_text, _clr_accent_back);
	SetFillColor(_clr_accent_text);
}

void Painter::SetForeground() {
	SetFillColor(_clr_text);
}

void Painter::SetColorEmboss()
{
	WinPortRGB clr_fade;
	/* near to black / near to white means LAB */
	int blackb = _clr_back.r + _clr_back.g + _clr_back.b;
	int blackf = _clr_text.r + _clr_text.g + _clr_text.b;
	if (blackb < 0x5f || blackf < 0x5f || blackb > 700 || blackf > 700) 
		clr_fade = ComputeEmbossColor_LAB(_clr_back, GetSoftenColorIf(_clr_text));
	else
		clr_fade = ComputeEmbossColor_HSL(_clr_back, _clr_text);
	SetFillColor(clr_fade);
}

void Painter::SetColorSoften()
{
	WinPortRGB fade = GetSoftenColorIf(_clr_text);
	SetFillColor(fade);
}

void Painter::SetColorRed()
{
	WinPortRGB clr_fade(0xff, 0, 0);
	SetFillColor(clr_fade);
}

int Painter::GetFontAscent()
{
	return text_accent;
}

void Painter::FillPixel(int left, int top)
{
	FillRectangle(left, top, left, top);
}

void Painter::DrawEllipticArcS(int left, int top, int width, int height, double startDeg, double endDeg, int thickness) 
{
	// use curved instead of rounded
	//     \ < 90
	//    / < 180
	//    \ < 270
	//    / < 360
	if (endDeg == 0) endDeg = 360;
	if (startDeg < 90 && endDeg >= 90) {
		SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
		SDL_RenderDrawLine(renderer, origin_x + left + width / 2, origin_y + top, origin_x + left + width, origin_y + top + height / 2);
	}
	if (startDeg < 180 && endDeg >= 180) {
		SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
		SDL_RenderDrawLine(renderer, origin_x + left + width / 2, origin_y + top, origin_x + left, origin_y + top + height / 2);
	}
	if (startDeg < 270 && endDeg >= 270) {
		SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
		SDL_RenderDrawLine(renderer, origin_x + left + width / 2, origin_y + top + height, origin_x + left, origin_y + top + height / 2);
	}
	if (startDeg < 360 && endDeg == 360) {
		SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
		SDL_RenderDrawLine(renderer, origin_x + left + width / 2, origin_y + top + height, origin_x + left + width, origin_y + top + height / 2);
	}
}

void Painter::DrawEllipticArcE(int left, int top, int width, int height, double startDeg, double endDeg, int thickness) 
{
	if (endDeg == 0) endDeg = 360;

    double cx = left + width / 2.0;
    double cy = top + height / 2.0;
    double rx = width / 2.0;
    double ry = height / 2.0;

    double start = startDeg * M_PI / 180.0;
    double end   = endDeg   * M_PI / 180.0;

    const int segments = 180; // smoothness
    double step = (end - start) / segments;

	SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    for (int t = 0; t < thickness; ++t) {
        double innerRx = rx - t;
        double innerRy = ry - t;

        double prevX = cx + innerRx * cos(start);
        double prevY = cy + innerRy * sin(start);

        for (int i = 1; i <= segments; ++i) {
            double a = start + step * i;
            double x = cx + innerRx * cos(a);
            double y = cy + innerRy * sin(a);

            SDL_RenderDrawLine(renderer, origin_x + prevX, origin_y + prevY, origin_x + x, origin_y + y);

            prevX = x;
            prevY = y;
        }
    }
}

void Painter::FillEllipticPieS(int left, int top, int width, int height, double startDeg, double endDeg) 
{
	if (endDeg == 0) endDeg = 360;
	int centerX = origin_x + left + width / 2;
	int centerY = origin_y + top + height / 2;

	if (startDeg < 90 && endDeg >= 90) {
		SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
		DrawFilledTriangle(renderer, origin_x + left + width / 2, origin_y + top, origin_x + left + width, origin_y + top + height / 2, centerX, centerY);
	}
	if (startDeg < 180 && endDeg >= 180) {
		SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
		DrawFilledTriangle(renderer, origin_x + left + width / 2, origin_y + top, origin_x + left, origin_y + top + height / 2, centerX, centerY);
	}
	if (startDeg < 270 && endDeg >= 270) {
		SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
		DrawFilledTriangle(renderer, origin_x + left + width / 2, origin_y + top + height, origin_x + left, origin_y + top + height / 2, centerX, centerY);
	}
	if (startDeg < 360 && endDeg >= 360) {
		SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
		DrawFilledTriangle(renderer, origin_x + left + width / 2, origin_y + top + height, origin_x + left + width, origin_y + top + height / 2, centerX, centerY);
	}
}

void Painter::FillEllipticPieE(int left, int top, int width, int height, double startDeg, double endDeg)
{
	if (endDeg == 0) endDeg = 360;

    double cx = left + width / 2.0;
    double cy = top + height / 2.0;
    double rx = width / 2.0;
    double ry = height / 2.0;

    double start = startDeg * M_PI / 180.0;
    double end   = endDeg   * M_PI / 180.0;

    const int segments = 180;
    double step = (end - start) / segments;

    double prevX = cx + rx * cos(start);
    double prevY = cy + ry * sin(start);

	SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    for (int i = 1; i <= segments; ++i) {
        double a = start + step * i;
        double x = cx + rx * cos(a);
        double y = cy + ry * sin(a);

        // Draw triangle: center → prev → current
        SDL_RenderDrawLine(renderer, origin_x + cx, origin_y + cy, origin_x + prevX, origin_y + prevY);
        SDL_RenderDrawLine(renderer, origin_x + prevX,origin_y +  prevY, origin_x + x, origin_y + y);
        SDL_RenderDrawLine(renderer, origin_x + x, origin_y + y, origin_x + cx, origin_y + cy);

        prevX = x;
        prevY = y;
    }
}

void Painter::DrawLine(int X1, int Y1, int X2, int Y2, int thickness) {
	SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
	SDL_RenderDrawLine(renderer, origin_x + X1, origin_y + Y1, origin_x + X2, origin_y + Y2);
}

void Painter::SaveBrush() {}
void Painter::RestoreBrush() { SetFillColor(_clr_text); }


	struct CharMetrics
	{
		inline CharMetrics(Painter &p, unsigned int start_y, unsigned int cx) :
			left(cx * p.fw),
			right(left + p.fw - 1),
			top(start_y),
			bottom(top + p.fh - 1)
		{
		}

		int left;
		int right;
		int top;
		int bottom;


		inline int height() { return bottom - top; }
		inline int width() { return right - left; }
	};

	struct SingleLineBoxMetrics : CharMetrics
	{
		inline SingleLineBoxMetrics(Painter &p, unsigned int start_y, unsigned int cx)
			: CharMetrics(p, start_y, cx),
			  middle_y(top + p.fh / 2 - p.thickness / 2),
			  middle_x(left + p.fw / 2 - p.thickness / 2)
		{
		}

		int middle_y;
		int middle_x;
	};

	struct DoubleLineBoxMetrics : SingleLineBoxMetrics
	{
		inline DoubleLineBoxMetrics(Painter &p, unsigned int start_y, unsigned int cx)
			: SingleLineBoxMetrics(p, start_y, cx)
		{
			int ofs = std::min(p.fh, p.fw) / 4;

			middle1_y = middle_y - ofs;
			middle1_x = middle_x - ofs;

			middle2_y = middle_y + ofs;
			middle2_x = middle_x + ofs;
		}

		int middle1_y, middle2_y;
		int middle1_x, middle2_x;
	};

	template <int COUNT>
		struct Dashes
	{
		Dashes(int total_len)
		{
			period = total_len / COUNT;
			len = period * 2 / 3;
			if (len == 0) len = 1;

			auto underflow = total_len - period * COUNT;
			if (underflow > 1) {
				auto xperiod = period + 1;
				auto overflow = xperiod * COUNT - total_len;
				if (overflow < underflow && xperiod * (COUNT - 1) + len < total_len) {
					period = xperiod;
				}
			}
		}

		int period; // distance between begins of each visible dash parts
		int len; // visible dash part len
	};

	template <DrawT DRAW>
		static void Draw_Thicker(Painter &p, unsigned int start_y, unsigned int cx) /* ─ */
	{
		auto saved_thickness = p.thickness;
		p.thickness = 1 + (p.thickness * 3) / 2;
		DRAW(p, start_y, cx);
		p.thickness = saved_thickness;
	}


	template <int COUNT>
		static void Draw_DashesH(Painter &p, unsigned int start_y, unsigned int cx) /* ┄ */
	{
		SingleLineBoxMetrics m(p, start_y, cx);
		Dashes<COUNT> d(p.fw);

		for (int i = 0; i < COUNT; ++i) {
			p.FillRectangle(m.left + i * d.period, m.middle_y, m.left + i * d.period + d.len - 1, m.middle_y + p.thickness - 1);
		}

		if (p.MayDrawFadedEdges()) {
			p.SetColorFaded();
			for (int i = 0; i < COUNT; ++i) {
				p.FillRectangle(m.left + i * d.period, m.middle_y - 1, m.left + i * d.period + d.len - 1, m.middle_y - 1);
			}
		}
	}

	template <int COUNT>
		static void Draw_DashesV(Painter &p, unsigned int start_y, unsigned int cx) /* ┄ */
	{
		SingleLineBoxMetrics m(p, start_y, cx);
		Dashes<COUNT> d(p.fh);

		for (int i = 0; i < COUNT; ++i) {
			p.FillRectangle(m.middle_x, m.top + i * d.period, m.middle_x + p.thickness - 1, m.top + i * d.period + d.len - 1);
		}

		if (p.MayDrawFadedEdges()) {
			p.SetColorFaded();
			for (int i = 0; i < COUNT; ++i) {
				p.FillRectangle(m.middle_x - 1, m.top + i * d.period, m.middle_x - 1, m.top + i * d.period + d.len - 1);
			}
		}
	}

	template <bool TOP_DOWN>
		static void Draw_VerticalArrow(Painter &p, unsigned int start_y, unsigned int cx) /* ↑↓ */
	{
		SingleLineBoxMetrics m(p, start_y, cx);
		int top = m.top + p.fh / 8;
		int bottom = m.bottom - p.fh / 8;
		int arrow = std::min(p.fw / 4, p.fh / 4);

		p.FillRectangle(m.middle_x, top, m.middle_x + p.thickness - 1, bottom);
		for (int i = arrow; i > 0; --i) if (TOP_DOWN) {
			p.FillPixel(m.middle_x - i, bottom - i);
			p.FillPixel(m.middle_x + p.thickness - 1 + i, bottom - i);
		} else {
			p.FillPixel(m.middle_x - i, top + i);
			p.FillPixel(m.middle_x + p.thickness - 1 + i, top + i);
		}
		if (p.MayDrawFadedEdges()) {
			p.SetColorFaded();
			p.FillRectangle(m.middle_x - 1, top, m.middle_x - 1, bottom);
			for (int i = arrow; i > 0; --i) if (TOP_DOWN) {
				p.FillPixel(m.middle_x - i - 1, bottom - i);
				p.FillPixel(m.middle_x + p.thickness - 1 + i - 1, bottom - i);
			} else {
				p.FillPixel(m.middle_x - i - 1, top + i);
				p.FillPixel(m.middle_x + p.thickness - 1 + i - 1, top + i);
			}
		}
	}

	template <bool LEFT_RIGHT>
		static void Draw_HorizontalArrow(Painter &p, unsigned int start_y, unsigned int cx) /* ←→ */
	{
		SingleLineBoxMetrics m(p, start_y, cx);
		int left = m.left + p.fw / 8;
		int right = m.right - p.fw / 8;
		int arrow = std::min(p.fw / 4, p.fh / 4);

		p.FillRectangle(m.left, m.middle_y, m.right, m.middle_y + p.thickness - 1);
		for (int i = arrow; i > 0; --i) if (LEFT_RIGHT) {
			p.FillPixel(right - i, m.middle_y - i);
			p.FillPixel(right - i, m.middle_y + p.thickness - 1 + i);
		} else {
			p.FillPixel(left + i, m.middle_y - i);
			p.FillPixel(left + i, m.middle_y + p.thickness - 1 + i);
		}
		if (p.MayDrawFadedEdges()) {
			p.SetColorFaded();
			p.FillRectangle(m.left, m.middle_y - 1, m.right, m.middle_y - 1);
			for (int i = arrow; i > 0; --i) if (LEFT_RIGHT) {
				p.FillPixel(right - i, m.middle_y - 1 - i);
				p.FillPixel(right - i, m.middle_y - 1 + p.thickness - 1 + i);
			} else {
				p.FillPixel(left + i, m.middle_y - 1 - i);
				p.FillPixel(left + i, m.middle_y - 1 + p.thickness - 1 + i);
			}
		}
	}

    /* borders OLD */

	static void Draw_2500(Painter &p, unsigned int start_y, unsigned int cx) /* ─ */
	{
		SingleLineBoxMetrics m(p, start_y, cx);
		p.FillRectangle(m.left, m.middle_y, m.right, m.middle_y + p.thickness - 1);
		if (p.MayDrawFadedEdges()) {
			p.SetColorFaded();
			p.FillRectangle(m.left, m.middle_y - 1, m.right, m.middle_y - 1);
		}
	}

	static void Draw_2502(Painter &p, unsigned int start_y, unsigned int cx) /* │ */
	{
		SingleLineBoxMetrics m(p, start_y, cx);
		p.FillRectangle(m.middle_x, m.top, m.middle_x + p.thickness - 1, m.bottom);
		if (p.MayDrawFadedEdges()) {
			p.SetColorFaded();
			p.FillRectangle(m.middle_x - 1, m.top, m.middle_x - 1, m.bottom);
		}
	}

	static void Draw_250C(Painter &p, unsigned int start_y, unsigned int cx) /* ┌ */
	{
		SingleLineBoxMetrics m(p, start_y, cx);
		p.FillRectangle(m.middle_x, m.middle_y, m.right, m.middle_y + p.thickness - 1);
		p.FillRectangle(m.middle_x, m.middle_y, m.middle_x + p.thickness - 1, m.bottom);
		if (p.MayDrawFadedEdges()) {
			p.SetColorFaded();
			p.FillRectangle(m.middle_x, m.middle_y - 1, m.right, m.middle_y - 1);
			p.FillRectangle(m.middle_x - 1, m.middle_y, m.middle_x - 1, m.bottom);

			p.SetColorExtraFaded();
			p.FillPixel(m.middle_x - 1, m.middle_y - 1);
		}
	}


	static void Draw_2510(Painter &p, unsigned int start_y, unsigned int cx) /* ┐ */
	{
		SingleLineBoxMetrics m(p, start_y, cx);
		p.FillRectangle(m.left, m.middle_y, m.middle_x, m.middle_y + p.thickness - 1);
		p.FillRectangle(m.middle_x, m.middle_y, m.middle_x + p.thickness - 1, m.bottom);
		if (p.MayDrawFadedEdges()) {
			p.SetColorFaded();
			p.FillRectangle(m.left, m.middle_y - 1, m.middle_x + p.thickness - 1, m.middle_y - 1);
			p.FillRectangle(m.middle_x - 1, m.middle_y + p.thickness, m.middle_x - 1, m.bottom);
		}
	}


	static void Draw_2514(Painter &p, unsigned int start_y, unsigned int cx) /* └ */
	{
		SingleLineBoxMetrics m(p, start_y, cx);
		p.FillRectangle(m.middle_x, m.middle_y, m.right, m.middle_y + p.thickness - 1);
		p.FillRectangle(m.middle_x, m.top, m.middle_x + p.thickness - 1, m.middle_y);
		if (p.MayDrawFadedEdges()) {
			p.SetColorFaded();
			p.FillRectangle(m.middle_x + p.thickness, m.middle_y - 1, m.right, m.middle_y - 1);
			p.FillRectangle(m.middle_x - 1, m.top, m.middle_x - 1, m.middle_y + p.thickness - 1);
		}
	}


	static void Draw_2518(Painter &p, unsigned int start_y, unsigned int cx) /* ┘ */
	{
		SingleLineBoxMetrics m(p, start_y, cx);
		p.FillRectangle(m.left, m.middle_y, m.middle_x + p.thickness - 1, m.middle_y + p.thickness - 1);
		p.FillRectangle(m.middle_x, m.top, m.middle_x + p.thickness - 1, m.middle_y);

		if (p.MayDrawFadedEdges()) {
			p.FillPixel(m.middle_x - 1, m.middle_y - 1); // special pixel, avoid its overlap later

			p.SetColorFaded();

			p.FillRectangle(m.left, m.middle_y - 1, m.middle_x - 2, m.middle_y - 1);
			p.FillRectangle(m.middle_x - 1, m.top, m.middle_x - 1, m.middle_y - 2);
		}
	}


	static void Draw_251C(Painter &p, unsigned int start_y, unsigned int cx) /* ├ */
	{
		SingleLineBoxMetrics m(p, start_y, cx);
		p.FillRectangle(m.middle_x, m.middle_y, m.right, m.middle_y + p.thickness - 1);
		p.FillRectangle(m.middle_x, m.top, m.middle_x + p.thickness - 1, m.bottom);
		if (p.MayDrawFadedEdges()) {
			p.SetColorFaded();
			p.FillRectangle(m.middle_x + p.thickness, m.middle_y - 1, m.right, m.middle_y - 1);
			p.FillRectangle(m.middle_x - 1, m.top, m.middle_x - 1, m.bottom);
		}
	}


	static void Draw_2524(Painter &p, unsigned int start_y, unsigned int cx) /* ┤ */
	{
		SingleLineBoxMetrics m(p, start_y, cx);
		p.FillRectangle(m.left, m.middle_y, m.middle_x, m.middle_y + p.thickness - 1);
		p.FillRectangle(m.middle_x, m.top, m.middle_x + p.thickness - 1, m.bottom);

		if (p.MayDrawFadedEdges()) {
			p.FillPixel(m.middle_x - 1, m.middle_y - 1); // special pixel
			p.SetColorFaded();
			p.FillRectangle(m.left, m.middle_y - 1, m.middle_x - 2, m.middle_y - 1);
			p.FillRectangle(m.middle_x - 1, m.top, m.middle_x - 1, m.middle_y - 2);
			p.FillRectangle(m.middle_x - 1, m.middle_y + p.thickness, m.middle_x - 1, m.bottom);
		}
	}


	static void Draw_252C(Painter &p, unsigned int start_y, unsigned int cx) /* ┬ */
	{
		SingleLineBoxMetrics m(p, start_y, cx);
		p.FillRectangle(m.left, m.middle_y, m.right, m.middle_y + p.thickness - 1);
		p.FillRectangle(m.middle_x, m.middle_y, m.middle_x + p.thickness - 1, m.bottom);
		if (p.MayDrawFadedEdges()) {
			p.SetColorFaded();
			p.FillRectangle(m.left, m.middle_y - 1, m.right, m.middle_y - 1);
			p.FillRectangle(m.middle_x - 1, m.middle_y + p.thickness, m.middle_x - 1, m.bottom);
		}
	}


	static void Draw_2534(Painter &p, unsigned int start_y, unsigned int cx) /* ┴ */
	{
		SingleLineBoxMetrics m(p, start_y, cx);
		p.FillRectangle(m.left, m.middle_y, m.right, m.middle_y + p.thickness - 1);
		p.FillRectangle(m.middle_x, m.top, m.middle_x + p.thickness - 1, m.middle_y);

		if (p.MayDrawFadedEdges()) {
			p.FillPixel(m.middle_x - 1, m.middle_y - 1); // special pixel
			p.SetColorFaded();
			p.FillRectangle(m.left, m.middle_y - 1, m.middle_x - 2, m.middle_y - 1);
			p.FillRectangle(m.middle_x + p.thickness, m.middle_y - 1, m.right, m.middle_y - 1);
			p.FillRectangle(m.middle_x - 1, m.top, m.middle_x - 1, m.middle_y - 2);
		}
	}


	static void Draw_253C(Painter &p, unsigned int start_y, unsigned int cx) /* ┼ */
	{
		SingleLineBoxMetrics m(p, start_y, cx);
		p.FillRectangle(m.left, m.middle_y, m.right, m.middle_y + p.thickness - 1);
		p.FillRectangle(m.middle_x, m.top, m.middle_x + p.thickness - 1, m.bottom);
		if (p.MayDrawFadedEdges()) {
			p.FillPixel(m.middle_x - 1, m.middle_y - 1); // special pixel
			p.SetColorFaded();
			p.FillRectangle(m.left, m.middle_y - 1, m.middle_x - 2, m.middle_y - 1);
			p.FillRectangle(m.middle_x + p.thickness, m.middle_y - 1, m.right, m.middle_y - 1);
			p.FillRectangle(m.middle_x - 1, m.top, m.middle_x - 1, m.middle_y - 2);
			p.FillRectangle(m.middle_x - 1, m.middle_y + p.thickness, m.middle_x - 1, m.bottom);
		}
	}

    /* single lines modern look */

	static void Draw_2500_new(Painter &p, unsigned int start_y, unsigned int cx) /* ─ */
	{
		SingleLineBoxMetrics m(p, start_y, cx);
		p.SetColorSoften();
		p.FillRectangle(m.left, m.middle_y, m.right, m.middle_y + p.thickness - 1);
		p.SetColorEmboss();
		p.FillRectangle(m.left, m.middle_y + p.thickness, m.right, m.middle_y + p.thickness);
	}

	static void Draw_2502_new(Painter &p, unsigned int start_y, unsigned int cx) /* │ */
	{
		SingleLineBoxMetrics m(p, start_y, cx);
		p.SetColorSoften();
		p.FillRectangle(m.middle_x, m.top, m.middle_x + p.thickness - 1, m.bottom);
		p.SetColorEmboss();
		p.FillRectangle(m.middle_x + p.thickness, m.top, m.middle_x + p.thickness, m.bottom);
	}

	static inline void DrawSingleEllipse(Painter &p, SingleLineBoxMetrics &m, double start, double end, bool left, bool top, int thickness = 0)
	{
        int wx = (m.right - m.left - thickness) / 2;
        int wy = (m.bottom - m.top - thickness) / 2;
		p.DrawEllipticArc(
			(left ? m.left - wx : m.middle_x + thickness), 
			(top ? m.top - wy : m.middle_y + thickness), 
			wx * 2,
			wy * 2, 
			start, 
			end, 
			p.thickness);
	}

	static inline void DrawSingleEllipseEmboss(Painter &p, SingleLineBoxMetrics &m, double start, double end, bool left, bool top)
	{
		p.SaveBrush();
		p.SetColorEmboss();
		DrawSingleEllipse(p, m, start, end, left, top, 2);
		p.RestoreBrush();
	}

	static void Draw_250C_new(Painter &p, unsigned int start_y, unsigned int cx) /* ┌ */
	{
		SingleLineBoxMetrics m(p, start_y, cx);
		if (SDLBackend::options && SDLBackend::options->UseRoundedBorders) {
			DrawSingleEllipseEmboss(p, m, 90, 180, false, false);
			p.SetColorSoften();
			DrawSingleEllipse(p, m, 90, 180, false, false);
		}
		else {
			p.SetColorSoften();
			p.FillRectangle(m.middle_x, m.middle_y, m.right, m.middle_y + p.thickness - 1);
			p.FillRectangle(m.middle_x, m.middle_y, m.middle_x + p.thickness - 1, m.bottom);
			p.SetColorEmboss();
			p.FillRectangle(m.middle_x + p.thickness, m.middle_y + p.thickness, m.right, m.middle_y + p.thickness);
			p.FillRectangle(m.middle_x + p.thickness, m.middle_y + p.thickness, m.middle_x + p.thickness, m.bottom);
		}
	}

	static void Draw_2510_new(Painter &p, unsigned int start_y, unsigned int cx) /* ┐ */
	{
		SingleLineBoxMetrics m(p, start_y, cx);
		if (SDLBackend::options && SDLBackend::options->UseRoundedBorders) {
			DrawSingleEllipseEmboss(p, m, 0, 90, true, false);
			p.SetColorSoften();
			DrawSingleEllipse(p, m, 0, 90, true, false);
		}
		else {
			p.SetColorSoften();
			p.FillRectangle(m.left, m.middle_y, m.middle_x, m.middle_y + p.thickness - 1);
			p.FillRectangle(m.middle_x, m.middle_y, m.middle_x + p.thickness - 1, m.bottom);
			p.SetColorEmboss();
			p.FillRectangle(m.left, m.middle_y + p.thickness, m.middle_x + p.thickness, m.middle_y + p.thickness);
			p.FillRectangle(m.middle_x + p.thickness, m.middle_y + p.thickness, m.middle_x + p.thickness, m.bottom);
		}
	}

	static void Draw_2514_new(Painter &p, unsigned int start_y, unsigned int cx) /* └ */
	{
		SingleLineBoxMetrics m(p, start_y, cx);
		if (SDLBackend::options && SDLBackend::options->UseRoundedBorders) {
			DrawSingleEllipseEmboss(p, m, 180, 270, false, true);
			p.SetColorSoften();
			DrawSingleEllipse(p, m, 180, 270, false, true);
		}
		else {
			p.SetColorSoften();
			p.FillRectangle(m.middle_x, m.middle_y, m.right, m.middle_y + p.thickness - 1);
			p.FillRectangle(m.middle_x, m.top, m.middle_x + p.thickness - 1, m.middle_y);
			p.SetColorEmboss();
			p.FillRectangle(m.middle_x + p.thickness, m.middle_y + p.thickness, m.right, m.middle_y + p.thickness);
			p.FillRectangle(m.middle_x + p.thickness, m.top, m.middle_x + p.thickness, m.middle_y + p.thickness);
		}
	}

	static void Draw_2518_new(Painter &p, unsigned int start_y, unsigned int cx) /* ┘ */
	{
		SingleLineBoxMetrics m(p, start_y, cx);
		if (SDLBackend::options && SDLBackend::options->UseRoundedBorders) {
			DrawSingleEllipseEmboss(p, m, 270, 360, true, true);
			p.SetColorSoften();
			DrawSingleEllipse(p, m, 270, 360, true, true);
		}
		else {
			p.SetColorSoften();
			p.FillRectangle(m.left, m.middle_y, m.middle_x + p.thickness - 1, m.middle_y + p.thickness - 1);
			p.FillRectangle(m.middle_x, m.top, m.middle_x + p.thickness - 1, m.middle_y);
			p.SetColorEmboss();
			p.FillRectangle(m.left, m.middle_y + p.thickness, m.middle_x + p.thickness, m.middle_y + p.thickness);
			p.FillRectangle(m.middle_x + p.thickness, m.top, m.middle_x + p.thickness, m.middle_y + p.thickness);
		}
	}

	static void Draw_251C_new(Painter &p, unsigned int start_y, unsigned int cx) /* ├ */
	{
		SingleLineBoxMetrics m(p, start_y, cx);
		p.SetColorSoften();
		p.FillRectangle(m.middle_x, m.middle_y, m.right, m.middle_y + p.thickness - 1);
		p.FillRectangle(m.middle_x, m.top, m.middle_x + p.thickness - 1, m.bottom);
		p.SetColorEmboss();
		p.FillRectangle(m.middle_x + 1, m.middle_y + p.thickness, m.right, m.middle_y + p.thickness);
		p.FillRectangle(m.middle_x + p.thickness, m.top, m.middle_x + p.thickness, m.middle_y + p.thickness);
		p.FillRectangle(m.middle_x + p.thickness, m.middle_y + p.thickness - 1, m.middle_x + p.thickness, m.middle_y + p.thickness);
	}

	static void Draw_2524_new(Painter &p, unsigned int start_y, unsigned int cx) /* ┤ */
	{
		SingleLineBoxMetrics m(p, start_y, cx);
		p.SetColorSoften();
		p.FillRectangle(m.left, m.middle_y, m.middle_x, m.middle_y + p.thickness - 1);
		p.FillRectangle(m.middle_x, m.top, m.middle_x + p.thickness - 1, m.bottom);
		p.SetColorEmboss();
		p.FillRectangle(m.left, m.middle_y + p.thickness, m.middle_x, m.middle_y + p.thickness);
		p.FillRectangle(m.middle_x + p.thickness, m.top, m.middle_x + p.thickness, m.bottom);
	}

	static void Draw_252C_new(Painter &p, unsigned int start_y, unsigned int cx) /* ┬ */
	{
		SingleLineBoxMetrics m(p, start_y, cx);
		p.SetColorSoften();
		p.FillRectangle(m.left, m.middle_y, m.right, m.middle_y + p.thickness - 1);
		p.FillRectangle(m.middle_x, m.middle_y, m.middle_x + p.thickness - 1, m.bottom);
		p.SetColorEmboss();
		p.FillRectangle(m.left, m.middle_y + p.thickness, m.middle_x - 1, m.middle_y + p.thickness);
		p.FillRectangle(m.middle_x + p.thickness, m.middle_y + p.thickness, m.right, m.middle_y + p.thickness);
		p.FillRectangle(m.middle_x + p.thickness, m.middle_y + p.thickness, m.middle_x + p.thickness, m.bottom);
	}

	static void Draw_2534_new(Painter &p, unsigned int start_y, unsigned int cx) /* ┴ */
	{
		SingleLineBoxMetrics m(p, start_y, cx);
		p.SetColorSoften();
		p.FillRectangle(m.left, m.middle_y, m.right, m.middle_y + p.thickness - 1);
		p.FillRectangle(m.middle_x, m.top, m.middle_x + p.thickness - 1, m.middle_y);
		p.SetColorEmboss();
		p.FillRectangle(m.left, m.middle_y + p.thickness, m.right, m.middle_y + p.thickness);
		p.FillRectangle(m.middle_x + p.thickness, m.top, m.middle_x + p.thickness, m.middle_y - 1);
	}

	static void Draw_253C_new(Painter &p, unsigned int start_y, unsigned int cx) /* ┼ */
	{
		SingleLineBoxMetrics m(p, start_y, cx);
		p.SetColorSoften();
		p.FillRectangle(m.left, m.middle_y, m.right, m.middle_y + p.thickness - 1);
		p.FillRectangle(m.middle_x, m.top, m.middle_x + p.thickness - 1, m.bottom);
		p.SetColorEmboss();
		p.FillRectangle(m.left, m.middle_y + p.thickness, m.middle_x - 1, m.middle_y + p.thickness);
		p.FillRectangle(m.middle_x + p.thickness, m.middle_y + p.thickness - 1, m.right, m.middle_y + p.thickness);
		p.FillRectangle(m.middle_x + p.thickness, m.top, m.middle_x - 1, m.middle_y - 1);
		p.FillRectangle(m.middle_x + p.thickness, m.middle_y + p.thickness, m.middle_x + p.thickness, m.bottom);
	}

    /* double lines */

	static void Draw_2550(Painter &p, unsigned int start_y, unsigned int cx) /* ═ */
	{
		DoubleLineBoxMetrics m(p, start_y, cx);
		p.FillRectangle(m.left, m.middle1_y, m.right, m.middle1_y + p.thickness - 1);
		p.FillRectangle(m.left, m.middle2_y, m.right, m.middle2_y + p.thickness - 1);
		if (p.MayDrawFadedEdges()) {
			p.SetColorFaded();
			p.FillRectangle(m.left, m.middle1_y - 1, m.right, m.middle1_y - 1);
			p.FillRectangle(m.left, m.middle2_y - 1, m.right, m.middle2_y - 1);
		}
	}


	static void Draw_2551(Painter &p, unsigned int start_y, unsigned int cx) /* ║ */
	{
		DoubleLineBoxMetrics m(p, start_y, cx);
		p.FillRectangle(m.middle1_x, m.top, m.middle1_x + p.thickness - 1, m.bottom);
		p.FillRectangle(m.middle2_x, m.top, m.middle2_x + p.thickness - 1, m.bottom);
		if (p.MayDrawFadedEdges()) {
			p.SetColorFaded();
			p.FillRectangle(m.middle1_x - 1, m.top, m.middle1_x - 1, m.bottom);
			p.FillRectangle(m.middle2_x - 1, m.top, m.middle2_x - 1, m.bottom);
		}
	}


	static void Draw_2554(Painter &p, unsigned int start_y, unsigned int cx) /* ╔ */
	{
		DoubleLineBoxMetrics m(p, start_y, cx);
		p.FillRectangle(m.middle1_x, m.middle1_y, m.right, m.middle1_y + p.thickness - 1);
		p.FillRectangle(m.middle2_x, m.middle2_y, m.right, m.middle2_y + p.thickness - 1);

		p.FillRectangle(m.middle1_x, m.middle1_y, m.middle1_x + p.thickness - 1, m.bottom);
		p.FillRectangle(m.middle2_x, m.middle2_y, m.middle2_x + p.thickness - 1, m.bottom);

		if (p.MayDrawFadedEdges()) {
			p.SetColorFaded();
			p.FillRectangle(m.middle1_x, m.middle1_y - 1, m.right, m.middle1_y - 1);
			p.FillRectangle(m.middle2_x, m.middle2_y - 1, m.right, m.middle2_y - 1);

			p.FillRectangle(m.middle1_x - 1, m.middle1_y, m.middle1_x - 1, m.bottom);
			p.FillRectangle(m.middle2_x - 1, m.middle2_y, m.middle2_x - 1, m.bottom);

			p.SetColorExtraFaded();
			p.FillPixel(m.middle1_x - 1, m.middle1_y - 1);
			p.FillPixel(m.middle2_x - 1, m.middle2_y - 1);
		}
	}


	static void Draw_2557(Painter &p, unsigned int start_y, unsigned int cx) /* ╗ */
	{
		DoubleLineBoxMetrics m(p, start_y, cx);
		p.FillRectangle(m.left, m.middle1_y, m.middle2_x, m.middle1_y + p.thickness - 1);
		p.FillRectangle(m.left, m.middle2_y, m.middle1_x, m.middle2_y + p.thickness - 1);

		p.FillRectangle(m.middle2_x, m.middle1_y, m.middle2_x + p.thickness - 1, m.bottom);
		p.FillRectangle(m.middle1_x, m.middle2_y, m.middle1_x + p.thickness - 1, m.bottom);
		if (p.MayDrawFadedEdges()) {
			p.SetColorFaded();
			p.FillRectangle(m.left, m.middle1_y - 1, m.middle2_x + p.thickness - 1, m.middle1_y - 1);
			p.FillRectangle(m.left, m.middle2_y - 1, m.middle1_x + p.thickness - 1, m.middle2_y - 1);

			p.FillRectangle(m.middle2_x - 1, m.middle1_y + p.thickness, m.middle2_x - 1, m.bottom);
			p.FillRectangle(m.middle1_x - 1, m.middle2_y + p.thickness, m.middle1_x - 1, m.bottom);
		}
	}


	static void Draw_255A(Painter &p, unsigned int start_y, unsigned int cx) /* ╚ */
	{
		DoubleLineBoxMetrics m(p, start_y, cx);
		p.FillRectangle(m.middle2_x, m.middle1_y, m.right, m.middle1_y + p.thickness - 1);
		p.FillRectangle(m.middle1_x, m.middle2_y, m.right, m.middle2_y + p.thickness - 1);

		p.FillRectangle(m.middle1_x, m.top, m.middle1_x + p.thickness - 1, m.middle2_y);
		p.FillRectangle(m.middle2_x, m.top, m.middle2_x + p.thickness - 1, m.middle1_y);

		if (p.MayDrawFadedEdges()) {
			p.SetColorFaded();
			p.FillRectangle(m.middle2_x + p.thickness, m.middle1_y - 1, m.right, m.middle1_y - 1);
			p.FillRectangle(m.middle1_x + p.thickness, m.middle2_y - 1, m.right, m.middle2_y - 1);

			p.FillRectangle(m.middle1_x - 1, m.top, m.middle1_x - 1, m.middle2_y + p.thickness - 1);
			p.FillRectangle(m.middle2_x - 1, m.top, m.middle2_x - 1, m.middle1_y + p.thickness - 1);
		}
	}


	static void Draw_255D(Painter &p, unsigned int start_y, unsigned int cx) /* ╝ */ // + thickness
	{
		DoubleLineBoxMetrics m(p, start_y, cx);
		p.FillRectangle(m.left, m.middle1_y, m.middle1_x + p.thickness - 1, m.middle1_y + p.thickness - 1);
		p.FillRectangle(m.left, m.middle2_y, m.middle2_x + p.thickness - 1, m.middle2_y + p.thickness - 1);

		p.FillRectangle(m.middle1_x, m.top, m.middle1_x + p.thickness - 1, m.middle1_y);
		p.FillRectangle(m.middle2_x, m.top, m.middle2_x + p.thickness - 1, m.middle2_y);

		if (p.MayDrawFadedEdges()) {
			p.FillPixel(m.middle1_x - 1, m.middle1_y - 1);
			p.FillPixel(m.middle2_x - 1, m.middle2_y - 1);

			p.SetColorFaded();
			p.FillRectangle(m.left, m.middle1_y - 1, m.middle1_x - 2, m.middle1_y - 1);
			p.FillRectangle(m.left, m.middle2_y - 1, m.middle2_x - 2, m.middle2_y - 1);

			p.FillRectangle(m.middle1_x - 1, m.top, m.middle1_x - 1, m.middle1_y - 2);
			p.FillRectangle(m.middle2_x - 1, m.top, m.middle2_x - 1, m.middle2_y - 2);
		}
	}


	static void Draw_255F(Painter &p, unsigned int start_y, unsigned int cx) /* ╟ */
	{
		DoubleLineBoxMetrics m(p, start_y, cx);
		p.FillRectangle(m.middle2_x, m.middle_y, m.right, m.middle_y + p.thickness - 1);
		p.FillRectangle(m.middle1_x, m.top, m.middle1_x + p.thickness - 1, m.bottom);
		p.FillRectangle(m.middle2_x, m.top, m.middle2_x + p.thickness - 1, m.bottom);
		if (p.MayDrawFadedEdges()) {
			p.SetColorFaded();
			p.FillRectangle(m.middle2_x + p.thickness, m.middle_y - 1, m.right, m.middle_y - 1);
			p.FillRectangle(m.middle1_x - 1, m.top, m.middle1_x - 1, m.bottom);
			p.FillRectangle(m.middle2_x - 1, m.top, m.middle2_x - 1, m.bottom);
		}
	}


	static void Draw_2562(Painter &p, unsigned int start_y, unsigned int cx) /* ╢ */
	{
		DoubleLineBoxMetrics m(p, start_y, cx);
		p.FillRectangle(m.left, m.middle_y, m.middle1_x, m.middle_y + p.thickness - 1);
		p.FillRectangle(m.middle1_x, m.top, m.middle1_x + p.thickness - 1, m.bottom);
		p.FillRectangle(m.middle2_x, m.top, m.middle2_x + p.thickness - 1, m.bottom);


		if (p.MayDrawFadedEdges()) {
			p.FillPixel(m.middle1_x - 1, m.middle_y - 1); // special pixel

			p.SetColorFaded();
			p.FillRectangle(m.left, m.middle_y - 1, m.middle1_x - 2, m.middle_y - 1);
			p.FillRectangle(m.middle1_x - 1, m.top, m.middle1_x - 1, m.middle_y - 2);
			p.FillRectangle(m.middle1_x - 1, m.middle_y + p.thickness, m.middle1_x - 1, m.bottom);
			p.FillRectangle(m.middle2_x - 1, m.top, m.middle2_x - 1, m.bottom);
		}
	}


	///

	static void Draw_2560(Painter &p, unsigned int start_y, unsigned int cx) /* ╠ */
	{
		DoubleLineBoxMetrics m(p, start_y, cx);
		p.FillRectangle(m.middle2_x, m.middle1_y, m.right, m.middle1_y + p.thickness - 1);
		p.FillRectangle(m.middle2_x, m.middle2_y, m.right, m.middle2_y + p.thickness - 1);
		p.FillRectangle(m.middle1_x, m.top, m.middle1_x + p.thickness - 1, m.bottom);
		p.FillRectangle(m.middle2_x, m.top, m.middle2_x + p.thickness - 1, m.middle1_y);
		p.FillRectangle(m.middle2_x, m.middle2_y, m.middle2_x + p.thickness - 1, m.bottom);
		if (p.MayDrawFadedEdges()) {
			p.SetColorFaded();
			p.FillRectangle(m.middle2_x + p.thickness, m.middle1_y - 1, m.right, m.middle1_y - 1);
			p.FillRectangle(m.middle2_x, m.middle2_y - 1, m.right, m.middle2_y - 1);
			p.FillRectangle(m.middle1_x - 1, m.top, m.middle1_x - 1, m.bottom);
			p.FillRectangle(m.middle2_x - 1, m.top, m.middle2_x - 1, m.middle1_y + p.thickness - 1);
			p.FillRectangle(m.middle2_x - 1, m.middle2_y, m.middle2_x - 1, m.bottom);

			p.SetColorExtraFaded();
			p.FillPixel(m.middle2_x - 1, m.middle2_y - 1);
		}
	}


	static void Draw_2563(Painter &p, unsigned int start_y, unsigned int cx) /* ╣ */ // + thickness
	{
		DoubleLineBoxMetrics m(p, start_y, cx);
		p.FillRectangle(m.left, m.middle1_y, m.middle1_x + p.thickness - 1, m.middle1_y + p.thickness - 1);
		p.FillRectangle(m.left, m.middle2_y, m.middle1_x, m.middle2_y + p.thickness - 1);
		p.FillRectangle(m.middle1_x, m.top, m.middle1_x + p.thickness - 1, m.middle1_y);
		p.FillRectangle(m.middle1_x, m.middle2_y, m.middle1_x + p.thickness - 1, m.bottom);
		p.FillRectangle(m.middle2_x, m.top, m.middle2_x + p.thickness - 1, m.bottom);
		if (p.MayDrawFadedEdges()) {
			p.FillPixel(m.middle1_x - 1, m.middle1_y - 1); // special pixel

			p.SetColorFaded();
			p.FillRectangle(m.left, m.middle1_y - 1, m.middle1_x - 2, m.middle1_y - 1);
			p.FillRectangle(m.left, m.middle2_y - 1, m.middle1_x + p.thickness - 1, m.middle2_y - 1);
			p.FillRectangle(m.middle1_x - 1, m.top, m.middle1_x - 1, m.middle1_y - 2);
			p.FillRectangle(m.middle1_x - 1, m.middle2_y + p.thickness, m.middle1_x - 1, m.bottom);
			p.FillRectangle(m.middle2_x - 1, m.top, m.middle2_x - 1, m.bottom);
		}
	}

	static void Draw_2565(Painter &p, unsigned int start_y, unsigned int cx) /* ╥ */
	{
		DoubleLineBoxMetrics m(p, start_y, cx);
		p.FillRectangle(m.left, m.middle_y, m.right, m.middle_y + p.thickness - 1);
		p.FillRectangle(m.middle1_x, m.middle_y, m.middle1_x + p.thickness - 1, m.bottom);
		p.FillRectangle(m.middle2_x, m.middle_y, m.middle2_x + p.thickness - 1, m.bottom);
		if (p.MayDrawFadedEdges()) {
			p.SetColorFaded();
			p.FillRectangle(m.left, m.middle_y - 1, m.right, m.middle_y - 1);
			p.FillRectangle(m.middle1_x - 1, m.middle_y + p.thickness, m.middle1_x - 1, m.bottom);
			p.FillRectangle(m.middle2_x - 1, m.middle_y + p.thickness, m.middle2_x - 1, m.bottom);
		}
	}

	static void Draw_2566(Painter &p, unsigned int start_y, unsigned int cx) /* ╦ */
	{
		DoubleLineBoxMetrics m(p, start_y, cx);
		p.FillRectangle(m.left, m.middle1_y, m.right, m.middle1_y + p.thickness - 1);
		p.FillRectangle(m.left, m.middle2_y, m.middle1_x, m.middle2_y + p.thickness - 1);
		p.FillRectangle(m.middle2_x, m.middle2_y, m.right, m.middle2_y + p.thickness - 1);
		p.FillRectangle(m.middle1_x, m.middle2_y, m.middle1_x + p.thickness - 1, m.bottom);
		p.FillRectangle(m.middle2_x, m.middle2_y, m.middle2_x + p.thickness - 1, m.bottom);
		if (p.MayDrawFadedEdges()) {
			p.SetColorFaded();
			p.FillRectangle(m.left, m.middle1_y - 1, m.right, m.middle1_y - 1);
			p.FillRectangle(m.left, m.middle2_y - 1, m.middle1_x + p.thickness - 1, m.middle2_y - 1);
			p.FillRectangle(m.middle2_x, m.middle2_y - 1, m.right, m.middle2_y - 1);
			p.FillRectangle(m.middle1_x - 1, m.middle2_y + p.thickness, m.middle1_x - 1, m.bottom);
			p.FillRectangle(m.middle2_x - 1, m.middle2_y, m.middle2_x - 1, m.bottom);

			p.SetColorExtraFaded();
			p.FillPixel(m.middle2_x - 1, m.middle2_y - 1); // special pixel
		}
	}

	static void Draw_2567(Painter &p, unsigned int start_y, unsigned int cx) /* ╧ */
	{
		DoubleLineBoxMetrics m(p, start_y, cx);
		p.FillRectangle(m.left, m.middle1_y, m.right, m.middle1_y + p.thickness - 1);
		p.FillRectangle(m.left, m.middle2_y, m.right, m.middle2_y + p.thickness - 1);
		p.FillRectangle(m.middle_x, m.top, m.middle_x + p.thickness - 1, m.middle1_y);
		if (p.MayDrawFadedEdges()) {
			p.FillPixel(m.middle_x - 1, m.middle1_y - 1);

			p.SetColorFaded();
			p.FillRectangle(m.left, m.middle1_y - 1, m.middle_x - 2, m.middle1_y - 1);
			p.FillRectangle(m.middle_x + p.thickness, m.middle1_y - 1, m.right, m.middle1_y - 1);
			p.FillRectangle(m.left, m.middle2_y - 1, m.right, m.middle2_y - 1);
			p.FillRectangle(m.middle_x - 1, m.top, m.middle_x - 1, m.middle1_y - 2);
		}
	}



	static void Draw_2569(Painter &p, unsigned int start_y, unsigned int cx) /* ╩ */ // + thickness
	{
		DoubleLineBoxMetrics m(p, start_y, cx);
		p.FillRectangle(m.left, m.middle1_y, m.middle1_x + p.thickness - 1, m.middle1_y + p.thickness - 1);
		p.FillRectangle(m.middle2_x, m.middle1_y, m.right, m.middle1_y + p.thickness - 1);
		p.FillRectangle(m.left, m.middle2_y, m.right, m.middle2_y + p.thickness - 1);
		p.FillRectangle(m.middle1_x, m.top, m.middle1_x + p.thickness - 1, m.middle1_y);
		p.FillRectangle(m.middle2_x, m.top, m.middle2_x + p.thickness - 1, m.middle1_y);
		if (p.MayDrawFadedEdges()) {
			p.FillPixel(m.middle1_x - 1, m.middle1_y - 1);

			p.SetColorFaded();
			p.FillRectangle(m.left, m.middle1_y - 1, m.middle1_x - 2, m.middle1_y - 1);
			p.FillRectangle(m.middle2_x + p.thickness, m.middle1_y - 1, m.right, m.middle1_y - 1);
			p.FillRectangle(m.left, m.middle2_y - 1, m.right, m.middle2_y - 1);
			p.FillRectangle(m.middle1_x - 1, m.top, m.middle1_x - 1, m.middle1_y - 2);
			p.FillRectangle(m.middle2_x - 1, m.top, m.middle2_x - 1, m.middle1_y + p.thickness - 1);
		}
	}


	static void Draw_256C(Painter &p, unsigned int start_y, unsigned int cx) /* ╬ */ // + thickness
	{
		DoubleLineBoxMetrics m(p, start_y, cx);
		p.FillRectangle(m.left, m.middle1_y, m.middle1_x + p.thickness - 1, m.middle1_y + p.thickness - 1);
		p.FillRectangle(m.middle2_x, m.middle1_y, m.right, m.middle1_y + p.thickness - 1);
		p.FillRectangle(m.left, m.middle2_y, m.middle1_x, m.middle2_y + p.thickness - 1);
		p.FillRectangle(m.middle2_x, m.middle2_y, m.right, m.middle2_y + p.thickness - 1);
		p.FillRectangle(m.middle1_x, m.top, m.middle1_x + p.thickness - 1, m.middle1_y);
		p.FillRectangle(m.middle1_x, m.middle2_y, m.middle1_x + p.thickness - 1, m.bottom);
		p.FillRectangle(m.middle2_x, m.top, m.middle2_x + p.thickness - 1, m.middle1_y);
		p.FillRectangle(m.middle2_x, m.middle2_y, m.middle2_x + p.thickness - 1, m.bottom);

		if (p.MayDrawFadedEdges()) {
			p.FillPixel(m.middle1_x - 1, m.middle1_y - 1);

			p.SetColorFaded();
			p.FillRectangle(m.left, m.middle1_y - 1, m.middle1_x - 2, m.middle1_y - 1); // don't overlap compensation pixel
			p.FillRectangle(m.middle2_x + p.thickness, m.middle1_y - 1, m.right, m.middle1_y - 1);
			p.FillRectangle(m.left, m.middle2_y - 1, m.middle1_x + p.thickness - 1, m.middle2_y - 1);
			p.FillRectangle(m.middle2_x, m.middle2_y - 1, m.right, m.middle2_y - 1);

			p.FillRectangle(m.middle1_x - 1, m.top, m.middle1_x - 1, m.middle1_y - 2); // don't overlap compensation pixel
			p.FillRectangle(m.middle1_x - 1, m.middle2_y + p.thickness, m.middle1_x - 1, m.bottom);
			p.FillRectangle(m.middle2_x - 1, m.top, m.middle2_x - 1, m.middle1_y + p.thickness - 1);
			p.FillRectangle(m.middle2_x - 1, m.middle2_y, m.middle2_x - 1, m.bottom);

			p.SetColorExtraFaded();
			p.FillPixel(m.middle2_x - 1, m.middle2_y - 1);
		}
	}


	static void Draw_2564(Painter &p, unsigned int start_y, unsigned int cx) /* ╤ */
	{
		DoubleLineBoxMetrics m(p, start_y, cx);
		p.FillRectangle(m.left, m.middle1_y, m.right, m.middle1_y + p.thickness - 1);
		p.FillRectangle(m.left, m.middle2_y, m.right, m.middle2_y + p.thickness - 1);
		p.FillRectangle(m.middle_x, m.middle2_y, m.middle_x + p.thickness - 1, m.bottom);
		if (p.MayDrawFadedEdges()) {
			p.SetColorFaded();
			p.FillRectangle(m.left, m.middle1_y - 1, m.right, m.middle1_y - 1);
			p.FillRectangle(m.left, m.middle2_y - 1, m.right, m.middle2_y - 1);
			p.FillRectangle(m.middle_x - 1, m.middle2_y + p.thickness, m.middle_x - 1, m.bottom);
		}
	}

	static void Draw_256A(Painter &p, unsigned int start_y, unsigned int cx) /* ╪ */
	{
		DoubleLineBoxMetrics m(p, start_y, cx);
		p.FillRectangle(m.left, m.middle1_y, m.right, m.middle1_y + p.thickness - 1);
		p.FillRectangle(m.left, m.middle2_y, m.right, m.middle2_y + p.thickness - 1);
		p.FillRectangle(m.middle_x, m.top, m.middle_x + p.thickness - 1, m.bottom);
		if (p.MayDrawFadedEdges()) {
			p.FillPixel(m.middle_x - 1, m.middle1_y - 1); // special pixel
			p.FillPixel(m.middle_x - 1, m.middle2_y - 1); // special pixel

			p.SetColorFaded();
			p.FillRectangle(m.left, m.middle1_y - 1, m.middle_x - 2, m.middle1_y - 1);
			p.FillRectangle(m.middle_x + p.thickness, m.middle1_y - 1, m.right, m.middle1_y - 1);
			p.FillRectangle(m.left, m.middle2_y - 1, m.middle_x - 2, m.middle2_y - 1);
			p.FillRectangle(m.middle_x + p.thickness, m.middle2_y - 1, m.right, m.middle2_y - 1);
			p.FillRectangle(m.middle_x - 1, m.top, m.middle_x - 1, m.middle1_y - 2);
			p.FillRectangle(m.middle_x - 1, m.middle1_y + p.thickness, m.middle_x - 1, m.middle2_y - 2);
			p.FillRectangle(m.middle_x - 1, m.middle2_y + p.thickness, m.middle_x - 1, m.bottom);
		}
	}


	static void Draw_256B(Painter &p, unsigned int start_y, unsigned int cx) /* ╫ */
	{
		DoubleLineBoxMetrics m(p, start_y, cx);
		p.FillRectangle(m.left, m.middle_y, m.right, m.middle_y + p.thickness - 1);
		p.FillRectangle(m.middle1_x, m.top, m.middle1_x + p.thickness - 1, m.bottom);
		p.FillRectangle(m.middle2_x, m.top, m.middle2_x + p.thickness - 1, m.bottom);
		if (p.MayDrawFadedEdges()) {
			p.FillPixel(m.middle1_x - 1, m.middle_y - 1); // special pixel
			p.FillPixel(m.middle2_x - 1, m.middle_y - 1); // special pixel

			p.SetColorFaded();

			p.FillRectangle(m.left, m.middle_y - 1, m.middle1_x - 2, m.middle_y - 1);
			p.FillRectangle(m.middle1_x + p.thickness, m.middle_y - 1, m.middle2_x - 2, m.middle_y - 1);
			p.FillRectangle(m.middle2_x + p.thickness, m.middle_y - 1, m.right, m.middle_y - 1);

			p.FillRectangle(m.middle1_x - 1, m.top, m.middle1_x - 1, m.middle_y - 2);
			p.FillRectangle(m.middle1_x - 1, m.middle_y + p.thickness, m.middle1_x - 1, m.bottom);

			p.FillRectangle(m.middle2_x - 1, m.top, m.middle2_x - 1, m.middle_y - 2);
			p.FillRectangle(m.middle2_x - 1, m.middle_y + p.thickness, m.middle2_x - 1, m.bottom);
		}
	}


	static void Draw_2561(Painter &p, unsigned int start_y, unsigned int cx) /* ╡ */ // + p.thickness - 1
	{
		DoubleLineBoxMetrics m(p, start_y, cx);
		p.FillRectangle(m.left, m.middle1_y, m.middle_x + p.thickness - 1, m.middle1_y + p.thickness - 1);
		p.FillRectangle(m.left, m.middle2_y, m.middle_x, m.middle2_y + p.thickness - 1);
		p.FillRectangle(m.middle_x, m.top, m.middle_x + p.thickness - 1, m.middle1_y);
		p.FillRectangle(m.middle_x, m.middle2_y, m.middle_x + p.thickness - 1, m.bottom);
		if (p.MayDrawFadedEdges()) {
			p.FillPixel(m.middle_x - 1, m.middle1_y - 1); // special pixel

			p.SetColorFaded();
			p.FillRectangle(m.left, m.middle1_y - 1, m.middle_x - 2, m.middle1_y - 1);
			p.FillRectangle(m.left, m.middle2_y - 1, m.middle_x + p.thickness - 1, m.middle2_y - 1);
			p.FillRectangle(m.middle_x - 1, m.top, m.middle_x - 1, m.middle1_y - 2);
			p.FillRectangle(m.middle_x - 1, m.middle2_y + p.thickness, m.middle_x - 1, m.bottom);
		}
	}


	static void Draw_255E(Painter &p, unsigned int start_y, unsigned int cx) /* ╞ */
	{
		DoubleLineBoxMetrics m(p, start_y, cx);
		p.FillRectangle(m.middle_x, m.middle1_y, m.right, m.middle1_y + p.thickness - 1);
		p.FillRectangle(m.middle_x, m.middle2_y, m.right, m.middle2_y + p.thickness - 1);
		p.FillRectangle(m.middle_x, m.top, m.middle_x + p.thickness - 1, m.middle1_y);
		p.FillRectangle(m.middle_x, m.middle2_y, m.middle_x + p.thickness - 1, m.bottom);
		if (p.MayDrawFadedEdges()) {
			p.SetColorFaded();
			p.FillRectangle(m.middle_x + p.thickness, m.middle1_y - 1, m.right, m.middle1_y - 1);
			p.FillRectangle(m.middle_x, m.middle2_y - 1, m.right, m.middle2_y - 1);
			p.FillRectangle(m.middle_x - 1, m.top, m.middle_x - 1, m.middle1_y + p.thickness - 1);
			p.FillRectangle(m.middle_x - 1, m.middle2_y, m.middle_x - 1, m.bottom);

			p.SetColorExtraFaded();
			p.FillPixel(m.middle_x - 1, m.middle2_y - 1);
		}
	}


	static void Draw_2556(Painter &p, unsigned int start_y, unsigned int cx) /* ╖ */
	{
		DoubleLineBoxMetrics m(p, start_y, cx);
		p.FillRectangle(m.left, m.middle_y, m.middle2_x, m.middle_y + p.thickness - 1);
		p.FillRectangle(m.middle1_x, m.middle_y, m.middle1_x + p.thickness - 1, m.bottom);
		p.FillRectangle(m.middle2_x, m.middle_y, m.middle2_x + p.thickness - 1, m.bottom);
		if (p.MayDrawFadedEdges()) {
			p.SetColorFaded();
			p.FillRectangle(m.left, m.middle_y - 1, m.middle2_x + p.thickness - 1, m.middle_y - 1);
			p.FillRectangle(m.middle1_x - 1, m.middle_y + p.thickness, m.middle1_x - 1, m.bottom);
			p.FillRectangle(m.middle2_x - 1, m.middle_y + p.thickness, m.middle2_x - 1, m.bottom);
		}
	}


	static void Draw_2555(Painter &p, unsigned int start_y, unsigned int cx) /* ╕ */
	{
		DoubleLineBoxMetrics m(p, start_y, cx);
		p.FillRectangle(m.left, m.middle1_y, m.middle_x + p.thickness - 1, m.middle1_y + p.thickness - 1);
		p.FillRectangle(m.left, m.middle2_y, m.middle_x, m.middle2_y + p.thickness - 1);
		p.FillRectangle(m.middle_x, m.middle1_y, m.middle_x + p.thickness - 1, m.bottom);
		if (p.MayDrawFadedEdges()) {
			p.FillPixel(m.middle_x - 1, m.middle2_y - 1); // special pixel

			p.SetColorFaded();
			p.FillRectangle(m.left, m.middle1_y - 1, m.middle_x + p.thickness - 1, m.middle1_y - 1);
			p.FillRectangle(m.left, m.middle2_y - 1, m.middle_x - 2, m.middle2_y - 1);
			p.FillRectangle(m.middle_x - 1, m.middle1_y + p.thickness, m.middle_x - 1, m.middle2_y - 2);
			p.FillRectangle(m.middle_x - 1, m.middle2_y + p.thickness, m.middle_x - 1, m.bottom);
		}
	}


	static void Draw_2568(Painter &p, unsigned int start_y, unsigned int cx) /* ╨ */
	{
		DoubleLineBoxMetrics m(p, start_y, cx);
		p.FillRectangle(m.left, m.middle_y, m.right, m.middle_y + p.thickness - 1);
		p.FillRectangle(m.middle1_x, m.top, m.middle1_x + p.thickness - 1, m.middle_y);
		p.FillRectangle(m.middle2_x, m.top, m.middle2_x + p.thickness - 1, m.middle_y);
		if (p.MayDrawFadedEdges()) {
			p.FillPixel(m.middle1_x - 1, m.middle_y - 1);
			p.FillPixel(m.middle2_x - 1, m.middle_y - 1);

			p.SetColorFaded();
			p.FillRectangle(m.left, m.middle_y - 1, m.middle1_x - 2, m.middle_y - 1);
			p.FillRectangle(m.middle1_x + p.thickness, m.middle_y - 1, m.middle2_x - 2, m.middle_y - 1);
			p.FillRectangle(m.middle2_x + p.thickness, m.middle_y - 1, m.right, m.middle_y - 1);
			p.FillRectangle(m.middle1_x - 1, m.top, m.middle1_x - 1, m.middle_y - 2);
			p.FillRectangle(m.middle2_x - 1, m.top, m.middle2_x - 1, m.middle_y - 2);
		}
	}


	static void Draw_255C(Painter &p, unsigned int start_y, unsigned int cx) /* ╜ */ // + thickness
	{
		DoubleLineBoxMetrics m(p, start_y, cx);
		p.FillRectangle(m.left, m.middle_y, m.middle2_x + p.thickness - 1, m.middle_y + p.thickness - 1);
		p.FillRectangle(m.middle1_x, m.top, m.middle1_x + p.thickness - 1, m.middle_y);
		p.FillRectangle(m.middle2_x, m.top, m.middle2_x + p.thickness - 1, m.middle_y);
		if (p.MayDrawFadedEdges()) {
			p.FillPixel(m.middle1_x - 1, m.middle_y - 1);
			p.FillPixel(m.middle2_x - 1, m.middle_y - 1);

			p.SetColorFaded();
			p.FillRectangle(m.left, m.middle_y - 1, m.middle1_x - 2, m.middle_y - 1);
			p.FillRectangle(m.middle1_x + p.thickness, m.middle_y - 1, m.middle2_x - 2, m.middle_y - 1);
			p.FillRectangle(m.middle1_x - 1, m.top, m.middle1_x - 1, m.middle_y - 2);
			p.FillRectangle(m.middle2_x - 1, m.top, m.middle2_x - 1, m.middle_y - 2);
		}
	}


	static void Draw_2559(Painter &p, unsigned int start_y, unsigned int cx) /* ╙ */
	{
		DoubleLineBoxMetrics m(p, start_y, cx);
		p.FillRectangle(m.middle1_x, m.middle_y, m.right, m.middle_y + p.thickness - 1);
		p.FillRectangle(m.middle1_x, m.top, m.middle1_x + p.thickness - 1, m.middle_y);
		p.FillRectangle(m.middle2_x, m.top, m.middle2_x + p.thickness - 1, m.middle_y);
		if (p.MayDrawFadedEdges()) {
			p.FillPixel(m.middle2_x - 1, m.middle_y - 1);

			p.SetColorFaded();
			p.FillRectangle(m.middle1_x + p.thickness, m.middle_y - 1, m.middle2_x - 2, m.middle_y - 1);
			p.FillRectangle(m.middle2_x + p.thickness, m.middle_y - 1, m.right, m.middle_y - 1);

			p.FillRectangle(m.middle1_x - 1, m.top, m.middle1_x - 1, m.middle_y + p.thickness - 1);
			p.FillRectangle(m.middle2_x - 1, m.top, m.middle2_x - 1, m.middle_y - 2);
		}
	}


	static void Draw_2558(Painter &p, unsigned int start_y, unsigned int cx) /* ╘ */
	{
		DoubleLineBoxMetrics m(p, start_y, cx);
		p.FillRectangle(m.middle_x, m.middle1_y, m.right, m.middle1_y + p.thickness - 1);
		p.FillRectangle(m.middle_x, m.middle2_y, m.right, m.middle2_y + p.thickness - 1);
		p.FillRectangle(m.middle_x, m.top, m.middle_x + p.thickness - 1, m.middle2_y);
		if (p.MayDrawFadedEdges()) {
			p.SetColorFaded();
			p.FillRectangle(m.middle_x + p.thickness, m.middle1_y - 1, m.right, m.middle1_y - 1);
			p.FillRectangle(m.middle_x + p.thickness, m.middle2_y - 1, m.right, m.middle2_y - 1);
			p.FillRectangle(m.middle_x - 1, m.top, m.middle_x - 1, m.middle2_y + p.thickness - 1);
		}
	}


	static void Draw_2552(Painter &p, unsigned int start_y, unsigned int cx) /* ╒ */
	{
		DoubleLineBoxMetrics m(p, start_y, cx);
		p.FillRectangle(m.middle_x, m.middle1_y, m.right, m.middle1_y + p.thickness - 1);
		p.FillRectangle(m.middle_x, m.middle2_y, m.right, m.middle2_y + p.thickness - 1);
		p.FillRectangle(m.middle_x, m.middle1_y, m.middle_x + p.thickness - 1, m.bottom);
		if (p.MayDrawFadedEdges()) {
			p.SetColorFaded();
			p.FillRectangle(m.middle_x, m.middle1_y - 1, m.right, m.middle1_y - 1);
			p.FillRectangle(m.middle_x + p.thickness, m.middle2_y - 1, m.right, m.middle2_y - 1);
			p.FillRectangle(m.middle_x - 1, m.middle1_y, m.middle_x - 1, m.bottom);

			p.SetColorExtraFaded();
			p.FillPixel(m.middle_x - 1, m.middle1_y - 1);
		}
	}


	static void Draw_2553(Painter &p, unsigned int start_y, unsigned int cx) /* ╓ */
	{
		DoubleLineBoxMetrics m(p, start_y, cx);
		p.FillRectangle(m.middle1_x, m.middle_y, m.right, m.middle_y + p.thickness - 1);
		p.FillRectangle(m.middle1_x, m.middle_y, m.middle1_x + p.thickness - 1, m.bottom);
		p.FillRectangle(m.middle2_x, m.middle_y, m.middle2_x + p.thickness - 1, m.bottom);
		if (p.MayDrawFadedEdges()) {
			p.SetColorFaded();
			p.FillRectangle(m.middle1_x, m.middle_y - 1, m.right, m.middle_y - 1);
			p.FillRectangle(m.middle1_x - 1, m.middle_y, m.middle1_x - 1, m.bottom);
			p.FillRectangle(m.middle2_x - 1, m.middle_y + p.thickness, m.middle2_x - 1, m.bottom);

			p.SetColorExtraFaded();
			p.FillPixel(m.middle1_x - 1, m.middle_y - 1);
		}
	}

	static void Draw_255B(Painter &p, unsigned int start_y, unsigned int cx) /* ╛*/ // + thickness
	{
		DoubleLineBoxMetrics m(p, start_y, cx);
		p.FillRectangle(m.left, m.middle1_y, m.middle_x, m.middle1_y + p.thickness - 1);
		p.FillRectangle(m.left, m.middle2_y, m.middle_x, m.middle2_y + p.thickness - 1);
		p.FillRectangle(m.middle_x, m.top, m.middle_x + p.thickness - 1, m.middle2_y + p.thickness - 1);
		if (p.MayDrawFadedEdges()) {
			p.FillPixel(m.middle_x - 1, m.middle1_y - 1);
			p.FillPixel(m.middle_x - 1, m.middle2_y - 1);

			p.SetColorFaded();
			p.FillRectangle(m.left, m.middle1_y - 1, m.middle_x - 2, m.middle1_y - 1);
			p.FillRectangle(m.left, m.middle2_y - 1, m.middle_x - 2, m.middle2_y - 1);
			p.FillRectangle(m.middle_x - 1, m.top, m.middle_x - 1, m.middle1_y - 2);
			p.FillRectangle(m.middle_x - 1, m.middle1_y + p.thickness, m.middle_x - 1, m.middle2_y - 2);
		}
	}

    /* double lines modern look */

	static void Draw_2550_new(Painter &p, unsigned int start_y, unsigned int cx) /* ═ */
	{
		DoubleLineBoxMetrics m(p, start_y, cx);
		p.SetColorSoften();
		p.FillRectangle(m.left, m.middle1_y, m.right, m.middle1_y + p.thickness - 1);
		p.FillRectangle(m.left, m.middle2_y, m.right, m.middle2_y + p.thickness - 1);
		p.SetColorEmboss();
		p.FillRectangle(m.left, m.middle1_y + p.thickness, m.right, m.middle1_y + p.thickness);
		p.FillRectangle(m.left, m.middle2_y + p.thickness, m.right, m.middle2_y + p.thickness);
	}

	static void Draw_2551_new(Painter &p, unsigned int start_y, unsigned int cx) /* ║ */
	{
		DoubleLineBoxMetrics m(p, start_y, cx);
		p.SetColorSoften();
		p.FillRectangle(m.middle1_x, m.top, m.middle1_x + p.thickness - 1, m.bottom);
		p.FillRectangle(m.middle2_x, m.top, m.middle2_x + p.thickness - 1, m.bottom);
		p.SetColorEmboss();
		p.FillRectangle(m.middle1_x + p.thickness, m.top, m.middle1_x + p.thickness, m.bottom);
		p.FillRectangle(m.middle2_x + p.thickness, m.top, m.middle2_x + p.thickness, m.bottom);
	}

	static inline void DrawDoubleEllipseHiPart(Painter &p, DoubleLineBoxMetrics &m, double start, double end, bool left, bool top, int thickness = 0)
	{
        int wx1 = m.right - m.middle1_x - thickness;
        int wy1 = m.bottom - m.middle1_y - thickness;
		p.DrawEllipticArc(
			(left ? m.left - wx1 : m.middle1_x + thickness), 
			(top ? m.top - wy1 : m.middle1_y + thickness), 
			wx1 * 2,
			wy1 * 2, 
			start, 
			end, 
			p.thickness);
	}

	static inline void DrawDoubleEllipseLoPart(Painter &p, DoubleLineBoxMetrics &m, double start, double end, bool left, bool top, int thickness = 0)
	{
        int wx2 = m.right - m.middle2_x - thickness;
        int wy2 = m.bottom - m.middle2_y - thickness;
		p.DrawEllipticArc(
			(left ? m.left - wx2 : m.middle2_x + thickness), 
			(top ? m.top - wy2 : m.middle2_y + thickness), 
			wx2 * 2,
			wy2 * 2, 
			start, 
			end,
			p.thickness);
	}

	static inline void DrawDoubleEllipse(Painter &p, DoubleLineBoxMetrics &m, double start, double end, bool left, bool top)
	{
		DrawDoubleEllipseHiPart(p, m, start, end, left, top);
		DrawDoubleEllipseLoPart(p, m, start, end, left, top);
	}

	static inline void DrawDoubleEllipseHiPartEmboss(Painter &p, DoubleLineBoxMetrics &m, double start, double end, bool left, bool top)
	{
		p.SaveBrush();
		p.SetColorEmboss();
		DrawDoubleEllipseHiPart(p, m, start, end, left, top, 2);
		p.RestoreBrush();
	}

	static inline void DrawDoubleEllipseLoPartEmboss(Painter &p, DoubleLineBoxMetrics &m, double start, double end, bool left, bool top)
	{
		DrawDoubleEllipseLoPart(p, m, start, end, left, top, 2);
	}

	static inline void DrawDoubleEllipseEmboss(Painter &p, DoubleLineBoxMetrics &m, double start, double end, bool left, bool top)
	{
		DrawDoubleEllipseHiPartEmboss(p, m, start, end, left, top);
		DrawDoubleEllipseLoPartEmboss(p, m, start, end, left, top);
	}

	static void Draw_2554_new(Painter &p, unsigned int start_y, unsigned int cx) /* ╔ */
	{
		DoubleLineBoxMetrics m(p, start_y, cx);
		if (SDLBackend::options && SDLBackend::options->UseRoundedBorders) {
			DrawDoubleEllipseEmboss(p, m, 90, 180, false, false);
			p.SetColorSoften();
			DrawDoubleEllipse(p, m, 90, 180, false, false);
		}
		else {
			p.SetColorSoften();
			p.FillRectangle(m.middle1_x, m.middle1_y, m.right, m.middle1_y + p.thickness - 1);
			p.FillRectangle(m.middle2_x, m.middle2_y, m.right, m.middle2_y + p.thickness - 1);
			p.FillRectangle(m.middle1_x, m.middle1_y, m.middle1_x + p.thickness - 1, m.bottom);
			p.FillRectangle(m.middle2_x, m.middle2_y, m.middle2_x + p.thickness - 1, m.bottom);
			p.SetColorEmboss();
			p.FillRectangle(m.middle1_x + p.thickness, m.middle1_y + p.thickness, m.right, m.middle1_y + p.thickness);
			p.FillRectangle(m.middle2_x + p.thickness, m.middle2_y + p.thickness, m.right, m.middle2_y + p.thickness);
			p.FillRectangle(m.middle1_x + p.thickness, m.middle1_y + p.thickness, m.middle1_x + p.thickness, m.bottom);
			p.FillRectangle(m.middle2_x + p.thickness, m.middle2_y + p.thickness, m.middle2_x + p.thickness , m.bottom);
		}
	}

	static void Draw_2557_new(Painter &p, unsigned int start_y, unsigned int cx) /* ╗ */
	{
		DoubleLineBoxMetrics m(p, start_y, cx);
		if (SDLBackend::options && SDLBackend::options->UseRoundedBorders) {
			DrawDoubleEllipseEmboss(p, m, 0, 90, true, false);
			p.SetColorSoften();
			DrawDoubleEllipse(p, m, 0, 90, true, false);
		}
		else {
			p.SetColorSoften();
			p.FillRectangle(m.left, m.middle1_y, m.middle2_x, m.middle1_y + p.thickness - 1);
			p.FillRectangle(m.left, m.middle2_y, m.middle1_x, m.middle2_y + p.thickness - 1);
			p.FillRectangle(m.middle2_x, m.middle1_y, m.middle2_x + p.thickness - 1, m.bottom);
			p.FillRectangle(m.middle1_x, m.middle2_y, m.middle1_x + p.thickness - 1, m.bottom);
			p.SetColorEmboss();
			p.FillRectangle(m.left, m.middle1_y + p.thickness, m.middle2_x + p.thickness, m.middle1_y + p.thickness);
			p.FillRectangle(m.left, m.middle2_y + p.thickness, m.middle1_x + p.thickness, m.middle2_y + p.thickness);
			p.FillRectangle(m.middle2_x + p.thickness, m.middle1_y + p.thickness, m.middle2_x + p.thickness, m.bottom);
			p.FillRectangle(m.middle1_x + p.thickness, m.middle2_y + p.thickness, m.middle1_x + p.thickness, m.bottom);
		}
	}

	static void Draw_255A_new(Painter &p, unsigned int start_y, unsigned int cx) /* ╚ */
	{
		DoubleLineBoxMetrics m(p, start_y, cx);
		if (SDLBackend::options && SDLBackend::options->UseRoundedBorders) {
			DrawDoubleEllipseEmboss(p, m, 180, 270, false, true);
			p.SetColorSoften();
			DrawDoubleEllipse(p, m, 180, 270, false, true);
		}
		else {
			p.SetColorSoften();
			p.FillRectangle(m.middle2_x, m.middle1_y, m.right, m.middle1_y + p.thickness - 1);
			p.FillRectangle(m.middle1_x, m.middle2_y, m.right, m.middle2_y + p.thickness - 1);
			p.FillRectangle(m.middle1_x, m.top, m.middle1_x + p.thickness - 1, m.middle2_y);
			p.FillRectangle(m.middle2_x, m.top, m.middle2_x + p.thickness - 1, m.middle1_y);
			p.SetColorEmboss();
			p.FillRectangle(m.middle2_x + p.thickness, m.middle1_y + p.thickness, m.right, m.middle1_y + p.thickness);
			p.FillRectangle(m.middle1_x + p.thickness, m.middle2_y + p.thickness, m.right, m.middle2_y + p.thickness);
			p.FillRectangle(m.middle1_x + p.thickness, m.top, m.middle1_x + p.thickness, m.middle2_y + p.thickness);
			p.FillRectangle(m.middle2_x + p.thickness, m.top, m.middle2_x + p.thickness, m.middle1_y + p.thickness);
		}
	}

	static void Draw_255D_new(Painter &p, unsigned int start_y, unsigned int cx) /* ╝ */ // + thickness
	{
		DoubleLineBoxMetrics m(p, start_y, cx);
		p.SetColorSoften();
		if (SDLBackend::options && SDLBackend::options->UseRoundedBorders) {
			DrawDoubleEllipseEmboss(p, m, 270, 360, true, true);
			DrawDoubleEllipse(p, m, 270, 360, true, true);
		}
		else {
			p.FillRectangle(m.left, m.middle1_y, m.middle1_x + p.thickness - 1, m.middle1_y + p.thickness - 1);
			p.FillRectangle(m.left, m.middle2_y, m.middle2_x + p.thickness - 1, m.middle2_y + p.thickness - 1);
			p.FillRectangle(m.middle1_x, m.top, m.middle1_x + p.thickness - 1, m.middle1_y);
			p.FillRectangle(m.middle2_x, m.top, m.middle2_x + p.thickness - 1, m.middle2_y);
			p.SetColorEmboss();
			p.FillRectangle(m.left, m.middle1_y + p.thickness, m.middle1_x + p.thickness, m.middle1_y + p.thickness);
			p.FillRectangle(m.left, m.middle2_y + p.thickness, m.middle2_x + p.thickness, m.middle2_y + p.thickness);
			p.FillRectangle(m.middle1_x + p.thickness, m.top, m.middle1_x + p.thickness, m.middle1_y + p.thickness);
			p.FillRectangle(m.middle2_x + p.thickness, m.top, m.middle2_x + p.thickness, m.middle2_y + p.thickness);
		}
	}

	static void Draw_255F_new(Painter &p, unsigned int start_y, unsigned int cx) /* ╟ */
	{
		DoubleLineBoxMetrics m(p, start_y, cx);
		p.SetColorSoften();
		p.FillRectangle(m.middle2_x, m.middle_y, m.right, m.middle_y + p.thickness - 1);
		p.FillRectangle(m.middle1_x, m.top, m.middle1_x + p.thickness - 1, m.bottom);
		p.FillRectangle(m.middle2_x, m.top, m.middle2_x + p.thickness - 1, m.bottom);
		p.SetColorEmboss();
		p.FillRectangle(m.middle1_x + p.thickness, m.top, m.middle1_x + p.thickness, m.bottom);
		p.FillRectangle(m.middle2_x + p.thickness, m.middle_y + p.thickness, m.right, m.middle_y + p.thickness);
		p.FillRectangle(m.middle2_x + p.thickness, m.top, m.middle2_x + p.thickness, m.middle2_y - 1);
		p.FillRectangle(m.middle2_x + p.thickness, m.middle2_y + p.thickness, m.middle2_x + p.thickness, m.bottom);
	}

	static void Draw_2562_new(Painter &p, unsigned int start_y, unsigned int cx) /* ╢ */
	{
		DoubleLineBoxMetrics m(p, start_y, cx);
		p.SetColorSoften();
		p.FillRectangle(m.left, m.middle_y, m.middle1_x, m.middle_y + p.thickness - 1);
		p.FillRectangle(m.middle1_x, m.top, m.middle1_x + p.thickness - 1, m.bottom);
		p.FillRectangle(m.middle2_x, m.top, m.middle2_x + p.thickness - 1, m.bottom);
		p.SetColorEmboss();
		p.FillRectangle(m.left, m.middle_y + p.thickness, m.middle1_x - 1, m.middle_y + p.thickness);
		p.FillRectangle(m.middle1_x + p.thickness, m.top, m.middle1_x + p.thickness, m.bottom);
		p.FillRectangle(m.middle2_x + p.thickness, m.top, m.middle2_x + p.thickness, m.bottom);
	}

	static void Draw_2560_new(Painter &p, unsigned int start_y, unsigned int cx) /* ╠ */
	{
		DoubleLineBoxMetrics m(p, start_y, cx);
		p.SetColorSoften();
		if (SDLBackend::options && SDLBackend::options->UseRoundedBorders) {
			p.FillRectangle(m.middle1_x, m.top, m.middle1_x + p.thickness - 1, m.bottom);
			DrawDoubleEllipseLoPartEmboss(p, m, 180, 270, false, true);
			DrawDoubleEllipseLoPartEmboss(p, m, 90, 180, false, false);
			DrawDoubleEllipseLoPart(p, m, 180, 270, false, true);
			DrawDoubleEllipseLoPart(p, m, 90, 180, false, false);
			p.SetColorEmboss();
			p.FillRectangle(m.middle1_x + p.thickness, m.top, m.middle1_x + p.thickness, m.bottom);
		}
		else {
			p.FillRectangle(m.middle2_x, m.middle1_y, m.right, m.middle1_y + p.thickness - 1);
			p.FillRectangle(m.middle2_x, m.middle2_y, m.right, m.middle2_y + p.thickness - 1);
			p.FillRectangle(m.middle1_x, m.top, m.middle1_x + p.thickness - 1, m.bottom);
			p.FillRectangle(m.middle2_x, m.top, m.middle2_x + p.thickness - 1, m.middle1_y);
			p.FillRectangle(m.middle2_x, m.middle2_y, m.middle2_x + p.thickness - 1, m.bottom);
			p.SetColorEmboss();
			p.FillRectangle(m.middle2_x + p.thickness, m.middle1_y + p.thickness, m.right, m.middle1_y + p.thickness);
			p.FillRectangle(m.middle2_x + p.thickness, m.middle2_y + p.thickness, m.right, m.middle2_y + p.thickness);
			p.FillRectangle(m.middle1_x + p.thickness, m.top, m.middle1_x + p.thickness, m.bottom);
			p.FillRectangle(m.middle2_x + p.thickness, m.top, m.middle2_x + p.thickness, m.middle1_y + p.thickness);
			p.FillRectangle(m.middle2_x + p.thickness, m.middle2_y + p.thickness, m.middle2_x + p.thickness, m.bottom);
		}
	}

	static void Draw_2563_new(Painter &p, unsigned int start_y, unsigned int cx) /* ╣ */ // + thickness
	{
		DoubleLineBoxMetrics m(p, start_y, cx);
		p.SetColorSoften();
		if (SDLBackend::options && SDLBackend::options->UseRoundedBorders) {
			p.FillRectangle(m.middle2_x, m.top, m.middle2_x + p.thickness - 1, m.bottom);
			DrawDoubleEllipseLoPartEmboss(p, m, 270, 360, true, true);
			DrawDoubleEllipseLoPartEmboss(p, m, 0, 90, true, false);
			DrawDoubleEllipseLoPart(p, m, 270, 360, true, true);
			DrawDoubleEllipseLoPart(p, m, 0, 90, true, false);
			p.SetColorEmboss();
			p.FillRectangle(m.middle2_x + p.thickness, m.top, m.middle2_x + p.thickness, m.bottom);
		}
		else {
			p.FillRectangle(m.left, m.middle1_y, m.middle1_x + p.thickness - 1, m.middle1_y + p.thickness - 1);
			p.FillRectangle(m.left, m.middle2_y, m.middle1_x, m.middle2_y + p.thickness - 1);
			p.FillRectangle(m.middle1_x, m.top, m.middle1_x + p.thickness - 1, m.middle1_y);
			p.FillRectangle(m.middle1_x, m.middle2_y, m.middle1_x + p.thickness - 1, m.bottom);
			p.FillRectangle(m.middle2_x, m.top, m.middle2_x + p.thickness - 1, m.bottom);
			p.SetColorEmboss();
			p.FillRectangle(m.left, m.middle1_y + p.thickness, m.middle1_x + p.thickness, m.middle1_y + p.thickness);
			p.FillRectangle(m.left, m.middle2_y + p.thickness, m.middle1_x + p.thickness, m.middle2_y + p.thickness);
			p.FillRectangle(m.middle1_x + p.thickness, m.top, m.middle1_x + p.thickness, m.middle1_y + p.thickness);
			p.FillRectangle(m.middle1_x + p.thickness, m.middle2_y + p.thickness, m.middle1_x + p.thickness, m.bottom);
			p.FillRectangle(m.middle2_x + p.thickness, m.top, m.middle2_x + p.thickness, m.bottom);
		}
	}

	static void Draw_2565_new(Painter &p, unsigned int start_y, unsigned int cx) /* ╥ */
	{
		DoubleLineBoxMetrics m(p, start_y, cx);
		p.SetColorSoften();
		p.FillRectangle(m.left, m.middle_y, m.right, m.middle_y + p.thickness - 1);
		p.FillRectangle(m.middle1_x, m.middle_y, m.middle1_x + p.thickness - 1, m.bottom);
		p.FillRectangle(m.middle2_x, m.middle_y, m.middle2_x + p.thickness - 1, m.bottom);
		p.SetColorEmboss();
		p.FillRectangle(m.left, m.middle_y + p.thickness, m.middle1_x - 1, m.middle_y + p.thickness);
		p.FillRectangle(m.middle2_x + p.thickness, m.middle_y + p.thickness, m.right, m.middle_y + p.thickness);
		p.FillRectangle(m.middle1_x + p.thickness, m.middle_y + p.thickness, m.middle1_x + p.thickness, m.bottom);
		p.FillRectangle(m.middle2_x + p.thickness, m.middle_y + p.thickness, m.middle2_x + p.thickness, m.bottom);
	}

	static void Draw_2566_new(Painter &p, unsigned int start_y, unsigned int cx) /* ╦ */
	{
		DoubleLineBoxMetrics m(p, start_y, cx);
		p.SetColorSoften();
		if (SDLBackend::options && SDLBackend::options->UseRoundedBorders) {
			p.FillRectangle(m.left, m.middle1_y, m.right, m.middle1_y + p.thickness - 1);
			DrawDoubleEllipseLoPartEmboss(p, m, 0, 90, true, false);
			DrawDoubleEllipseLoPartEmboss(p, m, 90, 180, false, false);
			DrawDoubleEllipseLoPart(p, m, 0, 90, true, false);
			DrawDoubleEllipseLoPart(p, m, 90, 180, false, false);
			p.SetColorEmboss();
			p.FillRectangle(m.left, m.middle1_y + p.thickness, m.right, m.middle1_y + p.thickness);
		}
		else {
			p.FillRectangle(m.left, m.middle1_y, m.right, m.middle1_y + p.thickness - 1);
			p.FillRectangle(m.left, m.middle2_y, m.middle1_x, m.middle2_y + p.thickness - 1);
			p.FillRectangle(m.middle2_x, m.middle2_y, m.right, m.middle2_y + p.thickness - 1);
			p.FillRectangle(m.middle1_x, m.middle2_y, m.middle1_x + p.thickness - 1, m.bottom);
			p.FillRectangle(m.middle2_x, m.middle2_y, m.middle2_x + p.thickness - 1, m.bottom);
			p.SetColorEmboss();
			p.FillRectangle(m.left, m.middle1_y + p.thickness, m.right, m.middle1_y + p.thickness);
			p.FillRectangle(m.left, m.middle2_y + p.thickness, m.middle1_x + p.thickness, m.middle2_y + p.thickness);
			p.FillRectangle(m.middle2_x + p.thickness, m.middle2_y + p.thickness, m.right, m.middle2_y + p.thickness);
			p.FillRectangle(m.middle1_x + p.thickness, m.middle2_y + p.thickness, m.middle1_x + p.thickness, m.bottom);
			p.FillRectangle(m.middle2_x + p.thickness, m.middle2_y + p.thickness, m.middle2_x + p.thickness, m.bottom);
		}
	}

	static void Draw_2567_new(Painter &p, unsigned int start_y, unsigned int cx) /* ╧ */
	{
		DoubleLineBoxMetrics m(p, start_y, cx);
		p.SetColorSoften();
		p.FillRectangle(m.left, m.middle1_y, m.right, m.middle1_y + p.thickness - 1);
		p.FillRectangle(m.left, m.middle2_y, m.right, m.middle2_y + p.thickness - 1);
		p.FillRectangle(m.middle_x, m.top, m.middle_x + p.thickness - 1, m.middle1_y);
		p.SetColorEmboss();
		p.FillRectangle(m.left, m.middle1_y + p.thickness, m.right, m.middle1_y + p.thickness);
		p.FillRectangle(m.left, m.middle2_y + p.thickness, m.right, m.middle2_y + p.thickness);
		p.FillRectangle(m.middle_x + p.thickness, m.top, m.middle_x + p.thickness, m.middle1_y - 1);
	}

	static void Draw_2569_new(Painter &p, unsigned int start_y, unsigned int cx) /* ╩ */ // + thickness
	{
		DoubleLineBoxMetrics m(p, start_y, cx);
		p.SetColorSoften();
		if (SDLBackend::options && SDLBackend::options->UseRoundedBorders) {
			p.FillRectangle(m.left, m.middle2_y, m.right, m.middle2_y + p.thickness - 1);
			DrawDoubleEllipseLoPartEmboss(p, m, 180, 270, false, true);
			DrawDoubleEllipseLoPartEmboss(p, m, 270, 360, true, true);
			DrawDoubleEllipseLoPart(p, m, 180, 270, false, true);
			DrawDoubleEllipseLoPart(p, m, 270, 360, true, true);
			p.SetColorEmboss();
			p.FillRectangle(m.left, m.middle2_y + p.thickness, m.right, m.middle2_y + p.thickness);
		}
		else {
			p.FillRectangle(m.left, m.middle1_y, m.middle1_x + p.thickness - 1, m.middle1_y + p.thickness - 1);
			p.FillRectangle(m.middle2_x, m.middle1_y, m.right, m.middle1_y + p.thickness - 1);
			p.FillRectangle(m.left, m.middle2_y, m.right, m.middle2_y + p.thickness - 1);
			p.FillRectangle(m.middle1_x, m.top, m.middle1_x + p.thickness - 1, m.middle1_y);
			p.FillRectangle(m.middle2_x, m.top, m.middle2_x + p.thickness - 1, m.middle1_y);
			p.SetColorEmboss();
			p.FillRectangle(m.left, m.middle1_y + p.thickness, m.middle1_x + p.thickness, m.middle1_y + p.thickness);
			p.FillRectangle(m.middle2_x + p.thickness, m.middle1_y + p.thickness, m.right, m.middle1_y + p.thickness);
			p.FillRectangle(m.left, m.middle2_y + p.thickness, m.right, m.middle2_y + p.thickness);
			p.FillRectangle(m.middle1_x + p.thickness, m.top, m.middle1_x + p.thickness, m.middle1_y - 1);
			p.FillRectangle(m.middle2_x + p.thickness, m.top, m.middle2_x + p.thickness, m.middle1_y - 1);
		}
	}

	static void Draw_256C_new(Painter &p, unsigned int start_y, unsigned int cx) /* ╬ */ // + thickness
	{
		DoubleLineBoxMetrics m(p, start_y, cx);
		p.SetColorSoften();
		if (SDLBackend::options && SDLBackend::options->UseRoundedBorders) {
			DrawDoubleEllipseLoPartEmboss(p, m, 180, 270, false, true);
			DrawDoubleEllipseLoPartEmboss(p, m, 270, 360, true, true);
			DrawDoubleEllipseLoPartEmboss(p, m, 0, 90, true, false);
			DrawDoubleEllipseLoPartEmboss(p, m, 90, 180, false, false);
			DrawDoubleEllipseLoPart(p, m, 180, 270, false, true);
			DrawDoubleEllipseLoPart(p, m, 270, 360, true, true);
			DrawDoubleEllipseLoPart(p, m, 0, 90, true, false);
			DrawDoubleEllipseLoPart(p, m, 90, 180, false, false);
		}
		else {
			p.FillRectangle(m.left, m.middle1_y, m.middle1_x + p.thickness - 1, m.middle1_y + p.thickness - 1);
			p.FillRectangle(m.middle2_x, m.middle1_y, m.right, m.middle1_y + p.thickness - 1);
			p.FillRectangle(m.left, m.middle2_y, m.middle1_x, m.middle2_y + p.thickness - 1);
			p.FillRectangle(m.middle2_x, m.middle2_y, m.right, m.middle2_y + p.thickness - 1);
			p.FillRectangle(m.middle1_x, m.top, m.middle1_x + p.thickness - 1, m.middle1_y);
			p.FillRectangle(m.middle1_x, m.middle2_y, m.middle1_x + p.thickness - 1, m.bottom);
			p.FillRectangle(m.middle2_x, m.top, m.middle2_x + p.thickness - 1, m.middle1_y);
			p.FillRectangle(m.middle2_x, m.middle2_y, m.middle2_x + p.thickness - 1, m.bottom);
			p.SetColorEmboss();
			p.FillRectangle(m.left, m.middle1_y + p.thickness, m.middle1_x + p.thickness, m.middle1_y + p.thickness);
			p.FillRectangle(m.middle2_x + p.thickness, m.middle1_y + p.thickness, m.right, m.middle1_y + p.thickness);
			p.FillRectangle(m.left, m.middle2_y + p.thickness, m.middle1_x + p.thickness, m.middle2_y + p.thickness);
			p.FillRectangle(m.middle2_x + p.thickness, m.middle2_y + p.thickness, m.right, m.middle2_y + p.thickness);
			p.FillRectangle(m.middle1_x + p.thickness, m.top, m.middle1_x + p.thickness, m.middle1_y + p.thickness);
			p.FillRectangle(m.middle1_x + p.thickness, m.middle2_y + p.thickness, m.middle1_x + p.thickness, m.bottom);
			p.FillRectangle(m.middle2_x + p.thickness, m.top, m.middle2_x + p.thickness, m.middle1_y + p.thickness);
			p.FillRectangle(m.middle2_x + p.thickness, m.middle2_y + p.thickness, m.middle2_x + p.thickness, m.bottom);
		}
	}

    /* todo mixed new look */

	static void Draw_2564_new(Painter &p, unsigned int start_y, unsigned int cx) /* ╤ */
	{
		DoubleLineBoxMetrics m(p, start_y, cx);
		p.FillRectangle(m.left, m.middle1_y, m.right, m.middle1_y + p.thickness - 1);
		p.FillRectangle(m.left, m.middle2_y, m.right, m.middle2_y + p.thickness - 1);
		p.FillRectangle(m.middle_x, m.middle2_y, m.middle_x + p.thickness - 1, m.bottom);
		if (p.MayDrawFadedEdges()) {
			p.SetColorFaded();
			p.FillRectangle(m.left, m.middle1_y - 1, m.right, m.middle1_y - 1);
			p.FillRectangle(m.left, m.middle2_y - 1, m.right, m.middle2_y - 1);
			p.FillRectangle(m.middle_x - 1, m.middle2_y + p.thickness, m.middle_x - 1, m.bottom);
		}
	}

	static void Draw_256A_new(Painter &p, unsigned int start_y, unsigned int cx) /* ╪ */
	{
		DoubleLineBoxMetrics m(p, start_y, cx);
		p.FillRectangle(m.left, m.middle1_y, m.right, m.middle1_y + p.thickness - 1);
		p.FillRectangle(m.left, m.middle2_y, m.right, m.middle2_y + p.thickness - 1);
		p.FillRectangle(m.middle_x, m.top, m.middle_x + p.thickness - 1, m.bottom);
		if (p.MayDrawFadedEdges()) {
			p.FillPixel(m.middle_x - 1, m.middle1_y - 1); // special pixel
			p.FillPixel(m.middle_x - 1, m.middle2_y - 1); // special pixel

			p.SetColorFaded();
			p.FillRectangle(m.left, m.middle1_y - 1, m.middle_x - 2, m.middle1_y - 1);
			p.FillRectangle(m.middle_x + p.thickness, m.middle1_y - 1, m.right, m.middle1_y - 1);
			p.FillRectangle(m.left, m.middle2_y - 1, m.middle_x - 2, m.middle2_y - 1);
			p.FillRectangle(m.middle_x + p.thickness, m.middle2_y - 1, m.right, m.middle2_y - 1);
			p.FillRectangle(m.middle_x - 1, m.top, m.middle_x - 1, m.middle1_y - 2);
			p.FillRectangle(m.middle_x - 1, m.middle1_y + p.thickness, m.middle_x - 1, m.middle2_y - 2);
			p.FillRectangle(m.middle_x - 1, m.middle2_y + p.thickness, m.middle_x - 1, m.bottom);
		}
	}


	static void Draw_256B_new(Painter &p, unsigned int start_y, unsigned int cx) /* ╫ */
	{
		DoubleLineBoxMetrics m(p, start_y, cx);
		p.FillRectangle(m.left, m.middle_y, m.right, m.middle_y + p.thickness - 1);
		p.FillRectangle(m.middle1_x, m.top, m.middle1_x + p.thickness - 1, m.bottom);
		p.FillRectangle(m.middle2_x, m.top, m.middle2_x + p.thickness - 1, m.bottom);
		if (p.MayDrawFadedEdges()) {
			p.FillPixel(m.middle1_x - 1, m.middle_y - 1); // special pixel
			p.FillPixel(m.middle2_x - 1, m.middle_y - 1); // special pixel

			p.SetColorFaded();

			p.FillRectangle(m.left, m.middle_y - 1, m.middle1_x - 2, m.middle_y - 1);
			p.FillRectangle(m.middle1_x + p.thickness, m.middle_y - 1, m.middle2_x - 2, m.middle_y - 1);
			p.FillRectangle(m.middle2_x + p.thickness, m.middle_y - 1, m.right, m.middle_y - 1);

			p.FillRectangle(m.middle1_x - 1, m.top, m.middle1_x - 1, m.middle_y - 2);
			p.FillRectangle(m.middle1_x - 1, m.middle_y + p.thickness, m.middle1_x - 1, m.bottom);

			p.FillRectangle(m.middle2_x - 1, m.top, m.middle2_x - 1, m.middle_y - 2);
			p.FillRectangle(m.middle2_x - 1, m.middle_y + p.thickness, m.middle2_x - 1, m.bottom);
		}
	}


	static void Draw_2561_new(Painter &p, unsigned int start_y, unsigned int cx) /* ╡ */ // + p.thickness - 1
	{
		DoubleLineBoxMetrics m(p, start_y, cx);
		p.FillRectangle(m.left, m.middle1_y, m.middle_x + p.thickness - 1, m.middle1_y + p.thickness - 1);
		p.FillRectangle(m.left, m.middle2_y, m.middle_x, m.middle2_y + p.thickness - 1);
		p.FillRectangle(m.middle_x, m.top, m.middle_x + p.thickness - 1, m.middle1_y);
		p.FillRectangle(m.middle_x, m.middle2_y, m.middle_x + p.thickness - 1, m.bottom);
		if (p.MayDrawFadedEdges()) {
			p.FillPixel(m.middle_x - 1, m.middle1_y - 1); // special pixel

			p.SetColorFaded();
			p.FillRectangle(m.left, m.middle1_y - 1, m.middle_x - 2, m.middle1_y - 1);
			p.FillRectangle(m.left, m.middle2_y - 1, m.middle_x + p.thickness - 1, m.middle2_y - 1);
			p.FillRectangle(m.middle_x - 1, m.top, m.middle_x - 1, m.middle1_y - 2);
			p.FillRectangle(m.middle_x - 1, m.middle2_y + p.thickness, m.middle_x - 1, m.bottom);
		}
	}


	static void Draw_255E_new(Painter &p, unsigned int start_y, unsigned int cx) /* ╞ */
	{
		DoubleLineBoxMetrics m(p, start_y, cx);
		p.FillRectangle(m.middle_x, m.middle1_y, m.right, m.middle1_y + p.thickness - 1);
		p.FillRectangle(m.middle_x, m.middle2_y, m.right, m.middle2_y + p.thickness - 1);
		p.FillRectangle(m.middle_x, m.top, m.middle_x + p.thickness - 1, m.middle1_y);
		p.FillRectangle(m.middle_x, m.middle2_y, m.middle_x + p.thickness - 1, m.bottom);
		if (p.MayDrawFadedEdges()) {
			p.SetColorFaded();
			p.FillRectangle(m.middle_x + p.thickness, m.middle1_y - 1, m.right, m.middle1_y - 1);
			p.FillRectangle(m.middle_x, m.middle2_y - 1, m.right, m.middle2_y - 1);
			p.FillRectangle(m.middle_x - 1, m.top, m.middle_x - 1, m.middle1_y + p.thickness - 1);
			p.FillRectangle(m.middle_x - 1, m.middle2_y, m.middle_x - 1, m.bottom);

			p.SetColorExtraFaded();
			p.FillPixel(m.middle_x - 1, m.middle2_y - 1);
		}
	}


	static void Draw_2556_new(Painter &p, unsigned int start_y, unsigned int cx) /* ╖ */
	{
		DoubleLineBoxMetrics m(p, start_y, cx);
		p.FillRectangle(m.left, m.middle_y, m.middle2_x, m.middle_y + p.thickness - 1);
		p.FillRectangle(m.middle1_x, m.middle_y, m.middle1_x + p.thickness - 1, m.bottom);
		p.FillRectangle(m.middle2_x, m.middle_y, m.middle2_x + p.thickness - 1, m.bottom);
		if (p.MayDrawFadedEdges()) {
			p.SetColorFaded();
			p.FillRectangle(m.left, m.middle_y - 1, m.middle2_x + p.thickness - 1, m.middle_y - 1);
			p.FillRectangle(m.middle1_x - 1, m.middle_y + p.thickness, m.middle1_x - 1, m.bottom);
			p.FillRectangle(m.middle2_x - 1, m.middle_y + p.thickness, m.middle2_x - 1, m.bottom);
		}
	}


	static void Draw_2555_new(Painter &p, unsigned int start_y, unsigned int cx) /* ╕ */
	{
		DoubleLineBoxMetrics m(p, start_y, cx);
		p.FillRectangle(m.left, m.middle1_y, m.middle_x + p.thickness - 1, m.middle1_y + p.thickness - 1);
		p.FillRectangle(m.left, m.middle2_y, m.middle_x, m.middle2_y + p.thickness - 1);
		p.FillRectangle(m.middle_x, m.middle1_y, m.middle_x + p.thickness - 1, m.bottom);
		if (p.MayDrawFadedEdges()) {
			p.FillPixel(m.middle_x - 1, m.middle2_y - 1); // special pixel

			p.SetColorFaded();
			p.FillRectangle(m.left, m.middle1_y - 1, m.middle_x + p.thickness - 1, m.middle1_y - 1);
			p.FillRectangle(m.left, m.middle2_y - 1, m.middle_x - 2, m.middle2_y - 1);
			p.FillRectangle(m.middle_x - 1, m.middle1_y + p.thickness, m.middle_x - 1, m.middle2_y - 2);
			p.FillRectangle(m.middle_x - 1, m.middle2_y + p.thickness, m.middle_x - 1, m.bottom);
		}
	}


	static void Draw_2568_new(Painter &p, unsigned int start_y, unsigned int cx) /* ╨ */
	{
		DoubleLineBoxMetrics m(p, start_y, cx);
		p.FillRectangle(m.left, m.middle_y, m.right, m.middle_y + p.thickness - 1);
		p.FillRectangle(m.middle1_x, m.top, m.middle1_x + p.thickness - 1, m.middle_y);
		p.FillRectangle(m.middle2_x, m.top, m.middle2_x + p.thickness - 1, m.middle_y);
		if (p.MayDrawFadedEdges()) {
			p.FillPixel(m.middle1_x - 1, m.middle_y - 1);
			p.FillPixel(m.middle2_x - 1, m.middle_y - 1);

			p.SetColorFaded();
			p.FillRectangle(m.left, m.middle_y - 1, m.middle1_x - 2, m.middle_y - 1);
			p.FillRectangle(m.middle1_x + p.thickness, m.middle_y - 1, m.middle2_x - 2, m.middle_y - 1);
			p.FillRectangle(m.middle2_x + p.thickness, m.middle_y - 1, m.right, m.middle_y - 1);
			p.FillRectangle(m.middle1_x - 1, m.top, m.middle1_x - 1, m.middle_y - 2);
			p.FillRectangle(m.middle2_x - 1, m.top, m.middle2_x - 1, m.middle_y - 2);
		}
	}


	static void Draw_255C_new(Painter &p, unsigned int start_y, unsigned int cx) /* ╜ */ // + thickness
	{
		DoubleLineBoxMetrics m(p, start_y, cx);
		p.FillRectangle(m.left, m.middle_y, m.middle2_x + p.thickness - 1, m.middle_y + p.thickness - 1);
		p.FillRectangle(m.middle1_x, m.top, m.middle1_x + p.thickness - 1, m.middle_y);
		p.FillRectangle(m.middle2_x, m.top, m.middle2_x + p.thickness - 1, m.middle_y);
		if (p.MayDrawFadedEdges()) {
			p.FillPixel(m.middle1_x - 1, m.middle_y - 1);
			p.FillPixel(m.middle2_x - 1, m.middle_y - 1);

			p.SetColorFaded();
			p.FillRectangle(m.left, m.middle_y - 1, m.middle1_x - 2, m.middle_y - 1);
			p.FillRectangle(m.middle1_x + p.thickness, m.middle_y - 1, m.middle2_x - 2, m.middle_y - 1);
			p.FillRectangle(m.middle1_x - 1, m.top, m.middle1_x - 1, m.middle_y - 2);
			p.FillRectangle(m.middle2_x - 1, m.top, m.middle2_x - 1, m.middle_y - 2);
		}
	}


	static void Draw_2559_new(Painter &p, unsigned int start_y, unsigned int cx) /* ╙ */
	{
		DoubleLineBoxMetrics m(p, start_y, cx);
		p.FillRectangle(m.middle1_x, m.middle_y, m.right, m.middle_y + p.thickness - 1);
		p.FillRectangle(m.middle1_x, m.top, m.middle1_x + p.thickness - 1, m.middle_y);
		p.FillRectangle(m.middle2_x, m.top, m.middle2_x + p.thickness - 1, m.middle_y);
		if (p.MayDrawFadedEdges()) {
			p.FillPixel(m.middle2_x - 1, m.middle_y - 1);

			p.SetColorFaded();
			p.FillRectangle(m.middle1_x + p.thickness, m.middle_y - 1, m.middle2_x - 2, m.middle_y - 1);
			p.FillRectangle(m.middle2_x + p.thickness, m.middle_y - 1, m.right, m.middle_y - 1);

			p.FillRectangle(m.middle1_x - 1, m.top, m.middle1_x - 1, m.middle_y + p.thickness - 1);
			p.FillRectangle(m.middle2_x - 1, m.top, m.middle2_x - 1, m.middle_y - 2);
		}
	}


	static void Draw_2558_new(Painter &p, unsigned int start_y, unsigned int cx) /* ╘ */
	{
		DoubleLineBoxMetrics m(p, start_y, cx);
		p.FillRectangle(m.middle_x, m.middle1_y, m.right, m.middle1_y + p.thickness - 1);
		p.FillRectangle(m.middle_x, m.middle2_y, m.right, m.middle2_y + p.thickness - 1);
		p.FillRectangle(m.middle_x, m.top, m.middle_x + p.thickness - 1, m.middle2_y);
		if (p.MayDrawFadedEdges()) {
			p.SetColorFaded();
			p.FillRectangle(m.middle_x + p.thickness, m.middle1_y - 1, m.right, m.middle1_y - 1);
			p.FillRectangle(m.middle_x + p.thickness, m.middle2_y - 1, m.right, m.middle2_y - 1);
			p.FillRectangle(m.middle_x - 1, m.top, m.middle_x - 1, m.middle2_y + p.thickness - 1);
		}
	}


	static void Draw_2552_new(Painter &p, unsigned int start_y, unsigned int cx) /* ╒ */
	{
		DoubleLineBoxMetrics m(p, start_y, cx);
		p.FillRectangle(m.middle_x, m.middle1_y, m.right, m.middle1_y + p.thickness - 1);
		p.FillRectangle(m.middle_x, m.middle2_y, m.right, m.middle2_y + p.thickness - 1);
		p.FillRectangle(m.middle_x, m.middle1_y, m.middle_x + p.thickness - 1, m.bottom);
		if (p.MayDrawFadedEdges()) {
			p.SetColorFaded();
			p.FillRectangle(m.middle_x, m.middle1_y - 1, m.right, m.middle1_y - 1);
			p.FillRectangle(m.middle_x + p.thickness, m.middle2_y - 1, m.right, m.middle2_y - 1);
			p.FillRectangle(m.middle_x - 1, m.middle1_y, m.middle_x - 1, m.bottom);

			p.SetColorExtraFaded();
			p.FillPixel(m.middle_x - 1, m.middle1_y - 1);
		}
	}


	static void Draw_2553_new(Painter &p, unsigned int start_y, unsigned int cx) /* ╓ */
	{
		DoubleLineBoxMetrics m(p, start_y, cx);
		p.FillRectangle(m.middle1_x, m.middle_y, m.right, m.middle_y + p.thickness - 1);
		p.FillRectangle(m.middle1_x, m.middle_y, m.middle1_x + p.thickness - 1, m.bottom);
		p.FillRectangle(m.middle2_x, m.middle_y, m.middle2_x + p.thickness - 1, m.bottom);
		if (p.MayDrawFadedEdges()) {
			p.SetColorFaded();
			p.FillRectangle(m.middle1_x, m.middle_y - 1, m.right, m.middle_y - 1);
			p.FillRectangle(m.middle1_x - 1, m.middle_y, m.middle1_x - 1, m.bottom);
			p.FillRectangle(m.middle2_x - 1, m.middle_y + p.thickness, m.middle2_x - 1, m.bottom);

			p.SetColorExtraFaded();
			p.FillPixel(m.middle1_x - 1, m.middle_y - 1);
		}
	}

	static void Draw_255B_new(Painter &p, unsigned int start_y, unsigned int cx) /* ╛*/ // + thickness
	{
		DoubleLineBoxMetrics m(p, start_y, cx);
		p.FillRectangle(m.left, m.middle1_y, m.middle_x, m.middle1_y + p.thickness - 1);
		p.FillRectangle(m.left, m.middle2_y, m.middle_x, m.middle2_y + p.thickness - 1);
		p.FillRectangle(m.middle_x, m.top, m.middle_x + p.thickness - 1, m.middle2_y + p.thickness - 1);
		if (p.MayDrawFadedEdges()) {
			p.FillPixel(m.middle_x - 1, m.middle1_y - 1);
			p.FillPixel(m.middle_x - 1, m.middle2_y - 1);

			p.SetColorFaded();
			p.FillRectangle(m.left, m.middle1_y - 1, m.middle_x - 2, m.middle1_y - 1);
			p.FillRectangle(m.left, m.middle2_y - 1, m.middle_x - 2, m.middle2_y - 1);
			p.FillRectangle(m.middle_x - 1, m.top, m.middle_x - 1, m.middle1_y - 2);
			p.FillRectangle(m.middle_x - 1, m.middle1_y + p.thickness, m.middle_x - 1, m.middle2_y - 2);
		}
	}

    /* other characters */

	static void Draw_2580(Painter &p, unsigned int start_y, unsigned int cx) /* ▀ */
	{
		CharMetrics m(p, start_y, cx);
		p.FillRectangle(m.left, m.top, m.right, m.top + (p.fh / 2) - 1);
	}

	static void Draw_2581_2588(Painter &p, unsigned int start_y, unsigned int cx)
	{	/* '▁' '▂' '▃' '▄' '▅' '▆' '▇' '█' */
		CharMetrics m(p, start_y, cx);
		p.FillRectangle(m.left, m.top + (p.fh * (0x2588 - p.wc) / 8), m.right, m.bottom);
	}

	static void Draw_2589_258f(Painter &p, unsigned int start_y, unsigned int cx)
	{	/* '▉' '▊' '▋' '▌' '▍' '▎' '▏' */
		CharMetrics m(p, start_y, cx);
		p.FillRectangle(m.left, m.top, m.left + (p.fw * (0x2590 - p.wc) / 8) - 1, m.bottom);
	}

	static void Draw_2590(Painter &p, unsigned int start_y, unsigned int cx) /* ▐ */
	{
		CharMetrics m(p, start_y, cx);
		p.FillRectangle(m.left + (p.fw / 2), m.top, m.right, m.bottom);
	}

#if 0 // TODO: optimize
	static void Draw_2591_2593(Painter &p, unsigned int start_y, unsigned int cx)
	{	/* ░ ▒ ▓ */
		CharMetrics m(p, start_y, cx);
		int step = (0x2593 + 1 - p.wc) + 1;
		for (int y = m.top; y < m.bottom; y+= step) {
			for (int x = m.left; x < m.right; x+= step) {
				p.FillPixel(x, y);
			}
		}
	}
#endif

	static void Draw_2594(Painter &p, unsigned int start_y, unsigned int cx) /* ▔ */
	{
		CharMetrics m(p, start_y, cx);
		p.FillRectangle(m.left, m.top, m.right, m.top + (p.fh / 8));
	}

	static void Draw_2595(Painter &p, unsigned int start_y, unsigned int cx) /* ▕ */
	{
		CharMetrics m(p, start_y, cx);
		p.FillRectangle(m.left + (7 * p.fw / 8), m.top, m.right, m.bottom);
	}

	static void Draw_2596(Painter &p, unsigned int start_y, unsigned int cx) /* ▖ */
	{
		CharMetrics m(p, start_y, cx);
		p.FillRectangle(m.left, m.top + (p.fh / 2), m.left + (p.fw / 2) - 1, m.bottom);
	}

	static void Draw_2597(Painter &p, unsigned int start_y, unsigned int cx) /* ▗ */
	{
		CharMetrics m(p, start_y, cx);
		p.FillRectangle(m.left + (p.fw / 2), m.top + (p.fh / 2), m.right, m.bottom);
	}

	static void Draw_2598(Painter &p, unsigned int start_y, unsigned int cx) /* ▘ */
	{
		CharMetrics m(p, start_y, cx);
		p.FillRectangle(m.left, m.top, m.left + (p.fw / 2) - 1, m.top + (p.fh / 2) - 1);
	}

	static void Draw_2599(Painter &p, unsigned int start_y, unsigned int cx) /* ▙ */
	{
		CharMetrics m(p, start_y, cx);
		p.FillRectangle(m.left, m.top, m.left + (p.fw / 2) - 1, m.bottom);
		p.FillRectangle(m.left + (p.fw / 2), m.top + (p.fh) / 2, m.right, m.bottom);
	}

	static void Draw_259a(Painter &p, unsigned int start_y, unsigned int cx) /* ▚ */
	{
		CharMetrics m(p, start_y, cx);
		p.FillRectangle(m.left, m.top, m.left + (p.fw / 2) - 1, m.top + (p.fh) / 2 - 1);
		p.FillRectangle(m.left + (p.fw / 2), m.top + (p.fh) / 2, m.right, m.bottom);
	}

	static void Draw_259b(Painter &p, unsigned int start_y, unsigned int cx) /* ▛ */
	{
		CharMetrics m(p, start_y, cx);
		p.FillRectangle(m.left, m.top, m.right, m.top + (p.fh) / 2 - 1);
		p.FillRectangle(m.left, m.top, m.left + (p.fw / 2) - 1, m.bottom);
	}

	static void Draw_259c(Painter &p, unsigned int start_y, unsigned int cx) /* ▜ */
	{
		CharMetrics m(p, start_y, cx);
		p.FillRectangle(m.left, m.top, m.right, m.top + (p.fh / 2) - 1);
		p.FillRectangle(m.left + (p.fw / 2), m.top + (p.fh / 2), m.right, m.bottom);
	}

	static void Draw_259d(Painter &p, unsigned int start_y, unsigned int cx) /* ▝ */
	{
		CharMetrics m(p, start_y, cx);
		p.FillRectangle(m.left + (p.fw / 2), m.top, m.right, m.top + (p.fh / 2) - 1);
	}

	static void Draw_259e(Painter &p, unsigned int start_y, unsigned int cx) /* ▞ */
	{
		CharMetrics m(p, start_y, cx);
		p.FillRectangle(m.left + (p.fw / 2), m.top, m.right, m.top + (p.fh / 2) - 1);
		p.FillRectangle(m.left, m.top + (p.fh / 2), m.left + (p.fw / 2) - 1, m.bottom);
	}

	static void Draw_259f(Painter &p, unsigned int start_y, unsigned int cx) /* ▟ */
	{
		CharMetrics m(p, start_y, cx);
		p.FillRectangle(m.left + (p.fw / 2), m.top, m.right, m.top + (p.fh / 2) - 1);
		p.FillRectangle(m.left, m.top + (p.fh / 2), m.right, m.bottom);
	}

	static void Draw_1fb00_1fb3b(Painter &p, unsigned int start_y, unsigned int cx)
	{
		const struct SixBoxMetrics : CharMetrics
		{
			SixBoxMetrics(Painter &p, unsigned int start_y, unsigned int cx)
				: CharMetrics(p, start_y, cx),
				middle_x(left + p.fw / 2),
				line1_y(top + p.fh / 2 - p.fh / 6),
				line2_y(top + p.fh / 2 + p.fh / 6)
			{ }
			int middle_x;
			int line1_y, line2_y;
	/*
			##
			##
			##
	*/
		} m(p, start_y, cx);
		unsigned int bits = (p.wc - 0x1FB00) + 1;
		if (bits>= 0b010101) bits++;
		if (bits>= 0b101010) bits++;
		if (bits & 0b000001) p.FillRectangle(m.left, m.top, m.middle_x - 1, m.line1_y - 1);
		if (bits & 0b000010) p.FillRectangle(m.middle_x, m.top, m.right, m.line1_y - 1);
		if (bits & 0b000100) p.FillRectangle(m.left, m.line1_y, m.middle_x - 1, m.line2_y - 1);
		if (bits & 0b001000) p.FillRectangle(m.middle_x, m.line1_y, m.right, m.line2_y - 1);
		if (bits & 0b010000) p.FillRectangle(m.left, m.line2_y, m.middle_x - 1, m.bottom);
		if (bits & 0b100000) p.FillRectangle(m.middle_x, m.line2_y, m.right, m.bottom);
	}

	static void Draw_WCHAR_ESCAPING(Painter &p, unsigned int start_y, unsigned int cx)
	{
		SingleLineBoxMetrics m(p, start_y, cx);
		const unsigned int steps = 4;
		const unsigned int stepy = (m.bottom - m.top) / (steps * 2);
		const unsigned int stepx = (m.right - m.left) / steps;

		p.FillRectangle(m.left, m.middle_y, m.right, m.middle_y + p.thickness - 1);
		for (unsigned int i = 1; i < steps; ++i) {
			p.FillRectangle(m.left, m.middle_y - i * stepy, m.right - stepx * i, m.middle_y - i * stepy + p.thickness - 1);
			p.FillRectangle(m.left, m.middle_y + i * stepy, m.right - stepx * i, m.middle_y + i * stepy + p.thickness - 1);
		}
	}

	static inline int get2R(const Painter &p, const SingleLineBoxMetrics& m) {
		int wx = p.fw; // m.right - m.left;
		int wy = p.fh; // m.bottom - m.top;
		return p.prev_space
			? wy - 4 /* (wx < wy - 3 ? wy - 3 : wx) */
			: (wx > wy ? wx : wy) / 2;
	}

	static void Draw_checked_radio(Painter &p, unsigned int start_y, unsigned int cx) /* ⦿ */
	{
		SingleLineBoxMetrics m(p, start_y, cx);

		int _2r = get2R(p, m);
		int r = _2r / 2;
		int r2 = r / 2;
		// int ascent = p.GetFontAscent();

		int X1 = m.left,          Y1 = m.top + 1; //  + ascent - _2r + 2;
		int X2 = m.left + r - r2, Y2 = m.top + r - r2 + 1; // ascent - _2r + r - r2 + 2;

		p.DrawEllipticArc(X1, Y1, _2r, _2r, 0, 0, 2);
		p.SetAccentBackground();
		p.FillEllipticPie(X1 + 1, Y1 + 1, _2r - 2, _2r - 2, 0, 0);
		p.SetAccentForeground();
		p.FillEllipticPie(X2, Y2, r, r, 0, 0);
	}

	static void Draw_unchecked_radio(Painter &p, unsigned int start_y, unsigned int cx) /* ◯ */
	{
		SingleLineBoxMetrics m(p, start_y, cx);

		int _2r = get2R(p, m);

		int X1 = m.left,            Y1 = m.top + 1; //  + ascent - _2r + 2;

		p.DrawEllipticArc(X1, Y1, _2r, _2r, 0, 0, 2);
	}

	static void Draw_unchecked_box(Painter &p, unsigned int start_y, unsigned int cx) /* ☐ */
	{
		SingleLineBoxMetrics m(p, start_y, cx);

		int _2r =get2R(p, m);
		int r = _2r / 2;
		int ascent = p.GetFontAscent();

		// int X1 = m.right - _2r, X2 = m.right, Y1 = m.top + ascent - 2 * r + 1, Y2 = m.top + ascent + 1;
		int X1 = m.left, X2 = m.left + _2r, Y1 = m.top + ascent - 2 * r + 1, Y2 = m.top + ascent + 1;

		p.FillRectangle(X1, Y1, X1, Y2);
		p.FillRectangle(X1, Y1, X2, Y1);
		p.FillRectangle(X2, Y1, X2, Y2);
		p.FillRectangle(X1, Y2, X2, Y2);
	}
	
	static void Draw_checked_sign(Painter &p, unsigned int start_y, unsigned int cx) /* ✔ */
	{
		SingleLineBoxMetrics m(p, start_y, cx);

		int _2r = get2R(p, m);
		int r = _2r / 2;
		int ascent = p.GetFontAscent();

		// int X1 = m.right - _2r, X2 = m.right, Y1 = m.top + ascent - 2 * r + 1, Y2 = m.top + ascent + 1;
		int X1 = m.left, X2 = m.left + _2r, Y1 = m.top + ascent - 2 * r + 1, Y2 = m.top + ascent + 1;

		p.FillRectangle(X1, Y1, X1, Y2);
		p.FillRectangle(X1, Y1, X2, Y1);
		p.FillRectangle(X2, Y1, X2, Y2);
		p.FillRectangle(X1, Y2, X2, Y2);

		p.SetAccentBackground();
		p.FillRectangle(X1 + 1, Y1 + 1, X2 - 1, Y2 - 1);
		
		p.SetAccentForeground();
		p.DrawLine(X2 - 1, Y1 + 1, X1 + r, Y2 - 1, 1);
		p.DrawLine(X1 + 1, Y1 + r, X1 + r, Y2 - 1, 1);
	}

	static void Draw_Space(Painter &p, unsigned int start_y, unsigned int cx) /* empty */
	{
	}

	////////////////////////////////////////////////////////////////

	/* single */
#define BordersL(a)		( SDLBackend::options && SDLBackend::options->UseModernLook ? a ## _new : a )
#define BordersT(a)		( SDLBackend::options && SDLBackend::options->UseModernLook ? Draw_Thicker<a ## _new> : Draw_Thicker<a> )
#define BordersCL(a)	( /*SDLBackend::options && SDLBackend::options->UseNoBorders ? Draw_Space :*/ BordersL(a) )
#define BordersCT(a)	( /*SDLBackend::options && SDLBackend::options->UseNoBorders ? Draw_Space :*/ BordersT(a) )
#define BordersEL(b, a)	( SDLBackend::options && SDLBackend::options->UseNoBorders ? BordersL(b) : BordersL(a) )
#define BordersET(b, a)	( SDLBackend::options && SDLBackend::options->UseNoBorders ? BordersT(b) : BordersT(a) )

	/* double */
#define BordersD(b, a)	( SDLBackend::options && SDLBackend::options->UseSingleBordersOnly ? BordersL(b) : BordersL(a) )
#define BordersCD(b, a)	( /* SDLBackend::options && SDLBackend::options->UseNoBorders ? Draw_Space :*/ BordersD(b, a) )

#define BordersED(b, be, ae, a)	( SDLBackend::options && SDLBackend::options->UseNoBorders ? BordersD(be, ae) : BordersD(b, a) )

    /* 
    	Mnemonics:
        	Borders - mean border characters
              C - corner | E - line + corner join
              L - light  | T - thick | D - double
              ( 
              	[Thin replacemenmt Draw_f, 
              	[No edges thin replacement Draw_f, No edges replacement Draw_f,]] 
              	Original Draw_f )
    */

	DrawT Get(uint32_t c)
	{
		switch (c) {
			case 0x2190: return Draw_HorizontalArrow<false>; 		/* ← */
			case 0x2191: return Draw_VerticalArrow<false>; 			/* ↑ */
			case 0x2192: return Draw_HorizontalArrow<true>; 		/* → */
			case 0x2193: return Draw_VerticalArrow<true>; 			/* ↓ */
			case 0x2500: return Draw_2500; 			/* ─ */
			case 0x2501: return Draw_Thicker<Draw_2500>;	/* ━ */
			case 0x2502: return Draw_2502; 			/* │ */
			case 0x2503: return Draw_Thicker<Draw_2502>;	/* ┃ */
			case 0x2504: return Draw_DashesH<3>;			/* ┄ */
			case 0x2505: return Draw_Thicker<Draw_DashesH<3>>;	/* ┅ */
			case 0x2506: return Draw_DashesV<3>;			/* ┆ */
			case 0x2507: return Draw_Thicker<Draw_DashesV<3>>;	/* ┇ */
			case 0x2508: return Draw_DashesH<4>;			/* ┈ */
			case 0x2509: return Draw_Thicker<Draw_DashesH<4>>;	/* ┉ */
			case 0x250a: return Draw_DashesV<4>;			/* ┊ */
			case 0x250b: return Draw_Thicker<Draw_DashesV<4>>;	/* ┋ */
			case 0x254c: return Draw_DashesH<2>;			/* ╌ */
			case 0x254d: return Draw_Thicker<Draw_DashesH<2>>;	/* ╍ */
			case 0x254e: return Draw_DashesV<2>;			/* ╎ */
			case 0x254f: return Draw_Thicker<Draw_DashesV<2>>;	/* ╏ */
			case 0x250c: return BordersCL(Draw_250C);		/* ┌ */
			case 0x250f: return BordersCT(Draw_250C);		/* ┏ */
			case 0x2510: return BordersCL(Draw_2510);		/* ┐ */
			case 0x2513: return BordersCT(Draw_2510);		/* ┓ */
			case 0x2514: return BordersCL(Draw_2514);		/* └ */
			case 0x2517: return BordersCT(Draw_2514);		/* ┗ */
			case 0x2518: return BordersCL(Draw_2518);		/* ┘ */
			case 0x251b: return BordersCT(Draw_2518);		/* ┛ */
			case 0x251c: return BordersEL(Draw_2502, Draw_251C);		/* ├ */
			case 0x2523: return BordersET(Draw_2502, Draw_251C);		/* ┣ */
			case 0x2524: return BordersEL(Draw_2502, Draw_2524);		/* ┤ */
			case 0x252b: return BordersET(Draw_2502, Draw_2524);		/* ┫ */
			case 0x252c: return BordersEL(Draw_2500, Draw_252C);		/* ┬ */
			case 0x2533: return BordersET(Draw_2500, Draw_252C);		/* ┳ */
			case 0x2534: return BordersEL(Draw_2500, Draw_2534);		/* ┴ */
			case 0x253b: return BordersET(Draw_2500, Draw_2534);		/* ┻ */
			case 0x253c: return BordersCL(Draw_253C);		/* ┼ */
			case 0x254b: return BordersCT(Draw_253C);		/* ╋ */
			case 0x2550: return BordersD(Draw_2500, Draw_2550);		/* ═ */
			case 0x2551: return BordersD(Draw_2502, Draw_2551);		/* ║ */
			case 0x2552: return BordersCD(Draw_250C, Draw_2552);		/* ╒ */
			case 0x2553: return BordersCD(Draw_250C, Draw_2553);		/* ╓ */
			case 0x2554: return BordersCD(Draw_250C, Draw_2554);  	/* ╔ */
			case 0x2555: return BordersCD(Draw_2510, Draw_2555); 	/* ╕ */
			case 0x2556: return BordersCD(Draw_2510, Draw_2556);		/* ╖ */
			case 0x2557: return BordersCD(Draw_2510, Draw_2557); 	/* ╗ */
			case 0x2558: return BordersCD(Draw_2514, Draw_2558);		/* ╘ */
			case 0x2559: return BordersCD(Draw_2514, Draw_2559);		/* ╙ */
			case 0x255a: return BordersCD(Draw_2514, Draw_255A);		/* ╚ */
			case 0x255B: return BordersCD(Draw_2518, Draw_255B); 	/* ╛*/ // + thickness
			case 0x255C: return BordersCD(Draw_2518, Draw_255C); 	/* ╜ */ // + thickness
			case 0x255d: return BordersCD(Draw_2518, Draw_255D); 	/* ╝ */
			case 0x255E: return BordersL(Draw_255E);	 			/* ╞ */
			case 0x255f: return BordersED(Draw_251C, Draw_2502, Draw_2551, Draw_255F);		/* ╟ */
			case 0x2560: return BordersED(Draw_251C, Draw_2502, Draw_2551, Draw_2560);		/* ╠ */
			case 0x2561: return BordersL(Draw_2561); 				/* ╡ */ // + p.thickness - 1
			case 0x2562: return BordersED(Draw_2524, Draw_2502, Draw_2551, Draw_2562);		/* ╢ */
			case 0x2563: return BordersED(Draw_2524, Draw_2502, Draw_2551, Draw_2563);		/* ╣ */ // + thickness
			case 0x2564: return BordersED(Draw_252C, Draw_2500, Draw_2550, Draw_2564);		/* ╤ */
			case 0x2565: return BordersCD(Draw_252C, Draw_2565);		/* ╥ */
			case 0x2566: return BordersED(Draw_252C, Draw_2500, Draw_2550, Draw_2566);		/* ╦ */
			case 0x2567: return BordersED(Draw_2534, Draw_2500, Draw_2550, Draw_2567);		/* ╧ */
			case 0x2568: return BordersCD(Draw_2534, Draw_2568); 	/* ╨ */
			case 0x2569: return BordersED(Draw_2534, Draw_2500, Draw_2550, Draw_2569); 	/* ╩ */ // + thickness
			case 0x256A: return BordersD(Draw_253C, Draw_256A);		/* ╪ */
			case 0x256B: return BordersD(Draw_253C, Draw_256B); 	/* ╫ */
			case 0x256C: return BordersD(Draw_253C, Draw_256C); 	/* ╬ */ // + thickness

			// controls
			case L'☐':   return Draw_unchecked_box;
			case L'⦿':   return Draw_checked_radio;
			case L'◯':   return Draw_unchecked_radio;
			case L'✔':	 return Draw_checked_sign;

//not fadable
			case 0x2580: return Draw_2580; /* ▀ */

			case 0x2581 ... 0x2588: return Draw_2581_2588; /* '▁' '▂' '▃' '▄' '▅' '▆' '▇' '█' */
			case 0x2589 ... 0x258f: return Draw_2589_258f;   /* '▉' '▊' '▋' '▌' '▍' '▎' '▏' */

			case 0x2590: return Draw_2590; /* ▐ */

			// case 0x2591 ... 0x2593 : return Draw_2591_2593; /* ░ ▒ ▓ */

			case 0x2594: return Draw_2594; /* ▔ */
			case 0x2595: return Draw_2595; /* ▕ */

			case 0x2596: return Draw_2596; /* ▖ */
			case 0x2597: return Draw_2597; /* ▗ */
			case 0x2598: return Draw_2598; /* ▘ */
			case 0x2599: return Draw_2599; /* ▙ */
			case 0x259a: return Draw_259a; /* ▚ */
			case 0x259b: return Draw_259b; /* ▛ */
			case 0x259c: return Draw_259c; /* ▜ */
			case 0x259d: return Draw_259d; /* ▝ */
			case 0x259e: return Draw_259e; /* ▞ */
			case 0x259f: return Draw_259f; /* ▟ */

			case 0x1FB00 ... 0x1FB3b: return Draw_1fb00_1fb3b;

			case WCHAR_ESCAPING: return Draw_WCHAR_ESCAPING;
		}

		return nullptr;
	}
}

/*

        0 	1 	2 	3 	4 	5 	6 	7 	8 	9 	A 	B 	C 	D 	E 	F
U+219x  ← 	↑ 	→ 	↓

U+250x 	─ 	━ 	│ 	┃ 	┄ 	┅ 	┆ 	┇ 	┈ 	┉ 	┊ 	┋ 	┌ 	┍ 	┎ 	┏

U+251x 	┐ 	┑ 	┒ 	┓ 	└ 	┕ 	┖ 	┗ 	┘ 	┙ 	┚ 	┛ 	├ 	┝ 	┞ 	┟

U+252x 	┠ 	┡ 	┢ 	┣ 	┤ 	┥ 	┦ 	┧ 	┨ 	┩ 	┪ 	┫ 	┬ 	┭ 	┮ 	┯

U+253x 	┰ 	┱ 	┲ 	┳ 	┴ 	┵ 	┶ 	┷ 	┸ 	┹ 	┺ 	┻ 	┼ 	┽ 	┾ 	┿

U+254x 	╀ 	╁ 	╂ 	╃ 	╄ 	╅ 	╆ 	╇ 	╈ 	╉ 	╊ 	╋ 	╌ 	╍ 	╎ 	╏

U+255x 	═ 	║ 	╒ 	╓ 	╔ 	╕ 	╖ 	╗ 	╘ 	╙ 	╚ 	╛ 	╜ 	╝ 	╞ 	╟

U+256x 	╠ 	╡ 	╢ 	╣ 	╤ 	╥ 	╦ 	╧ 	╨ 	╩ 	╪ 	╫ 	╬ 	╭ 	╮ 	╯

U+257x 	╰ 	╱ 	╲ 	╳ 	╴ 	╵ 	╶ 	╷ 	╸ 	╹ 	╺ 	╻ 	╼ 	╽ 	╾ 	╿

U+258x 	▀ 	▁ 	▂ 	▃ 	▄ 	▅ 	▆ 	▇ 	█ 	▉ 	▊ 	▋ 	▌ 	▍ 	▎ 	▏

U+259x 	▐ 	░ 	▒ 	▓ 	▔ 	▕ 	▖ 	▗ 	▘ 	▙ 	▚ 	▛ 	▜ 	▝ 	▞ 	▟

U+1FB0x	🬀	🬁	🬂	🬃	🬄	🬅	🬆	🬇	🬈	🬉	🬊	🬋	🬌	🬍	🬎	🬏
U+1FB1x	🬐	🬑	🬒	🬓	🬔	🬕	🬖	🬗	🬘	🬙	🬚	🬛	🬜	🬝	🬞	🬟
U+1FB2x	🬠	🬡	🬢	🬣	🬤	🬥	🬦	🬧	🬨	🬩	🬪	🬫	🬬	🬭	🬮	🬯
U+1FB3x	🬰	🬱	🬲	🬳	🬴	🬵	🬶	🬷	🬸	🬹	🬺	🬻

*/
