#include "Backend.h"
#include "wxWinTranslations.h"
#include <wx/fontdlg.h>
#include <wx/fontenum.h>
#include <wx/textfile.h>
#include <wx/graphics.h>
#include "Paint.h"
#include "PathHelpers.h"
#include "WinPort.h"
#include <utils.h>
#include <BackendOptions.h>

#define COLOR_ATTRIBUTES ( FOREGROUND_INTENSITY | BACKGROUND_INTENSITY | \
					FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | \
					BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE )

#define DYNAMIC_FONTS

#ifdef __APPLE__
# define DEFAULT_FONT_SIZE	20
#else
# define DEFAULT_FONT_SIZE	16
#endif

WinPortRGB ComputeEmbossColor_HSL(const WinPortRGB& xbg, const WinPortRGB& xline);
WinPortRGB ComputeEmbossColor_LAB(const WinPortRGB& xbg, const WinPortRGB& xline);

namespace WXCustomDrawChar
{
	extern BackendOptions* options;
};

/////////////////////////////////////////////////////////////////////////////////
static const char *g_known_good_fonts[] = { "Ubuntu", "Terminus", "DejaVu",
											"Liberation", "Droid", "Monospace", "PT Mono", "Menlo",
											nullptr};


class FixedFontLookup : wxFontEnumerator
{
	wxString _any, _known_good;
	virtual bool OnFacename(const wxString &face_name)
	{
		_any = face_name;
		for (const char **p = g_known_good_fonts; *p; ++p) {
			if (face_name.find(*p)!=wxString::npos) {
				_known_good = face_name;
			}
		}

		/* unfortunately following code gives nothing interesting
		wxFont f(wxFontInfo(DEFAULT_FONT_SIZE).Underlined().FaceName(face_name));
		if (f.IsOk()) {
			fprintf(stderr, "FONT family %u encoding %u face_name='%ls' \n",
				(unsigned int)f.GetFamily(), (unsigned int)f.GetEncoding(), static_cast<const wchar_t*>(face_name.wc_str()));
		} else {
			fprintf(stderr, "BAD FONT: face_name='%ls'\n", static_cast<const wchar_t*>(face_name.wc_str()));
		} */
		return true;
	}
public:

	wxString Query()
	{
		_any.Empty();
		_known_good.Empty();
		EnumerateFacenames(wxFONTENCODING_SYSTEM, true);
		fprintf(stderr, "FixedFontLookup: _any='%ls' _known_good='%ls'\n",
			static_cast<const wchar_t*>(_any.wc_str()),
			static_cast<const wchar_t*>(_known_good.wc_str()));
		return _known_good.IsEmpty() ? _any : _known_good;
	}
};

static bool LoadFontFromSettings(wxFont& font)
{
	const std::string &path = InMyConfig("font");
	wxTextFile file(path);
	if (file.Exists() && file.Open()) {
		for (wxString str = file.GetFirstLine(); !file.Eof(); str = file.GetNextLine()) {
			font.SetNativeFontInfo(str);
			if (font.IsOk()) {
				printf("LoadFontFromSettings: used %ls\n",
					static_cast<const wchar_t*>(str.wc_str()));
				return true;
			}
		}
	}

	return false;
}

static bool ChooseFontAndSaveToSettings(wxWindow *parent, wxFont& font)
{
	font = wxGetFontFromUser(parent, font);
	if (font.IsOk()) {
		const std::string &path = InMyConfig("font");
		unlink(path.c_str());
		wxTextFile file;
		file.Create(path);

		file.InsertLine(font.GetNativeFontInfoDesc(), 0);
		file.Write();
		return true;
	}

	return false;
}

static void InitializeFont(wxWindow *parent, wxFont& font)
{
	if (LoadFontFromSettings(font))
		return;


	for (;;) {
		FixedFontLookup ffl;
		wxString fixed_font = ffl.Query();
		if (!fixed_font.empty()) {
			font = wxFont(DEFAULT_FONT_SIZE, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, fixed_font);
		}
		if (fixed_font.empty() || !font.IsOk())
			font = wxFont(wxSystemSettings::GetFont(wxSYS_ANSI_FIXED_FONT));
#if defined(__WXOSX__) && !wxCHECK_VERSION(3, 1, 0)
		return;//older (not sure what exactly version) wxwidgets crashes in wxGetFontFromUser under OSX, this allows at least to start
#else
		if (ChooseFontAndSaveToSettings(parent, font))
			return;
#endif
	}
}

ConsolePaintContext::ConsolePaintContext(wxWindow *window) :
	_window(window), _font_width(12), _font_height(16), _font_descent(0), _font_thickness(2),
	_buffered_paint(false), _sharp(false), _stage(STG_NOT_REFRESHED)
{
	_char_fit_cache.checked.resize(0xffff);
	_char_fit_cache.result.resize(0xffff);
	_fonts.reserve(32);

	_window->SetBackgroundColour(*wxBLACK);
	wxFont font;
	InitializeFont(_window, font);
	SetFont(font);
}



class FontSizeInspector
{
	wxBitmap _bitmap;
	wxMemoryDC _dc;

	int _max_width, _prev_width;
	int _max_height, _prev_height;
	int _max_descent;
	bool _unstable_size, _fractional_size;

	void InspectChar(const wchar_t c)
	{
		wchar_t wz[2] = { c, 0};
		wxCoord width = 0, height = 0, descent = 0;
		_dc.GetTextExtent(wz, &width, &height, &descent);

		if (_max_width < width) _max_width = width;
		if (_max_height < height) _max_height = height;
		if (_max_descent < descent) _max_descent = descent;

		if ( _prev_width != width ) {
			if (_prev_width!=-1)
				_unstable_size = true;
			_prev_width = width;
		}
		if ( _prev_height != height ) {
			if (_prev_height!=-1) _unstable_size = true;
			_prev_height = height;
		}
	}

	void DetectFractionalSize(const wchar_t *chars)
	{
		// If font is non-monospaced there is no sense to detect if widths are fractional
		if (_unstable_size) return;
		_fractional_size = _dc.GetTextExtent(chars).GetWidth() != (int)(_max_width * wcslen(chars));
	}

	public:
	FontSizeInspector(wxFont& font)
		: _bitmap(48, 48, wxBITMAP_SCREEN_DEPTH),
		_max_width(4), _prev_width(-1),
		_max_height(6), _prev_height(-1),
		_max_descent(0),
		_unstable_size(false), _fractional_size(false)
	{
		_dc.SelectObject(_bitmap);
		_dc.SetFont(font);
	}

	void InspectChars(const wchar_t *chars)
	{
		for(const wchar_t *s = chars; *s; ++s)
			InspectChar(*s);
#if defined(__WXOSX__)
		// There are font rendering artifacts on MacOS if buffering is enabled and font size differs from 10, 15, 20;
		// E.g. if font size = 13, one char in a string has width 9px (GetTextExtent returns 9), but total string width
		// is less than N*9px, because internally one char could have fractional width.
		// We need to disable buffering for certain font sizes as done for non-monospaced ("unstable size") fonts.
		DetectFractionalSize(chars);
#endif
	}

	bool IsUnstableSize() const { return _unstable_size; }
	bool IsFractionalSize() const { return _fractional_size; }
	int GetMaxWidth() const { return _max_width; }
	int GetMaxHeight() const { return _max_height; }
	int GetMaxDescent() const { return _max_descent; }
};



void ConsolePaintContext::SetFont(wxFont font)
{
	FontSizeInspector fsi(font);
	fsi.InspectChars(L" 1234567890-=!@#$%^&*()_+qwertyuiop[]asdfghjkl;'zxcvbnm,./QWERTYUIOP{}ASDFGHJKL:\"ZXCVBNM<>?");
	//fsi.InspectChars(L"QWERTYUIOPASDFGHJKL");

	bool is_unstable = fsi.IsUnstableSize();
	bool is_fractional = fsi.IsFractionalSize();
	_font_width = fsi.GetMaxWidth();
	_font_height = fsi.GetMaxHeight();
	_font_descent = fsi.GetMaxDescent();
	//font_height+= _font_height/4;

	_font_thickness = (_font_width > 8) ? _font_width / 8 : 1;
	switch (font.GetWeight()) {
		case wxFONTWEIGHT_LIGHT:
			if (_font_thickness > 1) {
				--_font_thickness;
			}
			break;

		case wxFONTWEIGHT_BOLD:
			++_font_thickness;
			break;

		case wxFONTWEIGHT_NORMAL:
		default:
			;
	}

	fprintf(stderr, "Font %u x %u . %u: '%ls' - %s\n", _font_width, _font_height, _font_thickness,
		static_cast<const wchar_t*>(font.GetFaceName().wc_str()),
		font.IsFixedWidth() ?
			(is_unstable ?
				"monospaced unstable" :
				(is_fractional ?
					"monospaced stable (fractional)" :
					"monospaced stable (integer)")) :
			"not monospaced");

	struct stat s{};

	_custom_draw_enabled = stat(InMyConfig("nocustomdraw").c_str(), &s) != 0;
	_buffered_paint = false;

	if (font.IsFixedWidth() && !is_unstable && !is_fractional) {
		if (stat(InMyConfig("nobuffering").c_str(), &s) != 0)
			_buffered_paint = true;
	}

	_fonts.clear();
	_fonts.push_back(font);
}

