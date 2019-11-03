#include "ConsoleOutput.h"
#include "ConsoleInput.h"
#include "wxWinTranslations.h"
#include <wx/fontdlg.h>
#include <wx/fontenum.h>
#include <wx/textfile.h>
#include <wx/graphics.h>
#include "Paint.h"
#include "PathHelpers.h"
#include "utils.h"


#define ALL_ATTRIBUTES ( FOREGROUND_INTENSITY | BACKGROUND_INTENSITY | \
					FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE |  \
					BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE )

#define DYNAMIC_FONTS

#ifdef __APPLE__
# define DEFAULT_FONT_SIZE	20
#else
# define DEFAULT_FONT_SIZE	16
#endif

extern ConsoleOutput g_winport_con_out;

static unsigned int DivCeil(unsigned int v, unsigned int d)
{
	unsigned int r = v / d;
	return (r * d == v) ? r : r + 1;
}



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
		
		/* unfortunatelly following code gives nothing interesting
		wxFont f(wxFontInfo(DEFAULT_FONT_SIZE).Underlined().FaceName(face_name));
		if (f.IsOk()) {
			fprintf(stderr, "FONT family %u encoding %u face_name='%ls' \n", 
				(unsigned int)f.GetFamily(), (unsigned int)f.GetEncoding(), face_name.wc_str());
		} else {
			fprintf(stderr, "BAD FONT: face_name='%ls'\n", face_name.wc_str());
		} */
		return true;
	}
