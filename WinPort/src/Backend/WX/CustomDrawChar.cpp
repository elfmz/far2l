#include "CustomDrawChar.h"

namespace WXCustomDrawChar
{

	struct CharMetrics
	{
		inline CharMetrics(const FontMetrics &fm, unsigned int start_y, unsigned int cx)
		{
			left = cx * fm.fw;
			right = left + fm.fw - 1;
			top = start_y;
			bottom = top + fm.fh - 1;
		}

		wxCoord left;
		wxCoord right;
		wxCoord top;
		wxCoord bottom;
	};

	struct SingleLineBoxMetrics : CharMetrics
	{
		inline SingleLineBoxMetrics(const FontMetrics &fm, unsigned int start_y, unsigned int cx)
			: CharMetrics(fm, start_y, cx)
		{
			middle_y = top + fm.fh / 2 - fm.thickness / 2;
			middle_x = left + fm.fw / 2 - fm.thickness / 2;
		}

		wxCoord middle_y;
		wxCoord middle_x;
	};

	struct DoubleLineBoxMetrics : SingleLineBoxMetrics
	{
		inline DoubleLineBoxMetrics(const FontMetrics &fm, unsigned int start_y, unsigned int cx)
			: SingleLineBoxMetrics(fm, start_y, cx)
		{
			wxCoord ofs = std::min(fm.fh, fm.fw) / 4;

			middle1_y = middle_y - ofs;
			middle1_x = middle_x - ofs;

			middle2_y = middle_y + ofs;
			middle2_x = middle_x + ofs;
		}

		wxCoord middle1_y, middle2_y;
		wxCoord middle1_x, middle2_x;
	};

	static inline void FillRectangle(wxPaintDC &dc, wxCoord left, wxCoord top, wxCoord right, wxCoord bottom)
	{
		dc.DrawRectangle(left, top, right + 1 - left , bottom + 1 - top);
	}


	static void Draw_2500(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ─ */
	{
		SingleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left, m.middle_y, m.right, m.middle_y + fm.thickness - 1);
	}

	static void Draw_2502(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* │ */
	{
		SingleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.middle_x, m.top, m.middle_x + fm.thickness - 1, m.bottom);
	}

	static void Draw_250C(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ┌ */
	{
		SingleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.middle_x, m.middle_y, m.right, m.middle_y + fm.thickness - 1);
		FillRectangle(dc, m.middle_x, m.middle_y, m.middle_x + fm.thickness - 1, m.bottom);
	}

	static void Draw_2510(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ┐ */
	{
		SingleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left, m.middle_y, m.middle_x, m.middle_y + fm.thickness - 1);
		FillRectangle(dc, m.middle_x, m.middle_y, m.middle_x + fm.thickness - 1, m.bottom);
	}

	static void Draw_2514(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* └ */
	{
		SingleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.middle_x, m.middle_y, m.right, m.middle_y + fm.thickness - 1);
		FillRectangle(dc, m.middle_x, m.top, m.middle_x + fm.thickness - 1, m.middle_y);
	}

	static void Draw_2518(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ┘ */
	{
		SingleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left, m.middle_y, m.middle_x + fm.thickness - 1, m.middle_y + fm.thickness - 1);
		FillRectangle(dc, m.middle_x, m.top, m.middle_x + fm.thickness - 1, m.middle_y);
	}

	static void Draw_251C(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ├ */
	{
		SingleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.middle_x, m.middle_y, m.right, m.middle_y + fm.thickness - 1);
		FillRectangle(dc, m.middle_x, m.top, m.middle_x + fm.thickness - 1, m.bottom);
	}

	static void Draw_2524(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ┤ */
	{
		SingleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left, m.middle_y, m.middle_x, m.middle_y + fm.thickness - 1);
		FillRectangle(dc, m.middle_x, m.top, m.middle_x + fm.thickness - 1, m.bottom);
	}

	static void Draw_252C(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ┬ */
	{
		SingleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left, m.middle_y, m.right, m.middle_y + fm.thickness - 1);
		FillRectangle(dc, m.middle_x, m.middle_y, m.middle_x + fm.thickness - 1, m.bottom);
	}

