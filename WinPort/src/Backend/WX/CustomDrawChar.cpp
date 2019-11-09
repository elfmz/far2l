#include "CustomDrawChar.h"

namespace WXCustomDrawChar
{

	struct CharMetrics
	{
		inline CharMetrics(ICharPaintContext &ctx, unsigned int start_y, unsigned int cx)
		{
			left = cx * ctx.fw;
			right = left + ctx.fw - 1;
			top = start_y;
			bottom = top + ctx.fh - 1;
		}

		wxCoord left;
		wxCoord right;
		wxCoord top;
		wxCoord bottom;
	};

	struct SingleLineBoxMetrics : CharMetrics
	{
		inline SingleLineBoxMetrics(ICharPaintContext &ctx, unsigned int start_y, unsigned int cx)
			: CharMetrics(ctx, start_y, cx)
		{
			middle_y = top + ctx.fh / 2 - ctx.thickness / 2;
			middle_x = left + ctx.fw / 2 - ctx.thickness / 2;
		}

		wxCoord middle_y;
		wxCoord middle_x;
	};

	struct DoubleLineBoxMetrics : SingleLineBoxMetrics
	{
		inline DoubleLineBoxMetrics(ICharPaintContext &ctx, unsigned int start_y, unsigned int cx)
			: SingleLineBoxMetrics(ctx, start_y, cx)
		{
			wxCoord ofs = std::min(ctx.fh, ctx.fw) / 4;

			middle1_y = middle_y - ofs;
			middle1_x = middle_x - ofs;

			middle2_y = middle_y + ofs;
			middle2_x = middle_x + ofs;
		}

		wxCoord middle1_y, middle2_y;
		wxCoord middle1_x, middle2_x;
	};

	template <DrawT DRAW>
		static void Draw_Thicker(wxPaintDC &dc, ICharPaintContext &ctx, unsigned int start_y, unsigned int cx) /* ─ */
	{
		auto saved_thickness = ctx.thickness;
		ctx.thickness = 1 + (ctx.thickness * 3) / 2;
		DRAW(dc, ctx, start_y, cx);
		ctx.thickness = saved_thickness;
	}


	static inline void FillRectangle(wxPaintDC &dc, wxCoord left, wxCoord top, wxCoord right, wxCoord bottom)
	{
		dc.DrawRectangle(left, top, right + 1 - left , bottom + 1 - top);
	}

	static inline void FillPixel(wxPaintDC &dc, wxCoord left, wxCoord top)
	{
		dc.DrawRectangle(left, top, 1, 1);
	}

	static void Draw_2500(wxPaintDC &dc, ICharPaintContext &ctx, unsigned int start_y, unsigned int cx) /* ─ */
	{
		SingleLineBoxMetrics m(ctx, start_y, cx);
		FillRectangle(dc, m.left, m.middle_y, m.right, m.middle_y + ctx.thickness - 1);
		if (ctx.SetColorFaded()) {
			FillRectangle(dc, m.left, m.middle_y - 1, m.right, m.middle_y - 1);
		}
	}

	static void Draw_2502(wxPaintDC &dc, ICharPaintContext &ctx, unsigned int start_y, unsigned int cx) /* │ */
	{
		SingleLineBoxMetrics m(ctx, start_y, cx);
		FillRectangle(dc, m.middle_x, m.top, m.middle_x + ctx.thickness - 1, m.bottom);
		if (ctx.SetColorFaded()) {
			FillRectangle(dc, m.middle_x - 1, m.top, m.middle_x - 1, m.bottom);
		}
	}


	static void Draw_250C(wxPaintDC &dc, ICharPaintContext &ctx, unsigned int start_y, unsigned int cx) /* ┌ */
	{
		SingleLineBoxMetrics m(ctx, start_y, cx);
		FillRectangle(dc, m.middle_x, m.middle_y, m.right, m.middle_y + ctx.thickness - 1);
		FillRectangle(dc, m.middle_x, m.middle_y, m.middle_x + ctx.thickness - 1, m.bottom);
		if (ctx.SetColorFaded()) {
			FillRectangle(dc, m.middle_x, m.middle_y - 1, m.right, m.middle_y - 1);
			FillRectangle(dc, m.middle_x - 1, m.middle_y, m.middle_x - 1, m.bottom);
			if (ctx.SetColorExtraFaded()) {
				FillPixel(dc, m.middle_x - 1, m.middle_y - 1);
			}
		}
	}


	static void Draw_2510(wxPaintDC &dc, ICharPaintContext &ctx, unsigned int start_y, unsigned int cx) /* ┐ */
	{
		SingleLineBoxMetrics m(ctx, start_y, cx);
		FillRectangle(dc, m.left, m.middle_y, m.middle_x, m.middle_y + ctx.thickness - 1);
		FillRectangle(dc, m.middle_x, m.middle_y, m.middle_x + ctx.thickness - 1, m.bottom);
		if (ctx.SetColorFaded()) {
			FillRectangle(dc, m.left, m.middle_y - 1, m.middle_x + ctx.thickness - 1, m.middle_y - 1);
			FillRectangle(dc, m.middle_x - 1, m.middle_y + ctx.thickness, m.middle_x - 1, m.bottom);
		}
	}


	static void Draw_2514(wxPaintDC &dc, ICharPaintContext &ctx, unsigned int start_y, unsigned int cx) /* └ */
	{
		SingleLineBoxMetrics m(ctx, start_y, cx);
		FillRectangle(dc, m.middle_x, m.middle_y, m.right, m.middle_y + ctx.thickness - 1);
		FillRectangle(dc, m.middle_x, m.top, m.middle_x + ctx.thickness - 1, m.middle_y);
		if (ctx.SetColorFaded()) {
			FillRectangle(dc, m.middle_x + ctx.thickness, m.middle_y - 1, m.right, m.middle_y - 1);
			FillRectangle(dc, m.middle_x - 1, m.top, m.middle_x - 1, m.middle_y + ctx.thickness - 1);
		}
	}


	static void Draw_2518(wxPaintDC &dc, ICharPaintContext &ctx, unsigned int start_y, unsigned int cx) /* ┘ */
	{
		SingleLineBoxMetrics m(ctx, start_y, cx);
		FillRectangle(dc, m.left, m.middle_y, m.middle_x + ctx.thickness - 1, m.middle_y + ctx.thickness - 1);
		FillRectangle(dc, m.middle_x, m.top, m.middle_x + ctx.thickness - 1, m.middle_y);
		if (ctx.SetColorFaded()) {
			FillRectangle(dc, m.left, m.middle_y - 1, m.middle_x - 1, m.middle_y - 1);
			FillRectangle(dc, m.middle_x - 1, m.top, m.middle_x - 1, m.middle_y - 1);
			if (ctx.SetColorExtraFaded()) {
				FillPixel(dc, m.middle_x - 1, m.middle_y - 1);
			}
		}
	}


	static void Draw_251C(wxPaintDC &dc, ICharPaintContext &ctx, unsigned int start_y, unsigned int cx) /* ├ */
	{
		SingleLineBoxMetrics m(ctx, start_y, cx);
		FillRectangle(dc, m.middle_x, m.middle_y, m.right, m.middle_y + ctx.thickness - 1);
		FillRectangle(dc, m.middle_x, m.top, m.middle_x + ctx.thickness - 1, m.bottom);
		if (ctx.SetColorFaded()) {
			FillRectangle(dc, m.middle_x + ctx.thickness, m.middle_y - 1, m.right, m.middle_y - 1);
			FillRectangle(dc, m.middle_x - 1, m.top, m.middle_x - 1, m.bottom);
		}
	}


