#include "CustomDrawChar.h"

namespace WXCustomDrawChar
{

	struct CharMetrics
	{
		CharMetrics(const FontMetrics &fm, unsigned int start_y, unsigned int cx)
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
		SingleLineBoxMetrics(const FontMetrics &fm, unsigned int start_y, unsigned int cx)
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
		DoubleLineBoxMetrics(const FontMetrics &fm, unsigned int start_y, unsigned int cx)
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

	static void FillRectangle(wxPaintDC &dc, wxCoord left, wxCoord top, wxCoord right, wxCoord bottom)
	{
		dc.DrawRectangle(left, top, right + 1 - left , bottom + 1 - top);
	}


	void Draw_2500(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ─ */
	{
		SingleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left, m.middle_y, m.right, m.middle_y + fm.thickness - 1);
	}

	void Draw_2502(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* │ */
	{
		SingleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.middle_x, m.top, m.middle_x + fm.thickness - 1, m.bottom);
	}

	void Draw_250C(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ┌ */
	{
		SingleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.middle_x, m.middle_y, m.right, m.middle_y + fm.thickness - 1);
		FillRectangle(dc, m.middle_x, m.middle_y, m.middle_x + fm.thickness - 1, m.bottom);
	}

	void Draw_2510(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ┐ */
	{
		SingleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left, m.middle_y, m.middle_x, m.middle_y + fm.thickness - 1);
		FillRectangle(dc, m.middle_x, m.middle_y, m.middle_x + fm.thickness - 1, m.bottom);
	}

	void Draw_2514(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* └ */
	{
		SingleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.middle_x, m.middle_y, m.right, m.middle_y + fm.thickness - 1);
		FillRectangle(dc, m.middle_x, m.top, m.middle_x + fm.thickness - 1, m.middle_y);
	}

	void Draw_2518(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ┘ */
	{
		SingleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left, m.middle_y, m.middle_x + fm.thickness - 1, m.middle_y + fm.thickness - 1);
		FillRectangle(dc, m.middle_x, m.top, m.middle_x + fm.thickness - 1, m.middle_y);
	}

	void Draw_251C(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ├ */
	{
		SingleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.middle_x, m.middle_y, m.right, m.middle_y + fm.thickness - 1);
		FillRectangle(dc, m.middle_x, m.top, m.middle_x + fm.thickness - 1, m.bottom);
	}

	void Draw_2524(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ┤ */
	{
		SingleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left, m.middle_y, m.middle_x, m.middle_y + fm.thickness - 1);
		FillRectangle(dc, m.middle_x, m.top, m.middle_x + fm.thickness - 1, m.bottom);
	}

	void Draw_252C(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ┬ */
	{
		SingleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left, m.middle_y, m.right, m.middle_y + fm.thickness - 1);
		FillRectangle(dc, m.middle_x, m.middle_y, m.middle_x + fm.thickness - 1, m.bottom);
	}

	void Draw_2534(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ┴ */
	{
		SingleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left, m.middle_y, m.right, m.middle_y + fm.thickness - 1);
		FillRectangle(dc, m.middle_x, m.top, m.middle_x + fm.thickness - 1, m.middle_y);
	}

	void Draw_253C(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ┼  */
	{
		SingleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left, m.middle_y, m.right, m.middle_y + fm.thickness - 1);
		FillRectangle(dc, m.middle_x, m.top, m.middle_x + fm.thickness - 1, m.bottom);
	}

	void Draw_2550(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ═ */
	{
		DoubleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left, m.middle1_y, m.right, m.middle1_y + fm.thickness - 1);
		FillRectangle(dc, m.left, m.middle2_y, m.right, m.middle2_y + fm.thickness - 1);
	}

	void Draw_2551(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ║ */
	{
		DoubleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.middle1_x, m.top, m.middle1_x + fm.thickness - 1, m.bottom);
		FillRectangle(dc, m.middle2_x, m.top, m.middle2_x + fm.thickness - 1, m.bottom);
	}

	void Draw_2554(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ╔  */
	{
		DoubleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.middle1_x, m.middle1_y, m.right, m.middle1_y + fm.thickness - 1);
		FillRectangle(dc, m.middle2_x, m.middle2_y, m.right, m.middle2_y + fm.thickness - 1);

		FillRectangle(dc, m.middle1_x, m.middle1_y, m.middle1_x + fm.thickness - 1, m.bottom);
		FillRectangle(dc, m.middle2_x, m.middle2_y, m.middle2_x + fm.thickness - 1, m.bottom);
	}

