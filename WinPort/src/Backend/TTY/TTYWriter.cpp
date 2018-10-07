#include <stdarg.h>
#include <assert.h>
#include <string>
#include "TTYWriter.h"
#include "ConvertUTF.h"

TTYWriter::Attributes::Attributes(WORD attributes) :
	bold ( (attributes & FOREGROUND_INTENSITY) != 0),
	underline( (attributes & BACKGROUND_INTENSITY) != 0),
	foreground(0), background(0)
{
	if (attributes&FOREGROUND_RED) foreground|= 1;
	if (attributes&FOREGROUND_GREEN) foreground|= 2;
	if (attributes&FOREGROUND_BLUE) foreground|= 4;

	if (attributes&BACKGROUND_RED) background|= 1;
	if (attributes&BACKGROUND_GREEN) background|= 2;
	if (attributes&BACKGROUND_BLUE) background|= 4;
}

bool TTYWriter::Attributes::operator ==(const Attributes &attr) const
{
	return (bold == attr.bold && underline == attr.underline
		&& foreground == attr.foreground && background == attr.background);
}

///////////////////////

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
	std::string tmp;
	for (;cnt; ++ci,--cnt) {
		Attributes attr(ci->Attributes);
		if (_attr != attr) {
			tmp = "\x1b[";
			if (_attr.bold != attr.bold)
				tmp+= attr.bold ? "1;" : "22;";
			if (_attr.underline != attr.underline)
				tmp+= attr.bold ? "4;" : "24;";
			if (_attr.foreground != attr.foreground) {
				tmp+= '3';
				tmp+= '0' + attr.foreground;
				tmp+= ';';
			}
			if (_attr.background != attr.background) {
				tmp+= '4';
				tmp+= '0' + attr.background;
				tmp+= ';';
			}
			assert(tmp[tmp.size() - 1] == ';');
			tmp[tmp.size() - 1] = 'm';
			if (!WriteRaw(tmp.c_str(), tmp.size()))
				return false;

			_attr = attr;
		}

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
