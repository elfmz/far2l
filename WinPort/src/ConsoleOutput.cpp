#include "ConsoleOutput.h"

#include <wx/wx.h>
#include <wx/display.h>

#define TAB_WIDTH	8

template <class I>
	static void ApplyConsoleSizeLimits(I &w, I &h)
{
	if (w < 24) w = 24;
	if (h < 12) h = 12;
}

template <class I> 
	void ApplyCoordinateLimits(I &v, unsigned int limit)
{
	if (v <= 0) v = 0;
	else if ((unsigned int)v >= limit) v = limit - 1;
}



const char *utf8index(const char *s, size_t bytes, size_t pos)
{    
    for ( ++pos; bytes; ++s, --bytes) {
        if ((*s & 0xC0) != 0x80) --pos;
        if (pos == 0) break;
    }
    return s;
}

size_t utf8_char_len(const char *s, size_t bytes)
{
	return utf8index(s, bytes, 1) - s;
}


ConsoleOutput::ConsoleOutput() :
	_title(L"WinPort"), _listener(NULL), 
	_mode(ENABLE_PROCESSED_OUTPUT|ENABLE_WRAP_AT_EOL_OUTPUT),
	_attributes(FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED)
{
	memset(&_cursor.pos, 0, sizeof(_cursor.pos));	
	_scroll_callback.pfn = NULL;
	_cursor.height = 13;
	_cursor.visible = true;
	_scroll_region.top = 0;
	_scroll_region.bottom = MAXSHORT;
	SetSize(80, 25);
}

void ConsoleOutput::SetListener(ConsoleOutputListener *listener)
{
	_listener = listener;
}


void ConsoleOutput::SetAttributes(USHORT attributes)
{
	std::lock_guard<std::mutex> lock(_mutex);
	_attributes = attributes;
}

USHORT ConsoleOutput::GetAttributes()
{
	std::lock_guard<std::mutex> lock(_mutex);
	return _attributes;
}

void ConsoleOutput::SetCursor(COORD pos)
{
	SMALL_RECT area[2];
	{
		std::lock_guard<std::mutex> lock(_mutex);
		if (_cursor.pos.X==pos.X && _cursor.pos.Y==pos.Y)
			return;

		area[0].Left = area[0].Right = _cursor.pos.X;
		area[0].Top = area[0].Bottom = _cursor.pos.Y;
		_cursor.pos = pos;
		area[1].Left = area[1].Right = _cursor.pos.X;
		area[1].Top = area[1].Bottom = _cursor.pos.Y;
	}
	if (_listener) {
		_listener->OnConsoleOutputUpdated(area[0]);
		_listener->OnConsoleOutputUpdated(area[1]);
	}
}

void ConsoleOutput::SetCursor(UCHAR height, bool visible)
{
	SMALL_RECT area;
	{
		std::lock_guard<std::mutex> lock(_mutex);
		_cursor.height = height;
		_cursor.visible = visible;
		area.Left = area.Right = _cursor.pos.X;
		area.Top = area.Bottom = _cursor.pos.Y;
	}
	if (_listener)
		_listener->OnConsoleOutputUpdated(area);
}

COORD ConsoleOutput::GetCursor()
{
	std::lock_guard<std::mutex> lock(_mutex);
	COORD out = _cursor.pos;
	return out;
}

COORD ConsoleOutput::GetCursor(UCHAR &height, bool &visible)
{
	std::lock_guard<std::mutex> lock(_mutex);
	height = _cursor.height;
	visible = _cursor.visible;
	return _cursor.pos;
}

void ConsoleOutput::SetSize(unsigned int width, unsigned int height)
{
	//if (height==23) abort();
	ApplyConsoleSizeLimits(width, height);
	{
		std::lock_guard<std::mutex> lock(_mutex);
		_scroll_region = {0, MAXSHORT};
		_buf.SetSize(width, height, _attributes);
	}
	if (_listener)
		_listener->OnConsoleOutputResized();
}

void ConsoleOutput::GetSize(unsigned int &width, unsigned int &height)
{
	std::lock_guard<std::mutex> lock(_mutex);
	_buf.GetSize(width, height);
//	fprintf(stderr, "GetSize: %u x %u\n", width, height);
}


