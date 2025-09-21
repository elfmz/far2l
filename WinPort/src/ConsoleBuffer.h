#pragma once
#include <vector>
#include "WinCompat.h"
#include "WinPort.h"

class ConsoleBuffer
{
	typedef std::vector<CHAR_INFO> ConsoleChars;

	ConsoleChars _console_chars;
	unsigned int _width;

	CHAR_INFO *InspectCopyArea(const COORD &data_size, const COORD &data_pos, SMALL_RECT &screen_rect);

public:
	ConsoleBuffer(); 

	enum WriteResult {
		WR_BAD = 0,
		WR_SAME = 1,
		WR_MODIFIED = 2
	};

	void SetSize(unsigned int width, unsigned int height, uint64_t attributes, COORD &cursor_pos);
	void GetSize(unsigned int &width, unsigned int &height);
	inline unsigned int GetWidth() const { return _width; }

	void Read(CHAR_INFO *data, COORD data_size, COORD data_pos, SMALL_RECT &screen_rect);
	void Write(const CHAR_INFO *data, COORD data_size, COORD data_pos, SMALL_RECT &screen_rect);
	bool Read(CHAR_INFO &data, COORD screen_pos);
	WriteResult Write(const CHAR_INFO &data, COORD screen_pos);

	inline CHAR_INFO *DirectLineAccess(size_t line_index)
	{
		size_t offset = line_index * _width;
		if (offset >= _console_chars.size()) {
			return nullptr;
		}

		return &_console_chars[offset];
	}
};
