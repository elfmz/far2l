#include "ConsoleOutput.h"
#include "WinPort.h"
#include <utils.h>

#define TAB_WIDTH	8
#define NO_AREA {MAXSHORT, MAXSHORT, 0, 0}

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
	_backend(NULL),
	_mode(ENABLE_PROCESSED_OUTPUT|ENABLE_WRAP_AT_EOL_OUTPUT),
	_attributes(FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED)
{
	memset(&_cursor.pos, 0, sizeof(_cursor.pos));	
	MB2Wide(APP_BASENAME, _title);
	_scroll_callback.pfn = NULL;
	_cursor.height = 15;
	_cursor.visible = true;
	_scroll_region.top = 0;
	_scroll_region.bottom = MAXSHORT;
	SetSize(80, 25);
}

void ConsoleOutput::SetBackend(IConsoleOutputBackend *backend)
{
	_backend = backend;
}


void ConsoleOutput::SetAttributes(DWORD64 attributes)
{
	std::lock_guard<std::mutex> lock(_mutex);
	_attributes = attributes;
}

DWORD64 ConsoleOutput::GetAttributes()
{
	std::lock_guard<std::mutex> lock(_mutex);
	return _attributes;
}

void ConsoleOutput::SetUpdateCellArea(SMALL_RECT &area, COORD pos)
{
	area.Left = area.Right = pos.X;
	area.Top = area.Bottom = pos.Y;
	CHAR_INFO ci{};
	if (_buf.Read(ci, pos)) {
		if (!ci.Char.UnicodeChar && area.Left > 0) {
			--area.Left;
		} else if (CI_FULL_WIDTH_CHAR(ci)) {
			++area.Right;
		}
	}
}

void ConsoleOutput::SetCursor(COORD pos)
{
	SMALL_RECT area[2];
	{
		std::lock_guard<std::mutex> lock(_mutex);
		if (_cursor.pos.X == pos.X && _cursor.pos.Y == pos.Y)
			return;

		SetUpdateCellArea(area[0], _cursor.pos);
		_cursor.pos = pos;
		SetUpdateCellArea(area[1], _cursor.pos);
	}
	if (_backend) {
		_backend->OnConsoleOutputUpdated(&area[0], 2);
	}
}

void ConsoleOutput::SetCursor(UCHAR height, bool visible)
{
	SMALL_RECT area;
	{
		std::lock_guard<std::mutex> lock(_mutex);
		_cursor.height = height;
		_cursor.visible = visible;
		SetUpdateCellArea(area, _cursor.pos);
	}
	if (_backend)
		_backend->OnConsoleOutputUpdated(&area, 1);
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
	if (_backend)
		_backend->OnConsoleOutputResized();
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
	if (!_backend) {
		unsigned int width = 80, height = 25;
		GetSize(width, height);
		rv = {(SHORT)(USHORT)width, (SHORT)(USHORT)height};
	} else
		rv = _backend->OnConsoleGetLargestWindowSize();
	ApplyConsoleSizeLimits(rv.X, rv.Y);
	return rv;
}

void ConsoleOutput::SetWindowMaximized(bool maximized)
{
	if (_backend)
		_backend->OnConsoleSetMaximized(maximized);
}