public:

	wxString Query()
	{
		_any.Empty();
		_known_good.Empty();
		EnumerateFacenames(wxFONTENCODING_SYSTEM, true);
		fprintf(stderr, "FixedFontLookup: _any='%ls' _known_good='%ls'\n", static_cast<const wchar_t*>(_any.wc_str()), static_cast<const wchar_t*>(_known_good.wc_str()));
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
				printf("LoadFontFromSettings: used %ls\n", static_cast<const wchar_t*>(str.wc_str()));
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
	_window(window), _font_width(12), _font_height(16), _font_thickness(2),
	_buffered_paint(false), _cursor_state(false), _sharp(false)
{
	_char_fit_cache.checked.resize(0xffff);
	_char_fit_cache.result.resize(0xffff);

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
	bool _unstable_size;
	
	void InspectChar(const wchar_t c)
	{
		wchar_t wz[2] = { c, 0};
		wxSize char_size = _dc.GetTextExtent(wz);
		const int width = char_size.GetWidth();
		const int height = char_size.GetHeight();
		
		if (_max_width < width) _max_width = width;
		if (_max_height < height) _max_height = height;
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
	
	public:
	FontSizeInspector(wxFont& font) 
		: _bitmap(48, 48,  wxBITMAP_SCREEN_DEPTH),
		_max_width(4), _prev_width(-1), 
		_max_height(6), _prev_height(-1), 
		_unstable_size(false)
	{
		_dc.SelectObject(_bitmap);
		_dc.SetFont(font);
	}

	void InspectChars(const wchar_t *s)
	{
		for(; *s; ++s)
			InspectChar(*s);
	}
	
	bool IsUnstableSize() const { return _unstable_size; }
	int GetMaxWidth() const { return _max_width; }
	int GetMaxHeight() const { return _max_height; }
};



void ConsolePaintContext::SetFont(wxFont font)
{
	FontSizeInspector fsi(font);
	fsi.InspectChars(L" 1234567890-=!@#$%^&*()_+qwertyuiop[]asdfghjkl;'zxcvbnm,./QWERTYUIOP{}ASDFGHJKL:\"ZXCVBNM<>?");
	//fsi.InspectChars(L"QWERTYUIOPASDFGHJKL");
	
	bool is_unstable = fsi.IsUnstableSize();
	_font_width = fsi.GetMaxWidth();
	_font_height = fsi.GetMaxHeight();
	//font_height+= _font_height/4;

	_font_thickness = (_font_width > 5) ? _font_width / 5 : 1;
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
	
	fprintf(stderr, "Font %u x %u . %u: '%ls' - %s\n", _font_width, _font_height, _font_thickness, static_cast<const wchar_t*>(font.GetFaceName().wc_str()), 
		font.IsFixedWidth() ? ( is_unstable ? "monospaced unstable" : "monospaced stable" ) : "not monospaced");
		
	if (font.IsFixedWidth() && !is_unstable) {
		struct stat s{};
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
	
uint8_t ConsolePaintContext::CharFitTest(wxPaintDC &dc, wchar_t c)
{
	const bool cacheable = ((size_t)c <= _char_fit_cache.checked.size());
	if (cacheable && _char_fit_cache.checked[ (size_t)c  - 1 ]) {
		return _char_fit_cache.result[ (size_t)c  - 1 ];
	}

	uint8_t font_index;
	_cft_tmp = wxUniChar(c);
	wxSize char_size = dc.GetTextExtent(_cft_tmp);
	if ((unsigned)char_size.GetWidth() == _font_width 
		&& (unsigned)char_size.GetHeight() == _font_height) {
		font_index = 0;
	} else {
		font_index = 0xff;
#ifdef DYNAMIC_FONTS
		for (uint8_t try_index = 1;;++try_index) {
			if (try_index==0xff || (
				(unsigned)char_size.GetWidth() <= _font_width && 
				(unsigned)char_size.GetHeight() <= _font_height)) 
				{
					if (font_index!=0xff) ApplyFont(dc);
					break;
				}

			while (_fonts.size() <= try_index) {
				wxFont smallest = _fonts.back();
				smallest.MakeSmaller();
				smallest.MakeBold();
				_fonts.emplace_back(smallest);
			}
			dc.SetFont(_fonts[try_index]);
			char_size = dc.GetTextExtent(_cft_tmp);
			font_index = try_index;
		}
#endif		
	}
	
	if (cacheable) {
		_char_fit_cache.result[ (size_t)c  - 1 ] = font_index;
		_char_fit_cache.checked[ (size_t)c  - 1 ] = true;
	}
	
	return font_index;
}

void ConsolePaintContext::ApplyFont(wxPaintDC &dc, uint8_t index)
{
	if (index < _fonts.size())
		dc.SetFont(_fonts[index]);
}

void ConsolePaintContext::OnPaint(SMALL_RECT *qedit)
{
	wxPaintDC dc(_window);
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
	unsigned int cw, ch; g_winport_con_out.GetSize(cw, ch);
	if (cw > MAXSHORT) cw = MAXSHORT;
	if (ch > MAXSHORT) ch = MAXSHORT;

	wxRegion rgn = _window->GetUpdateRegion();
	wxRect box = rgn.GetBox();
	SMALL_RECT area = {(SHORT) (box.GetLeft() / _font_width), (SHORT) (box.GetTop() / _font_height),
		(SHORT)DivCeil(box.GetRight(), _font_width), (SHORT)DivCeil(box.GetBottom(), _font_height)};

	if (area.Left < 0 ) area.Left = 0;
	if (area.Top < 0 ) area.Top = 0;
	if ((unsigned)area.Right >= cw) area.Right = cw - 1;
	if ((unsigned)area.Bottom >= ch) area.Bottom = ch - 1;
	if (area.Right < area.Left || area.Bottom < area.Top) return;

	wxString tmp;
	_line.resize(cw);
	ApplyFont(dc);

	ConsolePainter painter(this, dc, _buffer);
	for (unsigned int cy = (unsigned)area.Top; cy <= (unsigned)area.Bottom; ++cy) {
		COORD data_size = {(SHORT)cw, 1};
		COORD data_pos = {0, 0};
		SMALL_RECT screen_rect = {area.Left, (SHORT)cy, area.Right, (SHORT)cy};

		wxRegionContain lc = rgn.Contains(0, cy * _font_height, cw * _font_width, _font_height);

		if (lc == wxOutRegion) {
			continue;
		}
		g_winport_con_out.Read(&_line[area.Left], data_size, data_pos, screen_rect);
//		if (lc == wxPartRegion) abort();
		painter.LineBegin(cy);
		for (unsigned int cx = (unsigned)area.Left; cx <= (unsigned)area.Right; ++cx) {
			if (lc == wxPartRegion &&
			  rgn.Contains(cx * _font_width, cy * _font_height, _font_width, _font_height) == wxOutRegion) {
				painter.LineFlush(cx);
				continue;
			}

			unsigned short attributes = _line[cx].Attributes;			
			
			if (qedit && cx >= (unsigned)qedit->Left && cx <= (unsigned)qedit->Right 
				&& cy >= (unsigned)qedit->Top && cy <= (unsigned)qedit->Bottom) {
				attributes^= ALL_ATTRIBUTES;				
			}
						
			painter.NextChar(cx, attributes, _line[cx].Char.UnicodeChar);
		}
		painter.LineFlush(area.Right + 1);
	}		
}


void ConsolePaintContext::RefreshArea( const SMALL_RECT &area )
{
	wxRect rc;
	rc.SetLeft(((int)area.Left) * _font_width);
	rc.SetRight(((int)area.Right + 1) * _font_width);
	rc.SetTop(((int)area.Top) * _font_height);
	rc.SetBottom(((int)area.Bottom + 1) * _font_height);
	_window->Refresh(false, &rc);
}


void ConsolePaintContext::ToggleCursor() 
{
	_cursor_state = !_cursor_state; 
	const COORD &pos = g_winport_con_out.GetCursor();
	SMALL_RECT area = {pos.X, pos.Y, pos.X, pos.Y};
	RefreshArea( area );
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

/////////////////////////////

CursorProps::CursorProps(bool state) : visible(false), height(1)
{
	if (state) {
		const COORD pos = g_winport_con_out.GetCursor(height, visible);
		x = (unsigned int) pos.X;
		y = (unsigned int) pos.Y;
	}
}

//////////////////////

ConsolePainter::ConsolePainter(ConsolePaintContext *context, wxPaintDC &dc, wxString &buffer) : 
	_context(context), _dc(dc), _buffer(buffer), _cursor_props(context->GetCursorState()),
	 _start_cx((unsigned int)-1), _start_back_cx((unsigned int)-1), _prev_fit_font_index(0),
	_trans_pen(wxThePenList->FindOrCreatePen(wxColour(0, 0, 0), 1, wxPENSTYLE_TRANSPARENT))
{

	_dc.SetPen(*_trans_pen);
	_dc.SetBackgroundMode(wxPENSTYLE_TRANSPARENT);
	_buffer.Empty();
}
	
	
void ConsolePainter::SetBackgroundColor(const WinPortRGB &clr)
{
	if (_brush_clr.Change(clr)) {
		wxBrush* brush = wxTheBrushList->FindOrCreateBrush(wxColour(clr.r, clr.g, clr.b));
		_dc.SetBrush(*brush);
		_dc.SetBackground(*brush);
	}		
}
	
void ConsolePainter::PrepareBackground(unsigned int cx, const WinPortRGB &clr)
{
	const bool cursor_here = (_cursor_props.visible && cx==_cursor_props.x && _start_cy==_cursor_props.y) ;

	if (!cursor_here && _start_back_cx != (unsigned int)-1 && _clr_back == clr)
		return;

	FlushBackground(cx);

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
	WinPortRGB clr_xored(clr.r ^ 0xff, clr.g ^ 0xff, clr.b ^ 0xff);
	SetBackgroundColor(clr_xored);
	_dc.DrawRectangle(x, _start_y + fill_height, _context->FontWidth(), h);				

	if (fill_height) {
		SetBackgroundColor(clr);
		_dc.DrawRectangle(x, _start_y, _context->FontWidth(), fill_height);							
	}
}


void ConsolePainter::FlushBackground(unsigned int cx)
{
	if (_start_back_cx!= ((unsigned int)-1)) {
		SetBackgroundColor(_clr_back);
		_dc.DrawRectangle(_start_back_cx * _context->FontWidth(), _start_y, 
			(cx - _start_back_cx) * _context->FontWidth(), _context->FontHeight());			
		_start_back_cx = ((unsigned int)-1);
	}		
}

void ConsolePainter::FlushText()
{
	if (!_buffer.empty()) {
		_dc.SetTextForeground(wxColour(_clr_text.r, _clr_text.g, _clr_text.b));
		_dc.DrawText(_buffer, _start_cx * _context->FontWidth(), _start_y);
		_buffer.Empty();
	}
	_start_cx = (unsigned int)-1;
	_prev_fit_font_index = 0;
}


// U+0000..U+D7FF, U+E000..U+10FFFF

#define IS_VALID_WCHAR(c)    ( (((unsigned int)c) <= 0xd7ff) || (((unsigned int)c) >=0xe000 && ((unsigned int)c) <= 0x10ffff ) )

#define IS_CUSTOMDRAW_WCHAR(c) ((c) == 0x2500 || (c) == 0x2502 || (c) == 0x250c || (c) == 0x2510 || (c) == 0x2514 || (c) == 0x2518 \
			|| (c) == 0x251c || (c) == 0x2524 || (c) == 0x252c || (c) == 0x2534 || (c) == 0x253C || (c) == 0x2550 || (c) == 0x2551 \
			|| (c) == 0x2554 || (c) == 0x2557 || (c) == 0x255a || (c) == 0x255d || (c) == 0x255f || (c) == 0x2562)


void ConsolePainter::DrawLineH(wxCoord thickness, wxCoord y, wxCoord left, wxCoord right)
{
	_dc.DrawRectangle(left, y - thickness / 2, right + 1 - left , thickness);
}

void ConsolePainter::DrawLineV(wxCoord thickness, wxCoord x, wxCoord top, wxCoord bottom)
{
	_dc.DrawRectangle(x - thickness / 2, top, thickness, bottom + 1 - top);
}

void ConsolePainter::CustomDrawChar(unsigned int cx, wchar_t c, const WinPortRGB &clr_text)
{
	const wxCoord fw = _context->FontWidth(), fh = _context->FontHeight();
	const wxCoord thickness = _context->FontThickness();

	SetBackgroundColor(clr_text);

	const wxCoord left = cx * fw;
	const wxCoord right = left + fw - 1;
	const wxCoord top = _start_y;
	const wxCoord bottom = top + fh - 1;

	const wxCoord middle_y = top + fh / 2;
	const wxCoord middle_x = left + fw / 2;

	const wxCoord middle1_y = middle_y - thickness;
	const wxCoord middle2_y = middle_y + thickness;

	const wxCoord middle1_x = middle_x - thickness;
	const wxCoord middle2_x = middle_x + thickness;

	switch (c) {
		case 0x2500: /* ─ */
			DrawLineH(thickness, middle_y, left, right);
			break;

		case 0x2502: /* │ */
			DrawLineV(thickness, middle_x, top, bottom);
			break;

		case 0x250C: /* ┌ */
			DrawLineH(thickness, middle_y, middle_x - thickness / 2, right);
			DrawLineV(thickness, middle_x, middle_y, bottom);
			break;

		case 0x2510: /* ┐ */
			DrawLineH(thickness, middle_y, left, middle_x);
			DrawLineV(thickness, middle_x, middle_y, bottom);
			break;

		case 0x2514: /* └ */
			DrawLineH(thickness, middle_y, middle_x - thickness / 2, right);
			DrawLineV(thickness, middle_x, top, middle_y);
			break;

		case 0x2518: /* ┘ */
			DrawLineH(thickness, middle_y, left, middle_x);
			DrawLineV(thickness, middle_x, top, middle_y);
			break;

		case 0x251C: /* ├ */
			DrawLineH(thickness, middle_y, middle_x, right);
			DrawLineV(thickness, middle_x, top, bottom);
			break;

		case 0x2524: /* ┤ */
			DrawLineH(thickness, middle_y, left, middle_x);
			DrawLineV(thickness, middle_x, top, bottom);
			break;

		case 0x252C: /* ┬ */
			DrawLineH(thickness, middle_y, left, right);
			DrawLineV(thickness, middle_x, middle_y, bottom);
			break;

		case 0x2534: /* ┴ */
			DrawLineH(thickness, middle_y, left, right);
			DrawLineV(thickness, middle_x, top, middle_y);
			break;

		case 0x253C: /* ┼  */
			DrawLineH(thickness, middle_y, left, right);
			DrawLineV(thickness, middle_x, top, bottom);
			break;

		case 0x2550: /* ═ */
			DrawLineH(thickness, middle1_y, left, right);
			DrawLineH(thickness, middle2_y, left, right);
			break;

		case 0x2551: /* ║ */
			DrawLineV(thickness, middle1_x, top, bottom);
			DrawLineV(thickness, middle2_x, top, bottom);
			break;

		case 0x2554: /* ╔  */
			DrawLineH(thickness, middle1_y, middle1_x - thickness / 2, right);
			DrawLineV(thickness, middle1_x, middle1_y, bottom);
			DrawLineH(thickness, middle2_y, middle2_x - thickness / 2, right);
			DrawLineV(thickness, middle2_x, middle2_y, bottom);
			break;

		case 0x2557: /* ╗  */
			DrawLineH(thickness, middle1_y, left, middle2_x);
			DrawLineV(thickness, middle2_x, middle1_y, bottom);
			DrawLineH(thickness, middle2_y, left, middle1_x);
			DrawLineV(thickness, middle1_x, middle2_y, bottom);
			break;

		case 0x255A: /* ╚  */
			DrawLineH(thickness, middle2_y, middle1_x - thickness / 2, right);
			DrawLineV(thickness, middle1_x, top, middle2_y);
			DrawLineH(thickness, middle1_y, middle2_x - thickness / 2, right);
			DrawLineV(thickness, middle2_x, top, middle1_y);
			break;

		case 0x255D: /* ╝  */
			DrawLineH(thickness, middle1_y, left, middle1_x);
			DrawLineV(thickness, middle1_x, top, middle1_y);
			DrawLineH(thickness, middle2_y, left, middle2_x);
			DrawLineV(thickness, middle2_x, top, middle2_y);
			break;

		case 0x255F: /* ╟  */
			DrawLineV(thickness, middle1_x, top, bottom);
			DrawLineV(thickness, middle2_x, top, bottom);
			DrawLineH(thickness, middle_y, middle2_x, right);
			break;

		case 0x2562: /* ╢  */
			DrawLineV(thickness, middle1_x, top, bottom);
			DrawLineV(thickness, middle2_x, top, bottom);
			DrawLineH(thickness, middle_y, left, middle1_x);
			break;

	}
}

void ConsolePainter::NextChar(unsigned int cx, unsigned short attributes, wchar_t c)
{
	bool custom_draw = false;

	if (!c || c == L' ' || !IS_VALID_WCHAR(c)
	 || (custom_draw = IS_CUSTOMDRAW_WCHAR(c)) != false) {
		if (!_buffer.empty()) 
			FlushBackground(cx);
		FlushText();
	}

	PrepareBackground(cx, ConsoleBackground2RGB(attributes));

	if (!c || c == L' ' || !IS_VALID_WCHAR(c))
		return;

	const WinPortRGB &clr_text = ConsoleForeground2RGB(attributes);

	if (custom_draw) {
		FlushBackground(cx + 1);
		CustomDrawChar(cx, c, clr_text);
		_start_cx = (unsigned int)-1;
		_prev_fit_font_index = 0;

	} else {
		uint8_t fit_font_index = isCombinedUTF32(c) ? // workaround for 
			_prev_fit_font_index : _context->CharFitTest(_dc, c);
	
		if (fit_font_index == _prev_fit_font_index && _context->IsPaintBuffered()
			&& _start_cx != (unsigned int) -1 && _clr_text == clr_text) {
			_buffer+= c;
			return;
		}

		_prev_fit_font_index = fit_font_index;

		FlushBackground(cx + 1);
		FlushText();
		_start_cx = cx;
		_buffer = c;
		_clr_text = clr_text;

		if (fit_font_index != 0 && fit_font_index != 0xff) {
			_context->ApplyFont(_dc, fit_font_index);
			FlushText();
			_context->ApplyFont(_dc);
		}
	}
}
