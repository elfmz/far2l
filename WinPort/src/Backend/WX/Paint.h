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
	inline unsigned int FontDescent() const { return _font_descent; }

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

	WinPortRGB _clr_for_caret {255,255,255}, _caret_clr {0,0,0};
	WinPortRGB _clr_accent_text {0,0,0}, _clr_accent_back {0,0,0};
	bool _clr_accent_computed { false };

	friend struct WXCustomDrawCharPainter;

	void PrepareBackground(unsigned int cx, const WinPortRGB &clr, unsigned int nx);
	void FlushBackground(unsigned int cx_end);
	void FlushText(unsigned int cx_end);
	void FlushDecorations(unsigned int cx_end);

	void ComputeAccents(bool cache = true);
	void ComputeAccents(const WinPortRGB& clr_text, const WinPortRGB& clr_back, WinPortRGB& clr_a_text, WinPortRGB& clr_a_back);

	WinPortRGB GetEmbossColor(const WinPortRGB& clr);
	WinPortRGB GetCursorColor(const WinPortRGB& clr);
	WinPortRGB GetSoftenColorIf(const WinPortRGB& clr);

    struct CustomCharPos {
    	wchar_t cc;
    	WXCustomDrawChar::DrawT custom_draw;
    	DWORD64 attributes;
    	unsigned cx;
    	unsigned nx;
    	bool prev_space;

    	CustomCharPos(wchar_t _cc, WXCustomDrawChar::DrawT _custom_draw, DWORD64 _attributes, unsigned _cx, unsigned _nx, bool _prev_space) {
        	cc = _cc;
            custom_draw = _custom_draw;
            attributes = _attributes;
            cx = _cx;
            nx = _nx;
            prev_space = _prev_space;
    	}
    };

    struct HintPos {
        struct {
            HintContainerType Container; /* e.g menu, dialog, console, editor, viewer, panels, ... */
            HintObjectType Object; /* e.g push button, text, box, separator, combo box, ...  */
            
            int Focus: 1;
            int Hover: 1;
            int Enabled: 1;
            int Default: 1; 
            int Beveled: 1;
            int Shadow: 1;
        } Hint;

        int tag;

    	int cx;
    	int nx;
        int cy;

        unsigned int cw, ch;

    	DWORD64 attributes;
        std::wstring text;
        SMALL_RECT area;
    };

    std::vector<CustomCharPos> line_custom_chars;
    std::vector<HintPos> line_hints;

public:

#ifdef TAG_DEBUG
    struct HintHatch {
		HintContainerType Container;
		HintObjectType Object;
		int cx;
		int cy;
    };

	void DrawHatch(const std::vector<HintHatch>& hatched);
#endif

	ConsolePainter(ConsolePaintContext *context, wxPaintDC &dc, wxString &_buffer, CursorProps &cursor_props);
	void SetFillColor(const WinPortRGB &clr);

	void NextChar(unsigned int cx, DWORD64 attributes, const wchar_t *wcz, unsigned int nx, bool prev_space);

	inline void LineBegin(unsigned int cy)
	{
		_start_cy = cy;
		_start_y = cy * _context->FontHeight();
		line_custom_chars.clear();
	}

	inline void LineFlush(unsigned int cx_end)
	{
		FlushBackground(cx_end);
		FlushText(cx_end);

		for(size_t x = 0; x < line_custom_chars.size(); ++x) {
			CustomCharPos c = line_custom_chars[x];
			DrawCustomCharImpl(c.cc, c.custom_draw, c.attributes, c.cx, c.nx, c.prev_space);
		}
		line_custom_chars.clear();
		HintFlush();
	}

	inline void HintLineBegin(int cy, int cw, int ch) {}

	inline void HintFlush() {
		for(size_t x = 0; x < line_hints.size(); ++x) {
			HintPos c = line_hints[x];
			DrawHint(c);
		}
		line_hints.clear();
	}

	void ConsumeHintAt(const CHAR_INFO& ci, int cx, int nx, int cy, unsigned int cw, unsigned int ch, const SMALL_RECT& area, const wchar_t* text);
	void DrawHint(const HintPos& x);

	void DrawButtonDecorations(int cx_s, unsigned int cx_e, unsigned int cy, const WinPortRGB& clr_text, const WinPortRGB& clr_back, const HintPos& pos);
	void DrawCheckboxDecorations(int cx_s, unsigned int cx_e, unsigned int cy, const WinPortRGB& clr_text, const WinPortRGB& clr_back, const HintPos& pos);
	void DrawButtonDecorationsAsNew(int cx_s, unsigned int cx_e, unsigned int cy, const WinPortRGB& clr_text, const WinPortRGB& clr_back, const HintPos& pos);

	bool DrawCustomChar(wchar_t cc, WXCustomDrawChar::DrawT custom_draw, DWORD64 attributes, unsigned cx, unsigned nx, bool prev_space) ;
	bool DrawCustomCharImpl(wchar_t cc, WXCustomDrawChar::DrawT custom_draw, DWORD64 attributes, unsigned cx, unsigned nx, bool prev_space) ;

	void DrawGradientLine(wxCoord X1, wxCoord Y1, wxCoord X2, wxCoord Y2, const WinPortRGB& color1, const WinPortRGB& color2, wxCoord thickness = 1);
	void DrawHorizontalGradientLine(wxCoord X1, wxCoord Y1, wxCoord length, const WinPortRGB& color1, const WinPortRGB& color2, wxCoord thickness = 1);
	void DrawVerticalGradientLine(wxCoord X1, wxCoord Y1, wxCoord length, const WinPortRGB& color1, const WinPortRGB& color2, wxCoord thickness = 1);
	void DrawVerticalGradientLine(wxCoord X1, wxCoord Y1, wxCoord length, const WinPortRGB& color1, const WinPortRGB& color2, const WinPortRGB& color3, wxCoord thickness = 1);
	void DrawHorizontalGradientLine(wxCoord X1, wxCoord Y1, wxCoord length, const WinPortRGB& color1, const WinPortRGB& color2, const WinPortRGB& color3, wxCoord thickness = 1);

	void DrawHorizontalDashedGradientLine(wxCoord X1, wxCoord Y1, wxCoord length, const WinPortRGB& color1, const WinPortRGB& color2, int dashLength = 6, int gapLength  = 4, wxCoord thickness = 1);
	void DrawVerticalDashedGradientLine(wxCoord X1, wxCoord Y1, wxCoord length, const WinPortRGB& color1, const WinPortRGB& color2, int dashLength = 6, int gapLength  = 4, wxCoord thickness = 1);
	void DrawLiquidButtonBackground(wxCoord X1, wxCoord Y1, wxCoord w, wxCoord h, const WinPortRGB& colTop);
};