	static void Draw_2524(wxPaintDC &dc, ICharPaintContext &ctx, unsigned int start_y, unsigned int cx) /* ┤ */
	{
		SingleLineBoxMetrics m(ctx, start_y, cx);
		FillRectangle(dc, m.left, m.middle_y, m.middle_x, m.middle_y + ctx.thickness - 1);
		FillRectangle(dc, m.middle_x, m.top, m.middle_x + ctx.thickness - 1, m.bottom);
		if (ctx.SetColorFaded()) {
			FillRectangle(dc, m.left, m.middle_y - 1, m.middle_x - 1, m.middle_y - 1);
			FillRectangle(dc, m.middle_x - 1, m.top, m.middle_x - 1, m.middle_y - 1);
			FillRectangle(dc, m.middle_x - 1, m.middle_y + ctx.thickness, m.middle_x - 1, m.bottom);
			if (ctx.SetColorExtraFaded()) {
				FillPixel(dc, m.middle_x - 1, m.middle_y + ctx.thickness - 2); // WTF?
			}
		}
	}


	static void Draw_252C(wxPaintDC &dc, ICharPaintContext &ctx, unsigned int start_y, unsigned int cx) /* ┬ */
	{
		SingleLineBoxMetrics m(ctx, start_y, cx);
		FillRectangle(dc, m.left, m.middle_y, m.right, m.middle_y + ctx.thickness - 1);
		FillRectangle(dc, m.middle_x, m.middle_y, m.middle_x + ctx.thickness - 1, m.bottom);
		if (ctx.SetColorFaded()) {
			FillRectangle(dc, m.left, m.middle_y - 1, m.right, m.middle_y - 1);
			FillRectangle(dc, m.middle_x - 1, m.middle_y + ctx.thickness, m.middle_x - 1, m.bottom);
		}
	}


	static void Draw_2534(wxPaintDC &dc, ICharPaintContext &ctx, unsigned int start_y, unsigned int cx) /* ┴ */
	{
		SingleLineBoxMetrics m(ctx, start_y, cx);
		FillRectangle(dc, m.left, m.middle_y, m.right, m.middle_y + ctx.thickness - 1);
		FillRectangle(dc, m.middle_x, m.top, m.middle_x + ctx.thickness - 1, m.middle_y);
		if (ctx.SetColorFaded()) {
			FillRectangle(dc, m.left, m.middle_y - 1, m.middle_x - 1, m.middle_y - 1);
			FillRectangle(dc, m.middle_x + ctx.thickness, m.middle_y - 1, m.right, m.middle_y - 1);
			FillRectangle(dc, m.middle_x - 1, m.top, m.middle_x - 1, m.middle_y - 1);
			if (ctx.SetColorExtraFaded()) {
				FillPixel(dc, m.middle_x + ctx.thickness - 2, m.middle_y - 1);// wtf
			}
		}
	}


	static void Draw_253C(wxPaintDC &dc, ICharPaintContext &ctx, unsigned int start_y, unsigned int cx) /* ┼  */
	{
		SingleLineBoxMetrics m(ctx, start_y, cx);
		FillRectangle(dc, m.left, m.middle_y, m.right, m.middle_y + ctx.thickness - 1);
		FillRectangle(dc, m.middle_x, m.top, m.middle_x + ctx.thickness - 1, m.bottom);
		if (ctx.SetColorFaded()) {
			FillRectangle(dc, m.left, m.middle_y - 1, m.middle_x - 1, m.middle_y - 1);
			FillRectangle(dc, m.middle_x + ctx.thickness, m.middle_y - 1, m.right, m.middle_y - 1);
			FillRectangle(dc, m.middle_x - 1, m.top, m.middle_x - 1, m.middle_y - 1);
			FillRectangle(dc, m.middle_x - 1, m.middle_y + ctx.thickness, m.middle_x - 1, m.bottom);
		}
	}


	static void Draw_2550(wxPaintDC &dc, ICharPaintContext &ctx, unsigned int start_y, unsigned int cx) /* ═ */
	{
		DoubleLineBoxMetrics m(ctx, start_y, cx);
		FillRectangle(dc, m.left, m.middle1_y, m.right, m.middle1_y + ctx.thickness - 1);
		FillRectangle(dc, m.left, m.middle2_y, m.right, m.middle2_y + ctx.thickness - 1);
		if (ctx.SetColorFaded()) {
			FillRectangle(dc, m.left, m.middle1_y - 1, m.right, m.middle1_y - 1);
			FillRectangle(dc, m.left, m.middle2_y - 1, m.right, m.middle2_y - 1);
		}
	}


	static void Draw_2551(wxPaintDC &dc, ICharPaintContext &ctx, unsigned int start_y, unsigned int cx) /* ║ */
	{
		DoubleLineBoxMetrics m(ctx, start_y, cx);
		FillRectangle(dc, m.middle1_x, m.top, m.middle1_x + ctx.thickness - 1, m.bottom);
		FillRectangle(dc, m.middle2_x, m.top, m.middle2_x + ctx.thickness - 1, m.bottom);
		if (ctx.SetColorFaded()) {
			FillRectangle(dc, m.middle1_x - 1, m.top, m.middle1_x - 1, m.bottom);
			FillRectangle(dc, m.middle2_x - 1, m.top, m.middle2_x - 1, m.bottom);
		}
	}


	static void Draw_2554(wxPaintDC &dc, ICharPaintContext &ctx, unsigned int start_y, unsigned int cx) /* ╔  */
	{
		DoubleLineBoxMetrics m(ctx, start_y, cx);
		FillRectangle(dc, m.middle1_x, m.middle1_y, m.right, m.middle1_y + ctx.thickness - 1);
		FillRectangle(dc, m.middle2_x, m.middle2_y, m.right, m.middle2_y + ctx.thickness - 1);

		FillRectangle(dc, m.middle1_x, m.middle1_y, m.middle1_x + ctx.thickness - 1, m.bottom);
		FillRectangle(dc, m.middle2_x, m.middle2_y, m.middle2_x + ctx.thickness - 1, m.bottom);

		if (ctx.SetColorFaded()) {
			FillRectangle(dc, m.middle1_x, m.middle1_y - 1, m.right, m.middle1_y - 1);
			FillRectangle(dc, m.middle2_x, m.middle2_y - 1, m.right, m.middle2_y - 1);

			FillRectangle(dc, m.middle1_x - 1, m.middle1_y, m.middle1_x - 1, m.bottom);
			FillRectangle(dc, m.middle2_x - 1, m.middle2_y, m.middle2_x - 1, m.bottom);
			if (ctx.SetColorExtraFaded()) {
				FillPixel(dc, m.middle1_x - 1, m.middle1_y - 1);
				FillPixel(dc, m.middle2_x - 1, m.middle2_y - 1);
			}
		}
	}


	static void Draw_2557(wxPaintDC &dc, ICharPaintContext &ctx, unsigned int start_y, unsigned int cx) /* ╗  */
	{
		DoubleLineBoxMetrics m(ctx, start_y, cx);
		FillRectangle(dc, m.left, m.middle1_y, m.middle2_x, m.middle1_y + ctx.thickness - 1);
		FillRectangle(dc, m.left, m.middle2_y, m.middle1_x, m.middle2_y + ctx.thickness - 1);

		FillRectangle(dc, m.middle2_x, m.middle1_y, m.middle2_x + ctx.thickness - 1, m.bottom);
		FillRectangle(dc, m.middle1_x, m.middle2_y, m.middle1_x + ctx.thickness - 1, m.bottom);
		if (ctx.SetColorFaded()) {
			FillRectangle(dc, m.left, m.middle1_y - 1, m.middle2_x, m.middle1_y - 1);
			FillRectangle(dc, m.left, m.middle2_y - 1, m.middle1_x, m.middle2_y - 1);

			FillRectangle(dc, m.middle2_x - 1, m.middle1_y + ctx.thickness, m.middle2_x - 1, m.bottom);
			FillRectangle(dc, m.middle1_x - 1, m.middle2_y + ctx.thickness, m.middle1_x - 1, m.bottom);
		}
	}