void ConsolePaintContext::ShowFontDialog()
{
	wxFont font;
	if (!_fonts.empty()) {
		font = _fonts.front();
		if (!ChooseFontAndSaveToSettings(_window, font))
			return;

	} else
		InitializeFont(_window, font);

	SetFont(font);
}

uint8_t ConsolePaintContext::CharFitTest(wxPaintDC &dc, wchar_t wc, unsigned int nx)
{
#ifdef DYNAMIC_FONTS
	const bool cacheable = (size_t((uint32_t)wc) - 1 < _char_fit_cache.checked.size()); // && wcz[1] == 0
	if (cacheable && _char_fit_cache.checked[ size_t((uint32_t)wc) - 1 ]) {
		return _char_fit_cache.result[ size_t((uint32_t)wc) - 1 ];
	}

	uint8_t font_index = 0;
	_cft_tmp = wc;
	for (font_index = 0; font_index != 0xff; ++font_index) {
		if (font_index >= _fonts.size()) {
			const auto &prev = _fonts.back();
			auto pt_size = prev.GetPointSize();
			if (pt_size <= 4)
				break;

			_fonts.emplace_back(prev);
			_fonts.back().SetPointSize(pt_size - 1);
		}
		assert(font_index < _fonts.size());

		wxCoord w = _font_width, h = _font_height, d = _font_descent;
		dc.GetTextExtent(_cft_tmp, &w, &h, &d, NULL, &_fonts[font_index]);
		const unsigned limh = _font_height + std::max(0, int(d) - int(_font_descent));
		const unsigned limw = _font_width * nx;
		if (unsigned(h) <= limh && unsigned(w) <= limw) {
			break;
		}
	}
//	if (font_index > 0) {
//		fprintf(stderr, "CharFitTest('%lc') -> %u\n", wc, font_index);
//	}

	if (cacheable) {
		_char_fit_cache.result[ size_t((uint32_t)wc) - 1 ] = font_index;
		_char_fit_cache.checked[ size_t((uint32_t)wc) - 1 ] = true;
	}

	return font_index;

#else
	return 0;

#endif
}

void ConsolePaintContext::ApplyFont(wxPaintDC &dc, uint8_t index)
{
	if (index < _fonts.size())
		dc.SetFont(_fonts[index]);
}

void ConsolePaintContext::OnPaint(wxPaintDC &dc, SMALL_RECT *qedit)
{
	if (UNLIKELY(_stage == STG_NOT_REFRESHED)) {
		// not refreshed yet - so early start so nothing to paint yet
		// so simple fill with background color for the sake of faster start
		dc.SetBackground(GetBrush(g_wx_palette.background[0]));
		dc.Clear();
		return;
	}

#if wxUSE_GRAPHICS_CONTEXT
	wxGraphicsContext* gctx = dc.GetGraphicsContext();
	if (gctx) {
		if (_sharp) {
			gctx->SetInterpolationQuality(wxINTERPOLATION_FAST);
			gctx->SetAntialiasMode(wxANTIALIAS_NONE);
		} else {
			gctx->SetInterpolationQuality(wxINTERPOLATION_DEFAULT);
			gctx->SetAntialiasMode(wxANTIALIAS_DEFAULT);
		}
	}
#endif
	unsigned int cw, ch; g_winport_con_out->GetSize(cw, ch);
	if (UNLIKELY(cw > MAXSHORT)) cw = MAXSHORT;
	if (UNLIKELY(ch > MAXSHORT)) ch = MAXSHORT;

	wxRegion rgn = _window->GetUpdateRegion();
	wxRect box = rgn.GetBox();
	SMALL_RECT area = {SHORT(box.GetLeft() / _font_width), SHORT(box.GetTop() / _font_height),
		SHORT(box.GetRight() / _font_width), SHORT(box.GetBottom() / _font_height)};

	if (UNLIKELY(area.Left < 0)) {
		area.Left = 0;
	}
	if (UNLIKELY(area.Top < 0)) {
		area.Top = 0;
	}
	if (UNLIKELY((unsigned)area.Right >= cw)) {
		area.Right = cw - 1;
	}
	if (UNLIKELY((unsigned)area.Bottom >= ch)) {
		area.Bottom = ch - 1;
	}
	if (UNLIKELY(area.Right < area.Left) || UNLIKELY(area.Bottom < area.Top)) {
		return;
	}

	_line.resize(cw);
	ApplyFont(dc);

	_cursor_props.Update();

	ConsolePainter painter(this, dc, _buffer, _cursor_props);

	for (unsigned int cy = (unsigned)area.Top; cy <= (unsigned)area.Bottom; ++cy) {
		const CHAR_INFO *line;
		{
			// dont keep console output locked for a long time to avoid output slowdown
			IConsoleOutput::DirectLineAccess dla(g_winport_con_out, cy);
			line = dla.Line();
			unsigned int cur_cw = line ? dla.Width() : 0;
			if (cur_cw < cw) {
				memcpy(&_line[0], line, cur_cw * sizeof(*line));
				memset(&_line[cur_cw], 0, (cw - cur_cw) * sizeof(*line));
			} else {
				memcpy(&_line[0], line, cw * sizeof(*line));
			}
			line = &_line[0];
		}

		wxRegionContain lc = rgn.Contains(0, cy * _font_height, cw * _font_width, _font_height);
		if (lc == wxOutRegion) {
			continue;
		}

		painter.LineBegin(cy);
		wchar_t tmp_wcz[2] = {0, 0};
		DWORD64 attributes = line->Attributes;
		const unsigned int cx_begin = (area.Left > 0 && !line[area.Left].Char.UnicodeChar) ? area.Left - 1 : area.Left;
		const unsigned int cx_end = std::min(cw, (unsigned)area.Right + 1);
		bool prev_space = cx_begin > 0 ? line[cx_begin - 1].Char.UnicodeChar == L' ' : false;

		painter.HintLineBegin(cy, cw, ch);

#ifdef TAG_DEBUG
		// out-of clipping: collect tags
		std::vector<ConsolePainter::HintHatch> hatched;
#endif

		for (unsigned int cx = cx_begin; cx < cx_end; ++cx) {
			if (!line[cx].Char.UnicodeChar) {
				painter.LineFlush(cx + 1);
				continue;
			}
			const wchar_t *pwcz;
			if (UNLIKELY(CI_USING_COMPOSITE_CHAR(line[cx]))) {
				pwcz = WINPORT(CompositeCharLookup)(line[cx].Char.UnicodeChar);
			} else {
				tmp_wcz[0] = line[cx].Char.UnicodeChar ? wchar_t(line[cx].Char.UnicodeChar) : L' ';
				pwcz = tmp_wcz;
			}

			attributes = line[cx].Attributes;
			if (qedit && cx >= (unsigned)qedit->Left && cx <= (unsigned)qedit->Right
				&& cy >= (unsigned)qedit->Top && cy <= (unsigned)qedit->Bottom) {
				attributes^= COLOR_ATTRIBUTES;
				if (attributes & FOREGROUND_TRUECOLOR) {
					attributes^= 0x000000ffffff0000;
				}
				if (attributes & BACKGROUND_TRUECOLOR) {
					attributes^= 0xffffff0000000000;
				}
			}

			const int nx = (cx + 1 < cw && !line[cx + 1].Char.UnicodeChar) ? 2 : 1;
			painter.NextChar(cx, attributes, pwcz, nx, prev_space);
			prev_space = pwcz[0] == L' ';

			painter.ConsumeHintAt(line[cx], (int)cx, nx, (int)cy, cw, ch, area, pwcz);
#ifdef TAG_DEBUG			
			hatched.push_back({ line[cx].Extra.Hint.Container, line[cx].Extra.Hint.Object, (int)cx, (int)cy });
#endif
		}
		painter.LineFlush(area.Right + 1);

#ifdef TAG_DEBUG
		painter.DrawHatch(hatched);
#endif
	}

	painter.HintFlush();

	// check if there is unused space in right and bottom and fill it with black color
	const int right_edge = (area.Right + 1) * _font_width;
	const int bottom_edge = (area.Bottom + 1) * _font_height;
	if (right_edge <= box.GetRight()) {
		painter.SetFillColor(g_wx_palette.background[0]);
		dc.DrawRectangle((area.Right + 1) * _font_width, box.GetTop(),
			box.GetRight() + 1 - right_edge, box.GetHeight());
	}
	if (bottom_edge <= box.GetBottom()) {
		painter.SetFillColor(g_wx_palette.background[0]);
		dc.DrawRectangle(box.GetLeft(), bottom_edge,
			box.GetWidth(), box.GetBottom() + 1 - bottom_edge);
	}


	if (UNLIKELY(_stage == STG_REFRESHED)) {
		_stage = STG_PAINTED;
		fprintf(stderr, "FIRST_PAINT: %lu msec\n", (unsigned long)GetProcessUptimeMSec());
	}
}