COORD ConsoleOutput::GetLargestConsoleWindowSize()
{
	COORD rv;
	if (!_listener) {
		unsigned int width = 80, height = 25;
		GetSize(width, height);
		rv = {(SHORT)(USHORT)width, (SHORT)(USHORT)height};
	} else
		rv = _listener->OnConsoleGetLargestWindowSize();
	ApplyConsoleSizeLimits(rv.X, rv.Y);
	return rv;
}

void ConsoleOutput::SetWindowInfo(bool absolute, const SMALL_RECT &rect)
{
	SetSize(rect.Right - rect.Left + 1, rect.Bottom - rect.Top + 1);
	if (_listener) {
		COORD pos = {rect.Left, rect.Top};
		_listener->OnConsoleOutputWindowMoved(absolute, pos);
	}
}

void ConsoleOutput::SetTitle(const WCHAR *title)
{
	{
		std::lock_guard<std::mutex> lock(_mutex);
		if (_title==title)
			return;

		_title = title;
	}
	if (_listener)
		_listener->OnConsoleOutputTitleChanged();
}

std::wstring ConsoleOutput::GetTitle()
{
	std::lock_guard<std::mutex> lock(_mutex);
	return _title;
}


DWORD ConsoleOutput::GetMode()
{
	std::lock_guard<std::mutex> lock(_mutex);
	return _mode;
}

void ConsoleOutput::SetMode(DWORD mode)
{
	std::lock_guard<std::mutex> lock(_mutex);	
	_mode = mode;
}

void ConsoleOutput::Read(CHAR_INFO *data, COORD data_size, COORD data_pos, SMALL_RECT &screen_rect)
{
	std::lock_guard<std::mutex> lock(_mutex);
	_buf.Read(data, data_size, data_pos, screen_rect);
}

void ConsoleOutput::Write(const CHAR_INFO *data, COORD data_size, COORD data_pos, SMALL_RECT &screen_rect)
{
	{
		std::lock_guard<std::mutex> lock(_mutex);
		_buf.Write(data, data_size, data_pos, screen_rect);
	}
	if (_listener)
		_listener->OnConsoleOutputUpdated(screen_rect);
}

bool ConsoleOutput::Read(CHAR_INFO &data, COORD screen_pos)
{
	std::lock_guard<std::mutex> lock(_mutex);
	return _buf.Read(data, screen_pos);
}

bool ConsoleOutput::Write(const CHAR_INFO &data, COORD screen_pos)
{
	{
		std::lock_guard<std::mutex> lock(_mutex);
		switch (_buf.Write(data, screen_pos)) {
			case ConsoleBuffer::WR_BAD: return false;
			case ConsoleBuffer::WR_SAME: return true;
			case ConsoleBuffer::WR_MODIFIED: break;
		}
	}

	if (_listener) {
		SMALL_RECT area = {screen_pos.X, screen_pos.Y, screen_pos.X, screen_pos.Y};
		_listener->OnConsoleOutputUpdated(area);
	}
	return true;
}

static inline void AffectArea(SMALL_RECT &area, SHORT x, SHORT y)
{
	if (area.Left > x) area.Left = x;
	if (area.Right < x) area.Right = x;
	if (area.Top > y) area.Top = y;
	if (area.Bottom < y) area.Bottom = y;
}

static inline void AffectArea(SMALL_RECT &area, const SMALL_RECT &r)
{
	if (area.Left > r.Left) area.Left = r.Left;
	if (area.Right < r.Right) area.Right = r.Right;
	if (area.Top > r.Top) area.Top = r.Top;
	if (area.Bottom < r.Bottom) area.Bottom = r.Bottom;
}