	static void Draw_255A(wxPaintDC &dc, ICharPaintContext &ctx, unsigned int start_y, unsigned int cx) /* ╚  */
	{
		DoubleLineBoxMetrics m(ctx, start_y, cx);
		FillRectangle(dc, m.middle2_x, m.middle1_y, m.right, m.middle1_y + ctx.thickness - 1);
		FillRectangle(dc, m.middle1_x, m.middle2_y, m.right, m.middle2_y + ctx.thickness - 1);

		FillRectangle(dc, m.middle1_x, m.top, m.middle1_x + ctx.thickness - 1, m.middle2_y);
		FillRectangle(dc, m.middle2_x, m.top, m.middle2_x + ctx.thickness - 1, m.middle1_y);

		if (ctx.SetColorFaded()) {
			FillRectangle(dc, m.middle2_x + ctx.thickness, m.middle1_y - 1, m.right, m.middle1_y - 1);
			FillRectangle(dc, m.middle1_x + ctx.thickness, m.middle2_y - 1, m.right, m.middle2_y - 1);

			FillRectangle(dc, m.middle1_x - 1, m.top, m.middle1_x - 1, m.middle2_y);
			FillRectangle(dc, m.middle2_x - 1, m.top, m.middle2_x - 1, m.middle1_y);
		}
	}


	static void Draw_255D(wxPaintDC &dc, ICharPaintContext &ctx, unsigned int start_y, unsigned int cx) /* ╝  */ // + thickness
	{
		DoubleLineBoxMetrics m(ctx, start_y, cx);
		FillRectangle(dc, m.left, m.middle1_y, m.middle1_x + ctx.thickness - 1, m.middle1_y + ctx.thickness - 1);
		FillRectangle(dc, m.left, m.middle2_y, m.middle2_x + ctx.thickness - 1, m.middle2_y + ctx.thickness - 1);

		FillRectangle(dc, m.middle1_x, m.top, m.middle1_x + ctx.thickness - 1, m.middle1_y);
		FillRectangle(dc, m.middle2_x, m.top, m.middle2_x + ctx.thickness - 1, m.middle2_y);

		if (ctx.SetColorFaded()) {
			FillRectangle(dc, m.left, m.middle1_y - 1, m.middle1_x - 1, m.middle1_y - 1);
			FillRectangle(dc, m.left, m.middle2_y - 1, m.middle2_x - 1, m.middle2_y - 1);

			FillRectangle(dc, m.middle1_x - 1, m.top, m.middle1_x - 1, m.middle1_y - 1);
			FillRectangle(dc, m.middle2_x - 1, m.top, m.middle2_x - 1, m.middle2_y - 1);

			if (ctx.SetColorExtraFaded()) {
				FillPixel(dc, m.middle1_x + ctx.thickness - 2, m.middle1_y + ctx.thickness - 2);
				FillPixel(dc, m.middle2_x + ctx.thickness - 2, m.middle2_y + ctx.thickness - 2);
			}
		}
	}


	static void Draw_255F(wxPaintDC &dc, ICharPaintContext &ctx, unsigned int start_y, unsigned int cx) /* ╟  */
	{
		DoubleLineBoxMetrics m(ctx, start_y, cx);
		FillRectangle(dc, m.middle2_x, m.middle_y, m.right, m.middle_y + ctx.thickness - 1);
		FillRectangle(dc, m.middle1_x, m.top, m.middle1_x + ctx.thickness - 1, m.bottom);
		FillRectangle(dc, m.middle2_x, m.top, m.middle2_x + ctx.thickness - 1, m.bottom);
		if (ctx.SetColorFaded()) {
			FillRectangle(dc, m.middle2_x + ctx.thickness, m.middle_y - 1, m.right, m.middle_y - 1);
			FillRectangle(dc, m.middle1_x - 1, m.top, m.middle1_x - 1, m.bottom);
			FillRectangle(dc, m.middle2_x - 1, m.top, m.middle2_x - 1, m.bottom);
		}
	}


	static void Draw_2562(wxPaintDC &dc, ICharPaintContext &ctx, unsigned int start_y, unsigned int cx) /* ╢  */
	{
		DoubleLineBoxMetrics m(ctx, start_y, cx);
		FillRectangle(dc, m.left, m.middle_y, m.middle1_x, m.middle_y + ctx.thickness - 1);
		FillRectangle(dc, m.middle1_x, m.top, m.middle1_x + ctx.thickness - 1, m.bottom);
		FillRectangle(dc, m.middle2_x, m.top, m.middle2_x + ctx.thickness - 1, m.bottom);
		if (ctx.SetColorFaded()) {
			FillRectangle(dc, m.left, m.middle_y - 1, m.middle1_x - 1, m.middle_y - 1);
			FillRectangle(dc, m.middle1_x - 1, m.top, m.middle1_x - 1, m.middle_y - 1);
			FillRectangle(dc, m.middle1_x - 1, m.middle_y + ctx.thickness, m.middle1_x - 1, m.bottom);
			FillRectangle(dc, m.middle2_x - 1, m.top, m.middle2_x - 1, m.bottom);

			if (ctx.SetColorExtraFaded()) {
				FillPixel(dc, m.middle1_x - 1, m.middle_y + ctx.thickness - 2);
			}
		}
	}


	///

	static void Draw_2560(wxPaintDC &dc, ICharPaintContext &ctx, unsigned int start_y, unsigned int cx) /* ╠ */
	{
		DoubleLineBoxMetrics m(ctx, start_y, cx);
		FillRectangle(dc, m.middle2_x, m.middle1_y, m.right, m.middle1_y + ctx.thickness - 1);
		FillRectangle(dc, m.middle2_x, m.middle2_y, m.right, m.middle2_y + ctx.thickness - 1);
		FillRectangle(dc, m.middle1_x, m.top, m.middle1_x + ctx.thickness - 1, m.bottom);
		FillRectangle(dc, m.middle2_x, m.top, m.middle2_x + ctx.thickness - 1, m.middle1_y);
		FillRectangle(dc, m.middle2_x, m.middle2_y, m.middle2_x + ctx.thickness - 1, m.bottom);
		if (ctx.SetColorFaded()) {
			FillRectangle(dc, m.middle2_x + ctx.thickness, m.middle1_y - 1, m.right, m.middle1_y - 1);
			FillRectangle(dc, m.middle2_x, m.middle2_y - 1, m.right, m.middle2_y - 1);
			FillRectangle(dc, m.middle1_x - 1, m.top, m.middle1_x - 1, m.bottom);
			FillRectangle(dc, m.middle2_x - 1, m.top, m.middle2_x - 1, m.middle1_y + ctx.thickness - 1);
			FillRectangle(dc, m.middle2_x - 1, m.middle2_y, m.middle2_x - 1, m.bottom);
		}
        }


	static void Draw_2563(wxPaintDC &dc, ICharPaintContext &ctx, unsigned int start_y, unsigned int cx) /* ╣ */ // + thickness
	{
		DoubleLineBoxMetrics m(ctx, start_y, cx);
		FillRectangle(dc, m.left, m.middle1_y, m.middle1_x + ctx.thickness - 1, m.middle1_y + ctx.thickness - 1);
		FillRectangle(dc, m.left, m.middle2_y, m.middle1_x, m.middle2_y + ctx.thickness - 1);
		FillRectangle(dc, m.middle1_x, m.top, m.middle1_x + ctx.thickness - 1, m.middle1_y);
		FillRectangle(dc, m.middle1_x, m.middle2_y, m.middle1_x + ctx.thickness - 1, m.bottom);
		FillRectangle(dc, m.middle2_x, m.top, m.middle2_x + ctx.thickness - 1, m.bottom);
		if (ctx.SetColorFaded()) {
			FillRectangle(dc, m.left, m.middle1_y - 1, m.middle1_x - 1, m.middle1_y - 1);
			FillRectangle(dc, m.left, m.middle2_y - 1, m.middle1_x + ctx.thickness - 1, m.middle2_y - 1);
			FillRectangle(dc, m.middle1_x - 1, m.top, m.middle1_x - 1, m.middle1_y - 1);
			FillRectangle(dc, m.middle1_x - 1, m.middle2_y + ctx.thickness, m.middle1_x - 1, m.bottom);
			FillRectangle(dc, m.middle2_x - 1, m.top, m.middle2_x - 1, m.bottom);
		}
	}
	


