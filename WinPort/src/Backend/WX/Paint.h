#pragma once
#include <map>
#include <vector>
#include <wx/graphics.h>
#include "WinCompat.h"
#include "wxWinTranslations.h"
#include "CustomDrawChar.h"

struct CursorProps
{
	bool Blink();
	void Update();

	COORD pos{}, prev_pos{};
	SHORT combining_offset = 0;
	UCHAR height = 1;
	bool  visible = false;
	bool  blink_state = true;
};

///

class ConsolePaintContext
{
	std::vector<wxFont> _fonts;
	wxWindow *_window;
	unsigned int _font_width, _font_height, _font_descent, _font_thickness;
	bool _custom_draw_enabled, _buffered_paint, _sharp;
	enum {
		STG_NOT_REFRESHED,
		STG_REFRESHED,
		STG_PAINTED
	} _stage;
	CursorProps _cursor_props;
	struct {
		std::vector<bool> checked;
		std::vector<uint8_t> result;
	} _char_fit_cache;

	wxString _buffer;
	wxString _cft_tmp;

	std::vector<CHAR_INFO> _line;

	std::map<WinPortRGB, wxBrush> _color2brush;
	wxPen _transparent_pen{wxColour(0, 0, 0), 1, wxPENSTYLE_TRANSPARENT};

	void SetFont(wxFont font);
public:
	ConsolePaintContext(wxWindow *window);
	void ShowFontDialog();

	uint8_t CharFitTest(wxPaintDC &dc, wchar_t wcz, unsigned int nx);
	void ApplyFont(wxPaintDC &dc, uint8_t index = 0);
	void OnPaint(wxPaintDC &dc, SMALL_RECT *qedit = NULL);
	void RefreshArea( const SMALL_RECT &area );
	void BlinkCursor();
	void SetSharp(bool sharp);
	bool IsSharpSupported();

	wxBrush &GetBrush(const WinPortRGB &clr);
	inline wxPen &GetTransparentPen() {return _transparent_pen; }

	inline bool IsCustomDrawEnabled() const { return _custom_draw_enabled; }
	inline bool IsSharp() const { return _sharp; }
	inline bool IsPaintBuffered() const { return _buffered_paint; }
	inline unsigned int FontWidth() const { return _font_width; }
	inline unsigned int FontHeight() const { return _font_height; }
	inline unsigned int FontThickness() const { return _font_thickness; }

	inline bool CursorBlinkState() const { return _cursor_props.blink_state; }
};

///////////////////////////////

class ConsolePainter
{
	class RememberedColor
	{
		WinPortRGB _rgb;
		bool _valid;

	public:
		RememberedColor() : _valid(false) {}
		inline bool Change(const WinPortRGB &rgb)
		{
			if (!_valid || _rgb != rgb) {
				_valid = true;
				_rgb = rgb;
				return true;
			}
			return false;
		}

	} _brush_clr;

	ConsolePaintContext *_context;
	wxPaintDC &_dc;
	wxString &_buffer;
	CursorProps &_cursor_props;

	WinPortRGB _clr_text, _clr_back;
	unsigned int _start_cx, _start_cy, _start_back_cx;
	unsigned int _start_y;
	uint8_t _prev_fit_font_index;
	bool	_prev_underlined;
	bool	_prev_strikeout;
	std::map<WinPortRGB, wxPen *> _custom_draw_pens;

	friend struct WXCustomDrawCharPainter;

	void PrepareBackground(unsigned int cx, const WinPortRGB &clr, unsigned int nx);
	void FlushBackground(unsigned int cx_end);
	void FlushText(unsigned int cx_end);
	void FlushDecorations(unsigned int cx_end);

public:
	ConsolePainter(ConsolePaintContext *context, wxPaintDC &dc, wxString &_buffer, CursorProps &cursor_props);
	void SetFillColor(const WinPortRGB &clr);


	void NextChar(unsigned int cx, DWORD64 attributes, const wchar_t *wcz, unsigned int nx);

	inline void LineBegin(unsigned int cy)
	{
		_start_cy = cy;
		_start_y = cy * _context->FontHeight();
	}

	inline void LineFlush(unsigned int cx_end)
	{
		FlushBackground(cx_end);
		FlushText(cx_end);
	}
};