void ConsolePaintContext::RefreshArea( const SMALL_RECT &area )
{
	if (UNLIKELY(_stage == STG_NOT_REFRESHED)) {
		_stage = STG_REFRESHED;
	}

	wxRect rc;
	rc.SetLeft(((int)area.Left) * _font_width);
	rc.SetRight(((int)area.Right) * _font_width + _font_width - 1);
	rc.SetTop(((int)area.Top) * _font_height);
	rc.SetBottom(((int)area.Bottom) * _font_height + _font_height - 1);
	_window->Refresh(false, &rc);
}


void ConsolePaintContext::BlinkCursor()
{
	if (_cursor_props.Blink()) {
		SMALL_RECT area = {
			_cursor_props.pos.X, _cursor_props.pos.Y,
			_cursor_props.pos.X, _cursor_props.pos.Y
		};
		CHAR_INFO ci{};
		if (g_winport_con_out->Read(ci, _cursor_props.pos)) {
			if (!ci.Char.UnicodeChar && area.Left > 0) {
				--area.Left;
			} else if (CI_FULL_WIDTH_CHAR(ci)) {
				++area.Right;
			}
		}
		RefreshArea(area);
	}
}

void ConsolePaintContext::SetSharp(bool sharp)
{
	if (_sharp != sharp) {
		_sharp = sharp;
		_window->Refresh();
	}
}

bool ConsolePaintContext::IsSharpSupported()
{
#if wxUSE_GRAPHICS_CONTEXT
	return true;
#else
	return false;
#endif
}

wxBrush &ConsolePaintContext::GetBrush(const WinPortRGB &clr)
{
	auto it = _color2brush.find(clr);
	if (it != _color2brush.end()) {
		return it->second;
	}

	return _color2brush.emplace(clr, wxColour(clr.r, clr.g, clr.b)).first->second;
}

/////////////////////////////

bool CursorProps::Blink()
{
	bool prev_blink_state = blink_state;
	blink_state = !blink_state;
	Update();
	return (blink_state != prev_blink_state);
}

void CursorProps::Update()
{
	pos = g_winport_con_out->GetCursor(height, visible);
	if (prev_pos.X != pos.X || prev_pos.Y != pos.Y) {
		prev_pos = pos;
		blink_state = true;
	}
}

//////////////////////

ConsolePainter::ConsolePainter(ConsolePaintContext *context, wxPaintDC &dc, wxString &buffer, CursorProps &cursor_props) :
	_context(context), _dc(dc), _buffer(buffer), _cursor_props(cursor_props),
	_start_cx((unsigned int)-1), _start_back_cx((unsigned int)-1), _prev_fit_font_index(0),
	_prev_underlined(false), _prev_strikeout(false)
{
	_dc.SetPen(context->GetTransparentPen());
	_dc.SetBackgroundMode(wxPENSTYLE_TRANSPARENT);
	_buffer.Empty();
}


void ConsolePainter::SetFillColor(const WinPortRGB &clr)
{
	if (_brush_clr.Change(clr)) {
		wxBrush &brush = _context->GetBrush(clr);
		_dc.SetBrush(brush);
		_dc.SetBackground(brush);
	}
}

void ConsolePainter::PrepareBackground(unsigned int cx, const WinPortRGB &clr, unsigned int nx)
{
	const bool cursor_here = (_cursor_props.visible && _cursor_props.blink_state
		&& cx == (unsigned int)_cursor_props.pos.X
		&& _start_cy == (unsigned int)_cursor_props.pos.Y);

	if (!cursor_here && _start_back_cx != (unsigned int)-1 && _clr_back == clr)
		return;

	FlushBackground(cx + nx - 1);

	if (!cursor_here) {
		_clr_back = clr;
		_start_back_cx = cx;
		return;
	}

	_start_back_cx = (unsigned int)-1;

	const unsigned int x = cx * _context->FontWidth();
	unsigned int h = (_context->FontHeight() * _cursor_props.height) / 100;
	if (h==0) h = 1;
	unsigned int fill_height = _context->FontHeight() - h;
	if (fill_height > _context->FontHeight()) fill_height = _context->FontHeight();
	WinPortRGB clr_xored = GetCursorColor(clr);
	SetFillColor(clr_xored);
	_dc.DrawRectangle(x, _start_y + fill_height, _context->FontWidth() * nx, h);

	if (fill_height) {
		SetFillColor(clr);
		_dc.DrawRectangle(x, _start_y, _context->FontWidth() * nx, fill_height);
	}
}

void ConsolePainter::FlushBackground(unsigned int cx_end)
{
	if (_start_back_cx != ((unsigned int)-1)) {
		SetFillColor(_clr_back);
		_dc.DrawRectangle(_start_back_cx * _context->FontWidth(), _start_y,
			(cx_end - _start_back_cx) * _context->FontWidth(), _context->FontHeight());
		_start_back_cx = ((unsigned int)-1);
	}
}

void ConsolePainter::FlushText(unsigned int cx_end)
{
	if (!_buffer.empty()) {
		_dc.SetTextForeground(wxColour(_clr_text.r, _clr_text.g, _clr_text.b));
		_dc.DrawText(_buffer, _start_cx * _context->FontWidth(), _start_y);
		_buffer.Empty();
	}
	FlushDecorations(cx_end);
	_start_cx = (unsigned int)-1;
	_prev_fit_font_index = 0;
}

void ConsolePainter::FlushDecorations(unsigned int cx_end)
{
	if (!_prev_underlined && !_prev_strikeout) {
		return;
	}
	_dc.SetPen(wxColour(_clr_text.r, _clr_text.g, _clr_text.b));

	if (_prev_underlined) {
		_dc.DrawLine(_start_cx * _context->FontWidth(), _start_y + _context->FontHeight() - 1,
			cx_end * _context->FontWidth(), _start_y + _context->FontHeight() - 1);
		_prev_underlined = false;
	}

	if (_prev_strikeout) {
		_dc.DrawLine(_start_cx * _context->FontWidth(), _start_y + (_context->FontHeight() / 2),
			cx_end * _context->FontWidth(), _start_y + (_context->FontHeight() / 2));
		_prev_strikeout = false;
	}

	_dc.SetPen(_context->GetTransparentPen());
}

static inline unsigned char CalcFadeColor(unsigned char bg, unsigned char fg)
{
	unsigned out = fg;
	out*= 2;
	out+= bg;
	out/= 3;
	return (unsigned char)std::min(out, (unsigned)0xff);
}

static inline unsigned char CalcExtraFadeColor(unsigned char bg, unsigned char fg)
{
	unsigned out = bg;
	out+= fg;
	out/= 2;
	return (unsigned char)std::min(out, (unsigned)0xff);
}

// #define DEBUG_FADED_EDGES

struct WXCustomDrawCharPainter : WXCustomDrawChar::Painter
{
	ConsolePainter &_painter;
	const WinPortRGB &_clr_text;
	const WinPortRGB &_clr_back;

	inline WXCustomDrawCharPainter(ConsolePainter &painter, const WinPortRGB &clr_text, const WinPortRGB &clr_back, bool _prev_space)
		: _painter(painter), _clr_text(clr_text), _clr_back(clr_back)
	{
		fw = (wxCoord)_painter._context->FontWidth();
		fh = (wxCoord)_painter._context->FontHeight(),
		thickness = (wxCoord)_painter._context->FontThickness();
		prev_space = _prev_space;
		_painter.SetFillColor(clr_text);
	}

	inline bool MayDrawFadedEdgesImpl()
	{
		return (fw > 7 && fh > 7 && !_painter._context->IsSharp());
	}

	inline void SetColorFadedImpl()
	{
#ifndef DEBUG_FADED_EDGES
		WinPortRGB clr_fade(CalcFadeColor(_clr_back.r, _clr_text.r),
			CalcFadeColor(_clr_back.g, _clr_text.g), CalcFadeColor(_clr_back.b, _clr_text.b));
#else
		WinPortRGB clr_fade(0xff, 0, 0);
#endif
		_painter.SetFillColor(clr_fade);
	}

	inline void SetColorExtraFadedImpl()
	{
#ifndef DEBUG_FADED_EDGES
		WinPortRGB clr_fade(CalcExtraFadeColor(_clr_back.r, _clr_text.r),
			CalcExtraFadeColor(_clr_back.g, _clr_text.g), CalcExtraFadeColor(_clr_back.b, _clr_text.b));
#else
		WinPortRGB clr_fade(0, 0xff, 0);
#endif
		_painter.SetFillColor(clr_fade);
	}