	static void Draw_2566(wxPaintDC &dc, ICharPaintContext &ctx, unsigned int start_y, unsigned int cx) /* ╦ */
	{
		DoubleLineBoxMetrics m(ctx, start_y, cx);
		FillRectangle(dc, m.left, m.middle1_y, m.right, m.middle1_y + ctx.thickness - 1);
		FillRectangle(dc, m.left, m.middle2_y, m.middle1_x, m.middle2_y + ctx.thickness - 1);
		FillRectangle(dc, m.middle2_x, m.middle2_y, m.right, m.middle2_y + ctx.thickness - 1);
		FillRectangle(dc, m.middle1_x, m.middle2_y, m.middle1_x + ctx.thickness - 1, m.bottom);
		FillRectangle(dc, m.middle2_x, m.middle2_y, m.middle2_x + ctx.thickness - 1, m.bottom);
		if (ctx.SetColorFaded()) {
			FillRectangle(dc, m.left, m.middle1_y - 1, m.right, m.middle1_y - 1);
			FillRectangle(dc, m.left, m.middle2_y - 1, m.middle1_x + ctx.thickness - 1, m.middle2_y - 1);
			FillRectangle(dc, m.middle2_x, m.middle2_y - 1, m.right, m.middle2_y - 1);
			FillRectangle(dc, m.middle1_x - 1, m.middle2_y + ctx.thickness, m.middle1_x - 1, m.bottom);
			FillRectangle(dc, m.middle2_x - 1, m.middle2_y, m.middle2_x - 1, m.bottom);
		}
	}


	static void Draw_2569(wxPaintDC &dc, ICharPaintContext &ctx, unsigned int start_y, unsigned int cx) /* ╩  */ // + thickness
	{
		DoubleLineBoxMetrics m(ctx, start_y, cx);
		FillRectangle(dc, m.left, m.middle1_y, m.middle1_x + ctx.thickness - 1, m.middle1_y + ctx.thickness - 1);
		FillRectangle(dc, m.middle2_x, m.middle1_y, m.right, m.middle1_y + ctx.thickness - 1);
		FillRectangle(dc, m.left, m.middle2_y, m.right, m.middle2_y + ctx.thickness - 1);
		FillRectangle(dc, m.middle1_x, m.top, m.middle1_x + ctx.thickness - 1, m.middle1_y);
		FillRectangle(dc, m.middle2_x, m.top, m.middle2_x + ctx.thickness - 1, m.middle1_y);
		if (ctx.SetColorFaded()) {
			FillRectangle(dc, m.left, m.middle1_y - 1, m.middle1_x - 1, m.middle1_y - 1);
			FillRectangle(dc, m.middle2_x + ctx.thickness, m.middle1_y - 1, m.right, m.middle1_y - 1);
			FillRectangle(dc, m.left, m.middle2_y - 1, m.right, m.middle2_y - 1);
			FillRectangle(dc, m.middle1_x - 1, m.top, m.middle1_x - 1, m.middle1_y - 1);
			FillRectangle(dc, m.middle2_x - 1, m.top, m.middle2_x - 1, m.middle1_y + ctx.thickness - 1);
		}
	}


	static void Draw_256C(wxPaintDC &dc, ICharPaintContext &ctx, unsigned int start_y, unsigned int cx) /* ╬ */ // + thickness
	{
		DoubleLineBoxMetrics m(ctx, start_y, cx);
		FillRectangle(dc, m.left, m.middle1_y, m.middle1_x + ctx.thickness - 1, m.middle1_y + ctx.thickness - 1);
		FillRectangle(dc, m.middle2_x, m.middle1_y, m.right, m.middle1_y + ctx.thickness - 1);
		FillRectangle(dc, m.left, m.middle2_y, m.middle1_x, m.middle2_y + ctx.thickness - 1);
		FillRectangle(dc, m.middle2_x, m.middle2_y, m.right, m.middle2_y + ctx.thickness - 1);
		FillRectangle(dc, m.middle1_x, m.top, m.middle1_x + ctx.thickness - 1, m.middle1_y);
		FillRectangle(dc, m.middle1_x, m.middle2_y, m.middle1_x + ctx.thickness - 1, m.bottom);
		FillRectangle(dc, m.middle2_x, m.top, m.middle2_x + ctx.thickness - 1, m.middle1_y);
		FillRectangle(dc, m.middle2_x, m.middle2_y, m.middle2_x + ctx.thickness - 1, m.bottom);

		FillPixel(dc, m.middle1_x + ctx.thickness - 2, m.middle1_y + ctx.thickness - 2);

		if (ctx.SetColorFaded()) {
			FillRectangle(dc, m.left, m.middle1_y - 1, m.middle1_x - 1, m.middle1_y - 1);
			FillRectangle(dc, m.middle2_x + ctx.thickness, m.middle1_y - 1, m.right, m.middle1_y - 1);
			FillRectangle(dc, m.left, m.middle2_y - 1, m.middle1_x + ctx.thickness - 1, m.middle2_y - 1);
			FillRectangle(dc, m.middle2_x, m.middle2_y - 1, m.right, m.middle2_y - 1);

			FillRectangle(dc, m.middle1_x - 1, m.top, m.middle1_x - 1, m.middle1_y - 1);
			FillRectangle(dc, m.middle1_x - 1, m.middle2_y + ctx.thickness, m.middle1_x - 1, m.bottom);
			FillRectangle(dc, m.middle2_x - 1, m.top, m.middle2_x - 1, m.middle1_y + ctx.thickness - 1);
			FillRectangle(dc, m.middle2_x - 1, m.middle2_y, m.middle2_x - 1, m.bottom);


			if (ctx.SetColorExtraFaded()) {
				FillPixel(dc, m.middle2_x - 1, m.middle2_y - 1);
			}
		}
	}


	static void Draw_2564(wxPaintDC &dc, ICharPaintContext &ctx, unsigned int start_y, unsigned int cx) /* ╤ */
	{
		DoubleLineBoxMetrics m(ctx, start_y, cx);
		FillRectangle(dc, m.left, m.middle1_y, m.right, m.middle1_y + ctx.thickness - 1);
		FillRectangle(dc, m.left, m.middle2_y, m.right, m.middle2_y + ctx.thickness - 1);
		FillRectangle(dc, m.middle_x, m.middle2_y, m.middle_x + ctx.thickness - 1, m.bottom);
		if (ctx.SetColorFaded()) {
			FillRectangle(dc, m.left, m.middle1_y - 1, m.right, m.middle1_y - 1);
			FillRectangle(dc, m.left, m.middle2_y - 1, m.right, m.middle2_y - 1);
			FillRectangle(dc, m.middle_x - 1, m.middle2_y + ctx.thickness, m.middle_x - 1, m.bottom);
		}
	}


	static void Draw_2567(wxPaintDC &dc, ICharPaintContext &ctx, unsigned int start_y, unsigned int cx) /* ╧ */
	{
		DoubleLineBoxMetrics m(ctx, start_y, cx);
		FillRectangle(dc, m.left, m.middle1_y, m.right, m.middle1_y + ctx.thickness - 1);
		FillRectangle(dc, m.left, m.middle2_y, m.right, m.middle2_y + ctx.thickness - 1);
		FillRectangle(dc, m.middle_x, m.top, m.middle_x + ctx.thickness - 1, m.middle1_y);
		if (ctx.SetColorFaded()) {
			FillRectangle(dc, m.left, m.middle1_y - 1, m.middle_x - 1, m.middle1_y - 1);
			FillRectangle(dc, m.middle_x + ctx.thickness, m.middle1_y - 1, m.right, m.middle1_y - 1);
			FillRectangle(dc, m.left, m.middle2_y - 1, m.right, m.middle2_y - 1);
			FillRectangle(dc, m.middle_x - 1, m.top, m.middle_x - 1, m.middle1_y - 1);
		}
	}

