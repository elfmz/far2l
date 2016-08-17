#include "stdafx.h"
#include "ConsoleOutput.h"

#define TAB_WIDTH	8

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


ConsoleOutput::ConsoleOutput() : _attributes(FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED), 
	_listener(0), _title(L"WinPort"), _mode(ENABLE_PROCESSED_OUTPUT|ENABLE_WRAP_AT_EOL_OUTPUT)
{
	_largest_window_size.X = 80;
	_largest_window_size.Y = 25;
	memset(&_cursor.pos, 0, sizeof(_cursor.pos));	
	_cursor.height = 13;
	_cursor.visible = true;
	SetSize(_largest_window_size.X, _largest_window_size.Y);
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

void ConsoleOutput::SetCursor(const COORD &pos)
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
	return _cursor.pos;
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
	{
		std::lock_guard<std::mutex> lock(_mutex);
		_buf.SetSize(width, height, _attributes);
	}
	if (_listener)
		_listener->OnConsoleOutputResized();
}

void ConsoleOutput::GetSize(unsigned int &width, unsigned int &height)
{
	std::lock_guard<std::mutex> lock(_mutex);
	_buf.GetSize(width, height);
}

void ConsoleOutput::SetLargestConsoleWindowSize(COORD size)
{
	std::lock_guard<std::mutex> lock(_mutex);
	_largest_window_size = size;
}

COORD ConsoleOutput::GetLargestConsoleWindowSize()
{
	std::lock_guard<std::mutex> lock(_mutex);
	return _largest_window_size;
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

bool ConsoleOutput::Write(const CHAR_INFO &data, COORD screen_pos)
{
	{
		std::lock_guard<std::mutex> lock(_mutex);
		if (!_buf.Write(data, screen_pos))
			return false;
	}

	if (_listener) {
		SMALL_RECT area = {screen_pos.X, screen_pos.Y, screen_pos.X, screen_pos.Y};
		_listener->OnConsoleOutputUpdated(area);
	}
	return true;
}

void ConsoleOutput::ScrollOutputOnOverflow()
{
	unsigned int width, height;
	_buf.GetSize(width, height);
	if (height < 2 || width==0)
		return;
		
	std::vector<CHAR_INFO> tmp(width * (height - 1) );
	if (tmp.empty())
		return;
		
	COORD tmp_size = {(SHORT)width, (SHORT)height - 1};
	COORD tmp_pos = {0, 0};
	SMALL_RECT scr_rect = {0, 1, width - 1, height - 1};
	_buf.Read(&tmp[0], tmp_size, tmp_pos, scr_rect);
	if (scr_rect.Left!=0 || scr_rect.Top!=1 || scr_rect.Right!=(width-1) || scr_rect.Bottom!=(height-1)) {
		fprintf(stderr, "ConsoleOutput::ScrollOutputOnOverflow: bug\n");
		return;
	}
	scr_rect.Top = 0;
	scr_rect.Bottom = height - 2;
	_buf.Write(&tmp[0], tmp_size, tmp_pos, scr_rect);
	
	scr_rect.Left = 0;
	scr_rect.Right = width - 1;
	scr_rect.Top = scr_rect.Bottom = height- 1;
	tmp_size.Y = 1;
	for (unsigned int i = 0; i < width; ++i) {
		CHAR_INFO &ci = tmp[i];
		ci.Char.UnicodeChar = ' ';
		ci.Attributes = _attributes;
	}
	_buf.Write(&tmp[0], tmp_size, tmp_pos, scr_rect);
}

bool ConsoleOutput::ModifySequenceEntityAt(const SequenceModifier &sm, COORD pos)
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
				return false;
			ch.Char.UnicodeChar = sm.chr;
			break;

		case SequenceModifier::SM_FILL_ATTR:
			if (!_buf.Read(ch, pos))
				return false;
			ch.Attributes = sm.attr;
			break;
	}
	return _buf.Write(ch, pos);
}

size_t ConsoleOutput::ModifySequenceAt(SequenceModifier &sm, COORD &pos)
{
	size_t rv = 0;
	SMALL_RECT area;
	bool scrolled = false;
	{
		std::lock_guard<std::mutex> lock(_mutex);
		area.Left = area.Right = pos.X; 
		area.Top = area.Bottom = pos.Y;

		unsigned int width, height;
		_buf.GetSize(width, height);
		for (;;) {
			if (!sm.count) break;
			if (pos.X >= width) {
				if ( sm.kind!=SequenceModifier::SM_WRITE_STR || (_mode&ENABLE_WRAP_AT_EOL_OUTPUT)!=0) {
					pos.X = 0;
					pos.Y++;
					if (pos.Y>=height) {
						pos.Y--;
						ScrollOutputOnOverflow();
						scrolled = true;
					} else 
						area.Bottom++;
				
					area.Left = 0;				
				} else
					pos.X = width - 1;
			}

			if (sm.kind==SequenceModifier::SM_WRITE_STR && *sm.str==L'\b' && (_mode&ENABLE_PROCESSED_OUTPUT)!=0) {
				if (pos.X > 0) {
					pos.X--;
					if (area.Left > pos.X) area.Left = pos.X;
				} else if (pos.Y > 0) {
					pos.Y--;
					pos.X = width  - 1;
					if (area.Top > pos.Y) area.Top = pos.Y;
					area.Right = width  -1;
				}
				CHAR_INFO ch;
				ch.Char.UnicodeChar = L' ';
				ch.Attributes = _attributes;
				_buf.Write(ch, pos);

			} else if (sm.kind==SequenceModifier::SM_WRITE_STR && *sm.str==L'\r' && (_mode&ENABLE_PROCESSED_OUTPUT)!=0) {
				pos.X = 0;
				area.Left = 0;

			} else if (sm.kind==SequenceModifier::SM_WRITE_STR && *sm.str==L'\n' && (_mode&ENABLE_PROCESSED_OUTPUT)!=0) {
				pos.X = 0;
				area.Left = 0;
				pos.Y++;
				if (pos.Y>=height) {
					pos.Y--;
					ScrollOutputOnOverflow();
					scrolled = true;
				} else
					area.Bottom++;
			} else {
				ModifySequenceEntityAt(sm, pos);
				area.Right++;
				if (area.Right == width) 
					area.Right = width - 1;
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
		if (scrolled) {
			area.Left = 0;
			area.Top = 0;
			area.Right = width - 1;
			area.Bottom = height - 1;
		}
	}
	if (rv && _listener) {
		_listener->OnConsoleOutputUpdated(area);
	}
	return rv;
}

size_t ConsoleOutput::WriteString(const WCHAR *data, size_t count)
{
	SequenceModifier sm = {SequenceModifier::SM_WRITE_STR, count};
	sm.str = data;
	return ModifySequenceAt(sm, _cursor.pos);
}


size_t ConsoleOutput::WriteStringAt(const WCHAR *data, size_t count, COORD &pos)
{
	SequenceModifier sm = {SequenceModifier::SM_WRITE_STR, count};
	sm.str = data;
	return ModifySequenceAt(sm, pos);
}


size_t ConsoleOutput::FillCharacterAt(WCHAR cCharacter, size_t count, COORD &pos)
{
	SequenceModifier sm = {SequenceModifier::SM_FILL_CHAR, count};
	sm.chr = cCharacter;
	return ModifySequenceAt(sm, pos);
}


size_t ConsoleOutput::FillAttributeAt(WORD wAttribute, size_t count, COORD &pos)
{
	SequenceModifier sm = {SequenceModifier::SM_FILL_ATTR, count};
	sm.attr = wAttribute;
	return ModifySequenceAt(sm, pos);
}

