#pragma once

class ConsolePaintContext
{
	std::vector<wxFont> _fonts;
	wxWindow *_window;
	unsigned int _font_width, _font_height;
	bool _buffered_paint, _cursor_state, _sharp;
	struct {
		std::vector<bool> checked;
		std::vector<uint8_t> result;
	} _char_fit_cache;

	std::vector<CHAR_INFO> _line;
	wxString _buffer;
	
	void SetFont(wxFont font);
public:
	ConsolePaintContext(wxWindow *window);
	void ShowFontDialog();
	
	uint8_t CharFitTest(wxPaintDC &dc, wchar_t c);
	void ApplyFont(wxPaintDC &dc, uint8_t index = 0);
	void OnPaint(SMALL_RECT *qedit = NULL);	
	void RefreshArea( const SMALL_RECT &area );
	void ToggleCursor();
	void SetSharp(bool sharp);
	bool IsSharpSupported();
	
	inline bool IsPaintBuffered() const { return _buffered_paint; }
	inline bool GetCursorState() const { return _cursor_state; }
	inline unsigned int FontWidth() const { return _font_width; }
	inline unsigned int FontHeight() const { return _font_height; }
};

///////////////////////////////

struct CursorProps
{
	CursorProps(bool state);

	unsigned int x, y;
	bool  visible;
	UCHAR height;
};

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
	CursorProps _cursor_props;
	WinPortRGB _clr_text, _clr_back;
	unsigned int _start_cx, _start_cy, _start_back_cx;
	unsigned int _start_y;
	uint8_t _prev_fit_font_index;
	
	void SetBackgroundColor(const WinPortRGB &clr);
	void PrepareBackground(unsigned int cx, const WinPortRGB &clr);
	void FlushBackground(unsigned int cx);
	void FlushText();
		
public:
	ConsolePainter(ConsolePaintContext *context, wxPaintDC &dc, wxString &_buffer);
	

	void NextChar(unsigned int cx, unsigned short attributes, wchar_t c);
	
	inline void LineBegin(unsigned int cy)
	{
		_start_cy = cy;
		_start_y = cy * _context->FontHeight();
	}
	
	inline void LineFlush(unsigned int cx_end)
	{
		FlushBackground(cx_end);
		FlushText();
	}
};