	static void Draw_2565(wxPaintDC &dc, ICharPaintContext &ctx, unsigned int start_y, unsigned int cx) /* ╥ */
	{
		DoubleLineBoxMetrics m(ctx, start_y, cx);
		FillRectangle(dc, m.left, m.middle_y, m.right, m.middle_y + ctx.thickness - 1);
		FillRectangle(dc, m.middle1_x, m.middle_y, m.middle1_x + ctx.thickness - 1, m.bottom);
		FillRectangle(dc, m.middle2_x, m.middle_y, m.middle2_x + ctx.thickness - 1, m.bottom);
		if (ctx.SetColorFaded()) {
			FillRectangle(dc, m.left, m.middle_y - 1, m.right, m.middle_y - 1);
			FillRectangle(dc, m.middle1_x - 1, m.middle_y + ctx.thickness, m.middle1_x - 1, m.bottom);
			FillRectangle(dc, m.middle2_x - 1, m.middle_y + ctx.thickness, m.middle2_x - 1, m.bottom);
		}
	}


	static void Draw_256A(wxPaintDC &dc, ICharPaintContext &ctx, unsigned int start_y, unsigned int cx) /* ╪ */
	{
		DoubleLineBoxMetrics m(ctx, start_y, cx);
		FillRectangle(dc, m.left, m.middle1_y, m.right, m.middle1_y + ctx.thickness - 1);
		FillRectangle(dc, m.left, m.middle2_y, m.right, m.middle2_y + ctx.thickness - 1);
		FillRectangle(dc, m.middle_x, m.top, m.middle_x + ctx.thickness - 1, m.bottom);
		if (ctx.SetColorFaded()) {
			FillRectangle(dc, m.left, m.middle1_y - 1, m.middle_x - 1, m.middle1_y - 1);
			FillRectangle(dc, m.middle_x + ctx.thickness, m.middle1_y - 1, m.right, m.middle1_y - 1);
			FillRectangle(dc, m.left, m.middle2_y - 1, m.middle_x - 1, m.middle2_y - 1);
			FillRectangle(dc, m.middle_x + ctx.thickness, m.middle2_y - 1, m.right, m.middle2_y - 1);
			FillRectangle(dc, m.middle_x - 1, m.top, m.middle_x - 1, m.middle1_y - 1);
			FillRectangle(dc, m.middle_x - 1, m.middle1_y + ctx.thickness, m.middle_x - 1, m.middle2_y - 1);
			FillRectangle(dc, m.middle_x - 1, m.middle2_y + ctx.thickness, m.middle_x - 1, m.bottom);
		}
	}


	static void Draw_256B(wxPaintDC &dc, ICharPaintContext &ctx, unsigned int start_y, unsigned int cx) /* ╫ */
	{
		DoubleLineBoxMetrics m(ctx, start_y, cx);
		FillRectangle(dc, m.left, m.middle_y, m.right, m.middle_y + ctx.thickness - 1);
		FillRectangle(dc, m.middle1_x, m.top, m.middle1_x + ctx.thickness - 1, m.bottom);
		FillRectangle(dc, m.middle2_x, m.top, m.middle2_x + ctx.thickness - 1, m.bottom);
		if (ctx.SetColorFaded()) {
			FillRectangle(dc, m.left, m.middle_y - 1, m.middle1_x - 1, m.middle_y - 1);
			FillRectangle(dc, m.middle1_x + ctx.thickness, m.middle_y - 1, m.middle2_x - 1, m.middle_y - 1);
			FillRectangle(dc, m.middle2_x + ctx.thickness, m.middle_y - 1, m.right, m.middle_y - 1);

			FillRectangle(dc, m.middle1_x - 1, m.top, m.middle1_x - 1, m.middle_y - 1);
			FillRectangle(dc, m.middle1_x - 1, m.middle_y + ctx.thickness, m.middle1_x - 1, m.bottom);

			FillRectangle(dc, m.middle2_x - 1, m.top, m.middle2_x - 1, m.middle_y - 1);
			FillRectangle(dc, m.middle2_x - 1, m.middle_y + ctx.thickness, m.middle2_x - 1, m.bottom);
		}
	}


	static void Draw_2561(wxPaintDC &dc, ICharPaintContext &ctx, unsigned int start_y, unsigned int cx) /* ╡ */ // + ctx.thickness - 1
	{
		DoubleLineBoxMetrics m(ctx, start_y, cx);
		FillRectangle(dc, m.left, m.middle1_y, m.middle_x + ctx.thickness - 1, m.middle1_y + ctx.thickness - 1);
		FillRectangle(dc, m.left, m.middle2_y, m.middle_x, m.middle2_y + ctx.thickness - 1);
		FillRectangle(dc, m.middle_x, m.top, m.middle_x + ctx.thickness - 1, m.middle1_y);
		FillRectangle(dc, m.middle_x, m.middle2_y, m.middle_x + ctx.thickness - 1, m.bottom);
		if (ctx.SetColorFaded()) {
			FillRectangle(dc, m.left, m.middle1_y - 1, m.middle_x - 1, m.middle1_y - 1);
			FillRectangle(dc, m.left, m.middle2_y - 1, m.middle_x + ctx.thickness - 1, m.middle2_y - 1);
			FillRectangle(dc, m.middle_x - 1, m.top, m.middle_x - 1, m.middle1_y - 1);
			FillRectangle(dc, m.middle_x - 1, m.middle2_y + ctx.thickness, m.middle_x - 1, m.bottom);
		}
	}


	static void Draw_255E(wxPaintDC &dc, ICharPaintContext &ctx, unsigned int start_y, unsigned int cx) /* ╞ */
	{
		DoubleLineBoxMetrics m(ctx, start_y, cx);
		FillRectangle(dc, m.middle_x, m.middle1_y, m.right, m.middle1_y + ctx.thickness - 1);
		FillRectangle(dc, m.middle_x, m.middle2_y, m.right, m.middle2_y + ctx.thickness - 1);
		FillRectangle(dc, m.middle_x, m.top, m.middle_x + ctx.thickness - 1, m.middle1_y);
		FillRectangle(dc, m.middle_x, m.middle2_y, m.middle_x + ctx.thickness - 1, m.bottom);
		if (ctx.SetColorFaded()) {
			FillRectangle(dc, m.middle_x + ctx.thickness, m.middle1_y - 1, m.right, m.middle1_y - 1);
			FillRectangle(dc, m.middle_x, m.middle2_y - 1, m.right, m.middle2_y - 1);
			FillRectangle(dc, m.middle_x - 1, m.top, m.middle_x - 1, m.middle1_y + ctx.thickness - 1);
			FillRectangle(dc, m.middle_x - 1, m.middle2_y, m.middle_x - 1, m.bottom);
		}
	}


	static void Draw_2556(wxPaintDC &dc, ICharPaintContext &ctx, unsigned int start_y, unsigned int cx) /* ╖ */
	{
		DoubleLineBoxMetrics m(ctx, start_y, cx);
		FillRectangle(dc, m.left, m.middle_y, m.middle2_x, m.middle_y + ctx.thickness - 1);
		FillRectangle(dc, m.middle1_x, m.middle_y, m.middle1_x + ctx.thickness - 1, m.bottom);
		FillRectangle(dc, m.middle2_x, m.middle_y, m.middle2_x + ctx.thickness - 1, m.bottom);
		if (ctx.SetColorFaded()) {
			FillRectangle(dc, m.left, m.middle_y - 1, m.middle2_x + ctx.thickness, m.middle_y - 1);
			FillRectangle(dc, m.middle1_x - 1, m.middle_y + ctx.thickness, m.middle1_x - 1, m.bottom);
			FillRectangle(dc, m.middle2_x - 1, m.middle_y + ctx.thickness, m.middle2_x - 1, m.bottom);
		}
	}