	inline void SetAccentBackgroundImpl() {
		_painter.ComputeAccents();
		_painter.SetFillColor(_painter._clr_accent_back);
	}
	
	inline void SetBackgroundImpl() {
		_painter.SetFillColor(_painter._clr_back);
	}
	
	inline void SetAccentForegroundImpl() {
		_painter.ComputeAccents();
		_painter.SetFillColor(_painter._clr_accent_text);
	}

	inline void SetForegroundImpl() {
		_painter.SetFillColor(_painter._clr_text);
	}

	inline void SetColorEmbossImpl()
	{
		WinPortRGB clr_fade;
		/* near to black / near to white means LAB */
		int blackb = _clr_back.r + _clr_back.g + _clr_back.b;
		int blackf = _clr_text.r + _clr_text.g + _clr_text.b;
		if (blackb < 0x5f || blackf < 0x5f || blackb > 700 || blackf > 700) 
			clr_fade = ComputeEmbossColor_LAB(_clr_back, _painter.GetSoftenColorIf(_clr_text));
		else
			clr_fade = ComputeEmbossColor_HSL(_clr_back, _clr_text);
		_painter.SetFillColor(clr_fade);
	}

	inline void SetColorSoftenImpl()
	{
		WinPortRGB fade = _painter.GetSoftenColorIf(_clr_text);
		_painter._dc.SetBrush(wxColour(fade.r, fade.r, fade.r));
		//SetColorRedImpl();
	}

	inline void SetColorRedImpl()
	{
		WinPortRGB clr_fade(0xff, 0, 0);
		_painter.SetFillColor(clr_fade);
	}

	inline int GetFontAscentImpl()
	{
		return _painter._context->FontHeight() - _painter._context->FontDescent();
	}

	inline void FillRectangleImpl(wxCoord left, wxCoord top, wxCoord right, wxCoord bottom)
	{
		_painter._dc.DrawRectangle(left, top, right + 1 - left , bottom + 1 - top);
	}

	inline void FillGradientRectangleImpl(wxCoord left, wxCoord top, wxCoord right, wxCoord bottom)
	{
		wxBrush oldBrush = _painter._dc.GetBrush(); 
		wxColour brushColor = oldBrush.GetColour();
		WinPortRGB x{ (unsigned char)brushColor.Red(), (unsigned char)brushColor.Green(), (unsigned char)brushColor.Blue() };
		_painter.DrawLiquidButtonBackground(left, top, right + 1 - left , bottom + 1 - top, x);
	}

	inline void DrawEllipticArcImpl(wxCoord left, wxCoord top, wxCoord width, wxCoord height, double start, double end, wxCoord thickness) {
		wxBrush oldBrush = _painter._dc.GetBrush(); 
		wxColour brushColor = oldBrush.GetColour();
		wxPen oldPen = _painter._dc.GetPen(); 

		_painter._dc.SetPen(wxPen(brushColor, thickness < 1 ? 1 : thickness));
		_painter._dc.SetBrush(*wxTRANSPARENT_BRUSH);

		_painter._dc.DrawEllipticArc(left, top, width, height, start, end);
		
		_painter._dc.SetBrush(oldBrush);
		_painter._dc.SetPen(oldPen);
	}

	inline void FillEllipticPieImpl(wxCoord left, wxCoord top, wxCoord width, wxCoord height, double start, double end) {
		_painter._dc.DrawEllipticArc(left, top, width, height, start, end);
	}

	inline void DrawLineImpl(wxCoord X1, wxCoord Y1, wxCoord X2, wxCoord Y2, wxCoord thickness) {
		wxBrush oldBrush = _painter._dc.GetBrush(); 
		wxColour brushColor = oldBrush.GetColour();
		wxPen oldPen = _painter._dc.GetPen(); 
		_painter._dc.SetPen(wxPen(brushColor, thickness < 1 ? 1 : thickness));

		_painter._dc.SetBrush(*wxTRANSPARENT_BRUSH);
		_painter._dc.DrawLine(X1, Y1, X2, Y2);
		_painter._dc.SetBrush(oldBrush);
		_painter._dc.SetPen(oldPen);
	}

	// wxBrush savedBrush;
	// wxPen savedPen;

	inline void SaveBrushImpl() {
		// savedBrush = _painter._dc.GetBrush();
		// savedPen = _painter._dc.GetPen();
	}

	inline void RestoreBrushImpl() {
		//_painter._dc.SetBrush(savedBrush);
		//_painter._dc.SetPen(savedPen);
		_painter.SetFillColor(_clr_text);
	}
};

// this code little bit wacky just to avoid virtual methods overhead
bool WXCustomDrawChar::Painter::MayDrawFadedEdges()
{
	return ((WXCustomDrawCharPainter *)this)->MayDrawFadedEdgesImpl();
}

void WXCustomDrawChar::Painter::SetColorFaded()
{
	((WXCustomDrawCharPainter *)this)->SetColorFadedImpl();
}

void WXCustomDrawChar::Painter::SetColorExtraFaded()
{
	((WXCustomDrawCharPainter *)this)->SetColorExtraFadedImpl();
}

void WXCustomDrawChar::Painter::SetAccentBackground() 
{
	((WXCustomDrawCharPainter *)this)->SetAccentBackgroundImpl();
}

void WXCustomDrawChar::Painter::SetBackground() 
{
	((WXCustomDrawCharPainter *)this)->SetBackgroundImpl();
}

void WXCustomDrawChar::Painter::SetAccentForeground()
{
	((WXCustomDrawCharPainter *)this)->SetAccentForegroundImpl();
}

void WXCustomDrawChar::Painter::SetForeground()
{
	((WXCustomDrawCharPainter *)this)->SetForegroundImpl();
}

void WXCustomDrawChar::Painter::SetColorEmboss()
{
	((WXCustomDrawCharPainter *)this)->SetColorEmbossImpl();
}

void WXCustomDrawChar::Painter::SetColorSoften()
{
	((WXCustomDrawCharPainter *)this)->SetColorSoftenImpl();
}

void WXCustomDrawChar::Painter::SetColorRed()
{
	((WXCustomDrawCharPainter *)this)->SetColorRedImpl();
}

int WXCustomDrawChar::Painter::GetFontAscent()
{
	return ((WXCustomDrawCharPainter *)this)->GetFontAscentImpl();
}

void WXCustomDrawChar::Painter::FillRectangle(wxCoord left, wxCoord top, wxCoord right, wxCoord bottom)
{
	((WXCustomDrawCharPainter *)this)->FillRectangleImpl(left, top, right, bottom);
}

void WXCustomDrawChar::Painter::FillGradientRectangle(wxCoord left, wxCoord top, wxCoord right, wxCoord bottom)
{
	((WXCustomDrawCharPainter *)this)->FillGradientRectangleImpl(left, top, right, bottom);
}

void WXCustomDrawChar::Painter::DrawEllipticArc(wxCoord left, wxCoord top, wxCoord width, wxCoord height, double start, double end, wxCoord thickness)
{
	((WXCustomDrawCharPainter *)this)->DrawEllipticArcImpl(left, top, width, height, start, end, thickness);
}

void WXCustomDrawChar::Painter::FillPixel(wxCoord left, wxCoord top)
{
	((WXCustomDrawCharPainter *)this)->FillRectangleImpl(left, top, left, top);
}

void WXCustomDrawChar::Painter::FillEllipticPie(wxCoord left, wxCoord top, wxCoord width, wxCoord height, double start, double end)
{
	((WXCustomDrawCharPainter *)this)->FillEllipticPieImpl(left, top, width, height, start, end);
}

void WXCustomDrawChar::Painter::DrawLine(wxCoord X1, wxCoord Y1, wxCoord X2, wxCoord Y2, wxCoord thickness) 
{
	((WXCustomDrawCharPainter *)this)->DrawLineImpl(X1, Y1, X2, Y2, thickness);
}

void WXCustomDrawChar::Painter::SaveBrush() {
	((WXCustomDrawCharPainter *)this)->SaveBrushImpl();
}

void WXCustomDrawChar::Painter::RestoreBrush() {
	((WXCustomDrawCharPainter *)this)->RestoreBrushImpl();
}

bool ConsolePainter::DrawCustomCharImpl(wchar_t cc, WXCustomDrawChar::DrawT custom_draw, DWORD64 attributes, unsigned cx, unsigned nx, bool prev_space) 
{
	WinPortRGB clr_back = WxConsoleBackground2RGB(attributes);
	PrepareBackground(cx, clr_back, nx);
	const bool underlined = (attributes & COMMON_LVB_UNDERSCORE) != 0;
	const bool strikeout = (attributes & COMMON_LVB_STRIKEOUT) != 0;
	WinPortRGB clr_text = WxConsoleForeground2RGB(attributes);

    if (custom_draw) {
		FlushBackground(cx + nx);
		WXCustomDrawCharPainter cdp(*this, clr_text, clr_back, prev_space);
		cdp.wc = cc;
		custom_draw(cdp, _start_y, cx);
		/* bold does not affect to custom draws as it are unicode glyphs, borders etc */
		if (underlined || strikeout) {
			_start_cx = cx;
			_prev_underlined = underlined;
			_prev_strikeout = strikeout;
			_clr_text = clr_text;
			FlushDecorations(cx + nx);
		}
		_start_cx = (unsigned int)-1;
		_prev_fit_font_index = 0;
	}
	return custom_draw != nullptr;
}

