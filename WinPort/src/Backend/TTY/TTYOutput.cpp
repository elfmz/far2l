#include <stdarg.h>
#include <assert.h>
#include <base64.h>
#include <string>
#include <os_call.hpp>
#include "TTYOutput.h"
#include "ConvertUTF.h"

#define ESC "\x1b"

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

TTYOutput::TTYOutput(int out, bool far2l_tty)
	:
	_out(out), _far2l_tty(far2l_tty)
{
	Format(ESC "7" ESC "[?47h" ESC "[?1049h");
	ChangeKeypad(true);
	ChangeMouse(true);
	Flush();
}

TTYOutput::~TTYOutput()
{
	try {
		ChangeCursor(true, 13);
		ChangeMouse(false);
		ChangeKeypad(false);
		Format(ESC "[0m" ESC "[?1049l" ESC "[?47l" ESC "8" "\r\n");
		Flush();

	} catch (std::exception &) {
	}
}

void TTYOutput::WriteReally(const char *str, int len)
{
	while (len > 0) {
		ssize_t wr = os_call_ssize(write, _out, (const void*)str, (size_t)len);
		if (wr <= 0) {
			throw std::runtime_error("TTYOutput::WriteReally: write");
		}
		len-= wr;
		str+= wr;
	}
}

void TTYOutput::FinalizeSameChars()
{
	if (!_same_chars.count) {
		return;
	}

	char buf[64];
	int len = 1;

	if (_same_chars.wch >= 0x80) {
		UTF8 *dst = (UTF8 *)&buf[0];
#if (__WCHAR_MAX__ > 0xffff)
		const UTF32* src = (const UTF32*)&_same_chars.wch;
		if (ConvertUTF32toUTF8 (&src, src + 1, &dst,
				dst + ARRAYSIZE(buf), lenientConversion) == conversionOK) {
#else
		const UTF16* src = (const UTF16*)&_same_chars.wch;
		if (ConvertUTF16toUTF8 (&src, src + 1, &dst,
				dst + ARRAYSIZE(buf), lenientConversion) == conversionOK) {
#endif
			len = (int)(dst - (UTF8 *)&buf[0]);
			assert(len <= ARRAYSIZE(buf));
		} else {
			buf[0] = '?';
		}
	} else {
		buf[0] = (char)(unsigned char)_same_chars.wch;
	}

	// When have queued enough count of same characters:
	// - Use repeat last char sequence when (#925 #929) terminal is far2l that definately supports it
	// - Under other terminals and if repeated char is space - use erase chars + move cursor forward
	// - Otherwise just output copies of repeated char sequence
	if (_same_chars.count <= 5
			|| (!_far2l_tty && (_same_chars.wch != L' ' || _same_chars.count <= 8))) {
		// output plain <count> copies of repeated char sequence
		_rawbuf.reserve(_rawbuf.size() + len * _same_chars.count);
		do {
			_rawbuf.insert(_rawbuf.end(), &buf[0], &buf[len]);
		} while (--_same_chars.count);

	} else {
		if (_far2l_tty) {
			_rawbuf.insert(_rawbuf.end(), &buf[0], &buf[len]);
			len = sprintf(buf, // repeat last character <count-1> times
				ESC "[%ub", _same_chars.count - 1);
		} else {
			len = sprintf(buf, // erase <count> chars and move cursor forward by <count>
				ESC "[%uX" ESC "[%uC", _same_chars.count, _same_chars.count);
		}
		if (len >= 0) {
			assert(len <= ARRAYSIZE(buf));
			_rawbuf.insert(_rawbuf.end(), &buf[0], &buf[len]);
		}
		_same_chars.count = 0;
	}
}

void TTYOutput::WriteWChar(WCHAR wch)
{
	if (_same_chars.count == 0) {
		_same_chars.wch = wch;

	} else if (_same_chars.wch != wch) {
		FinalizeSameChars();
		_same_chars.wch = wch;
	}
	_same_chars.count++;
}

void TTYOutput::Write(const char *str, int len)
{
	if (len > 0) {
		FinalizeSameChars();
		_rawbuf.insert(_rawbuf.end(), str, str + len);
	}
}

void TTYOutput::Format(const char *fmt, ...)
{
	FinalizeSameChars();

	char tmp[0x100];

	va_list args, args_copy;
	va_start(args, fmt);

	va_copy(args_copy, args);
	int r = vsnprintf(tmp, sizeof(tmp), fmt, args_copy);
	va_end(args_copy);

	if (r >= (int)sizeof(tmp)) {
		const size_t prev_size = _rawbuf.size();
		_rawbuf.resize(prev_size + r + 0x10);
		size_t append_limit = _rawbuf.size() - prev_size;

		va_copy(args_copy, args);
		r = vsnprintf(&_rawbuf[prev_size], append_limit, fmt, args_copy);
		va_end(args_copy);

		if (r > 0) {
			_rawbuf.resize(prev_size + std::min(append_limit, (size_t)r));
		} else {
			_rawbuf.resize(prev_size);
		}

	} else if (r > 0) {
		_rawbuf.insert(_rawbuf.end(), &tmp[0], &tmp[r]);
	}

	va_end(args);

	if (r < 0) {
		throw std::runtime_error("TTYOutput::Format: bad format");
	}
}

void TTYOutput::Flush()
{
	FinalizeSameChars();
	if (!_rawbuf.empty()) {
		WriteReally(&_rawbuf[0], _rawbuf.size());
		_rawbuf.resize(0);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void TTYOutput::ChangeCursor(bool visible, bool force)
{
	if (force || _cursor.visible != visible) {
		Format(ESC "[?25%c", visible ? 'h' : 'l');
		_cursor.visible = visible;
	}
}

void TTYOutput::MoveCursorStrict(unsigned int y, unsigned int x)
{
// ESC[#;#H Moves cursor to line #, column #
	if (x == 1) {
		if (y == 1) {
			Write(ESC "[H", 3);
		} else {
			Format(ESC "[%dH", y);
		}
	} else {
		Format(ESC "[%d;%dH", y, x);
	}
	_cursor.x = x;
	_cursor.y = y;
}

void TTYOutput::MoveCursorLazy(unsigned int y, unsigned int x)
{
	if (_cursor.y != y && _cursor.x != x) {
		MoveCursorStrict(y, x);

	} else if (x != _cursor.x) {
		if (x != 1) {
			Format(ESC "[%uG", x);
		} else {
			Write(ESC "[G", 3);
		}
		_cursor.x = x;

	} else if (y != _cursor.y) {
		if (y != 1) {
			Format(ESC "[%ud", y);
		} else {
			Write(ESC "[d", 3);
		}
		_cursor.y = y;
	}
}

void TTYOutput::WriteLine(const CHAR_INFO *ci, unsigned int cnt)
{
	std::string tmp;
	for (;cnt; ++ci,--cnt) {
		const bool is_space = (ci->Char.UnicodeChar <= 0x1f
			|| ci->Char.UnicodeChar == ' ' || ci->Char.UnicodeChar == 0x7f);

		Attributes attr(ci->Attributes);
		if (_attr != attr && (!is_space || _attr.background != attr.background
				|| _attr.background_intensive != attr.background_intensive) ) {

			tmp = ESC "[";
			if (_attr.foreground_intensive != attr.foreground_intensive)
				tmp+= attr.foreground_intensive ? "1;" : "22;";

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

		WriteWChar(is_space ? L' ' : ci->Char.UnicodeChar);
		++_cursor.x;
	}
}

void TTYOutput::ChangeKeypad(bool app)
{
	Format(ESC "[?1%c", app ? 'h' : 'l');
}

void TTYOutput::ChangeMouse(bool enable)
{
	Format(ESC "[?1000%c", enable ? 'h' : 'l');
	Format(ESC "[?1001%c", enable ? 'h' : 'l');
	Format(ESC "[?1002%c", enable ? 'h' : 'l');
}

void TTYOutput::ChangeTitle(std::string title)
{
	for (auto &c : title) {
		if (c == '\x07') c = '_';
	}
	Format(ESC "]2;%s\x07", title.c_str());
}

void TTYOutput::SendFar2lInterract(const StackSerializer &stk_ser)
{
	std::string request = ESC "_far2l:";
	request+= stk_ser.ToBase64();
	request+= '\x07';

	Write(request.c_str(), request.size());
}