	static void Draw_2555(wxPaintDC &dc, ICharPaintContext &ctx, unsigned int start_y, unsigned int cx) /* ╕ */
	{
		DoubleLineBoxMetrics m(ctx, start_y, cx);
		FillRectangle(dc, m.left, m.middle1_y, m.middle_x + ctx.thickness - 1, m.middle1_y + ctx.thickness - 1);
		FillRectangle(dc, m.left, m.middle2_y, m.middle_x, m.middle2_y + ctx.thickness - 1);
		FillRectangle(dc, m.middle_x, m.middle1_y, m.middle_x + ctx.thickness - 1, m.bottom);
		if (ctx.SetColorFaded()) {
			FillRectangle(dc, m.left, m.middle1_y - 1, m.middle_x + ctx.thickness - 1, m.middle1_y - 1);
			FillRectangle(dc, m.left, m.middle2_y - 1, m.middle_x - 1, m.middle2_y - 1);
			FillRectangle(dc, m.middle_x - 1, m.middle1_y + ctx.thickness, m.middle_x - 1, m.middle2_y - 1);
			FillRectangle(dc, m.middle_x - 1, m.middle2_y + ctx.thickness, m.middle_x - 1, m.bottom);
		}
	}


	static void Draw_2568(wxPaintDC &dc, ICharPaintContext &ctx, unsigned int start_y, unsigned int cx) /* ╨ */
	{
		DoubleLineBoxMetrics m(ctx, start_y, cx);
		FillRectangle(dc, m.left, m.middle_y, m.right, m.middle_y + ctx.thickness - 1);
		FillRectangle(dc, m.middle1_x, m.top, m.middle1_x + ctx.thickness - 1, m.middle_y);
		FillRectangle(dc, m.middle2_x, m.top, m.middle2_x + ctx.thickness - 1, m.middle_y);
		if (ctx.SetColorFaded()) {
			FillRectangle(dc, m.left, m.middle_y - 1, m.middle1_x - 1, m.middle_y - 1);
			FillRectangle(dc, m.middle1_x + ctx.thickness, m.middle_y - 1, m.middle2_x - 1, m.middle_y - 1);
			FillRectangle(dc, m.middle2_x + ctx.thickness, m.middle_y - 1, m.right, m.middle_y - 1);
			FillRectangle(dc, m.middle1_x - 1, m.top, m.middle1_x - 1, m.middle_y - 1);
			FillRectangle(dc, m.middle2_x - 1, m.top, m.middle2_x - 1, m.middle_y - 1);
		}
	}


	static void Draw_255C(wxPaintDC &dc, ICharPaintContext &ctx, unsigned int start_y, unsigned int cx) /* ╜ */ // + thickness
	{
		DoubleLineBoxMetrics m(ctx, start_y, cx);
		FillRectangle(dc, m.left, m.middle_y, m.middle2_x + ctx.thickness - 1, m.middle_y + ctx.thickness - 1);
		FillRectangle(dc, m.middle1_x, m.top, m.middle1_x + ctx.thickness - 1, m.middle_y);
		FillRectangle(dc, m.middle2_x, m.top, m.middle2_x + ctx.thickness - 1, m.middle_y);
		if (ctx.SetColorFaded()) {
			FillRectangle(dc, m.left, m.middle_y - 1, m.middle1_x - 1, m.middle_y - 1);
			FillRectangle(dc, m.middle1_x + ctx.thickness, m.middle_y - 1, m.middle2_x - 1, m.middle_y - 1);
			FillRectangle(dc, m.middle1_x - 1, m.top, m.middle1_x - 1, m.middle_y - 1);
			FillRectangle(dc, m.middle2_x - 1, m.top, m.middle2_x - 1, m.middle_y - 1);
		}
	}


	static void Draw_2559(wxPaintDC &dc, ICharPaintContext &ctx, unsigned int start_y, unsigned int cx) /* ╙ */
	{
		DoubleLineBoxMetrics m(ctx, start_y, cx);
		FillRectangle(dc, m.middle1_x, m.middle_y, m.right, m.middle_y + ctx.thickness - 1);
		FillRectangle(dc, m.middle1_x, m.top, m.middle1_x + ctx.thickness - 1, m.middle_y);
		FillRectangle(dc, m.middle2_x, m.top, m.middle2_x + ctx.thickness - 1, m.middle_y);
		if (ctx.SetColorFaded()) {
			FillRectangle(dc, m.middle1_x + ctx.thickness, m.middle_y - 1, m.middle2_x - 1, m.middle_y - 1);
			FillRectangle(dc, m.middle1_x - 1, m.top, m.middle1_x - 1, m.middle_y - 1);
			FillRectangle(dc, m.middle2_x - 1, m.top, m.middle2_x - 1, m.middle_y - 1);
			FillRectangle(dc, m.middle2_x + ctx.thickness, m.middle_y - 1, m.right, m.middle_y - 1);
		}
	}


	static void Draw_2558(wxPaintDC &dc, ICharPaintContext &ctx, unsigned int start_y, unsigned int cx) /* ╘ */
	{
		DoubleLineBoxMetrics m(ctx, start_y, cx);
		FillRectangle(dc, m.middle_x, m.middle1_y, m.right, m.middle1_y + ctx.thickness - 1);
		FillRectangle(dc, m.middle_x, m.middle2_y, m.right, m.middle2_y + ctx.thickness - 1);
		FillRectangle(dc, m.middle_x, m.top, m.middle_x + ctx.thickness - 1, m.middle2_y);
		if (ctx.SetColorFaded()) {
			FillRectangle(dc, m.middle_x + ctx.thickness, m.middle1_y - 1, m.right, m.middle1_y - 1);
			FillRectangle(dc, m.middle_x + ctx.thickness, m.middle2_y - 1, m.right, m.middle2_y - 1);
			FillRectangle(dc, m.middle_x - 1, m.top, m.middle_x - 1, m.middle2_y + ctx.thickness - 1);
		}
	}


	static void Draw_2552(wxPaintDC &dc, ICharPaintContext &ctx, unsigned int start_y, unsigned int cx) /* ╒ */
	{
		DoubleLineBoxMetrics m(ctx, start_y, cx);
		FillRectangle(dc, m.middle_x, m.middle1_y, m.right, m.middle1_y + ctx.thickness - 1);
		FillRectangle(dc, m.middle_x, m.middle2_y, m.right, m.middle2_y + ctx.thickness - 1);
		FillRectangle(dc, m.middle_x, m.middle1_y, m.middle_x + ctx.thickness - 1, m.bottom);
		if (ctx.SetColorFaded()) {
			FillRectangle(dc, m.middle_x, m.middle1_y - 1, m.right, m.middle1_y - 1);
			FillRectangle(dc, m.middle_x + ctx.thickness, m.middle2_y - 1, m.right, m.middle2_y - 1);
			FillRectangle(dc, m.middle_x - 1, m.middle1_y, m.middle_x - 1, m.bottom);
		}
	}


	static void Draw_2553(wxPaintDC &dc, ICharPaintContext &ctx, unsigned int start_y, unsigned int cx) /* ╓ */
	{
		DoubleLineBoxMetrics m(ctx, start_y, cx);
		FillRectangle(dc, m.middle1_x, m.middle_y, m.right, m.middle_y + ctx.thickness - 1);
		FillRectangle(dc, m.middle1_x, m.middle_y, m.middle1_x + ctx.thickness - 1, m.bottom);
		FillRectangle(dc, m.middle2_x, m.middle_y, m.middle2_x + ctx.thickness - 1, m.bottom);
		if (ctx.SetColorFaded()) {
			FillRectangle(dc, m.middle1_x, m.middle_y - 1, m.right, m.middle_y - 1);
			FillRectangle(dc, m.middle1_x - 1, m.middle_y, m.middle1_x - 1, m.bottom);
			FillRectangle(dc, m.middle2_x - 1, m.middle_y + ctx.thickness, m.middle2_x - 1, m.bottom);
		}
	}