	static void Draw_2534(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ┴ */
	{
		SingleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left, m.middle_y, m.right, m.middle_y + fm.thickness - 1);
		FillRectangle(dc, m.middle_x, m.top, m.middle_x + fm.thickness - 1, m.middle_y);
	}

	static void Draw_253C(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ┼  */
	{
		SingleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left, m.middle_y, m.right, m.middle_y + fm.thickness - 1);
		FillRectangle(dc, m.middle_x, m.top, m.middle_x + fm.thickness - 1, m.bottom);
	}

	static void Draw_2550(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ═ */
	{
		DoubleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left, m.middle1_y, m.right, m.middle1_y + fm.thickness - 1);
		FillRectangle(dc, m.left, m.middle2_y, m.right, m.middle2_y + fm.thickness - 1);
	}

	static void Draw_2551(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ║ */
	{
		DoubleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.middle1_x, m.top, m.middle1_x + fm.thickness - 1, m.bottom);
		FillRectangle(dc, m.middle2_x, m.top, m.middle2_x + fm.thickness - 1, m.bottom);
	}

	static void Draw_2554(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ╔  */
	{
		DoubleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.middle1_x, m.middle1_y, m.right, m.middle1_y + fm.thickness - 1);
		FillRectangle(dc, m.middle2_x, m.middle2_y, m.right, m.middle2_y + fm.thickness - 1);

		FillRectangle(dc, m.middle1_x, m.middle1_y, m.middle1_x + fm.thickness - 1, m.bottom);
		FillRectangle(dc, m.middle2_x, m.middle2_y, m.middle2_x + fm.thickness - 1, m.bottom);
	}

	static void Draw_2557(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ╗  */
	{
		DoubleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left, m.middle1_y, m.middle2_x, m.middle1_y + fm.thickness - 1);
		FillRectangle(dc, m.left, m.middle2_y, m.middle1_x, m.middle2_y + fm.thickness - 1);

		FillRectangle(dc, m.middle2_x, m.middle1_y, m.middle2_x + fm.thickness - 1, m.bottom);
		FillRectangle(dc, m.middle1_x, m.middle2_y, m.middle1_x + fm.thickness - 1, m.bottom);
	}

	static void Draw_255A(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ╚  */
	{
		DoubleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.middle2_x, m.middle1_y, m.right, m.middle1_y + fm.thickness - 1);
		FillRectangle(dc, m.middle1_x, m.middle2_y, m.right, m.middle2_y + fm.thickness - 1);

		FillRectangle(dc, m.middle1_x, m.top, m.middle1_x + fm.thickness - 1, m.middle2_y);
		FillRectangle(dc, m.middle2_x, m.top, m.middle2_x + fm.thickness - 1, m.middle1_y);
	}

	static void Draw_255D(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ╝  */ // + thickness
	{
		DoubleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left, m.middle1_y, m.middle1_x + fm.thickness - 1, m.middle1_y + fm.thickness - 1);
		FillRectangle(dc, m.left, m.middle2_y, m.middle2_x + fm.thickness - 1, m.middle2_y + fm.thickness - 1);

		FillRectangle(dc, m.middle1_x, m.top, m.middle1_x + fm.thickness - 1, m.middle1_y);
		FillRectangle(dc, m.middle2_x, m.top, m.middle2_x + fm.thickness - 1, m.middle2_y);
	}

	static void Draw_255F(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ╟  */
	{
		DoubleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.middle2_x, m.middle_y, m.right, m.middle_y + fm.thickness - 1);
		FillRectangle(dc, m.middle1_x, m.top, m.middle1_x + fm.thickness - 1, m.bottom);
		FillRectangle(dc, m.middle2_x, m.top, m.middle2_x + fm.thickness - 1, m.bottom);
	}

	static void Draw_2562(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ╢  */
	{
		DoubleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left, m.middle_y, m.middle1_x, m.middle_y + fm.thickness - 1);
		FillRectangle(dc, m.middle1_x, m.top, m.middle1_x + fm.thickness - 1, m.bottom);
		FillRectangle(dc, m.middle2_x, m.top, m.middle2_x + fm.thickness - 1, m.bottom);
	}


