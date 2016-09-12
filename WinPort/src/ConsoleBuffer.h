#pragma once
#include <vector>
#include "WinCompat.h"
#include "WinPort.h"

class ConsoleBuffer
{
	struct ConsoleChars : std::vector<CHAR_INFO> {} _console_chars;

	unsigned int _width;

	CHAR_INFO *InspectCopyArea(const COORD &data_size, const COORD &data_pos, SMALL_RECT &screen_rect);
public:
	ConsoleBuffer(); 
	enum WriteResult {
		WR_BAD = 0,
		WR_SAME = 1,
		WR_MODIFIED = 2
	};

	void SetSize(unsigned int width, unsigned int height, unsigned short attributes);
	void GetSize(unsigned int &width, unsigned int &height);

	void Read(CHAR_INFO *data, COORD data_size, COORD data_pos, SMALL_RECT &screen_rect);
	void Write(const CHAR_INFO *data, COORD data_size, COORD data_pos, SMALL_RECT &screen_rect);
	bool Read(CHAR_INFO &data, COORD screen_pos);
	WriteResult Write(const CHAR_INFO &data, COORD screen_pos);
};