	static void Draw_255B(wxPaintDC &dc, ICharPaintContext &ctx, unsigned int start_y, unsigned int cx) /* ╛*/ // + thickness
	{
		DoubleLineBoxMetrics m(ctx, start_y, cx);
		FillRectangle(dc, m.left, m.middle1_y, m.middle_x, m.middle1_y + ctx.thickness - 1);
		FillRectangle(dc, m.left, m.middle2_y, m.middle_x, m.middle2_y + ctx.thickness - 1);
		FillRectangle(dc, m.middle_x, m.top, m.middle_x + ctx.thickness - 1, m.middle2_y + ctx.thickness - 1);
		if (ctx.SetColorFaded()) {
			FillRectangle(dc, m.left, m.middle1_y - 1, m.middle_x - 1, m.middle1_y - 1);
			FillRectangle(dc, m.left, m.middle2_y - 1, m.middle_x - 1, m.middle2_y - 1);
			FillRectangle(dc, m.middle_x - 1, m.top, m.middle_x - 1, m.middle1_y - 1);
			FillRectangle(dc, m.middle_x - 1, m.middle1_y  + ctx.thickness, m.middle_x - 1, m.middle2_y - 1);
		}
	}

	static void Draw_2580(wxPaintDC &dc, ICharPaintContext &ctx, unsigned int start_y, unsigned int cx) /* ▀ */
	{
		CharMetrics m(ctx, start_y, cx);
		FillRectangle(dc, m.left, m.top, m.right, m.top + (ctx.fh / 2) - 1);
	}

	static void Draw_2581(wxPaintDC &dc, ICharPaintContext &ctx, unsigned int start_y, unsigned int cx) /* ▁ */
	{
		CharMetrics m(ctx, start_y, cx);
		FillRectangle(dc, m.left, m.top + (7 * ctx.fh / 8), m.right, m.bottom);
	}

	static void Draw_2582(wxPaintDC &dc, ICharPaintContext &ctx, unsigned int start_y, unsigned int cx) /* ▂ */
	{
		CharMetrics m(ctx, start_y, cx);
		FillRectangle(dc, m.left, m.top + (6 * ctx.fh / 8), m.right, m.bottom);
	}

	static void Draw_2583(wxPaintDC &dc, ICharPaintContext &ctx, unsigned int start_y, unsigned int cx) /* ▂ */
	{
		CharMetrics m(ctx, start_y, cx);
		FillRectangle(dc, m.left, m.top + (5 * ctx.fh / 8), m.right, m.bottom);
	}

	static void Draw_2584(wxPaintDC &dc, ICharPaintContext &ctx, unsigned int start_y, unsigned int cx) /* ▄ */
	{
		CharMetrics m(ctx, start_y, cx);
		FillRectangle(dc, m.left, m.top + (4 * ctx.fh / 8), m.right, m.bottom);
	}

	static void Draw_2585(wxPaintDC &dc, ICharPaintContext &ctx, unsigned int start_y, unsigned int cx) /* ▅ */
	{
		CharMetrics m(ctx, start_y, cx);
		FillRectangle(dc, m.left, m.top + (3 * ctx.fh / 8), m.right, m.bottom);
	}

	static void Draw_2586(wxPaintDC &dc, ICharPaintContext &ctx, unsigned int start_y, unsigned int cx) /* ▆ */
	{
		CharMetrics m(ctx, start_y, cx);
		FillRectangle(dc, m.left, m.top + (2 * ctx.fh / 8), m.right, m.bottom);
	}

	static void Draw_2587(wxPaintDC &dc, ICharPaintContext &ctx, unsigned int start_y, unsigned int cx) /* ▇ */
	{
		CharMetrics m(ctx, start_y, cx);
		FillRectangle(dc, m.left, m.top + (1 * ctx.fh / 8), m.right, m.bottom);
	}

	static void Draw_2588(wxPaintDC &dc, ICharPaintContext &ctx, unsigned int start_y, unsigned int cx) /* █ */
	{
		CharMetrics m(ctx, start_y, cx);
		FillRectangle(dc, m.left, m.top, m.right, m.bottom);
	}


	static void Draw_2589(wxPaintDC &dc, ICharPaintContext &ctx, unsigned int start_y, unsigned int cx) /* ▉ */
	{
		CharMetrics m(ctx, start_y, cx);
		FillRectangle(dc, m.left, m.top, m.left + (7 * ctx.fw / 8) - 1, m.bottom);
	}

	static void Draw_258a(wxPaintDC &dc, ICharPaintContext &ctx, unsigned int start_y, unsigned int cx) /* ▊ */
	{
		CharMetrics m(ctx, start_y, cx);
		FillRectangle(dc, m.left, m.top, m.left + (6 * ctx.fw / 8) - 1, m.bottom);
	}

	static void Draw_258b(wxPaintDC &dc, ICharPaintContext &ctx, unsigned int start_y, unsigned int cx) /* ▋ */
	{
		CharMetrics m(ctx, start_y, cx);
		FillRectangle(dc, m.left, m.top, m.left + (5 * ctx.fw / 8) - 1, m.bottom);
	}

	static void Draw_258c(wxPaintDC &dc, ICharPaintContext &ctx, unsigned int start_y, unsigned int cx) /* ▌ */
	{
		CharMetrics m(ctx, start_y, cx);
		FillRectangle(dc, m.left, m.top, m.left + (4 * ctx.fw / 8) - 1, m.bottom);
	}

	static void Draw_258d(wxPaintDC &dc, ICharPaintContext &ctx, unsigned int start_y, unsigned int cx) /* ▍ */
	{
		CharMetrics m(ctx, start_y, cx);
		FillRectangle(dc, m.left, m.top, m.left + (3 * ctx.fw / 8) - 1, m.bottom);
	}

	static void Draw_258e(wxPaintDC &dc, ICharPaintContext &ctx, unsigned int start_y, unsigned int cx) /* ▎ */
	{
		CharMetrics m(ctx, start_y, cx);
		FillRectangle(dc, m.left, m.top, m.left + (2 * ctx.fw / 8) - 1, m.bottom);
	}

	static void Draw_258f(wxPaintDC &dc, ICharPaintContext &ctx, unsigned int start_y, unsigned int cx) /* ▏ */
	{
		CharMetrics m(ctx, start_y, cx);
		FillRectangle(dc, m.left, m.top, m.left + (1 * ctx.fw / 8) - 1, m.bottom);
	}

	static void Draw_2590(wxPaintDC &dc, ICharPaintContext &ctx, unsigned int start_y, unsigned int cx) /* ▐ */
	{
		CharMetrics m(ctx, start_y, cx);
		FillRectangle(dc, m.left + (ctx.fw / 2), m.top, m.right, m.bottom);
	}

	static void Draw_2594(wxPaintDC &dc, ICharPaintContext &ctx, unsigned int start_y, unsigned int cx) /* ▔ */
	{
		CharMetrics m(ctx, start_y, cx);
		FillRectangle(dc, m.left, m.top, m.right, m.top + (ctx.fh / 8));
	}

	static void Draw_2595(wxPaintDC &dc, ICharPaintContext &ctx, unsigned int start_y, unsigned int cx) /* ▕ */
	{
		CharMetrics m(ctx, start_y, cx);
		FillRectangle(dc, m.left + (7 * ctx.fw / 8), m.top, m.right, m.bottom);
	}