	void Draw_2557(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ╗  */
	{
		DoubleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left, m.middle1_y, m.middle2_x, m.middle1_y + fm.thickness - 1);
		FillRectangle(dc, m.left, m.middle2_y, m.middle1_x, m.middle2_y + fm.thickness - 1);

		FillRectangle(dc, m.middle2_x, m.middle1_y, m.middle2_x + fm.thickness - 1, m.bottom);
		FillRectangle(dc, m.middle1_x, m.middle2_y, m.middle1_x + fm.thickness - 1, m.bottom);
	}

	void Draw_255A(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ╚  */
	{
		DoubleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.middle2_x, m.middle1_y, m.right, m.middle1_y + fm.thickness - 1);
		FillRectangle(dc, m.middle1_x, m.middle2_y, m.right, m.middle2_y + fm.thickness - 1);

		FillRectangle(dc, m.middle1_x, m.top, m.middle1_x + fm.thickness - 1, m.middle2_y);
		FillRectangle(dc, m.middle2_x, m.top, m.middle2_x + fm.thickness - 1, m.middle1_y);
	}

	void Draw_255D(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ╝  */ // + thickness
	{
		DoubleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left, m.middle1_y, m.middle1_x + fm.thickness - 1, m.middle1_y + fm.thickness - 1);
		FillRectangle(dc, m.left, m.middle2_y, m.middle2_x + fm.thickness - 1, m.middle2_y + fm.thickness - 1);

		FillRectangle(dc, m.middle1_x, m.top, m.middle1_x + fm.thickness - 1, m.middle1_y);
		FillRectangle(dc, m.middle2_x, m.top, m.middle2_x + fm.thickness - 1, m.middle2_y);
	}

	void Draw_255F(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ╟  */
	{
		DoubleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.middle2_x, m.middle_y, m.right, m.middle_y + fm.thickness - 1);
		FillRectangle(dc, m.middle1_x, m.top, m.middle1_x + fm.thickness - 1, m.bottom);
		FillRectangle(dc, m.middle2_x, m.top, m.middle2_x + fm.thickness - 1, m.bottom);
	}

	void Draw_2562(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ╢  */
	{
		DoubleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left, m.middle_y, m.middle1_x, m.middle_y + fm.thickness - 1);
		FillRectangle(dc, m.middle1_x, m.top, m.middle1_x + fm.thickness - 1, m.bottom);
		FillRectangle(dc, m.middle2_x, m.top, m.middle2_x + fm.thickness - 1, m.bottom);
	}


	///
	void Draw_2560(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ╠ */
	{
		DoubleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.middle2_x, m.middle1_y, m.right, m.middle1_y + fm.thickness - 1);
		FillRectangle(dc, m.middle2_x, m.middle2_y, m.right, m.middle2_y + fm.thickness - 1);
		FillRectangle(dc, m.middle1_x, m.top, m.middle1_x + fm.thickness - 1, m.bottom);
		FillRectangle(dc, m.middle2_x, m.top, m.middle2_x + fm.thickness - 1, m.middle1_y);
		FillRectangle(dc, m.middle2_x, m.middle2_y, m.middle2_x + fm.thickness - 1, m.bottom);
        }

	void Draw_2563(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ╣ */ // + thickness
	{
		DoubleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left, m.middle1_y, m.middle1_x + fm.thickness - 1, m.middle1_y + fm.thickness - 1);
		FillRectangle(dc, m.left, m.middle2_y, m.middle1_x, m.middle2_y + fm.thickness - 1);
		FillRectangle(dc, m.middle1_x, m.top, m.middle1_x + fm.thickness - 1, m.middle1_y);
		FillRectangle(dc, m.middle1_x, m.middle2_y, m.middle1_x + fm.thickness - 1, m.bottom);
		FillRectangle(dc, m.middle2_x, m.top, m.middle2_x + fm.thickness - 1, m.bottom);
	}
	

	void Draw_2566(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ╦ */
	{
		DoubleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left, m.middle1_y, m.right, m.middle1_y + fm.thickness - 1);
		FillRectangle(dc, m.left, m.middle2_y, m.middle1_x, m.middle2_y + fm.thickness - 1);
		FillRectangle(dc, m.middle2_x, m.middle2_y, m.right, m.middle2_y + fm.thickness - 1);
		FillRectangle(dc, m.middle1_x, m.middle2_y, m.middle1_x + fm.thickness - 1, m.bottom);
		FillRectangle(dc, m.middle2_x, m.middle2_y, m.middle2_x + fm.thickness - 1, m.bottom);
	}