	///
	static void Draw_2560(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ╠ */
	{
		DoubleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.middle2_x, m.middle1_y, m.right, m.middle1_y + fm.thickness - 1);
		FillRectangle(dc, m.middle2_x, m.middle2_y, m.right, m.middle2_y + fm.thickness - 1);
		FillRectangle(dc, m.middle1_x, m.top, m.middle1_x + fm.thickness - 1, m.bottom);
		FillRectangle(dc, m.middle2_x, m.top, m.middle2_x + fm.thickness - 1, m.middle1_y);
		FillRectangle(dc, m.middle2_x, m.middle2_y, m.middle2_x + fm.thickness - 1, m.bottom);
        }

	static void Draw_2563(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ╣ */ // + thickness
	{
		DoubleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left, m.middle1_y, m.middle1_x + fm.thickness - 1, m.middle1_y + fm.thickness - 1);
		FillRectangle(dc, m.left, m.middle2_y, m.middle1_x, m.middle2_y + fm.thickness - 1);
		FillRectangle(dc, m.middle1_x, m.top, m.middle1_x + fm.thickness - 1, m.middle1_y);
		FillRectangle(dc, m.middle1_x, m.middle2_y, m.middle1_x + fm.thickness - 1, m.bottom);
		FillRectangle(dc, m.middle2_x, m.top, m.middle2_x + fm.thickness - 1, m.bottom);
	}
	

	static void Draw_2566(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ╦ */
	{
		DoubleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left, m.middle1_y, m.right, m.middle1_y + fm.thickness - 1);
		FillRectangle(dc, m.left, m.middle2_y, m.middle1_x, m.middle2_y + fm.thickness - 1);
		FillRectangle(dc, m.middle2_x, m.middle2_y, m.right, m.middle2_y + fm.thickness - 1);
		FillRectangle(dc, m.middle1_x, m.middle2_y, m.middle1_x + fm.thickness - 1, m.bottom);
		FillRectangle(dc, m.middle2_x, m.middle2_y, m.middle2_x + fm.thickness - 1, m.bottom);
	}

	static void Draw_2569(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ╩  */ // + thickness
	{
		DoubleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left, m.middle1_y, m.middle1_x + fm.thickness - 1, m.middle1_y + fm.thickness - 1);
		FillRectangle(dc, m.middle2_x, m.middle1_y, m.right, m.middle1_y + fm.thickness - 1);
		FillRectangle(dc, m.left, m.middle2_y, m.right, m.middle2_y + fm.thickness - 1);
		FillRectangle(dc, m.middle1_x, m.top, m.middle1_x + fm.thickness - 1, m.middle1_y);
		FillRectangle(dc, m.middle2_x, m.top, m.middle2_x + fm.thickness - 1, m.middle1_y);
	}

	static void Draw_256C(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ╬ */ // + thickness
	{
		DoubleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left, m.middle1_y, m.middle1_x + fm.thickness - 1, m.middle1_y + fm.thickness - 1);
		FillRectangle(dc, m.middle2_x, m.middle1_y, m.right, m.middle1_y + fm.thickness - 1);
		FillRectangle(dc, m.left, m.middle2_y, m.middle1_x, m.middle2_y + fm.thickness - 1);
		FillRectangle(dc, m.middle2_x, m.middle2_y, m.right, m.middle2_y + fm.thickness - 1);
		FillRectangle(dc, m.middle1_x, m.top, m.middle1_x + fm.thickness - 1, m.middle1_y);
		FillRectangle(dc, m.middle1_x, m.middle2_y, m.middle1_x + fm.thickness - 1, m.bottom);
		FillRectangle(dc, m.middle2_x, m.top, m.middle2_x + fm.thickness - 1, m.middle1_y);
		FillRectangle(dc, m.middle2_x, m.middle2_y, m.middle2_x + fm.thickness - 1, m.bottom);
	}

	static void Draw_2564(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ╤ */
	{
		DoubleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left, m.middle1_y, m.right, m.middle1_y + fm.thickness - 1);
		FillRectangle(dc, m.left, m.middle2_y, m.right, m.middle2_y + fm.thickness - 1);
		FillRectangle(dc, m.middle_x, m.middle2_y, m.middle_x + fm.thickness - 1, m.bottom);
	}