void ConsoleOutput::ScrollOutputOnOverflow(SMALL_RECT &area)
{
	unsigned int width, height;
	_buf.GetSize(width, height);
	
	if (height > ((unsigned int)_scroll_region.bottom) + 1)
		height = ((unsigned int)_scroll_region.bottom) + 1;

	if ( (height - _scroll_region.top) < 2 || width==0)
		return;

	_temp_chars.resize(width * (height - 1) );
	if (_temp_chars.empty())
		return;
	
	COORD tmp_pos = {0, 0};
	
	if (_scroll_callback.pfn) {
		COORD line_size = {(SHORT)width, 1};
		SMALL_RECT line_rect = {0, 0, (SHORT)(width - 1), 0};
		_buf.Read(&_temp_chars[0], line_size, tmp_pos, line_rect);
		_scroll_callback.pfn(_scroll_callback.context, _scroll_region.top, width, &_temp_chars[0]);
	}
	
	COORD tmp_size = {(SHORT)width, (SHORT)(height - 1 - _scroll_region.top)};
	
	SMALL_RECT scr_rect = {0, (SHORT)(_scroll_region.top + 1), (SHORT)(width - 1), (SHORT)(height - 1) };
	_buf.Read(&_temp_chars[0], tmp_size, tmp_pos, scr_rect);
	if (scr_rect.Left!=0 || scr_rect.Top!=(int)(_scroll_region.top + 1) 
		|| scr_rect.Right!=(int)(width-1) || scr_rect.Bottom!=(int)(height-1)) {
		fprintf(stderr, "ConsoleOutput::ScrollOutputOnOverflow: bug\n");
		return;
	}
	scr_rect.Top = _scroll_region.top;
	scr_rect.Bottom = height - 2;
	_buf.Write(&_temp_chars[0], tmp_size, tmp_pos, scr_rect);
	AffectArea(area, scr_rect);
	
	scr_rect.Left = 0;
	scr_rect.Right = width - 1;
	scr_rect.Top = scr_rect.Bottom = height - 1;
	tmp_size.Y = 1;
	for (unsigned int i = 0; i < width; ++i) {
		CHAR_INFO &ci = _temp_chars[i];
		ci.Char.UnicodeChar = ' ';
		ci.Attributes = _attributes;
	}
	_buf.Write(&_temp_chars[0], tmp_size, tmp_pos, scr_rect);
	AffectArea(area, scr_rect);
}

void ConsoleOutput::ModifySequenceEntityAt(SequenceModifier &sm, COORD pos)
{
	CHAR_INFO ch;

	switch (sm.kind) {
		case SequenceModifier::SM_WRITE_STR:
			ch.Char.UnicodeChar = *sm.str;
			ch.Attributes = _attributes;
			if ((_mode&ENABLE_PROCESSED_OUTPUT)!=0 && ch.Char.UnicodeChar==L'\t')
				 ch.Char.UnicodeChar = L' ';
			break;

		case SequenceModifier::SM_FILL_CHAR:
			if (!_buf.Read(ch, pos))
				return;

			ch.Char.UnicodeChar = sm.chr;
			break;

		case SequenceModifier::SM_FILL_ATTR:
			if (!_buf.Read(ch, pos))
				return;

			ch.Attributes = sm.attr;
			break;
	}
	
	if (_buf.Write(ch, pos) == ConsoleBuffer::WR_MODIFIED)
		AffectArea(sm.area, pos.X, pos.Y);
}

