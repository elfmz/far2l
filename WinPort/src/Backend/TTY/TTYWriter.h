#pragma once
#include <vector>
#include <WinCompat.h>

class TTYWriter
{
	unsigned int _y = -1, _x = -1;

	std::vector<char> _rawbuf;
	bool WriteRaw(const char *str, int len);
	bool FormatRaw(const char *fmt, ...);
public:

	bool MoveCursor(unsigned int y, unsigned int x, bool force = false);
	bool WriteLine(const CHAR_INFO *ci, unsigned int cnt);

};