	void Draw_2569(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ╩  */ // + thickness
	{
		DoubleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left, m.middle1_y, m.middle1_x + fm.thickness - 1, m.middle1_y + fm.thickness - 1);
		FillRectangle(dc, m.middle2_x, m.middle1_y, m.right, m.middle1_y + fm.thickness - 1);
		FillRectangle(dc, m.left, m.middle2_y, m.right, m.middle2_y + fm.thickness - 1);
		FillRectangle(dc, m.middle1_x, m.top, m.middle1_x + fm.thickness - 1, m.middle1_y);
		FillRectangle(dc, m.middle2_x, m.top, m.middle2_x + fm.thickness - 1, m.middle1_y);
	}

	void Draw_256C(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ╬ */ // + thickness
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

	void Draw_2564(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ╤ */
	{
		DoubleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left, m.middle1_y, m.right, m.middle1_y + fm.thickness - 1);
		FillRectangle(dc, m.left, m.middle2_y, m.right, m.middle2_y + fm.thickness - 1);
		FillRectangle(dc, m.middle_x, m.middle2_y, m.middle_x + fm.thickness - 1, m.bottom);
	}

	void Draw_2567(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ╧ */
	{
		DoubleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left, m.middle1_y, m.right, m.middle1_y + fm.thickness - 1);
		FillRectangle(dc, m.left, m.middle2_y, m.right, m.middle2_y + fm.thickness - 1);
		FillRectangle(dc, m.middle_x, m.top, m.middle_x + fm.thickness - 1, m.middle1_y);
	}
///
	void Draw_2565(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ╥ */
	{
		DoubleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left, m.middle_y, m.right, m.middle_y + fm.thickness - 1);
		FillRectangle(dc, m.middle1_x, m.middle_y, m.middle1_x + fm.thickness - 1, m.bottom);
		FillRectangle(dc, m.middle2_x, m.middle_y, m.middle2_x + fm.thickness - 1, m.bottom);
	}

	void Draw_256A(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ╪ */
	{
		DoubleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left, m.middle1_y, m.right, m.middle1_y + fm.thickness - 1);
		FillRectangle(dc, m.left, m.middle2_y, m.right, m.middle2_y + fm.thickness - 1);
		FillRectangle(dc, m.middle_x, m.top, m.middle_x + fm.thickness - 1, m.bottom);
	}

	void Draw_256B(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ╫ */
	{
		DoubleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left, m.middle_y, m.right, m.middle_y + fm.thickness - 1);
		FillRectangle(dc, m.middle1_x, m.top, m.middle1_x + fm.thickness - 1, m.bottom);
		FillRectangle(dc, m.middle2_x, m.top, m.middle2_x + fm.thickness - 1, m.bottom);
	}

	void Draw_2561(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ╡ */ // + fm.thickness - 1
	{
		DoubleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left, m.middle1_y, m.middle_x + fm.thickness - 1, m.middle1_y + fm.thickness - 1);
		FillRectangle(dc, m.left, m.middle2_y, m.middle_x, m.middle2_y + fm.thickness - 1);
		FillRectangle(dc, m.middle_x, m.top, m.middle_x + fm.thickness - 1, m.middle1_y);
		FillRectangle(dc, m.middle_x, m.middle2_y, m.middle_x + fm.thickness - 1, m.bottom);
	}

	void Draw_255E(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ╞ */
	{
		DoubleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.middle_x, m.middle1_y, m.right, m.middle1_y + fm.thickness - 1);
		FillRectangle(dc, m.middle_x, m.middle2_y, m.right, m.middle2_y + fm.thickness - 1);
		FillRectangle(dc, m.middle_x, m.top, m.middle_x + fm.thickness - 1, m.middle1_y);
		FillRectangle(dc, m.middle_x, m.middle2_y, m.middle_x + fm.thickness - 1, m.bottom);
	}

	void Draw_2556(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ╖ */
	{
		DoubleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left, m.middle_y, m.middle2_x, m.middle_y + fm.thickness - 1);
		FillRectangle(dc, m.middle1_x, m.middle_y, m.middle1_x + fm.thickness - 1, m.bottom);
		FillRectangle(dc, m.middle2_x, m.middle_y, m.middle2_x + fm.thickness - 1, m.bottom);
	}

	void Draw_2555(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ╕ */
	{
		DoubleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left, m.middle1_y, m.middle_x + fm.thickness - 1, m.middle1_y + fm.thickness - 1);
		FillRectangle(dc, m.left, m.middle2_y, m.middle_x, m.middle2_y + fm.thickness - 1);
		FillRectangle(dc, m.middle_x, m.middle1_y, m.middle_x + fm.thickness - 1, m.bottom);
	}

	void Draw_2568(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ╨ */
	{
		DoubleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left, m.middle_y, m.right, m.middle_y + fm.thickness - 1);
		FillRectangle(dc, m.middle1_x, m.top, m.middle1_x + fm.thickness - 1, m.middle_y);
		FillRectangle(dc, m.middle2_x, m.top, m.middle2_x + fm.thickness - 1, m.middle_y);
	}

	void Draw_255C(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ╜ */ // + thickness
	{
		DoubleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left, m.middle_y, m.middle2_x + fm.thickness - 1, m.middle_y + fm.thickness - 1);
		FillRectangle(dc, m.middle1_x, m.top, m.middle1_x + fm.thickness - 1, m.middle_y);
		FillRectangle(dc, m.middle2_x, m.top, m.middle2_x + fm.thickness - 1, m.middle_y);
	}

	void Draw_2559(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ╙ */
	{
		DoubleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.middle1_x, m.middle_y, m.right, m.middle_y + fm.thickness - 1);
		FillRectangle(dc, m.middle1_x, m.top, m.middle1_x + fm.thickness - 1, m.middle_y);
		FillRectangle(dc, m.middle2_x, m.top, m.middle2_x + fm.thickness - 1, m.middle_y);
	}

	void Draw_2558(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ╘ */
	{
		DoubleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.middle_x, m.middle1_y, m.right, m.middle1_y + fm.thickness - 1);
		FillRectangle(dc, m.middle_x, m.middle2_y, m.right, m.middle2_y + fm.thickness - 1);
		FillRectangle(dc, m.middle_x, m.top, m.middle_x + fm.thickness - 1, m.middle2_y);
	}

	void Draw_2552(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ╒ */
	{
		DoubleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.middle_x, m.middle1_y, m.right, m.middle1_y + fm.thickness - 1);
		FillRectangle(dc, m.middle_x, m.middle2_y, m.right, m.middle2_y + fm.thickness - 1);
		FillRectangle(dc, m.middle_x, m.middle1_y, m.middle_x + fm.thickness - 1, m.bottom);
	}

	void Draw_2553(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ╓ */
	{
		DoubleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.middle1_x, m.middle_y, m.right, m.middle_y + fm.thickness - 1);
		FillRectangle(dc, m.middle1_x, m.middle_y, m.middle1_x + fm.thickness - 1, m.bottom);
		FillRectangle(dc, m.middle2_x, m.middle_y, m.middle2_x + fm.thickness - 1, m.bottom);
	}

	void Draw_255B(wxPaintDC &dc, const FontMetrics &fm, unsigned int start_y, unsigned int cx) /* ╛*/ // + thickness
	{
		DoubleLineBoxMetrics m(fm, start_y, cx);
		FillRectangle(dc, m.left, m.middle1_y, m.middle_x, m.middle1_y + fm.thickness - 1);
		FillRectangle(dc, m.left, m.middle2_y, m.middle_x, m.middle2_y + fm.thickness - 1);
		FillRectangle(dc, m.middle_x, m.top, m.middle_x + fm.thickness - 1, m.middle2_y + fm.thickness - 1);
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
			case 0x2554: return Draw_2554;
			case 0x2557: return Draw_2557;
			case 0x255a: return Draw_255A;
			case 0x255d: return Draw_255D;
			case 0x255f: return Draw_255F;
			case 0x2562: return Draw_2562;

			case 0x2560: return Draw_2560; /* ╠ */
			case 0x2563: return Draw_2563; /* ╣ */ // + thickness
			case 0x2566: return Draw_2566; /* ╦ */
			case 0x2569: return Draw_2569; /* ╩  */ // + thickness
			case 0x256C: return Draw_256C; /* ╬ */ // + thickness
			case 0x2564: return Draw_2564; /* ╤ */
			case 0x2567: return Draw_2567; /* ╧ */
			case 0x2565: return Draw_2565; /* ╥ */
			case 0x256A: return Draw_256A; /* ╪ */
			case 0x256B: return Draw_256B; /* ╫ */
			case 0x2561: return Draw_2561; /* ╡ */ // + fm.thickness - 1
			case 0x255E: return Draw_255E; /* ╞ */
			case 0x2556: return Draw_2556; /* ╖ */
			case 0x2555: return Draw_2555; /* ╕ */
			case 0x2568: return Draw_2568; /* ╨ */
			case 0x255C: return Draw_255C; /* ╜ */ // + thickness
			case 0x2559: return Draw_2559; /* ╙ */
			case 0x2558: return Draw_2558; /* ╘ */
			case 0x2552: return Draw_2552; /* ╒ */
			case 0x2553: return Draw_2553; /* ╓ */
			case 0x255B: return Draw_255B; /* ╛*/ // + thickness
		}
		return nullptr;
	}
}
