#include <stdarg.h>
#include <assert.h>
#include <base64.h>
#include <string>
#include "TTYOutput.h"
#include "ConvertUTF.h"

TTYOutput::Attributes::Attributes(WORD attributes) :
	foreground_intensive( (attributes & FOREGROUND_INTENSITY) != 0),
	background_intensive( (attributes & BACKGROUND_INTENSITY) != 0),
	foreground(0), background(0)
{
	if (attributes&FOREGROUND_RED) foreground|= 1;
	if (attributes&FOREGROUND_GREEN) foreground|= 2;
	if (attributes&FOREGROUND_BLUE) foreground|= 4;

	if (attributes&BACKGROUND_RED) background|= 1;
	if (attributes&BACKGROUND_GREEN) background|= 2;
	if (attributes&BACKGROUND_BLUE) background|= 4;
}

bool TTYOutput::Attributes::operator ==(const Attributes &attr) const
{
	return (foreground_intensive == attr.foreground_intensive
		&& background_intensive == attr.background_intensive
		&& foreground == attr.foreground && background == attr.background);
}

///////////////////////

TTYOutput::TTYOutput(int out) : _out(out)
{
}

void TTYOutput::WriteReally(const char *str, int len)
{
	while (len > 0) {
		ssize_t wr = write(_out, str, len);
		if (wr <= 0) {
			throw std::runtime_error("TTYOutput::WriteReally: write");
		}
		len-= wr;
		str+= wr;
	}
}

void TTYOutput::Write(const char *str, int len)
{
	const size_t prev_size = _rawbuf.size();
	if (2 * len >= AUTO_FLUSH_THRESHOLD) {
		Flush();
		WriteReally(str, len);

	} else if (len > 0) {
		_rawbuf.resize(prev_size + len);
		memcpy(&_rawbuf[prev_size], str, len);
		if (_rawbuf.size() >= AUTO_FLUSH_THRESHOLD)
			Flush();
	}
}

void TTYOutput::Format(const char *fmt, ...)
{
	const size_t prev_size = _rawbuf.size();
	_rawbuf.resize(prev_size + strlen(fmt) + 0x40);
	for (;;) {
		va_list va;
		va_start(va, fmt);
		size_t append_limit = _rawbuf.size() - prev_size;
		int r = vsnprintf (&_rawbuf[prev_size], append_limit, fmt, va);
		va_end(va);
		if (r < 0) {
			_rawbuf.resize(prev_size);
			throw std::runtime_error("TTYOutput::Format: bad format");
		}
		if ((size_t)r < append_limit) {
			_rawbuf.resize(prev_size + r);
			break;
		}
		_rawbuf.resize(_rawbuf.size() + _rawbuf.size() / 2 + 0x100);
	}
	if (_rawbuf.size() >= AUTO_FLUSH_THRESHOLD)
		Flush();
}

void TTYOutput::Flush()
{
	if (!_rawbuf.empty()) {
		WriteReally(&_rawbuf[0], _rawbuf.size());
		_rawbuf.resize(0);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void TTYOutput::SetScreenBuffer(bool alternate)
{
	Format("\x1b[?1049%c", alternate ? 'h' : 'l');
}

void TTYOutput::ChangeCursor(bool visible, bool force)
{
	if (force || _cursor.visible != visible) {
		Format("\x1b[?25%c", visible ? 'h' : 'l');
		_cursor.visible = visible;
	}
}

void TTYOutput::MoveCursor(unsigned int y, unsigned int x, bool force)
{
	if (force || x != _cursor.x || y != _cursor.y) {
// ESC[#;#H Moves cursor to line #, column #
		Format("\x1b[%d;%dH", y, x);
		_cursor.x = x;
		_cursor.y = y;
	}
}

void TTYOutput::WriteLine(const CHAR_INFO *ci, unsigned int cnt)
{
	std::string tmp;
	for (;cnt; ++ci,--cnt) {
		Attributes attr(ci->Attributes);
		if (_attr != attr) {
			tmp = "\x1b[";
			if (_attr.foreground_intensive != attr.foreground_intensive)
				tmp+= attr.foreground_intensive ? "1;" : "22;";

//			if (_attr.underline != attr.underline)
//				tmp+= attr.underline ? "4;" : "24;";

			if (_attr.foreground != attr.foreground
			 || _attr.foreground_intensive != attr.foreground_intensive) {
				tmp+= attr.foreground_intensive ? '9' : '3';
				tmp+= '0' + attr.foreground;
				tmp+= ';';
			}
			if (_attr.background != attr.background
			 || _attr.background_intensive != attr.background_intensive) {
				if (attr.background_intensive) {
					tmp+= "10";
				} else
					tmp+= '4';
				tmp+= '0' + attr.background;
				tmp+= ';';
			}
			assert(tmp[tmp.size() - 1] == ';');
			tmp[tmp.size() - 1] = 'm';
			Write(tmp.c_str(), tmp.size());

			_attr = attr;
		}

		if (ci->Char.UnicodeChar <= 0x1f || ci->Char.UnicodeChar == ' ' || ci->Char.UnicodeChar == 0x7f) {
			Write(" ", 1);

		} else {
			UTF8 buf[16] = {};
			UTF8 *targetStart = &buf[0];

#if (__WCHAR_MAX__ > 0xffff)
			const UTF32* sourceStart = (const UTF32*)&ci->Char.UnicodeChar;

			if (ConvertUTF32toUTF8 (&sourceStart, sourceStart + 1, &targetStart,
				targetStart + sizeof(buf), lenientConversion) == conversionOK) {
				Write((const char *)&buf[0], targetStart - &buf[0]);
			} else {
				Write("?", 1);
			}

#else
			const UTF16* sourceStart = (const UTF16*)&ci->Char.UnicodeChar;

			if (ConvertUTF16toUTF8 (&sourceStart, sourceStart + 1, &targetStart,
				targetStart + sizeof(buf), lenientConversion) == conversionOK) {
				Write((const char *)&buf[0], targetStart - &buf[0]);
			} else {
				Write("?", 1);
			}
#endif
		}
		++_cursor.x;
	}
}

void TTYOutput::ChangeKeypad(bool app)
{
	Format("\x1b[?1%c", app ? 'h' : 'l');
}

void TTYOutput::ChangeMouse(bool enable)
{
	Format("\x1b[?1000%c", enable ? 'h' : 'l');
	Format("\x1b[?1001%c", enable ? 'h' : 'l');
	Format("\x1b[?1002%c", enable ? 'h' : 'l');
}

void TTYOutput::ChangeTitle(std::string title)
{
	for (auto &c : title) {
		if (c == '\x07') c = '_';
	}
	Format("\x1b]2;%s\x07", title.c_str());
}

void TTYOutput::SendFar2lInterract(const StackSerializer &stk_ser)
{
	std::string request = "\x1b_far2l:";
	request+= stk_ser.ToBase64();
	request+= '\x07';

	Write(request.c_str(), request.size());
}