	static void Draw_2567(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ╧ */
	{
		DoubleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left, m.middle1_y, m.right, m.middle1_y + fm.thickness - 1);
		FillRectangle(dc, m.left, m.middle2_y, m.right, m.middle2_y + fm.thickness - 1);
		FillRectangle(dc, m.middle_x, m.top, m.middle_x + fm.thickness - 1, m.middle1_y);
	}
///
	static void Draw_2565(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ╥ */
	{
		DoubleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left, m.middle_y, m.right, m.middle_y + fm.thickness - 1);
		FillRectangle(dc, m.middle1_x, m.middle_y, m.middle1_x + fm.thickness - 1, m.bottom);
		FillRectangle(dc, m.middle2_x, m.middle_y, m.middle2_x + fm.thickness - 1, m.bottom);
	}

	static void Draw_256A(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ╪ */
	{
		DoubleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left, m.middle1_y, m.right, m.middle1_y + fm.thickness - 1);
		FillRectangle(dc, m.left, m.middle2_y, m.right, m.middle2_y + fm.thickness - 1);
		FillRectangle(dc, m.middle_x, m.top, m.middle_x + fm.thickness - 1, m.bottom);
	}

	static void Draw_256B(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ╫ */
	{
		DoubleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left, m.middle_y, m.right, m.middle_y + fm.thickness - 1);
		FillRectangle(dc, m.middle1_x, m.top, m.middle1_x + fm.thickness - 1, m.bottom);
		FillRectangle(dc, m.middle2_x, m.top, m.middle2_x + fm.thickness - 1, m.bottom);
	}

	static void Draw_2561(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ╡ */ // + fm.thickness - 1
	{
		DoubleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left, m.middle1_y, m.middle_x + fm.thickness - 1, m.middle1_y + fm.thickness - 1);
		FillRectangle(dc, m.left, m.middle2_y, m.middle_x, m.middle2_y + fm.thickness - 1);
		FillRectangle(dc, m.middle_x, m.top, m.middle_x + fm.thickness - 1, m.middle1_y);
		FillRectangle(dc, m.middle_x, m.middle2_y, m.middle_x + fm.thickness - 1, m.bottom);
	}

	static void Draw_255E(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ╞ */
	{
		DoubleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.middle_x, m.middle1_y, m.right, m.middle1_y + fm.thickness - 1);
		FillRectangle(dc, m.middle_x, m.middle2_y, m.right, m.middle2_y + fm.thickness - 1);
		FillRectangle(dc, m.middle_x, m.top, m.middle_x + fm.thickness - 1, m.middle1_y);
		FillRectangle(dc, m.middle_x, m.middle2_y, m.middle_x + fm.thickness - 1, m.bottom);
	}

	static void Draw_2556(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ╖ */
	{
		DoubleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left, m.middle_y, m.middle2_x, m.middle_y + fm.thickness - 1);
		FillRectangle(dc, m.middle1_x, m.middle_y, m.middle1_x + fm.thickness - 1, m.bottom);
		FillRectangle(dc, m.middle2_x, m.middle_y, m.middle2_x + fm.thickness - 1, m.bottom);
	}

	static void Draw_2555(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ╕ */
	{
		DoubleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left, m.middle1_y, m.middle_x + fm.thickness - 1, m.middle1_y + fm.thickness - 1);
		FillRectangle(dc, m.left, m.middle2_y, m.middle_x, m.middle2_y + fm.thickness - 1);
		FillRectangle(dc, m.middle_x, m.middle1_y, m.middle_x + fm.thickness - 1, m.bottom);
	}

	static void Draw_2568(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ╨ */
	{
		DoubleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left, m.middle_y, m.right, m.middle_y + fm.thickness - 1);
		FillRectangle(dc, m.middle1_x, m.top, m.middle1_x + fm.thickness - 1, m.middle_y);
		FillRectangle(dc, m.middle2_x, m.top, m.middle2_x + fm.thickness - 1, m.middle_y);
	}

