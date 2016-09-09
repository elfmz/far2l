#include "stdafx.h"
#include "ConsoleOutput.h"
#include "ConsoleInput.h"
#include "wxWinTranslations.h"
#include <wx/fontdlg.h>
#include <wx/fontenum.h>
#include <wx/textfile.h>
#include "Paint.h"
#include "Utils.h"


#define ALL_ATTRIBUTES ( FOREGROUND_INTENSITY | BACKGROUND_INTENSITY | \
					FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE |  \
					BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE )

#define DYNAMIC_FONTS

extern ConsoleOutput g_wx_con_out;

static unsigned int DivCeil(unsigned int v, unsigned int d)
{
	unsigned int r = v / d;
	return (r * d == v) ? r : r + 1;
}


/////////////////////////////////////////////////////////////////////////////////

class FixedFontLookup : wxFontEnumerator
{
	wxString _result;
	virtual bool OnFacename(const wxString &font)
	{
		_result = font;
		return false;
	}
public:

	wxString Query()
	{
		_result.clear();
		EnumerateFacenames(wxFONTENCODING_SYSTEM, true);
		fprintf(stderr, "FixedFontLookup: %ls\n", _result.wc_str());
		return _result;
	}	
};

static void InitializeFont(wxWindow *parent, wxFont& font)
{
	const std::string &path = SettingsPath("font");
	wxTextFile file(path);
	if (file.Exists() && file.Open()) {
		for (wxString str = file.GetFirstLine(); !file.Eof(); str = file.GetNextLine()) {
			font.SetNativeFontInfo(str);
			if (font.IsOk()) {
				printf("InitializeFont: used %ls\n", str.wc_str());
				return;				
			}
		}
	} else 
		file.Create(path);
	
	for (;;) {
		FixedFontLookup ffl;
		wxString fixed_font = ffl.Query();
		if (!fixed_font.empty()) {
			font = wxFont(16, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, fixed_font);
		}
		if (fixed_font.empty() || !font.IsOk())
			font = wxFont(wxSystemSettings::GetFont(wxSYS_ANSI_FIXED_FONT));
		font = wxGetFontFromUser(parent, font);
		if (font.IsOk()) {
			file.InsertLine(font.GetNativeFontInfoDesc(), 0);
			file.Write();
			return;
		}
	}
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
		_max_width(8), _prev_width(-1), 
		_max_height(8), _prev_height(-1), 
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


ConsolePaintContext::ConsolePaintContext(wxWindow *window) :
	_window(window), _font_width(12), _font_height(16), 
	_buffered_paint(false), _cursor_state(false)
{
	_char_fit_cache.checked.resize(0xffff);
	_char_fit_cache.result.resize(0xffff);
	
	_window->SetBackgroundColour(*wxBLACK);
	wxFont font;
	InitializeFont(_window, font);
	FontSizeInspector fsi(font);
	fsi.InspectChars(L" 1234567890-=!@#$%^&*()_+qwertyuiop[]asdfghjkl;'zxcvbnm,./QWERTYUIOP{}ASDFGHJKL:\"ZXCVBNM<>?");
	//fsi.InspectChars(L"QWERTYUIOPASDFGHJKL");
	
	bool is_unstable = fsi.IsUnstableSize();
	_font_width = fsi.GetMaxWidth();
	_font_height = fsi.GetMaxHeight();
	//font_height+= _font_height/4;

	
	fprintf(stderr, "Font %u x %u: '%ls' - %s\n", _font_width, _font_height, font.GetFaceName().wc_str(), 
		font.IsFixedWidth() ? ( is_unstable ? "monospaced unstable" : "monospaced stable" ) : "not monospaced");
		
	if (font.IsFixedWidth() && !is_unstable) {
		struct stat s = {0};
		if (stat(SettingsPath("nobuffering").c_str(), &s)!=0)
			_buffered_paint = true;
	}
	_fonts.push_back(font);
}
	
uint8_t ConsolePaintContext::CharFitTest(wxPaintDC &dc, wchar_t c)
{
	const bool cacheable = ((size_t)c <= _char_fit_cache.checked.size());
	if (cacheable && _char_fit_cache.checked[ (size_t)c  - 1 ]) {
		return _char_fit_cache.result[ (size_t)c  - 1 ];
	}

	uint32_t font_index;
	const wchar_t wz[2] = { c, 0};
	wxSize char_size = dc.GetTextExtent(wz);
	if ((unsigned)char_size.GetWidth()==_font_width 
		&& (unsigned)char_size.GetHeight()==_font_height) {
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
				_fonts.emplace_back(smallest.MakeSmaller());
			}
			dc.SetFont(_fonts[try_index]);
			char_size = dc.GetTextExtent(wz);
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
	unsigned int cw, ch; g_wx_con_out.GetSize(cw, ch);
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

	ConsolePainter painter(this, dc);
	for (unsigned int cy = (unsigned)area.Top; cy <= (unsigned)area.Bottom; ++cy) {
		COORD data_size = {(SHORT)cw, 1};
		COORD data_pos = {0, 0};
		SMALL_RECT screen_rect = {area.Left, (SHORT)cy, area.Right, (SHORT)cy};

		if (rgn.Contains(0, cy * _font_height, cw * _font_width, _font_height)==wxOutRegion) {
			continue;
		}
		g_wx_con_out.Read(&_line[area.Left], data_size, data_pos, screen_rect);

		painter.LineBegin(cy);
		for (unsigned int cx = (unsigned)area.Left; cx <= (unsigned)area.Right; ++cx) {
			if (rgn.Contains(cx * _font_width, cy * _font_height, _font_width, _font_height)==wxOutRegion) {
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
	const COORD &pos = g_wx_con_out.GetCursor();
	SMALL_RECT area = {pos.X, pos.Y, pos.X, pos.Y};
	RefreshArea( area );
}


/////////////////////////////

CursorProps::CursorProps(bool state) : visible(false), height(1)
{
	if (state) {
		const COORD pos = g_wx_con_out.GetCursor(height, visible);
		x = (unsigned int) pos.X;
		y = (unsigned int) pos.Y;
	}
}

//////////////////////

ConsolePainter::ConsolePainter(ConsolePaintContext *context, wxPaintDC &dc) : 
	_context(context), _dc(dc), _cursor_props(context->GetCursorState()),
	 _start_cx((unsigned int)-1), _start_back_cx((unsigned int)-1)
{
	wxPen *trans_pen = wxThePenList->FindOrCreatePen(wxColour(0, 0, 0), 1, wxPENSTYLE_TRANSPARENT);
	_dc.SetPen(*trans_pen);
	_dc.SetBackgroundMode(wxPENSTYLE_TRANSPARENT);
}
	
	
void ConsolePainter::SetBackgroundColor(wxColour clr)
{
	if (_brush.Change(clr)) {
		wxBrush* brush = wxTheBrushList->FindOrCreateBrush(clr);
		_dc.SetBrush(*brush);
		_dc.SetBackground(*brush);
	}		
}
	
void ConsolePainter::PrepareBackground(unsigned int cx, wxColour clr)
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
	SetBackgroundColor(wxColour(clr.Red()^0xff, clr.Green()^0xff, clr.Blue()^0xff));
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
		_dc.SetTextForeground(_clr_text);
		_dc.DrawText(_buffer, _start_cx * _context->FontWidth(), _start_y);
		_buffer.clear();
	}
	_start_cx = (unsigned int)-1;
}


void ConsolePainter::NextChar(unsigned int cx, unsigned short attributes, wchar_t c)
{
	if (!c || c == L' ') {
		if (!_buffer.empty()) 
			FlushBackground(cx);
		FlushText();
	}

	PrepareBackground(cx, ConsoleBackground2wxColor(attributes));

	if (!c || c == L' ') 
		return;

	wxColour clr_text = ConsoleForeground2wxColor(attributes);
	
	uint8_t fit_font_index = _context->CharFitTest(_dc, c);
	
	if (fit_font_index==0 && _context->IsPaintBuffered() && 
		_start_cx != (unsigned int) -1 && _clr_text == clr_text) {
		_buffer+= c;
		return;
	}

	FlushBackground(cx + 1);
	FlushText();
	_start_cx = cx;
	_buffer = c;
	_clr_text = clr_text;
	if (fit_font_index!=0 && fit_font_index!=0xff) {
		_context->ApplyFont(_dc, fit_font_index);
		FlushText();
		_context->ApplyFont(_dc);
	}
}
