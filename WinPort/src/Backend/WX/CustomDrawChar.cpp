#include "CustomDrawChar.h"
#include "WideMB.h"

namespace WXCustomDrawChar
{

	struct CharMetrics
	{
		inline CharMetrics(Painter &p, unsigned int start_y, unsigned int cx) :
			left(cx * p.fw),
			right(left + p.fw - 1),
			top(start_y),
			bottom(top + p.fh - 1)
		{
		}

		wxCoord left;
		wxCoord right;
		wxCoord top;
		wxCoord bottom;
	};

	struct SingleLineBoxMetrics : CharMetrics
	{
		inline SingleLineBoxMetrics(Painter &p, unsigned int start_y, unsigned int cx)
			: CharMetrics(p, start_y, cx),
			  middle_y(top + p.fh / 2 - p.thickness / 2),
			  middle_x(left + p.fw / 2 - p.thickness / 2)
		{
		}

		wxCoord middle_y;
		wxCoord middle_x;
	};

	struct DoubleLineBoxMetrics : SingleLineBoxMetrics
	{
		inline DoubleLineBoxMetrics(Painter &p, unsigned int start_y, unsigned int cx)
			: SingleLineBoxMetrics(p, start_y, cx)
		{
			wxCoord ofs = std::min(p.fh, p.fw) / 4;

			middle1_y = middle_y - ofs;
			middle1_x = middle_x - ofs;

			middle2_y = middle_y + ofs;
			middle2_x = middle_x + ofs;
		}

		wxCoord middle1_y, middle2_y;
		wxCoord middle1_x, middle2_x;
	};

	template <wxCoord COUNT>
		struct Dashes
	{
		Dashes(wxCoord total_len)
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

		wxCoord period; // distance between begins of each visible dash parts
		wxCoord len; // visible dash part len
	};

	template <DrawT DRAW>
		static void Draw_Thicker(Painter &p, unsigned int start_y, unsigned int cx) /* ─ */
	{
		auto saved_thickness = p.thickness;
		p.thickness = 1 + (p.thickness * 3) / 2;
		DRAW(p, start_y, cx);
		p.thickness = saved_thickness;
	}


	template <wxCoord COUNT>
		static void Draw_DashesH(Painter &p, unsigned int start_y, unsigned int cx) /* ┄ */
	{
		SingleLineBoxMetrics m(p, start_y, cx);
		Dashes<COUNT> d(p.fw);

		for (wxCoord i = 0; i < COUNT; ++i) {
			p.FillRectangle(m.left + i * d.period, m.middle_y, m.left + i * d.period + d.len - 1, m.middle_y + p.thickness - 1);
		}

		if (p.MayDrawFadedEdges()) {
			p.SetColorFaded();
			for (wxCoord i = 0; i < COUNT; ++i) {
				p.FillRectangle(m.left + i * d.period, m.middle_y - 1, m.left + i * d.period + d.len - 1, m.middle_y - 1);
			}
		}
	}

	template <wxCoord COUNT>
		static void Draw_DashesV(Painter &p, unsigned int start_y, unsigned int cx) /* ┄ */
	{
		SingleLineBoxMetrics m(p, start_y, cx);
		Dashes<COUNT> d(p.fh);

		for (wxCoord i = 0; i < COUNT; ++i) {
			p.FillRectangle(m.middle_x, m.top + i * d.period, m.middle_x + p.thickness - 1, m.top + i * d.period + d.len - 1);
		}

		if (p.MayDrawFadedEdges()) {
			p.SetColorFaded();
			for (wxCoord i = 0; i < COUNT; ++i) {
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
		wxCoord step = (0x2593 + 1 - p.wc) + 1;
		for (wxCoord y = m.top; y < m.bottom; y+= step) {
			for (wxCoord x = m.left; x < m.right; x+= step) {
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
			wxCoord middle_x;
			wxCoord line1_y, line2_y;
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

	////////////////////////////////////////////////////////////////

	DrawT Get(const wchar_t c)
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
			case 0x250c: return Draw_250C;			/* ┌ */
			case 0x250f: return Draw_Thicker<Draw_250C>;	/* ┏ */
			case 0x2510: return Draw_2510;			/* ┐ */
			case 0x2513: return Draw_Thicker<Draw_2510>;	/* ┓ */
			case 0x2514: return Draw_2514;			/* └ */
			case 0x2517: return Draw_Thicker<Draw_2514>;	/* ┗ */
			case 0x2518: return Draw_2518;			/* ┘ */
			case 0x251b: return Draw_Thicker<Draw_2518>;	/* ┛ */
			case 0x251c: return Draw_251C;			/* ├ */
			case 0x2523: return Draw_Thicker<Draw_251C>;	/* ┣ */
			case 0x2524: return Draw_2524;			/* ┤ */
			case 0x252b: return Draw_Thicker<Draw_2524>;	/* ┫ */
			case 0x252c: return Draw_252C;			/* ┬ */
			case 0x2533: return Draw_Thicker<Draw_252C>;	/* ┳ */
			case 0x2534: return Draw_2534;			/* ┴ */
			case 0x253b: return Draw_Thicker<Draw_2534>;	/* ┻ */
			case 0x253c: return Draw_253C;			/* ┼ */
			case 0x254b: return Draw_Thicker<Draw_253C>;	/* ╋ */
			case 0x2550: return Draw_2550;			/* ═ */
			case 0x2551: return Draw_2551;			/* ║ */
			case 0x2552: return Draw_2552; 			/* ╒ */
			case 0x2553: return Draw_2553; 			/* ╓ */
			case 0x2554: return Draw_2554;
			case 0x2555: return Draw_2555; 			/* ╕ */
			case 0x2556: return Draw_2556; 			/* ╖ */
			case 0x2557: return Draw_2557;
			case 0x2558: return Draw_2558; 			/* ╘ */
			case 0x2559: return Draw_2559; 			/* ╙ */
			case 0x255a: return Draw_255A;
			case 0x255B: return Draw_255B; /* ╛*/ // + thickness
			case 0x255C: return Draw_255C; /* ╜ */ // + thickness
			case 0x255d: return Draw_255D;
			case 0x255E: return Draw_255E; /* ╞ */
			case 0x255f: return Draw_255F;
			case 0x2560: return Draw_2560; /* ╠ */
			case 0x2561: return Draw_2561; /* ╡ */ // + p.thickness - 1
			case 0x2562: return Draw_2562;
			case 0x2563: return Draw_2563; /* ╣ */ // + thickness
			case 0x2564: return Draw_2564; /* ╤ */
			case 0x2565: return Draw_2565; /* ╥ */
			case 0x2566: return Draw_2566; /* ╦ */
			case 0x2567: return Draw_2567; /* ╧ */
			case 0x2568: return Draw_2568; /* ╨ */
			case 0x2569: return Draw_2569; /* ╩ */ // + thickness
			case 0x256A: return Draw_256A; /* ╪ */
			case 0x256B: return Draw_256B; /* ╫ */
			case 0x256C: return Draw_256C; /* ╬ */ // + thickness

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