	static void Draw_255C(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ╜ */ // + thickness
	{
		DoubleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left, m.middle_y, m.middle2_x + fm.thickness - 1, m.middle_y + fm.thickness - 1);
		FillRectangle(dc, m.middle1_x, m.top, m.middle1_x + fm.thickness - 1, m.middle_y);
		FillRectangle(dc, m.middle2_x, m.top, m.middle2_x + fm.thickness - 1, m.middle_y);
	}

	static void Draw_2559(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ╙ */
	{
		DoubleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.middle1_x, m.middle_y, m.right, m.middle_y + fm.thickness - 1);
		FillRectangle(dc, m.middle1_x, m.top, m.middle1_x + fm.thickness - 1, m.middle_y);
		FillRectangle(dc, m.middle2_x, m.top, m.middle2_x + fm.thickness - 1, m.middle_y);
	}

	static void Draw_2558(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ╘ */
	{
		DoubleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.middle_x, m.middle1_y, m.right, m.middle1_y + fm.thickness - 1);
		FillRectangle(dc, m.middle_x, m.middle2_y, m.right, m.middle2_y + fm.thickness - 1);
		FillRectangle(dc, m.middle_x, m.top, m.middle_x + fm.thickness - 1, m.middle2_y);
	}

	static void Draw_2552(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ╒ */
	{
		DoubleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.middle_x, m.middle1_y, m.right, m.middle1_y + fm.thickness - 1);
		FillRectangle(dc, m.middle_x, m.middle2_y, m.right, m.middle2_y + fm.thickness - 1);
		FillRectangle(dc, m.middle_x, m.middle1_y, m.middle_x + fm.thickness - 1, m.bottom);
	}

	static void Draw_2553(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ╓ */
	{
		DoubleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.middle1_x, m.middle_y, m.right, m.middle_y + fm.thickness - 1);
		FillRectangle(dc, m.middle1_x, m.middle_y, m.middle1_x + fm.thickness - 1, m.bottom);
		FillRectangle(dc, m.middle2_x, m.middle_y, m.middle2_x + fm.thickness - 1, m.bottom);
	}

	static void Draw_255B(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ╛*/ // + thickness
	{
		DoubleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left, m.middle1_y, m.middle_x, m.middle1_y + fm.thickness - 1);
		FillRectangle(dc, m.left, m.middle2_y, m.middle_x, m.middle2_y + fm.thickness - 1);
		FillRectangle(dc, m.middle_x, m.top, m.middle_x + fm.thickness - 1, m.middle2_y + fm.thickness - 1);
	}

	static void Draw_2580(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ▀ */
	{
		CharMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left, m.top, m.right, (m.top + m.bottom) / 2);
	}

	static void Draw_2581(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /*  */
	{
		CharMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left, m.bottom - (fm.fh / 8), m.right, m.bottom);
	}

	static void Draw_2582(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ▂ */
	{
		CharMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left, m.bottom - 2 * (fm.fh / 8), m.right, m.bottom);
	}

	static void Draw_2583(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ▂ */
	{
		CharMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left, m.bottom - 3 * (fm.fh / 8), m.right, m.bottom);
	}

	static void Draw_2584(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ▄ */
	{
		CharMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left, m.bottom - 4 * (fm.fh / 8), m.right, m.bottom);
	}

	static void Draw_2585(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ▅ */
	{
		CharMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left, m.bottom - 5 * (fm.fh / 8), m.right, m.bottom);
	}

	static void Draw_2586(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ▆ */
	{
		CharMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left, m.bottom - 6 * (fm.fh / 8), m.right, m.bottom);
	}

	static void Draw_2587(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ▇ */
	{
		CharMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left, m.bottom - 7 * (fm.fh / 8), m.right, m.bottom);
	}

	static void Draw_2588(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* █ */
	{
		CharMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left, m.top, m.right, m.bottom);
	}


	static void Draw_2589(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ▉ */
	{
		CharMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left, m.top, m.right - (fm.fw / 8), m.bottom);
	}

	static void Draw_258a(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ▊ */
	{
		CharMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left, m.top, m.right - 2 * (fm.fw / 8), m.bottom);
	}

	static void Draw_258b(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ▋ */
	{
		CharMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left, m.top, m.right - 3 * (fm.fw / 8), m.bottom);
	}

	static void Draw_258c(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ▌ */
	{
		CharMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left, m.top, m.right - 4 * (fm.fw / 8), m.bottom);
	}

