#include "CustomDrawChar.h"

namespace WXCustomDrawChar
{

	struct Metrics
	{
		Metrics(ConsolePaintContext *context, unsigned int start_y, unsigned int cx)
		:
			fw(context->FontWidth()),
			fh(context->FontHeight()),
			thickness(context->FontThickness())
		{
			left = cx * fw;
			right = left + fw - 1;
			top = start_y;
			bottom = top + fh - 1;
		}

		wxCoord fw;
		wxCoord fh;
		wxCoord thickness;

		wxCoord left;
		wxCoord right;
		wxCoord top;
		wxCoord bottom;
	};

	struct SingleLineBoxMetrics : Metrics
	{
		SingleLineBoxMetrics(ConsolePaintContext *context, unsigned int start_y, unsigned int cx)
			: Metrics(context, start_y, cx)
		{
			middle_y = top + (fh - thickness) / 2;
			middle_x = left + (fw - thickness) / 2;
		}

		wxCoord middle_y;
		wxCoord middle_x;
	};

	struct DoubleLineBoxMetrics : SingleLineBoxMetrics
	{
		DoubleLineBoxMetrics(ConsolePaintContext *context, unsigned int start_y, unsigned int cx)
			: SingleLineBoxMetrics(context, start_y, cx)
		{
			middle1_y = middle_y - thickness;
			middle1_x = middle_x - thickness;

			middle2_y = middle_y + thickness;
			middle2_x = middle_x + thickness;
		}

		wxCoord middle1_y, middle2_y;
		wxCoord middle1_x, middle2_x;
	};

	static void FillRectangle(wxPaintDC &dc, wxCoord left, wxCoord top, wxCoord right, wxCoord bottom)
	{
		dc.DrawRectangle(left, top, right + 1 - left , bottom + 1 - top);
	}

	void Draw_2500(ConsolePaintContext *context, wxPaintDC &dc, unsigned int start_y, unsigned int cx) /* ─ */
	{
		SingleLineBoxMetrics m(context, start_y, cx);
		FillRectangle(dc, m.left, m.middle_y, m.right, m.middle_y + m.thickness - 1);
	}

	void Draw_2502(ConsolePaintContext *context, wxPaintDC &dc, unsigned int start_y, unsigned int cx) /* │ */
	{
		SingleLineBoxMetrics m(context, start_y, cx);
		FillRectangle(dc, m.middle_x, m.top, m.middle_x + m.thickness - 1, m.bottom);
	}

	void Draw_250C(ConsolePaintContext *context, wxPaintDC &dc, unsigned int start_y, unsigned int cx) /* ┌ */
	{
		SingleLineBoxMetrics m(context, start_y, cx);
		FillRectangle(dc, m.middle_x, m.middle_y, m.right, m.middle_y + m.thickness - 1);
		FillRectangle(dc, m.middle_x, m.middle_y, m.middle_x + m.thickness - 1, m.bottom);
	}

	void Draw_2510(ConsolePaintContext *context, wxPaintDC &dc, unsigned int start_y, unsigned int cx) /* ┐ */
	{
		SingleLineBoxMetrics m(context, start_y, cx);
		FillRectangle(dc, m.left, m.middle_y, m.middle_x, m.middle_y + m.thickness - 1);
		FillRectangle(dc, m.middle_x, m.middle_y, m.middle_x + m.thickness - 1, m.bottom);
	}

	void Draw_2514(ConsolePaintContext *context, wxPaintDC &dc, unsigned int start_y, unsigned int cx) /* └ */
	{
		SingleLineBoxMetrics m(context, start_y, cx);
		FillRectangle(dc, m.middle_x, m.middle_y, m.right, m.middle_y + m.thickness - 1);
		FillRectangle(dc, m.middle_x, m.top, m.middle_x + m.thickness - 1, m.middle_y);
	}