bool ConsolePainter::DrawCustomChar(wchar_t cc, WXCustomDrawChar::DrawT custom_draw, DWORD64 attributes, unsigned cx, unsigned nx, bool prev_space) {
	if (custom_draw) 
		line_custom_chars.push_back(CustomCharPos(cc, custom_draw, attributes, cx, nx, prev_space));
	return custom_draw != nullptr;
}

void ConsolePainter::NextChar(unsigned int cx, DWORD64 attributes, const wchar_t *wcz, unsigned int nx, bool prev_space)
{
	if (!wcz[0] || !WCHAR_IS_VALID(wcz[0])) {
		wcz = L" ";
	}

	WXCustomDrawChar::DrawT custom_draw = nullptr;

	if ((!wcz[1] && (wcz[0] == L' ' || (_context->IsCustomDrawEnabled()
	 && (custom_draw = WXCustomDrawChar::Get(wcz[0])) != nullptr)))) {
		if (!_buffer.empty())
			FlushBackground(cx + nx - 1);
		FlushText(cx + nx - 1);
	}

	const WinPortRGB &clr_back = WxConsoleBackground2RGB(attributes);
	PrepareBackground(cx, clr_back, nx);

	const bool underlined = (attributes & COMMON_LVB_UNDERSCORE) != 0;
	const bool strikeout = (attributes & COMMON_LVB_STRIKEOUT) != 0;

	if (!strikeout && !underlined && wcz[0] == L' ' && !wcz[1]) {
		return;
	}

	const WinPortRGB &clr_text = WxConsoleForeground2RGB(attributes);
	_clr_accent_computed = false;

	if (custom_draw && DrawCustomChar(wcz[0], custom_draw, attributes, cx, nx, prev_space)) {
		return;
	}

	uint8_t fit_font_index = _context->CharFitTest(_dc, *wcz, nx);

	if (fit_font_index == _prev_fit_font_index && _prev_underlined == underlined && _prev_strikeout == strikeout
		&& _start_cx != (unsigned int)-1 && _clr_text == clr_text && _context->IsPaintBuffered())
	{
		_buffer+= wcz;
		return;
	}

	FlushBackground(cx + nx);
	FlushText(cx);

	_prev_fit_font_index = fit_font_index;
	_prev_underlined = underlined;
	_prev_strikeout = strikeout;

	_start_cx = cx;
	_buffer = wcz;
	_clr_text = clr_text;

	if (fit_font_index != 0 && fit_font_index != 0xff) {
		_context->ApplyFont(_dc, fit_font_index);
		FlushText(cx + nx);
		_context->ApplyFont(_dc);
	}
}

#include <Colorspace.h>

static WinPortRGB RGBtoFar(const RGB& rgb) 
{
	return WinPortRGB((int)(rgb.r*255), (int)(rgb.g*255), (int)(rgb.b*255));
}

static WinPortRGB RGBtoFar(const iRGB& rgb) 
{
	return WinPortRGB(rgb.r, rgb.g, rgb.b);
}

static RGB FarToRGB(const WinPortRGB& c) 
{
	RGB rgb{c.r/255.0, c.g/255.0, c.b/255.0};
	return rgb;
}

WinPortRGB ConsolePainter::GetSoftenColorIf(const WinPortRGB& _clr_text) 
{
	WinPortRGB clr{_clr_text.r, _clr_text.g, _clr_text.b};
	if (WXCustomDrawChar::options && WXCustomDrawChar::options->UseSoftenBevels) {
		if (IsNearBlack(_clr_text.r, _clr_text.g, _clr_text.b) || IsNearWhite(_clr_text.r, _clr_text.g, _clr_text.b)) {
			RGB clrR = FarToRGB(clr);
			iRGB clrT = SoftenBlackish_LAB(clrR);
			clr = RGBtoFar(clrT);
		}
	}
	return clr;
}

WinPortRGB ConsolePainter::GetEmbossColor(const WinPortRGB& _clr_text) {
	WinPortRGB clr_fade;
	/* near to black / near to white means LAB */

	int blackb = _clr_back.r + _clr_back.g + _clr_back.b;
	int blackf = _clr_text.r + _clr_text.g + _clr_text.b;

	if (blackb < 0x5f || blackf < 0x5f || blackb > 700 || blackf > 700) 
		clr_fade = ComputeEmbossColor_LAB(_clr_back, GetSoftenColorIf(_clr_text));
	else
		clr_fade = ComputeEmbossColor_HSL(_clr_back, _clr_text);
	return clr_fade;
}

struct ColorsCache {
	WinPortRGB bg;
	WinPortRGB fg;
	WinPortRGB rg;
	int raised;
};

static ColorsCache _last_colors;

static bool ColorEq(const WinPortRGB& c1, const WinPortRGB& c2)
{
	return c1.r == c2.r && c1.g == c2.g && c1.b == c2.b;
}

WinPortRGB ComputeEmbossColor_HSL(const WinPortRGB& xbg, const WinPortRGB& xline) 
{
	if (_last_colors.raised && ColorEq(_last_colors.bg, xbg) && ColorEq(_last_colors.fg, xline)) {
		return _last_colors.rg;
	}

	RGB bg = FarToRGB(xbg);
	RGB fg = FarToRGB(xline);
	RGB r = ComputeRaiseColor_HSL(bg, fg);

	_last_colors.bg = xbg;
	_last_colors.fg = xline;
	_last_colors.rg = RGBtoFar(r);
	_last_colors.raised = 1;

	return _last_colors.rg;
}

WinPortRGB ComputeEmbossColor_LAB(const WinPortRGB& xbg, const WinPortRGB& xline) 
{
	if (_last_colors.raised && ColorEq(_last_colors.bg, xbg) && ColorEq(_last_colors.fg, xline)) {
		return _last_colors.rg;
	}

	RGB bg = FarToRGB(xbg);
	RGB fg = FarToRGB(xline);
	RGB r = ComputeRaiseColor_LAB(bg, fg);

	_last_colors.bg = xbg;
	_last_colors.fg = xline;
	_last_colors.rg = RGBtoFar(r);
	_last_colors.raised = 1;

	return _last_colors.rg;
}

void ConsolePainter::ComputeAccents(bool cache) {
	if (cache && _clr_accent_computed) return;

	HoverResult r = ComputeControlAccent(FarToRGB(_clr_text), FarToRGB(_clr_back));
	_clr_accent_computed = true;
	_clr_accent_text = RGBtoFar(r.fg_hover);
	_clr_accent_back = RGBtoFar(r.bg_hover);
}

void ConsolePainter::ComputeAccents(
		const WinPortRGB& c_text, const WinPortRGB& c_back,
		WinPortRGB& c_a_text, WinPortRGB& c_a_back) 
{
	HoverResult r = ComputeControlAccent(FarToRGB(c_text), FarToRGB(c_back));
	c_a_text = RGBtoFar(r.fg_hover);
	c_a_back = RGBtoFar(r.bg_hover);
}

WinPortRGB ConsolePainter::GetCursorColor(const WinPortRGB& clr) {
	// was: WinPortRGB clr_xored(clr.r ^ 0xff, clr.g ^ 0xff, clr.b ^ 0xff);

	if (_clr_for_caret == clr) return _caret_clr;
	_clr_for_caret = clr;

	// WinPortRGB clr_xored(255 - clr.r, 255 - clr.g, 255 - clr.b);
	WinPortRGB clr_xored(clr.r ^ 0xff, clr.g ^ 0xff, clr.b ^ 0xff);

	int lum1 = (clr_xored.r * 30 + clr_xored.g * 59 + clr_xored.b * 11) / 100;
	int lum2 = (clr.r * 30 + clr.g * 59 + clr.b * 11) / 100;

	bool is_bad_for_xor = abs(lum1 - lum2) < 40;
    
    if (is_bad_for_xor) {
        HSL hsl;
    	RGB rgb = toRGB(clr.r, clr.g, clr.b);

        RGBtoHSL(rgb, hsl.h, hsl.s, hsl.l);
        hsl.l = 1.0 - hsl.l;
        if (hsl.l > 0.5)
            hsl.l = std::min(1.0, hsl.l + 0.15);
        else
            hsl.l = std::max(0.0, hsl.l - 0.15);
        rgb = HSLtoRGB(hsl.h, hsl.s, hsl.l);
        iRGB r = toIRGB(rgb);

        clr_xored = WinPortRGB(r.r, r.g, r.b);
    }
    _caret_clr = clr_xored;
	return clr_xored;
}