size_t ConsoleOutput::ModifySequenceAt(SequenceModifier &sm, COORD &pos)
{
	size_t rv = 0;
	SMALL_RECT pos_areas[2];
	bool refresh_pos_areas = false;
	{
		std::lock_guard<std::mutex> lock(_mutex);
		pos_areas[0].Left = pos_areas[0].Right = pos.X;
		pos_areas[0].Top = pos_areas[0].Bottom = pos.Y;
		
		unsigned int width, height;
		_buf.GetSize(width, height);
		unsigned int scroll_edge = std::min(height, ((unsigned int)_scroll_region.bottom) + 1);

		for (;;) {
			if (!sm.count) break;
			if (sm.kind==SequenceModifier::SM_WRITE_STR && *sm.str==7 && (_mode&ENABLE_PROCESSED_OUTPUT)!=0 )
			{
				//TODO: Ding!
				--sm.count;
				++sm.str;
				continue;
			}
			
			if (pos.X >= (int)width) {
				if ( sm.kind!=SequenceModifier::SM_WRITE_STR || ( (_mode&ENABLE_WRAP_AT_EOL_OUTPUT)!=0 && 
						((_mode&ENABLE_PROCESSED_OUTPUT)==0 || (*sm.str!='\r'&& *sm.str!='\n')))) {
					pos.X = 0;
					pos.Y++;
					if (pos.Y >= (int) scroll_edge) {
						pos.Y--;
						ScrollOutputOnOverflow(sm.area);
					}
				} else
					pos.X = width - 1;
			}

			if (sm.kind==SequenceModifier::SM_WRITE_STR && *sm.str==L'\b' && (_mode&ENABLE_PROCESSED_OUTPUT)!=0) {
				if (pos.X > 0) {
					pos.X--;
				}
			} else if (sm.kind==SequenceModifier::SM_WRITE_STR && *sm.str==L'\r' && (_mode&ENABLE_PROCESSED_OUTPUT)!=0) {
				pos.X = 0;

			} else if ( sm.kind==SequenceModifier::SM_WRITE_STR && *sm.str==L'\n' && (_mode&ENABLE_PROCESSED_OUTPUT)!=0) {
				pos.X = 0;
				pos.Y++;
				if (pos.Y >= (int)scroll_edge) {
					pos.Y--;
					ScrollOutputOnOverflow(sm.area);
				}
			} else {
				ModifySequenceEntityAt(sm, pos);
				pos.X++;
			}
			if (sm.kind==SequenceModifier::SM_WRITE_STR) {
				if (*sm.str!=L'\t' || (pos.X%TAB_WIDTH)==0 || (_mode&ENABLE_PROCESSED_OUTPUT)==0) {
					++sm.str;
					--sm.count;
					++rv;
				}
			} else {
				--sm.count;
				++rv;				
			}
		}
		
		if (&pos == &_cursor.pos && (pos_areas[0].Left!=pos.X || pos_areas[0].Top!=pos.Y)) {
			refresh_pos_areas = true;
			pos_areas[1].Left = pos_areas[1].Right = pos.X;
			pos_areas[1].Top = pos_areas[1].Bottom = pos.Y;
		}
	}
	if (_listener) {
		if (refresh_pos_areas) {
			_listener->OnConsoleOutputUpdated(pos_areas[0]);
			_listener->OnConsoleOutputUpdated(pos_areas[1]);
		}
		if (sm.area.Right >= sm.area.Left && sm.area.Bottom >= sm.area.Top) 
			_listener->OnConsoleOutputUpdated(sm.area);
	}
	return rv;
}

size_t ConsoleOutput::WriteString(const WCHAR *data, size_t count)
{
	SequenceModifier sm = {SequenceModifier::SM_WRITE_STR, count, {MAXSHORT, MAXSHORT, 0, 0} };
	sm.str = data;
	return ModifySequenceAt(sm, _cursor.pos);
}


size_t ConsoleOutput::WriteStringAt(const WCHAR *data, size_t count, COORD &pos)
{
	SequenceModifier sm = {SequenceModifier::SM_WRITE_STR, count, {MAXSHORT, MAXSHORT, 0, 0} };
	sm.str = data;
	return ModifySequenceAt(sm, pos);
}


size_t ConsoleOutput::FillCharacterAt(WCHAR cCharacter, size_t count, COORD &pos)
{
	SequenceModifier sm = {SequenceModifier::SM_FILL_CHAR, count, {MAXSHORT, MAXSHORT, 0, 0} };
	sm.chr = cCharacter;
	return ModifySequenceAt(sm, pos);
}


size_t ConsoleOutput::FillAttributeAt(WORD wAttribute, size_t count, COORD &pos)
{
	SequenceModifier sm = {SequenceModifier::SM_FILL_ATTR, count};
	sm.attr = wAttribute;
	return ModifySequenceAt(sm, pos);
}


static void ClipRect(SMALL_RECT &rect, const SMALL_RECT &clip, COORD *offset = NULL)
{
	if (rect.Left < clip.Left) {
		if (offset) offset->X+= clip.Left - rect.Left;
		rect.Left = clip.Left;
	}
	if (rect.Top < clip.Top) {
		if (offset) offset->Y+= clip.Top - rect.Top;
		rect.Top = clip.Top;
	}
	if (rect.Right > clip.Right) rect.Right = clip.Right;
	if (rect.Bottom > clip.Bottom) rect.Bottom = clip.Bottom;	
}