	void Draw_2518(ConsolePaintContext *context, wxPaintDC &dc, unsigned int start_y, unsigned int cx) /* ┘ */
	{
		SingleLineBoxMetrics m(context, start_y, cx);
		FillRectangle(dc, m.left, m.middle_y, m.middle_x + m.thickness - 1, m.middle_y + m.thickness - 1);
		FillRectangle(dc, m.middle_x, m.top, m.middle_x + m.thickness - 1, m.middle_y);
	}

	void Draw_251C(ConsolePaintContext *context, wxPaintDC &dc, unsigned int start_y, unsigned int cx) /* ├ */
	{
		SingleLineBoxMetrics m(context, start_y, cx);
		FillRectangle(dc, m.middle_x, m.middle_y, m.right, m.middle_y + m.thickness - 1);
		FillRectangle(dc, m.middle_x, m.top, m.middle_x + m.thickness - 1, m.bottom);
	}

	void Draw_2524(ConsolePaintContext *context, wxPaintDC &dc, unsigned int start_y, unsigned int cx) /* ┤ */
	{
		SingleLineBoxMetrics m(context, start_y, cx);
		FillRectangle(dc, m.left, m.middle_y, m.middle_x, m.middle_y + m.thickness - 1);
		FillRectangle(dc, m.middle_x, m.top, m.middle_x + m.thickness - 1, m.bottom);
	}

	void Draw_252C(ConsolePaintContext *context, wxPaintDC &dc, unsigned int start_y, unsigned int cx) /* ┬ */
	{
		SingleLineBoxMetrics m(context, start_y, cx);
		FillRectangle(dc, m.left, m.middle_y, m.right, m.middle_y + m.thickness - 1);
		FillRectangle(dc, m.middle_x, m.middle_y, m.middle_x + m.thickness - 1, m.bottom);
	}

	void Draw_2534(ConsolePaintContext *context, wxPaintDC &dc, unsigned int start_y, unsigned int cx) /* ┴ */
	{
		SingleLineBoxMetrics m(context, start_y, cx);
		FillRectangle(dc, m.left, m.middle_y, m.right, m.middle_y + m.thickness - 1);
		FillRectangle(dc, m.middle_x, m.top, m.middle_x + m.thickness - 1, m.middle_y);
	}

	void Draw_253C(ConsolePaintContext *context, wxPaintDC &dc, unsigned int start_y, unsigned int cx) /* ┼  */
	{
		SingleLineBoxMetrics m(context, start_y, cx);
		FillRectangle(dc, m.left, m.middle_y, m.right, m.middle_y + m.thickness - 1);
		FillRectangle(dc, m.middle_x, m.top, m.middle_x + m.thickness - 1, m.bottom);
	}

	void Draw_2550(ConsolePaintContext *context, wxPaintDC &dc, unsigned int start_y, unsigned int cx) /* ═ */
	{
		DoubleLineBoxMetrics m(context, start_y, cx);
		FillRectangle(dc, m.left, m.middle1_y, m.right, m.middle1_y + m.thickness - 1);
		FillRectangle(dc, m.left, m.middle2_y, m.right, m.middle2_y + m.thickness - 1);
	}

	void Draw_2551(ConsolePaintContext *context, wxPaintDC &dc, unsigned int start_y, unsigned int cx) /* ║ */
	{
		DoubleLineBoxMetrics m(context, start_y, cx);
		FillRectangle(dc, m.middle1_x, m.top, m.middle1_x + m.thickness - 1, m.bottom);
		FillRectangle(dc, m.middle2_x, m.top, m.middle2_x + m.thickness - 1, m.bottom);
	}