void ConsolePainter::DrawGradientLine(wxCoord X1, wxCoord Y1, wxCoord X2, wxCoord Y2, const WinPortRGB& c1, const WinPortRGB& c2, wxCoord thickness)
{
    wxGraphicsContext* gc = wxGraphicsContext::Create(_dc);
    if (!gc) return;

    wxCoord dx = X2 - X1;
    wxCoord dy = Y2 - Y1;
    wxCoord length = std::sqrt(dx*dx + dy*dy);

    // Create gradient brush
    wxGraphicsGradientStops stops(wxColour(c1.r, c1.g, c1.b), wxColour(c2.r, c2.g, c2.b));
    wxGraphicsBrush brush = gc->CreateLinearGradientBrush(X1, Y1, X2, Y2, stops);

    gc->SetBrush(brush);
    gc->SetPen(*wxTRANSPARENT_PEN);

    // Draw a thin rectangle along the line
    gc->PushState();
    gc->Translate(X1, Y1);
    gc->Rotate(std::atan2(dy, dx));
    gc->DrawRectangle(0, - thickness / 2, length, thickness);
    gc->PopState();

    delete gc;
}

void ConsolePainter::DrawHorizontalGradientLine(wxCoord X1, wxCoord Y1, wxCoord length, const WinPortRGB& c1, const WinPortRGB& c2, wxCoord thickness) {
    wxGraphicsContext* gc = wxGraphicsContext::Create(_dc);
    if (!gc) return;

    wxGraphicsGradientStops stops(wxColour(c1.r, c1.g, c1.b), wxColour(c2.r, c2.g, c2.b));
    wxGraphicsBrush brush = gc->CreateLinearGradientBrush(X1, Y1, X1 + length, Y1, stops);

    gc->SetBrush(brush);
    gc->SetPen(*wxTRANSPARENT_PEN);

    gc->DrawRectangle(X1, Y1 - thickness / 2, length, thickness);

    delete gc;
}

void ConsolePainter::DrawHorizontalGradientLine(wxCoord X1, wxCoord Y1, wxCoord length, const WinPortRGB& c1, const WinPortRGB& c2, const WinPortRGB& c3, wxCoord thickness) {
    wxGraphicsContext* gc = wxGraphicsContext::Create(_dc);
    if (!gc) return;

    wxGraphicsGradientStops stops;
    stops.Add(wxColour(c1.r, c1.g, c1.b), 0.0);
    stops.Add(wxColour(c2.r, c2.g, c2.b), 0.5);
    stops.Add(wxColour(c3.r, c3.g, c3.b), 1.0);
    wxGraphicsBrush brush = gc->CreateLinearGradientBrush(X1, Y1, X1 + length, Y1, stops);

    gc->SetBrush(brush);
    gc->SetPen(*wxTRANSPARENT_PEN);

    gc->DrawRectangle(X1, Y1 - thickness / 2, length, thickness);

    delete gc;
}

void ConsolePainter::DrawVerticalGradientLine(wxCoord X1, wxCoord Y1, wxCoord length, const WinPortRGB& c1, const WinPortRGB& c2, wxCoord thickness) 
{
    wxGraphicsContext* gc = wxGraphicsContext::Create(_dc);
    if (!gc) return;

    wxGraphicsGradientStops stops(wxColour(c1.r, c1.g, c1.b), wxColour(c2.r, c2.g, c2.b));
    wxGraphicsBrush brush = gc->CreateLinearGradientBrush(X1, Y1, X1, Y1 + length, stops);

    gc->SetBrush(brush);
    gc->SetPen(*wxTRANSPARENT_PEN);

    gc->DrawRectangle(X1 - thickness / 2, Y1, thickness, length);

    delete gc;
}

void ConsolePainter::DrawVerticalGradientLine(wxCoord X1, wxCoord Y1, wxCoord length, const WinPortRGB& c1, const WinPortRGB& c2, const WinPortRGB& c3, wxCoord thickness) 
{
    wxGraphicsContext* gc = wxGraphicsContext::Create(_dc);
    if (!gc) return;

    wxGraphicsGradientStops stops;
    stops.Add(wxColour(c1.r, c1.g, c1.b), 0.0);
    stops.Add(wxColour(c2.r, c2.g, c2.b), 0.5);
    stops.Add(wxColour(c3.r, c3.g, c3.b), 1.0);
    wxGraphicsBrush brush = gc->CreateLinearGradientBrush(X1, Y1, X1, Y1 + length, stops);

    gc->SetBrush(brush);
    gc->SetPen(*wxTRANSPARENT_PEN);

    gc->DrawRectangle(X1 - thickness / 2, Y1, thickness, length);

    delete gc;
}

void ConsolePainter::DrawHorizontalDashedGradientLine(
	wxCoord X1, wxCoord Y1, wxCoord length, 
	const WinPortRGB& xc1, const WinPortRGB& xc2,
	int dashLength, int gapLength, 
	wxCoord thickness)
{
    wxGraphicsContext* gc = wxGraphicsContext::Create(_dc);
    if (!gc) return;

    wxColour c1(xc1.r, xc1.g, xc1.b);
    wxColour c2(xc2.r, xc2.g, xc2.b);

    int totalLength = length;
    int patternLength = dashLength + gapLength;

    for (int pos = 0; pos < totalLength; pos += patternLength) {
        int dashStart = X1 + pos;
        int dashEnd   = std::min(dashStart + dashLength, X1 + length);

        // Compute gradient factor at the center of the dash
        double t = double(dashStart + dashLength * 0.5 - X1) / totalLength;
        t = std::clamp(t, 0.0, 1.0);

        wxColour c(
            c1.Red()   + t * (c2.Red()   - c1.Red()),
            c1.Green() + t * (c2.Green() - c1.Green()),
            c1.Blue()  + t * (c2.Blue()  - c1.Blue())
        );

        gc->SetBrush(gc->CreateBrush(wxBrush(c)));
        gc->SetPen(*wxTRANSPARENT_PEN);

        gc->DrawRectangle(dashStart, Y1 - thickness / 2, dashEnd - dashStart, thickness);
    }

    delete gc;
}

void ConsolePainter::DrawVerticalDashedGradientLine(
	wxCoord X1, wxCoord Y1, wxCoord length, 
	const WinPortRGB& xc1, const WinPortRGB& xc2,
	int dashLength, int gapLength, 
	wxCoord thickness)
{
    wxGraphicsContext* gc = wxGraphicsContext::Create(_dc);
    if (!gc) return;

    wxColour c1(xc1.r, xc1.g, xc1.b);
    wxColour c2(xc2.r, xc2.g, xc2.b);

    int totalLength = length;
    int patternLength = dashLength + gapLength;

    for (int pos = 0; pos < totalLength; pos += patternLength) {
        int dashStart = Y1 + pos;
        int dashEnd   = std::min(dashStart + dashLength, Y1 + length);

        // Compute gradient factor at the center of the dash
        double t = double(dashStart + dashLength * 0.5 - Y1) / totalLength;
        t = std::clamp(t, 0.0, 1.0);

        wxColour c(
            c1.Red()   + t * (c2.Red()   - c1.Red()),
            c1.Green() + t * (c2.Green() - c1.Green()),
            c1.Blue()  + t * (c2.Blue()  - c1.Blue())
        );

        gc->SetBrush(gc->CreateBrush(wxBrush(c)));
        gc->SetPen(*wxTRANSPARENT_PEN);

        gc->DrawRectangle(X1 - thickness / 2, dashStart, thickness, dashEnd - dashStart);
    }

    delete gc;
}

void ConsolePainter::DrawLiquidButtonBackground(
	wxCoord X1, wxCoord Y1, wxCoord w, wxCoord h, 
    const WinPortRGB& r_col)
{

	LAB base = RGBtoLAB(FarToRGB(r_col));
	LAB topLab = base;
	topLab.L = std::min(topLab.L + 15.0, 100.0);

	LAB bottomLab = base;
	bottomLab.L = std::max(bottomLab.L - 10.0, 0.0);

	RGB f_colTop = LABtoRGB(topLab);
	RGB f_colBottom = LABtoRGB(bottomLab);

	WinPortRGB r_colTop = RGBtoFar(f_colTop);
	WinPortRGB r_colBottom = RGBtoFar(f_colBottom);

	wxColour colTop(r_colTop.r, r_colTop.g, r_colTop.b);
	wxColour colBottom(r_colBottom.r, r_colBottom.g, r_colBottom.b);

    // --- Top glossy highlight (about 35% of height) ---
    wxRect rTop(X1, Y1, w, h * 0.35);
    wxColour top1 = wxColour(
    	std::min(colTop.Red()   + 40, 255),
        std::min(colTop.Green() + 40, 255),
        std::min(colTop.Blue()  + 40, 255)
    );
    wxColour top2 = colTop;

    _dc.GradientFillLinear(rTop, top1, top2, wxSOUTH);

    // --- Middle body (about 45% of height) ---
    wxRect rMid(X1, Y1 + h * 0.35, w, h * 0.45);
    _dc.GradientFillLinear(rMid, colTop, colBottom, wxSOUTH);

    // --- Bottom shadow (about 20% of height) ---
    wxRect rBot(X1, Y1 + h * 0.80, w, h * 0.20);
    wxColour bot1 = colBottom;
    wxColour bot2 = wxColour(
        std::max(colBottom.Red()   - 30, 0),
        std::max(colBottom.Green() - 30, 0),
        std::max(colBottom.Blue()  - 30, 0)
    );

    _dc.GradientFillLinear(rBot, bot1, bot2, wxSOUTH);
}