void ConsoleOutput::SetWindowInfo(bool absolute, const SMALL_RECT &rect)
{
	SetSize(rect.Right - rect.Left + 1, rect.Bottom - rect.Top + 1);
	if (_backend) {
		COORD pos = {rect.Left, rect.Top};
		_backend->OnConsoleOutputWindowMoved(absolute, pos);
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
	if (_backend)
		_backend->OnConsoleOutputTitleChanged();
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
	if (_backend)
		_backend->OnConsoleOutputUpdated(&screen_rect, 1);
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

	if (_backend) {
		SMALL_RECT area;
		SetUpdateCellArea(area, screen_pos);
		_backend->OnConsoleOutputUpdated(&area, 1);
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

	_temp_chars.resize(size_t(width) * (height - 1) );
	if (_temp_chars.empty())
		return;
	
	COORD tmp_pos = {0, 0};
	
	if (_scroll_callback.pfn && _scroll_region.top == 0) {
		COORD line_size = {(SHORT)width, 1};
		SMALL_RECT line_rect = {0, 0, (SHORT)(width - 1), 0};
		_buf.Read(&_temp_chars[0], line_size, tmp_pos, line_rect);
		_scroll_callback.pfn(_scroll_callback.context, width, &_temp_chars[0]);
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
		CI_SET_WCATTR(_temp_chars[i], L' ', _attributes);
	}
	_buf.Write(&_temp_chars[0], tmp_size, tmp_pos, scr_rect);
	AffectArea(area, scr_rect);
}

SHORT ConsoleOutput::ModifySequenceEntityAt(SequenceModifier &sm, COORD pos, SMALL_RECT &area)
{
	CHAR_INFO ch;
	SHORT out = 1;

	switch (sm.kind) {
		case SequenceModifier::SM_WRITE_STR:
			if (IsCharPrefix(*sm.str)) {
				out = 0;
				CI_SET_WCHAR(ch, *sm.str);
				// surrogate pairs not used for UTF32, so dont need to do special tricks to keep it,
				// so let following normal character to overwrite abnormal surrogate pair prefixx

			} else if (IsCharSuffix(*sm.str) && _prev_pos.X >= 0) {
				out = 0;
				if (!_buf.Read(ch, _prev_pos)) {
					return false;
				}
				pos = _prev_pos;
				std::wstring tmp;
				if (CI_USING_COMPOSITE_CHAR(ch)) {
					tmp = WINPORT(CompositeCharLookup)(ch.Char.UnicodeChar);
				} else {
					tmp = ch.Char.UnicodeChar;
				}
				tmp+= *sm.str;
				CI_SET_COMPOSITE(ch, tmp.c_str());

			} else {
				CI_SET_WCHAR(ch, *sm.str);
				if ((_mode&ENABLE_PROCESSED_OUTPUT)!=0 && ch.Char.UnicodeChar==L'\t') {
					 CI_SET_WCHAR(ch, L' ');
				}
				if (IsCharFullWidth(ch.Char.UnicodeChar)) {
//					fprintf(stderr, "IsCharFullWidth: %lc [0x%llx]\n",
//						(WCHAR)ch.Char.UnicodeChar, (unsigned long long)ch.Char.UnicodeChar);
					out = 2;
				}
			}
			CI_SET_ATTR(ch, _attributes);
			_prev_pos = pos;
			break;

		case SequenceModifier::SM_FILL_CHAR:
			if (!_buf.Read(ch, pos))
				return out;

			CI_SET_WCHAR(ch, sm.chr);
			break;

		case SequenceModifier::SM_FILL_ATTR:
			if (!_buf.Read(ch, pos))
				return out;

			CI_SET_ATTR(ch, sm.attr);
			break;
	}
	
	if (_buf.Write(ch, pos) == ConsoleBuffer::WR_MODIFIED) {
		AffectArea(area, pos.X, pos.Y);
		if (out == 2) {
			CI_SET_WCHAR(ch, 0);
			pos.X++;
			if (_buf.Write(ch, pos) == ConsoleBuffer::WR_MODIFIED) {
				AffectArea(area, pos.X, pos.Y);
			}
		}
	}

	return out;
}

size_t ConsoleOutput::ModifySequenceAt(SequenceModifier &sm, COORD &pos)
{
	size_t rv = 0;
	SMALL_RECT areas[3] = {NO_AREA, NO_AREA, NO_AREA}; // pos1, pos2, main
	bool refresh_pos_areas = false;
	{
		std::lock_guard<std::mutex> lock(_mutex);
		SetUpdateCellArea(areas[0], pos);
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
						ScrollOutputOnOverflow(areas[2]);
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
				//pos.X = 0;
				pos.Y++;
				if (pos.Y >= (int)scroll_edge) {
					pos.Y--;
					ScrollOutputOnOverflow(areas[2]);
				}
			} else {
				pos.X+= ModifySequenceEntityAt(sm, pos, areas[2]);
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
		
		if (&pos == &_cursor.pos && (areas[0].Left!=pos.X || areas[0].Top!=pos.Y)) {
			refresh_pos_areas = true;
			areas[1].Left = areas[1].Right = pos.X;
			areas[1].Top = areas[1].Bottom = pos.Y;
		}
	}
	if (_backend) {
		bool refresh_main_area = (areas[2].Left <= areas[2].Right
			&& areas[2].Top <= areas[2].Bottom);

		if (refresh_pos_areas) {
			_backend->OnConsoleOutputUpdated(&areas[0], refresh_main_area ? 3 : 2);
		} else if (refresh_main_area) {
			_backend->OnConsoleOutputUpdated(&areas[2], 1);
		}
	}
	return rv;
}

size_t ConsoleOutput::WriteString(const WCHAR *data, size_t count)
{
	SequenceModifier sm = {SequenceModifier::SM_WRITE_STR, count };
	sm.str = data;
	return ModifySequenceAt(sm, _cursor.pos);
}


size_t ConsoleOutput::WriteStringAt(const WCHAR *data, size_t count, COORD &pos)
{
	SequenceModifier sm = {SequenceModifier::SM_WRITE_STR, count };
	sm.str = data;
	return ModifySequenceAt(sm, pos);
}


size_t ConsoleOutput::FillCharacterAt(WCHAR cCharacter, size_t count, COORD &pos)
{
	SequenceModifier sm = {SequenceModifier::SM_FILL_CHAR, count };
	sm.chr = cCharacter;
	return ModifySequenceAt(sm, pos);
}


size_t ConsoleOutput::FillAttributeAt(DWORD64 qAttributes, size_t count, COORD &pos)
{
	SequenceModifier sm = {SequenceModifier::SM_FILL_ATTR, count };
	sm.attr = qAttributes;
	return ModifySequenceAt(sm, pos);
}


static void ClipRect(SMALL_RECT &rect, const SMALL_RECT &clip, COORD *offset = NULL)
{
	if (rect.Left < clip.Left) {
		if (offset) {
			offset->X+= clip.Left - rect.Left;
		}
		rect.Left = clip.Left;
	}
	if (rect.Top < clip.Top) {
		if (offset) {
			offset->Y+= clip.Top - rect.Top;
		}
		rect.Top = clip.Top;
	}
	if (rect.Right > clip.Right) {
		rect.Right = clip.Right;
	}
	if (rect.Bottom > clip.Bottom) {
		rect.Bottom = clip.Bottom;
	}
}

bool ConsoleOutput::Scroll(const SMALL_RECT *lpScrollRectangle, 
	const SMALL_RECT *lpClipRectangle,  COORD dwDestinationOrigin, const CHAR_INFO *lpFill)
{
	union {
		SMALL_RECT both[2];
		struct {
			SMALL_RECT dst; // must be first cuz it always updated
			SMALL_RECT src; // updated only if lpFill is not NULL
		} n;
	} areas;

	areas.n.src = *lpScrollRectangle;
	if (areas.n.src.Right < areas.n.src.Left || areas.n.src.Bottom < areas.n.src.Top)
		return false;

	COORD data_size = {(SHORT)(areas.n.src.Right + 1 - areas.n.src.Left), (SHORT)(areas.n.src.Bottom + 1 - areas.n.src.Top)};
	size_t total_chars = data_size.X;
	total_chars*= data_size.Y;
	COORD data_pos = {0, 0};
		
	areas.n.dst = {dwDestinationOrigin.X, dwDestinationOrigin.Y,
		(SHORT)(dwDestinationOrigin.X + data_size.X - 1), (SHORT)(dwDestinationOrigin.Y + data_size.Y - 1)};
	{
		std::lock_guard<std::mutex> lock(_mutex);
		_temp_chars.resize(total_chars);
		_buf.Read(&_temp_chars[0], data_size, data_pos, areas.n.src);

		fprintf(stderr, "!!!!SCROLL:[%i %i %i %i] -> [%i %i %i %i]",
			areas.n.src.Left, areas.n.src.Top, areas.n.src.Right, areas.n.src.Bottom,
			areas.n.dst.Left, areas.n.dst.Top, areas.n.dst.Right, areas.n.dst.Bottom);
	
		if (lpClipRectangle) {
			fprintf(stderr, " CLIP:[%i %i %i %i]",
				lpClipRectangle->Left, lpClipRectangle->Top, lpClipRectangle->Right, lpClipRectangle->Bottom);
			ClipRect(areas.n.src, *lpClipRectangle);
			ClipRect(areas.n.dst, *lpClipRectangle, &data_pos);

			fprintf(stderr, " CLIPPED:[%i %i %i %i] -> [%i %i %i %i] DP=[%i %i]",
				areas.n.src.Left, areas.n.src.Top, areas.n.src.Right, areas.n.src.Bottom,
				areas.n.dst.Left, areas.n.dst.Top, areas.n.dst.Right, areas.n.dst.Bottom,
				data_pos.X, data_pos.Y);
		}
		
		if (lpFill) {
			fprintf(stderr, " FILL:[%i %i %i %i]",
				areas.n.src.Left, areas.n.src.Top, areas.n.src.Right, areas.n.src.Bottom);
			COORD fill_pos;
			for (fill_pos.Y = areas.n.src.Top; fill_pos.Y <= areas.n.src.Bottom; fill_pos.Y++ )
			for (fill_pos.X = areas.n.src.Left; fill_pos.X <= areas.n.src.Right; fill_pos.X++ ) {
				_buf.Write(*lpFill, fill_pos);
			}
		}


		fprintf(stderr, " WRITE:[%i %i %i %i]",
			areas.n.dst.Left, areas.n.dst.Top, areas.n.dst.Right, areas.n.dst.Bottom);
		fprintf(stderr, "\n");
		_buf.Write(&_temp_chars[0], data_size, data_pos, areas.n.dst);
	}

	if (_backend)
		_backend->OnConsoleOutputUpdated(&areas.both[0], lpFill ? 2 : 1);

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
	if (_backend)
		_backend->OnConsoleAdhocQuickEdit();
}

DWORD ConsoleOutput::SetConsoleTweaks(DWORD tweaks)
{
	if (!_backend)
		return 0;

	return _backend->OnConsoleSetTweaks(tweaks);
}

void ConsoleOutput::ConsoleChangeFont()
{
	if (_backend)
		_backend->OnConsoleChangeFont();
}

bool ConsoleOutput::IsActive()
{
	return _backend ? _backend->OnConsoleIsActive() : false;
}

void ConsoleOutput::ConsoleDisplayNotification(const WCHAR *title, const WCHAR *text)
{
	if (_backend)
		_backend->OnConsoleDisplayNotification(title, text);
}

bool ConsoleOutput::ConsoleBackgroundMode(bool TryEnterBackgroundMode)
{
	return (_backend && _backend->OnConsoleBackgroundMode(TryEnterBackgroundMode));
}

bool ConsoleOutput::SetFKeyTitles(const CHAR **titles)
{
	return (_backend && _backend->OnConsoleSetFKeyTitles(titles));
}

BYTE ConsoleOutput::GetColorPalette()
{
	return _backend ? _backend->OnConsoleGetColorPalette() : 4;
}

const WCHAR *ConsoleOutput::LockedGetTitle()
{
	_mutex.lock();
	return _title.c_str();
}

CHAR_INFO *ConsoleOutput::LockedDirectLineAccess(size_t line_index, unsigned int &width)
{
	_mutex.lock();
	width = _buf.GetWidth();
	return _buf.DirectLineAccess(line_index);
}

void ConsoleOutput::Unlock()
{
	_mutex.unlock();
}