bool ConsoleOutput::Scroll(const SMALL_RECT *lpScrollRectangle, 
	const SMALL_RECT *lpClipRectangle,  COORD dwDestinationOrigin, const CHAR_INFO *lpFill)
{
	SMALL_RECT src_rect = *lpScrollRectangle;
	if (src_rect.Right < src_rect.Left || src_rect.Bottom < src_rect.Top) 
		return false;

	COORD data_size = {(SHORT)(src_rect.Right + 1 - src_rect.Left), (SHORT)(src_rect.Bottom + 1 - src_rect.Top)};
	size_t total_chars = data_size.X;
	total_chars*= data_size.Y;
	COORD data_pos = {0, 0};
		
	_temp_chars.resize(total_chars);
	SMALL_RECT dst_rect = {dwDestinationOrigin.X, dwDestinationOrigin.Y, 
		(SHORT)(dwDestinationOrigin.X + data_size.X - 1), (SHORT)(dwDestinationOrigin.Y + data_size.Y - 1)};
	{
		std::lock_guard<std::mutex> lock(_mutex);
		_buf.Read(&_temp_chars[0], data_size, data_pos, src_rect);

		fprintf(stderr, "!!!!SCROLL:[%u %u %u %u] -> [%u %u %u %u]",
			src_rect.Left, src_rect.Top, src_rect.Right, src_rect.Bottom,
			dst_rect.Left, dst_rect.Top, dst_rect.Right, dst_rect.Bottom);
	
		if (lpClipRectangle) {
			fprintf(stderr, " CLIP:[%u %u %u %u]",
				lpClipRectangle->Left, lpClipRectangle->Top, lpClipRectangle->Right, lpClipRectangle->Bottom);
			ClipRect(src_rect, *lpClipRectangle);
			ClipRect(dst_rect, *lpClipRectangle, &data_pos);

			fprintf(stderr, " CLIPPED:[%u %u %u %u] -> [%u %u %u %u] DP=[%u %u]",
				src_rect.Left, src_rect.Top, src_rect.Right, src_rect.Bottom,
				dst_rect.Left, dst_rect.Top, dst_rect.Right, dst_rect.Bottom,
				data_pos.X, data_pos.Y);
		}
		fprintf(stderr, "\n");
		
		if (lpFill) {
			COORD fill_pos;
			for (fill_pos.Y = src_rect.Top; fill_pos.Y <= src_rect.Bottom; fill_pos.Y++ )
			for (fill_pos.X = src_rect.Left; fill_pos.X <= src_rect.Right; fill_pos.X++ ) {
				_buf.Write(*lpFill, fill_pos);
			}
		}

		_buf.Write(&_temp_chars[0], data_size, data_pos, dst_rect);
	}

	if (_listener) {
		if (lpFill)
			_listener->OnConsoleOutputUpdated(src_rect);
		_listener->OnConsoleOutputUpdated(dst_rect);
	}
	return true;
}

void ConsoleOutput::SetScrollRegion(SHORT top, SHORT bottom)
{
	std::lock_guard<std::mutex> lock(_mutex);
	unsigned int width = 0, height = 0;
	_buf.GetSize(width, height);
	ApplyCoordinateLimits(bottom, height);
	ApplyCoordinateLimits(top, bottom);
	_scroll_region.top = top;
	_scroll_region.bottom = bottom;
}

void ConsoleOutput::GetScrollRegion(SHORT &top, SHORT &bottom)
{
	std::lock_guard<std::mutex> lock(_mutex);
	top = _scroll_region.top;
	bottom = _scroll_region.bottom;
}


void ConsoleOutput::SetScrollCallback(PCONSOLE_SCROLL_CALLBACK pCallback, PVOID pContext)
{
	std::lock_guard<std::mutex> lock(_mutex);
	_scroll_callback.pfn = pCallback;
	_scroll_callback.context = pContext;
}


void ConsoleOutput::AdhocQuickEdit()
{
	if (_listener)
		_listener->OnConsoleAdhocQuickEdit();
}