void ConsolePainter::ConsumeHintAt(const CHAR_INFO& ci, int cx, int nx, int cy, unsigned cw, unsigned ch, const SMALL_RECT& area, const wchar_t* pwcz) {
	if (WXCustomDrawChar::options && !WXCustomDrawChar::options->UseModernLook) return;

	if (ci.Extra.Hint.Container != HintDialog) return;
	if (ci.Extra.Hint.Object == HintObjectNone) return;

	if (ci.Extra.Hint.Object == HintButton)
		fprintf(stderr, "    ...(%d..%d, %d) = %d:%d +`%ls` tag=%d\n", 
			cx, cx + nx - 1, cy, 
			ci.Extra.Hint.Container, ci.Extra.Hint.Object, pwcz, 
			ci.Extra.Hint.Tag);

	std::wstring text(pwcz);

	if (line_hints.size() > 0) {
		for(int i = line_hints.size() - 1; i >= 0; --i) {
			if (line_hints[i].Hint.Object == ci.Extra.Hint.Object && line_hints[i].tag == ci.Extra.Hint.Tag) {
				line_hints[i].nx = cx /*+ nx - 1*/; // - line_hints[i].cx;
				line_hints[i].text += text;
				return;
			}
		}
	}

	HintPos pos { {
		ci.Extra.Hint.Container, ci.Extra.Hint.Object, 
		ci.Extra.Hint.Focus, ci.Extra.Hint.Hover, ci.Extra.Hint.Enabled, 
		ci.Extra.Hint.Default, ci.Extra.Hint.Beveled, ci.Extra.Hint.Shadow },
		ci.Extra.Hint.Tag,
		cx, cx /* + nx - 1*/, // nx 
		cy, 
		cw, ch,
		ci.Attributes, text,
		area };

	line_hints.push_back(pos);
}

void ConsolePainter::DrawHint(const HintPos& x) {
	WinPortRGB clr_back = WxConsoleBackground2RGB(x.attributes);
	WinPortRGB clr_text = WxConsoleForeground2RGB(x.attributes);

	int cx_start = x.cx;
	int cx_end = x.nx;
	int cy = x.cy;

	if (cy < 0 || cy < x.area.Top || cy > x.area.Bottom) return;
	if (cx_end < 0 || cx_end < x.area.Left) return;
	if (cx_start > x.area.Right || (unsigned)cx_start > x.cw) return;

	if (cx_start < x.area.Left) cx_start = x.area.Left;
	if (cx_start > x.area.Right) cx_start = x.area.Right;
	if (cx_end < x.area.Left) cx_end = x.area.Left;
	if (cx_end > x.area.Right) cx_end = x.area.Right;

	if (cx_end <= cx_start) return;

	switch(x.Hint.Object){
    case HintEdit:
    case HintFixEdit:
    case HintPswEdit:
    case HintMemoEdit:
    	break;
    case HintComboBox:
    	break;
    case HintButton:
    	fprintf(stderr, "...paint: button: %d..%d, %d -> %d..%d, %d `%ls` /..%d/ in %d..%d, %d..%d, tag=%d focus=%c hover=%c type=%d\n", 
        	x.cx, x.nx, x.cy,
    		cx_start, cx_end, cy, 
            x.text.c_str(), (int)(cx_start + x.text.size()),
            (int)x.area.Left, (int)x.area.Right, (int)x.area.Top, (int)x.area.Bottom,
    		((int)x.tag) & 0x00FF, x.Hint.Focus ? 'Y': 'N', x.Hint.Hover ? 'Y': 'N', x.Hint.Object);
        if(WXCustomDrawChar::options->Use3D)
	        DrawButtonDecorationsAsNew(cx_start, cx_end, cy, clr_text, clr_back, x);
		//else
		//	DrawButtonDecorations(cx_start, cx_end, cy, clr_text, clr_back, x);
    	break;
    case HintCheckbox:
    case HintRadioButton:
        DrawCheckboxDecorations(cx_start, cx_end, cy, clr_text, clr_back, x);
    	break;
    case HintListBox:
    	break;
    case HintLine:
    case HintBox:
    	break;
    case HintTitle:
    case HintImage:
    case HintScrollBar:
    case HintUserControl:
    case HintText:
    case HintVerticalText:
    default:
    	break;
	}
}

static wxColour colorTable[] = {
	wxColour(0, 0, 0, 255), // HintNone = 0,
	wxColour(0, 0, 0, 40), // HintConsoleBuffer = 1,
	wxColour(0, 255, 80, 100), // HintDialog,
	wxColour(0, 0, 0, 40), // HintMenu,
	wxColour(0, 0, 0, 40), // HintHMenu,
	wxColour(0, 0, 0, 40), // HintEditor,
	wxColour(0, 0, 0, 40), // HintViewer,
	wxColour(16, 80, 192, 80), // HintPanel,
	wxColour(0, 0, 0, 40), // HintTree,
	wxColour(0, 0, 0, 40), // HintQuickView,
	wxColour(0, 0, 0, 40), // HintScreenSaver,
	wxColour(0, 0, 0, 40), // HintInfoList,
	wxColour(0, 0, 0, 40), // HintHelpViewer,
	wxColour(0, 0, 0, 40), // HintCommandLine,
	wxColour(255, 0, 0, 40), // HintPanic
};

#ifdef TAG_DEBUG
void ConsolePainter::DrawHatch(const std::vector<HintHatch>& hatched) {
    wxGraphicsContext* dc = wxGraphicsContext::Create(_dc);
    if (!dc) return;

	auto oldPen   = _dc.GetPen();
	auto oldBrush = _dc.GetBrush();
	auto oldText  = _dc.GetTextForeground();

	_dc.SetPen(*wxTRANSPARENT_PEN);

	for(size_t i = 0; i < hatched.size(); ++i) {
		const HintHatch& x = hatched[i];

		wxCoord Y1 = x.cy * _context->FontHeight();
		wxCoord Y2 = Y1 + _context->FontHeight() - 1;

		wxCoord X1 = x.cx * _context->FontWidth();
		wxCoord X2 = X1 + _context->FontWidth() - 1;

		wxColour hatchColor = colorTable[x.Container];  // 80 = semi-transparent

		wxBrush hatchBrush(hatchColor /*, wxBRUSHSTYLE_CROSS_HATCH*/);
		dc->SetBrush(hatchBrush);

		// wxColour hatchPen = wxColor(hatchColor.Red(), hatchColor.Green(), hatchColor.Blue(), 255);
		// dc->SetPen(hatchPen);

		dc->DrawRectangle(X1, Y1, X2, Y2);
	}

	delete dc;

	_dc.SetPen(oldPen);
	_dc.SetBrush(oldBrush);
	_dc.SetTextForeground(oldText);
}
#endif

void ConsolePainter::DrawCheckboxDecorations(
	int cx_start, unsigned int cx_end, unsigned int cy, 
	const WinPortRGB& c_text, const WinPortRGB& c_back, 
	const HintPos& pos)
{
	if (pos.Hint.Focus) {
		//wxCoord Y1 = cy * _context->FontHeight();
		wxCoord Y2 = cy * _context->FontHeight() + _context->FontHeight() - 1;
		wxCoord X1 = (cx_start + 3) * _context->FontWidth();
		wxCoord X2 = (cx_end + 1) * _context->FontWidth()  - 1;
		wxCoord W = X2 - X1 + 1;
		//wxCoord H = Y2 - Y1 + 1;

		WinPortRGB c_a_text, c_a_back;
		ComputeAccents(c_text, c_back, c_a_text, c_a_back);

		WinPortRGB emboss = GetSoftenColorIf(c_text);

		//DrawHorizontalGradientLine(X1, Y2, W, c_back, c_a_back, 1);
		DrawHorizontalDashedGradientLine(X1 + 1, Y2, W, c_text, emboss, 6, 2, 2);
		//DrawHorizontalDashedGradientLine(X1 + 1, Y1, W, c_text, emboss, 6, 2, 2);
		//DrawVerticalDashedGradientLine(X1 + 1, Y1, H, c_text, emboss, 6, 2, 2);
		//DrawVerticalDashedGradientLine(X2 - 2, Y2, H, c_text, emboss, 6, 2, 2);
	}
}

