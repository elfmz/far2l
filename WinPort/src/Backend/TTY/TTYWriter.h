#pragma once
#include <vector>
#include <WinCompat.h>

class TTYWriter
{
	unsigned int _y = -1, _x = -1;
	struct Attributes
	{
		Attributes() = default;
		Attributes(const Attributes &) = default;
		Attributes(WORD attributes);

		bool bold = false;
		bool underline = false;
		unsigned char foreground = -1;
		unsigned char background = -1;

		bool operator ==(const Attributes &attr) const;
		bool operator !=(const Attributes &attr) const {return !(operator ==(attr)); }
	} _attr;

	std::vector<char> _rawbuf;
	bool WriteRaw(const char *str, int len);
	bool FormatRaw(const char *fmt, ...);
public:

	bool MoveCursor(unsigned int y, unsigned int x, bool force = false);
	bool WriteLine(const CHAR_INFO *ci, unsigned int cnt);

};