	static void Draw_2596(wxPaintDC &dc, ICharPaintContext &ctx, unsigned int start_y, unsigned int cx) /* ▖ */
	{
		CharMetrics m(ctx, start_y, cx);
		FillRectangle(dc, m.left, m.top + (ctx.fh / 2), m.left + (ctx.fw / 2) - 1, m.bottom);
	}

	static void Draw_2597(wxPaintDC &dc, ICharPaintContext &ctx, unsigned int start_y, unsigned int cx) /* ▗ */
	{
		CharMetrics m(ctx, start_y, cx);
		FillRectangle(dc, m.left + (ctx.fw / 2), m.top + (ctx.fh / 2), m.right, m.bottom);
	}

	static void Draw_2598(wxPaintDC &dc, ICharPaintContext &ctx, unsigned int start_y, unsigned int cx) /* ▘ */
	{
		CharMetrics m(ctx, start_y, cx);
		FillRectangle(dc, m.left, m.top, m.left + (ctx.fw / 2) - 1, m.top + (ctx.fh / 2) - 1);
	}

	static void Draw_2599(wxPaintDC &dc, ICharPaintContext &ctx, unsigned int start_y, unsigned int cx) /* ▙ */
	{
		CharMetrics m(ctx, start_y, cx);
		FillRectangle(dc, m.left, m.top, m.left + (ctx.fw / 2) - 1, m.bottom);
		FillRectangle(dc, m.left + (ctx.fw / 2), m.top + (ctx.fh) / 2, m.right, m.bottom);
	}

	static void Draw_259a(wxPaintDC &dc, ICharPaintContext &ctx, unsigned int start_y, unsigned int cx) /* ▚ */
	{
		CharMetrics m(ctx, start_y, cx);
		FillRectangle(dc, m.left, m.top, m.left + (ctx.fw / 2) - 1, m.top + (ctx.fh) / 2 - 1);
		FillRectangle(dc, m.left + (ctx.fw / 2), m.top + (ctx.fh) / 2, m.right, m.bottom);
	}

	static void Draw_259b(wxPaintDC &dc, ICharPaintContext &ctx, unsigned int start_y, unsigned int cx) /* ▛ */
	{
		CharMetrics m(ctx, start_y, cx);
		FillRectangle(dc, m.left, m.top, m.right, m.top + (ctx.fh) / 2 - 1);
		FillRectangle(dc, m.left, m.top, m.left + (ctx.fw / 2) - 1, m.bottom);
	}

	static void Draw_259c(wxPaintDC &dc, ICharPaintContext &ctx, unsigned int start_y, unsigned int cx) /* ▜ */
	{
		CharMetrics m(ctx, start_y, cx);
		FillRectangle(dc, m.left, m.top, m.right, m.top + (ctx.fh / 2) - 1);
		FillRectangle(dc, m.left + (ctx.fw / 2), m.top + (ctx.fh / 2), m.right, m.bottom);
	}

	static void Draw_259d(wxPaintDC &dc, ICharPaintContext &ctx, unsigned int start_y, unsigned int cx) /* ▝ */
	{
		CharMetrics m(ctx, start_y, cx);
		FillRectangle(dc, m.left + (ctx.fw / 2), m.top, m.right, m.top + (ctx.fh / 2) - 1);
	}

	static void Draw_259e(wxPaintDC &dc, ICharPaintContext &ctx, unsigned int start_y, unsigned int cx) /* ▞ */
	{
		CharMetrics m(ctx, start_y, cx);
		FillRectangle(dc, m.left + (ctx.fw / 2), m.top, m.right, m.top + (ctx.fh / 2) - 1);
		FillRectangle(dc, m.left, m.top + (ctx.fh / 2), m.left + (ctx.fw / 2) - 1, m.bottom);
	}

	static void Draw_259f(wxPaintDC &dc, ICharPaintContext &ctx, unsigned int start_y, unsigned int cx) /* ▟ */
	{
		CharMetrics m(ctx, start_y, cx);
		FillRectangle(dc, m.left + (ctx.fw / 2), m.top, m.right, m.top + (ctx.fh / 2) - 1);
		FillRectangle(dc, m.left, m.top + (ctx.fh / 2), m.right, m.bottom);
	}

	////////////////////////////////////////////////////////////////

	DrawT Get(const wchar_t c)
	{
		switch (c) {
			case 0x2500: return Draw_2500; 			/* ─ */
			case 0x2501: return Draw_Thicker<Draw_2500>;	/* ━ */
			case 0x2502: return Draw_2502; 			/* │ */
			case 0x2503: return Draw_Thicker<Draw_2502>;	/* ┃ */
			case 0x250c: return Draw_250C;			/* ┌ */
			case 0x250f: return Draw_Thicker<Draw_250C>;	/* ┏ */
			case 0x2510: return Draw_2510;			/* ┐ */
			case 0x2513: return Draw_Thicker<Draw_2510>;	/* ┓ */
			case 0x2514: return Draw_2514;			/* └ */
			case 0x2517: return Draw_Thicker<Draw_2514>;	/* ┗ */
			case 0x2518: return Draw_2518;			/* ┘ */
			case 0x251b: return Draw_Thicker<Draw_2518>;	/* ┛ */
			case 0x251c: return Draw_251C;			/* ├ */
			case 0x2523: return Draw_Thicker<Draw_252C>;	/* ┣ */
			case 0x2524: return Draw_2524;			/* ┤ */
			case 0x252b: return Draw_Thicker<Draw_2524>;	/* ┫ */
			case 0x252c: return Draw_252C;			/* ┬ */
			case 0x2533: return Draw_Thicker<Draw_252C>;	/* ┳ */
			case 0x2534: return Draw_2534;			/* ┴ */
			case 0x253b: return Draw_Thicker<Draw_2534>;	/* ┻ */
			case 0x253c: return Draw_253C;			/* ┼  */
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
			case 0x2561: return Draw_2561; /* ╡ */ // + ctx.thickness - 1
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

//not fadable
			case 0x2580: return Draw_2580; /* ▀ */
			case 0x2581: return Draw_2581; /* ▁ */
			case 0x2582: return Draw_2582; /* ▂ */
			case 0x2583: return Draw_2583; /* ▃ */
			case 0x2584: return Draw_2584; /* ▄ */
			case 0x2585: return Draw_2585; /* ▅ */
			case 0x2586: return Draw_2586; /* ▆ */
			case 0x2587: return Draw_2587; /* ▇ */
			case 0x2588: return Draw_2588; /* █ */
			case 0x2589: return Draw_2589; /* ▉ */
			case 0x258a: return Draw_258a; /* ▊ */
			case 0x258b: return Draw_258b; /* ▋ */
			case 0x258c: return Draw_258c; /* ▌ */
			case 0x258d: return Draw_258d; /* ▍ */
			case 0x258e: return Draw_258e; /* ▎ */
			case 0x258f: return Draw_258f; /* ▏ */
			case 0x2590: return Draw_2590; /* ▐ */
#if 0
			case 0x2591: return Draw_2591; /* ░ */
			case 0x2592: return Draw_2592; /* ▒ */
			case 0x2593: return Draw_2593; /* ▓ */
#endif
			case 0x2594: return Draw_2594; /* ▔ */
			case 0x2595: return Draw_2595; /* ▖ */
			case 0x2596: return Draw_2596; /* ▗ */
			case 0x2597: return Draw_2597; /* ▘ */
			case 0x2598: return Draw_2598; /* ▙ */
			case 0x2599: return Draw_2599; /* ▚ */
			case 0x259a: return Draw_259a; /* ▛ */
			case 0x259b: return Draw_259b; /* ▜ */
			case 0x259c: return Draw_259c; /* ▜ */
			case 0x259d: return Draw_259d; /* ▝ */
			case 0x259e: return Draw_259e; /* ▞ */
			case 0x259f: return Draw_259f; /* ▟ */

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

