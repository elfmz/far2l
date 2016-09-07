#pragma once

class ConsolePaintContext
{
	wxFont _font;
	wxWindow *_window;
	unsigned int _font_width, _font_height;
	bool _buffered_paint, _cursor_state;
	struct {
		std::vector<bool> checked;
		std::vector<bool> result;
	} _char_fit_cache;

	std::vector<CHAR_INFO> _line;
	
public:
	ConsolePaintContext(wxWindow *window);
	
	bool CachedCharFitTest(wxPaintDC &dc, wchar_t c);
	void OnPaint(SMALL_RECT *qedit = NULL);	
	void RefreshArea( const SMALL_RECT &area );
	void ToggleCursor();
	
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
		wxColour _clr;
		bool _valid;

	public:
		RememberedColor() : _valid(false) {}
		inline bool Change(wxColour clr)
		{
			if (!_valid || _clr!=clr) {
				_valid = true;
				_clr = clr;
				return true;
			}
			return false;
		}
		
	} _brush;
	
	
	wxString _buffer;
	ConsolePaintContext *_context;
	wxPaintDC &_dc;
	CursorProps _cursor_props;
	unsigned int _start_cx, _start_cy, _start_back_cx;
    unsigned int _start_y;
    wxColour _clr_text, _clr_back;
	
	void SetBackgroundColor(wxColour clr);
	void PrepareBackground(unsigned int cx, wxColour clr);
	void FlushBackground(unsigned int cx);
	void FlushText();
		
public:
	ConsolePainter(ConsolePaintContext *context, wxPaintDC &dc);
	

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

