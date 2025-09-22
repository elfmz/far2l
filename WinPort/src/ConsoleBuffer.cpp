
#include "ConsoleBuffer.h"

ConsoleBuffer::ConsoleBuffer() : _width(0)
{
}

void ConsoleBuffer::SetSize(unsigned int width, unsigned int height, uint64_t attributes, COORD &cursor_pos)
{
	if (width==_width && (width*height)==_console_chars.size() )
		return;

	CHAR_INFO fill_ci{};
	CI_SET_WCATTR(fill_ci, L' ', attributes);
	ConsoleChars new_chars(size_t(height) * width, fill_ci);
	if (_width && !_console_chars.empty()) {
		size_t nc_cursor_offset = (size_t)-1;
		ConsoleChars unwrapped_chars;
		for (size_t y = 0, ymax = _console_chars.size() / _width; y < ymax; ++y) {
			size_t w = _width;
			for (;w > 0; --w) {
				const auto &ci = _console_chars[_width * y + (w - 1)];
				if ((ci.Char.UnicodeChar && ci.Char.UnicodeChar != L' ') || (ci.Attributes & EXPLICIT_LINE_BREAK) != 0) {
					break;
				}
			}
			if (w > 0) {
				if (w > 1) for (auto x = w - 1; x--; ) {
					_console_chars[_width * y + x].Attributes &= ~EXPLICIT_LINE_BREAK;
				}
				auto line_begin = _console_chars.begin() + y * _width;
				unwrapped_chars.insert(unwrapped_chars.end(), line_begin, line_begin + w);
				if ((size_t)cursor_pos.Y == y) {
					int cx = ((size_t)cursor_pos.X < w) ? cursor_pos.X : w - 1;
					nc_cursor_offset = unwrapped_chars.size() - w + cx;
				}
			}
		}
		bool cursor_pos_adjusted = false;
		size_t x, y;
		for (size_t i = x = y = 0; i != unwrapped_chars.size(); ++i) {
			size_t ofs = y * width + x;
			if (ofs >= new_chars.size()) {
				if (nc_cursor_offset != (size_t)-1) {
					if (nc_cursor_offset >= width) {
						nc_cursor_offset-= width;
					} else {
						nc_cursor_offset = 0;
					}
				} else if (cursor_pos.Y > 0) {
					cursor_pos.Y--;
				} else {
					fprintf(stderr, "ConsoleBuffer: cursor underflow\n");
				}
				ofs-= width;
				--y;
				if (nc_cursor_offset != (size_t)-1 && ofs == nc_cursor_offset) {
					nc_cursor_offset = (size_t)-1;
					cursor_pos.Y = y;
					cursor_pos.X = x;
					cursor_pos_adjusted = true;
				}
				if (scroll_callback.pfn) {
					scroll_callback.pfn(scroll_callback.context, con_handle, width, &new_chars[0]);
				}
				memmove(&new_chars[0], &new_chars[width], (new_chars.size() - width) * sizeof(CHAR_INFO));
				std::fill(new_chars.end() - width, new_chars.end(), fill_ci);
			}
			auto ci = unwrapped_chars[i];
			if ( (ci.Attributes & EXPLICIT_LINE_BREAK) != 0) {
				x = 0;
				++y;
			} else {
				++x;
				if (x == width) {
					x = 0;
					++y;
				}
			}
			new_chars[ofs] = ci;
		}

		if (!cursor_pos_adjusted) { // put it at beginning of 1st free line
			if (x > 0) {
				x = 0;
				++y;
			}
			cursor_pos.X = (x < width) ? x : width;
			cursor_pos.Y = (y < height) ? y : height;
			fprintf(stderr, "ConsoleBuffer: cursor defaulted at %d.%d screen %u.%u \n", cursor_pos.X, cursor_pos.Y, width, height);
		}
	}

	_console_chars.swap(new_chars);
	_width = width;
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