	static void Draw_258d(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ▍ */
	{
		CharMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left, m.top, m.right - 5 * (fm.fw / 8), m.bottom);
	}

	static void Draw_258e(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ▎ */
	{
		CharMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left, m.top, m.right - 6 * (fm.fw / 8), m.bottom);
	}

	static void Draw_258f(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ▏ */
	{
		CharMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left, m.top, m.right - 7 * (fm.fw / 8), m.bottom);
	}

	static void Draw_2590(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ▐ */
	{
		CharMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left + (fm.fw / 2), m.top, m.right, m.bottom);
	}

	static void Draw_2594(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ▔ */
	{
		CharMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left, m.top, m.right, m.top + (fm.fh / 8));
	}

	static void Draw_2595(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ▕ */
	{
		CharMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.right - (fm.fh / 8), m.top, m.right, m.bottom);
	}


	static void Draw_2596(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ▖ */
	{
		CharMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left, m.top + (fm.fh / 2) - 1, m.left + (fm.fw / 2) - 1, m.bottom);
	}

	static void Draw_2597(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ▗ */
	{
		CharMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left + (fm.fw / 2), m.top + (fm.fh / 2) - 1, m.right, m.bottom);
	}

	static void Draw_2598(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ▘ */
	{
		CharMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left, m.top, m.left + (fm.fw / 2) - 1, m.top + (fm.fh / 2) - 1);
	}

	static void Draw_2599(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ▙ */
	{
		CharMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left, m.top, m.left + (fm.fw / 2) - 1, m.bottom);
		FillRectangle(dc, m.left + (fm.fw / 2), m.top + (fm.fh) / 2, m.right, m.bottom);
	}

	static void Draw_259a(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ▚ */
	{
		CharMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left, m.top, m.left + (fm.fw / 2) - 1, m.top + (fm.fh) / 2 - 1);
		FillRectangle(dc, m.left + (fm.fw / 2), m.top + (fm.fh) / 2, m.right, m.bottom);
	}

	static void Draw_259b(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ▛ */
	{
		CharMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left, m.top, m.right, m.top + (fm.fh) / 2 - 1);
		FillRectangle(dc, m.left, m.top, m.left + (fm.fw / 2) - 1, m.bottom);
	}

	static void Draw_259c(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ▜ */
	{
		CharMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left, m.top, m.right, m.top + (fm.fh / 2) - 1);
		FillRectangle(dc, m.left + (fm.fw / 2), m.top + (fm.fh / 2), m.right, m.bottom);
	}

	static void Draw_259d(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ▝ */
	{
		CharMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left + (fm.fw / 2), m.top, m.right, m.top + (fm.fh / 2) - 1);
	}

	static void Draw_259e(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ▞ */
	{
		CharMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left + (fm.fw / 2), m.top, m.right, m.top + (fm.fh / 2) - 1);
		FillRectangle(dc, m.left, m.top + (fm.fh / 2), m.left + (fm.fw / 2) - 1, m.bottom);
	}

	static void Draw_259f(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ▟ */
	{
		CharMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left + (fm.fw / 2), m.top, m.right, m.top + (fm.fh / 2) - 1);
		FillRectangle(dc, m.left, m.top + (fm.fh / 2), m.right, m.bottom);
	}

	////////////////////////////////////////////////////////////////

