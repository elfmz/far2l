
#include "ConsoleBuffer.h"

ConsoleBuffer::ConsoleBuffer() : _width(0)
{
}

void ConsoleBuffer::SetSize(unsigned int width, unsigned int height, uint64_t attributes)
{
	if (width==_width && (width*height)==_console_chars.size() )
		return;

	COORD prev_size = {(SHORT)_width, _width ? (SHORT)(_console_chars.size() / _width) : (SHORT)0 };
	ConsoleChars other_chars; 
	other_chars.resize(size_t(height) * width);
	_console_chars.swap(other_chars);
	_width = width;
	for (auto &i : _console_chars) {
		CI_SET_WCATTR(i, L' ', attributes);
	}

	if (!other_chars.empty() && !_console_chars.empty()) {
		COORD prev_pos = {0, 0};
		SMALL_RECT screen_rect = {0, 0, (SHORT)(width - 1), (SHORT)(height - 1)};
		Write(&other_chars[0], prev_size, prev_pos, screen_rect);
	}
	
}

void ConsoleBuffer::GetSize(unsigned int &width, unsigned int &height)
{
	width = _width;
	height = _width ? _console_chars.size() / _width : 0;
}

template <class T> T *OffsetMatrixPtr(T *p, size_t width, size_t x, size_t y)
{
	size_t index = y;
	index*= width;
	index+= x;
	return p + index;
}

CHAR_INFO *ConsoleBuffer::InspectCopyArea(const COORD &data_size, const COORD &data_pos, SMALL_RECT &screen_rect)
{
	if (data_pos.X < 0 || data_pos.Y < 0 || data_size.X < 0 || data_size.Y < 0
		|| data_pos.X >= data_size.X || data_pos.Y >= data_size.Y)
	{
		fprintf(stderr, "InspectCopyArea: bad coordinates, data_size:{%d, %d} data_pos:{%d, %d}\n",
			data_size.X, data_size.Y, data_pos.X, data_pos.Y);
		return nullptr;
	}

	unsigned int height = _console_chars.size() / _width;
	if ((int)_width <= screen_rect.Right)
		screen_rect.Right = _width - 1;
	if ((int)height <= screen_rect.Bottom)
		screen_rect.Bottom = height - 1;

	SHORT data_avail_width = data_size.X - data_pos.X;
	SHORT data_avail_height = data_size.Y - data_pos.Y;
	if (data_avail_width <= (screen_rect.Right - screen_rect.Left))
		screen_rect.Right = screen_rect.Left + data_avail_width - 1;
	if (data_avail_height <= (screen_rect.Bottom - screen_rect.Top))
		screen_rect.Bottom = screen_rect.Top + data_avail_height - 1;

	if ((int)_width <= screen_rect.Left || screen_rect.Right < screen_rect.Left || screen_rect.Left < 0) {
		fprintf(stderr, "InspectCopyArea: bad X-metrics, _width:%d screen_rect:{%d..%d}\n",
			_width, screen_rect.Left, screen_rect.Right);
		return nullptr;
	}
	if ((int)height <= screen_rect.Top || screen_rect.Bottom < screen_rect.Top || screen_rect.Top < 0) {
		fprintf(stderr, "InspectCopyArea: bad Y-metrics, height:%d screen_rect:{%d..%d}\n",
			height, screen_rect.Top, screen_rect.Bottom);
		return nullptr;
	}

	return OffsetMatrixPtr(&_console_chars[0], _width, screen_rect.Left, screen_rect.Top);
}

void ConsoleBuffer::Read(CHAR_INFO *data, COORD data_size, COORD data_pos, SMALL_RECT &screen_rect)
{
	CHAR_INFO *screen = InspectCopyArea(data_size, data_pos, screen_rect);
	if (!screen) 
		return;
	
	data = OffsetMatrixPtr(data, data_size.X, data_pos.X, data_pos.Y);
	for (SHORT y = screen_rect.Top; y <= screen_rect.Bottom; ++y) {
		memcpy(data, screen, (screen_rect.Right + 1 - screen_rect.Left) * sizeof(*data));
		screen+= _width;
		data+= data_size.X;
	}
}

static inline bool AreSameChars(const CHAR_INFO &one, const CHAR_INFO &another)
{
	return one.Char.UnicodeChar == another.Char.UnicodeChar && one.Attributes == another.Attributes;
}

void ConsoleBuffer::Write(const CHAR_INFO *data, COORD data_size, COORD data_pos, SMALL_RECT &screen_rect)
{
	CHAR_INFO *screen = InspectCopyArea(data_size, data_pos, screen_rect);
	if (!screen)
		return ;

	data = OffsetMatrixPtr(data, data_size.X, data_pos.X, data_pos.Y);
	for (SHORT y = screen_rect.Top; y <= screen_rect.Bottom; ++y) {
		memcpy(screen, data, (screen_rect.Right + 1 - screen_rect.Left) * sizeof(*data));
		screen+= _width;
		data+= data_size.X;
	}
}

bool ConsoleBuffer::Read(CHAR_INFO &ch, COORD screen_pos)
{
	if (screen_pos.X < 0 || screen_pos.Y < 0 || screen_pos.X >= (int)_width)
		return false;

	size_t index = screen_pos.Y;
	index*= _width;
	index+= screen_pos.X;

	if (index >= _console_chars.size())
		return false;

	ch = _console_chars[index];
	return true;
}

ConsoleBuffer::WriteResult ConsoleBuffer::Write(const CHAR_INFO &ch, COORD screen_pos)
{
	if (screen_pos.X < 0 || screen_pos.Y < 0 || screen_pos.X >= (int)_width)
		return WR_BAD;

	size_t index = screen_pos.Y;
	index*= _width;
	index+= screen_pos.X;

	if (index >= _console_chars.size())
		return WR_BAD;

	CHAR_INFO &dch = _console_chars[index];
	if (AreSameChars(dch, ch))
		return WR_SAME;
		
	dch = ch;
	return WR_MODIFIED;
}