	void Draw_2554(ConsolePaintContext *context, wxPaintDC &dc, unsigned int start_y, unsigned int cx) /* ╔  */
	{
		DoubleLineBoxMetrics m(context, start_y, cx);
		FillRectangle(dc, m.middle1_x, m.middle1_y, m.right, m.middle1_y + m.thickness - 1);
		FillRectangle(dc, m.middle2_x, m.middle2_y, m.right, m.middle2_y + m.thickness - 1);

		FillRectangle(dc, m.middle1_x, m.middle1_y, m.middle1_x + m.thickness - 1, m.bottom);
		FillRectangle(dc, m.middle2_x, m.middle2_y, m.middle2_x + m.thickness - 1, m.bottom);
	}

	void Draw_2557(ConsolePaintContext *context, wxPaintDC &dc, unsigned int start_y, unsigned int cx) /* ╗  */
	{
		DoubleLineBoxMetrics m(context, start_y, cx);
		FillRectangle(dc, m.left, m.middle1_y, m.middle2_x, m.middle1_y + m.thickness - 1);
		FillRectangle(dc, m.left, m.middle2_y, m.middle1_x, m.middle2_y + m.thickness - 1);

		FillRectangle(dc, m.middle2_x, m.middle1_y, m.middle2_x + m.thickness - 1, m.bottom);
		FillRectangle(dc, m.middle1_x, m.middle2_y, m.middle1_x + m.thickness - 1, m.bottom);
	}

	void Draw_255A(ConsolePaintContext *context, wxPaintDC &dc, unsigned int start_y, unsigned int cx) /* ╚  */
	{
		DoubleLineBoxMetrics m(context, start_y, cx);
		FillRectangle(dc, m.middle2_x, m.middle1_y, m.right, m.middle1_y + m.thickness - 1);
		FillRectangle(dc, m.middle1_x, m.middle2_y, m.right, m.middle2_y + m.thickness - 1);

		FillRectangle(dc, m.middle1_x, m.top, m.middle1_x + m.thickness - 1, m.middle2_y);
		FillRectangle(dc, m.middle2_x, m.top, m.middle2_x + m.thickness - 1, m.middle1_y);
	}

	void Draw_255D(ConsolePaintContext *context, wxPaintDC &dc, unsigned int start_y, unsigned int cx) /* ╝  */ // + thickness
	{
		DoubleLineBoxMetrics m(context, start_y, cx);
		FillRectangle(dc, m.left, m.middle1_y, m.middle1_x + m.thickness - 1, m.middle1_y + m.thickness - 1);
		FillRectangle(dc, m.left, m.middle2_y, m.middle2_x + m.thickness - 1, m.middle2_y + m.thickness - 1);

		FillRectangle(dc, m.middle1_x, m.top, m.middle1_x + m.thickness - 1, m.middle1_y);
		FillRectangle(dc, m.middle2_x, m.top, m.middle2_x + m.thickness - 1, m.middle2_y);
	}

	void Draw_255F(ConsolePaintContext *context, wxPaintDC &dc, unsigned int start_y, unsigned int cx) /* ╟  */
	{
		DoubleLineBoxMetrics m(context, start_y, cx);
		FillRectangle(dc, m.middle2_x, m.middle_y, m.right, m.middle_y + m.thickness - 1);
		FillRectangle(dc, m.middle1_x, m.top, m.middle1_x + m.thickness - 1, m.bottom);
		FillRectangle(dc, m.middle2_x, m.top, m.middle2_x + m.thickness - 1, m.bottom);
	}

	void Draw_2562(ConsolePaintContext *context, wxPaintDC &dc, unsigned int start_y, unsigned int cx) /* ╢  */
	{
		DoubleLineBoxMetrics m(context, start_y, cx);
		FillRectangle(dc, m.left, m.middle_y, m.middle1_x, m.middle_y + m.thickness - 1);
		FillRectangle(dc, m.middle1_x, m.top, m.middle1_x + m.thickness - 1, m.bottom);
		FillRectangle(dc, m.middle2_x, m.top, m.middle2_x + m.thickness - 1, m.bottom);
	}


	////////////////////////////////////////////////////////////////

	Draw_T Get(const wchar_t c)
	{
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
		}
		return nullptr;
	}
}