	Draw_T Get(const wchar_t c, bool &antialiasible)
	{
		antialiasible = true;
		switch (c) {
			case 0x2500: return Draw_2500;
			case 0x2502: return Draw_2502;
			case 0x250c: return Draw_250C;
			case 0x2510: return Draw_2510;
			case 0x2514: return Draw_2514;
			case 0x2518: return Draw_2518;
			case 0x251c: return Draw_251C;
			case 0x2524: return Draw_2524;
			case 0x252c: return Draw_252C;
			case 0x2534: return Draw_2534;
			case 0x253c: return Draw_253C;
			case 0x2550: return Draw_2550;
			case 0x2551: return Draw_2551;
			case 0x2552: return Draw_2552; /* ╒ */
			case 0x2553: return Draw_2553; /* ╓ */
			case 0x2554: return Draw_2554;
			case 0x2555: return Draw_2555; /* ╕ */
			case 0x2556: return Draw_2556; /* ╖ */
			case 0x2557: return Draw_2557;
			case 0x2558: return Draw_2558; /* ╘ */
			case 0x2559: return Draw_2559; /* ╙ */
			case 0x255a: return Draw_255A;
			case 0x255B: return Draw_255B; /* ╛*/ // + thickness
			case 0x255C: return Draw_255C; /* ╜ */ // + thickness
			case 0x255d: return Draw_255D;
			case 0x255E: return Draw_255E; /* ╞ */
			case 0x255f: return Draw_255F;
			case 0x2560: return Draw_2560; /* ╠ */
			case 0x2561: return Draw_2561; /* ╡ */ // + fm.thickness - 1
			case 0x2562: return Draw_2562;
			case 0x2563: return Draw_2563; /* ╣ */ // + thickness
			case 0x2564: return Draw_2564; /* ╤ */
			case 0x2565: return Draw_2565; /* ╥ */
			case 0x2566: return Draw_2566; /* ╦ */
			case 0x2567: return Draw_2567; /* ╧ */
			case 0x2568: return Draw_2568; /* ╨ */
			case 0x2569: return Draw_2569; /* ╩  */ // + thickness
			case 0x256A: return Draw_256A; /* ╪ */
			case 0x256B: return Draw_256B; /* ╫ */
			case 0x256C: return Draw_256C; /* ╬ */ // + thickness


			case 0x2580: antialiasible = false; return Draw_2580; /* ▀ */
			case 0x2581: antialiasible = false; return Draw_2581; /* ▁ */
			case 0x2582: antialiasible = false; return Draw_2582; /* ▂ */
			case 0x2583: antialiasible = false; return Draw_2583; /* ▃▁ */
			case 0x2584: antialiasible = false; return Draw_2584; /* ▄▁ */
			case 0x2585: antialiasible = false; return Draw_2585; /* ▅▁ */
			case 0x2586: antialiasible = false; return Draw_2586; /* ▆▁ */
			case 0x2587: antialiasible = false; return Draw_2587; /* ▇▁ */
			case 0x2588: antialiasible = false; return Draw_2588; /* █ */
			case 0x2589: antialiasible = false; return Draw_2589;
			case 0x258a: antialiasible = false; return Draw_258a;
			case 0x258b: antialiasible = false; return Draw_258b;
			case 0x258c: antialiasible = false; return Draw_258c;
			case 0x258d: antialiasible = false; return Draw_258d;
			case 0x258e: antialiasible = false; return Draw_258e;
			case 0x258f: antialiasible = false; return Draw_258f;
			case 0x2590: antialiasible = false; return Draw_2590;
#if 0
			case 0x2591: antialiasible = false; return Draw_2591; /* ░ */
			case 0x2592: antialiasible = false; return Draw_2592; /* ▒ */
			case 0x2593: antialiasible = false; return Draw_2593; /* ▓ */
#endif
			case 0x2594: antialiasible = false; return Draw_2594; /* ▔ */
			case 0x2595: antialiasible = false; return Draw_2595; /* ▖ */
			case 0x2596: antialiasible = false; return Draw_2596; /* ▗ */
			case 0x2597: antialiasible = false; return Draw_2597; /* ▘ */
			case 0x2598: antialiasible = false; return Draw_2598; /* ▙ */
			case 0x2599: antialiasible = false; return Draw_2599; /* ▚ */
			case 0x259a: antialiasible = false; return Draw_259a; /* ▛ */
			case 0x259b: antialiasible = false; return Draw_259b; /* ▜ */
			case 0x259c: antialiasible = false; return Draw_259c; /* ▜ */
			case 0x259d: antialiasible = false; return Draw_259d; /* ▝ */
			case 0x259e: antialiasible = false; return Draw_259e; /* ▞ */
			case 0x259f: antialiasible = false; return Draw_259f; /* ▟ */

		}
		return nullptr;
	}
}

/*

        0 	1 	2 	3 	4 	5 	6 	7 	8 	9 	A 	B 	C 	D 	E 	F

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

*/
