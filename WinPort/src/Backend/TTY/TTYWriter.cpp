#include <stdarg.h>
#include "TTYWriter.h"
#include "ConvertUTF.h"

bool TTYWriter::WriteRaw(const char *str, int len)
{
	for (size_t ofs = 0; len > 0;) {
		ssize_t wr = write(1, str, len);
		if (wr <= 0) {
			perror("TTYWriter::WriteRaw: write");
			return false;
		}
		len-= wr;
		ofs+= wr;
	}
	return true;
}

bool TTYWriter::FormatRaw(const char *fmt, ...)
{
	for (;;) {
		va_list va;
		va_start(va, fmt);
		int r = vsnprintf (&_rawbuf[0], _rawbuf.size(), fmt, va);
		va_end(va);
		if (r < 0) {
			fprintf(stderr, "TTYWriter::FormatRaw: bad format - '%s'\n", fmt);
			return false;
		}
		if (r < _rawbuf.size()) {
			return WriteRaw(&_rawbuf[0], r);
		}
		_rawbuf.resize(_rawbuf.size() + _rawbuf.size() / 2 + 0x100);
	}
}

bool TTYWriter::MoveCursor(unsigned int y, unsigned int x, bool force)
{
	if (force || x != _x || y != _y) {
// ESC[#;#H Moves cursor to line #, column #
		if (!FormatRaw("\x1b[%d;%dH", y, x))
			return false;
		_x = x;
		_y = y;
	}
	return true;
}

bool TTYWriter::WriteLine(const CHAR_INFO *ci, unsigned int cnt)
{
	for (;cnt; ++ci,--cnt) {
		if (ci->Char.UnicodeChar == 0 || ci->Char.UnicodeChar == ' ') {
			if (!WriteRaw(" ", 1))
				return false;
		} else if (ci->Char.UnicodeChar == 0x1b) {
			if (!WriteRaw("\x1b\x1b", 2))
				return false;

		} else {
			UTF8 buf[16] = {};
			const UTF32* sourceStart = (const UTF32*)&ci->Char.UnicodeChar;
			UTF8 *targetStart = &buf[0];

			if (ConvertUTF32toUTF8 (&sourceStart, sourceStart + 1, &targetStart,
				targetStart + sizeof(buf), lenientConversion) == conversionOK) {
				if (!WriteRaw((const char *)&buf[0], targetStart - &buf[0]))
					return false;
			} else {
				if (!WriteRaw("?", 1))
					return false;
			}
		}
		++_x;
	}
	return true;
}