void ConsolePainter::DrawButtonDecorations(
	int cx_start, unsigned int cx_end, unsigned int cy, 
	const WinPortRGB& c_text, const WinPortRGB& c_back,
	const HintPos& pos) 
{
	wxCoord Y1 = cy * _context->FontHeight(), Y2 = cy * _context->FontHeight() + _context->FontHeight() - 1;
	wxCoord X1 = cx_start * _context->FontWidth(), X2 = cx_end * _context->FontWidth() + _context->FontWidth() - 1;
	wxCoord W = X2 - X1 + 1, H = Y2 - Y1 + 1;

	WinPortRGB c_a_text, c_a_back;
	ComputeAccents(c_text, c_back, c_a_text, c_a_back);

	// DrawHorizontalGradientLine(X1, Y1, W, c_a_back, c_back, c_a_back, 0.5);
	DrawHorizontalGradientLine(X1, Y2, W, c_a_back, c_back, 1);
	DrawVerticalGradientLine(X1, Y1, H, c_a_back, c_back, c_a_back, 1);
	DrawVerticalGradientLine(X2, Y1, H, c_a_back, c_back, 1);

	WinPortRGB emboss = GetSoftenColorIf(c_text);

	DrawHorizontalGradientLine(X1 + 1, Y1, W - 2, c_a_text, emboss, c_a_text, 1);
	DrawHorizontalGradientLine(X1 + 1, Y2 - 1, W - 2, c_a_text, emboss, 1);
	DrawVerticalGradientLine(X1 + 1, Y1 + 1, H - 2, c_a_text, emboss, c_a_text, 1);
	DrawVerticalGradientLine(X2 - 1, Y1 + 1, H - 2, c_a_text, emboss, 1);

	if (pos.Hint.Focus)
		DrawHorizontalGradientLine(X1 + 1, Y2 - 1, W - 2, c_a_text, emboss, 2);
	else
		DrawHorizontalGradientLine(X1 + 1, Y2 - 1, W - 2, c_a_text, emboss, 1);
}

void DrawTextBaseline(wxDC& dc, const wchar_t* text, int x, int baselineY)
{
    int w, h, descent;
    dc.GetTextExtent(text, &w, &h, &descent);
    int y = baselineY - (h - descent);
    dc.DrawText(text, x, y);
}

void ConsolePainter::DrawButtonDecorationsAsNew(
	int cx_start, unsigned int cx_end, unsigned int cy, 
	const WinPortRGB& c_text, const WinPortRGB& c_back,
	const HintPos& pos) 
{
	wxCoord Y1 = cy * _context->FontHeight(), Y2 = (cy + 1) * _context->FontHeight() - 1;
	wxCoord X1 = cx_start * _context->FontWidth(), X2 = cx_end * _context->FontWidth() + _context->FontWidth() - 1;
	wxCoord W = X2 - X1 + 1, H = Y2 - Y1 + 1;

	DrawLiquidButtonBackground(X1, Y1, W, H, c_back);

	WinPortRGB c_a_text, c_a_back;
	ComputeAccents(c_text, c_back, c_a_text, c_a_back);

	// DrawHorizontalGradientLine(X1, Y1, W, c_a_back, c_back, c_a_back, 0.5);
	DrawHorizontalGradientLine(X1, Y2, W, c_a_back, c_back, 1);
	DrawVerticalGradientLine(X1, Y1, H, c_a_back, c_back, c_a_back, 1);
	DrawVerticalGradientLine(X2, Y1, H, c_a_back, c_back, 1);

	WinPortRGB emboss = GetSoftenColorIf(c_text);

	DrawHorizontalGradientLine(X1 + 1, Y1, W - 2, c_a_text, emboss, c_a_text, 1);
	DrawHorizontalGradientLine(X1 + 1, Y2 - 1, W - 2, c_a_text, emboss, 1);
	DrawVerticalGradientLine(X1 + 1, Y1 + 1, H - 2, c_a_text, emboss, c_a_text, 1);
	DrawVerticalGradientLine(X2 - 1, Y1 + 1, H - 2, c_a_text, emboss, 1);

	if (pos.Hint.Focus)
		DrawHorizontalGradientLine(X1 + 1, Y2 - 1, W - 2, c_a_text, emboss, 2);
	else
		DrawHorizontalGradientLine(X1 + 1, Y2 - 1, W - 2, c_a_text, emboss, 1);

	const bool underlined = (pos.attributes & COMMON_LVB_UNDERSCORE) != 0;
	const bool strikeout = (pos.attributes & COMMON_LVB_STRIKEOUT) != 0;

	// todo: highlight character to be displayed
	_dc.SetTextForeground(wxColour(c_text.r, c_text.g, c_text.b));
	int ascent = _context->FontHeight() - _context->FontDescent();
	DrawTextBaseline(_dc, pos.text.c_str(), X1, Y1 + ascent);

	if (!underlined && !strikeout) 	return;

	_dc.SetPen(wxColour(c_text.r, c_text.g, c_text.b));
	if (underlined) _dc.DrawLine(X1, Y2 - 3, X2, Y2 - 3);
	if (strikeout)  _dc.DrawLine(X1, Y1 + H / 2, X2, Y1 + H / 2);
	_dc.SetPen(_context->GetTransparentPen());
}

// Draws the scrollbar track (background)
void DrawScrollTrack(wxDC& dc, const wxRect& rect,  const wxColour& colLight, const wxColour& colDark)
{
    // Subtle vertical gradient
    dc.GradientFillLinear(rect, colLight, colDark, wxSOUTH);

    // Optional: inner shadow at top
    dc.SetPen(wxPen(wxColour(0,0,0,40), 1));
    dc.DrawLine(rect.x, rect.y, rect.x + rect.width, rect.y);

    // Optional: highlight at bottom
    dc.SetPen(wxPen(wxColour(255,255,255,40), 1));
    dc.DrawLine(rect.x, rect.y + rect.height - 1,
                rect.x + rect.width, rect.y + rect.height - 1);
}


// Draws the scrollbar thumb (liquid-metal style)
void DrawScrollThumb(wxDC& dc, const wxRect& rect, 
	const wxColour& colTop, const wxColour& colBottom, bool pressed = false, bool hover = false)
{
    wxRect r = rect;

    // Slight inset for nicer look
    r.Deflate(1);

    // --- State adjustments ---
    wxColour top = colTop;
    wxColour bottom = colBottom;

    if (hover)
    {
        // Slight brightness boost
        top = wxColour(
            std::min(top.Red() + 20, 255),
            std::min(top.Green() + 20, 255),
            std::min(top.Blue() + 20, 255)
        );
    }

    if (pressed)
    {
        // Darken both ends
        top = wxColour(
            std::max(top.Red() - 30, 0),
            std::max(top.Green() - 30, 0),
            std::max(top.Blue() - 30, 0)
        );
        bottom = wxColour(
            std::max(bottom.Red() - 30, 0),
            std::max(bottom.Green() - 30, 0),
            std::max(bottom.Blue() - 30, 0)
        );
    }

    // --- Draw 3-layer gradient for liquid-metal effect ---

    int h = r.height;

    // Top highlight band
    wxRect rTop(r.x, r.y, r.width, h * 0.35);
    wxColour top1 = wxColour(
        std::min(top.Red() + 40, 255),
        std::min(top.Green() + 40, 255),
        std::min(top.Blue() + 40, 255)
    );
    dc.GradientFillLinear(rTop, top1, top, wxSOUTH);

    // Middle body
    wxRect rMid(r.x, r.y + h * 0.35, r.width, h * 0.45);
    dc.GradientFillLinear(rMid, top, bottom, wxSOUTH);

    // Bottom shadow
    wxRect rBot(r.x, r.y + h * 0.80, r.width, h * 0.20);
    wxColour bot2 = wxColour(
        std::max(bottom.Red() - 30, 0),
        std::max(bottom.Green() - 30, 0),
        std::max(bottom.Blue() - 30, 0)
    );
    dc.GradientFillLinear(rBot, bottom, bot2, wxSOUTH);

    // Border
    dc.SetPen(wxPen(wxColour(0,0,0,80), 1));
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    dc.DrawRoundedRectangle(r, 3);
}

/*
Lab base = RGBtoLAB(120, 120, 120);

Lab topLab = base;    topLab.L += 12;
Lab botLab = base;    botLab.L -= 10;

wxColour colTop = LABtoRGB(topLab);
wxColour colBottom = LABtoRGB(botLab);

DrawScrollTrack(dc, trackRect,
                wxColour(240,240,240),
                wxColour(220,220,220));
DrawScrollThumb(dc, thumbRect,
                colTop, colBottom,
                isPressed, isHovered);
*/