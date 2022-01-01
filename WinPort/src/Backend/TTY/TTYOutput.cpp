#include <stdarg.h>
#include <assert.h>
#include <base64.h>
#include <string>
#include <sys/ioctl.h>
#ifdef __linux__
# include <termios.h>
# include <linux/kd.h>
# include <linux/keyboard.h>
#elif defined(__FreeBSD__)
# include <sys/ioctl.h>
# include <sys/kbio.h>
#endif
#include <os_call.hpp>
#include "TTYOutput.h"
#include "FarTTY.h"
#include "WideMB.h"

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
	_out(out), _far2l_tty(far2l_tty), _kernel_tty(false)
{
#if defined(__linux__) || defined(__FreeBSD__)
	unsigned long int leds = 0;
	if (ioctl(out, KDGETLED, &leds) == 0) {
		// running under linux 'real' TTY, such kind of terminal cannot be dropped due to lost connection etc
		// also detachable session makes impossible using of ioctl(_stdin, TIOCLINUX, &state) in child (#653),
		// so lets default to mortal mode in Linux/BSD TTY
		_kernel_tty = true;
	}
#endif

	Format(ESC "7" ESC "[?47h" ESC "[?1049h");
	ChangeKeypad(true);
	ChangeMouse(true);

	if (far2l_tty) {
		StackSerializer stk_ser;
		stk_ser.PushPOD((uint64_t)(FARTTY_FEAT_COMPACT_INPUT));
		stk_ser.PushPOD(FARTTY_INTERRACT_CHOOSE_EXTRA_FEATURES);
		stk_ser.PushPOD((uint8_t)0); // zero ID means not expecting reply
		SendFar2lInterract(stk_ser);
	}
	Flush();
}

TTYOutput::~TTYOutput()
{
	try {
		ChangeCursor(true, true);
		ChangeMouse(false);
		ChangeKeypad(false);
		if (!_kernel_tty) {
			Format(ESC "[0 q");
		}
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

	if (_same_chars.wch >= 0x80) {
		_same_chars.tmp.clear();
		Wide2MB_UnescapedAppend(_same_chars.wch, _same_chars.tmp);
	} else {
		_same_chars.tmp = (char)(unsigned char)_same_chars.wch;
	}

	// When have queued enough count of same characters:
	// - Use repeat last char sequence when (#925 #929) terminal is far2l that definately supports it
	// - Under other terminals and if repeated char is space - use erase chars + move cursor forward
	// - Otherwise just output copies of repeated char sequence
	if (_same_chars.count <= 5
			|| (!_far2l_tty && (_same_chars.wch != L' ' || _same_chars.count <= 8))) {

		// output plain <count> copies of repeated char sequence
		_rawbuf.reserve(_rawbuf.size() + _same_chars.tmp.size() * _same_chars.count);
		do {
			_rawbuf.insert(_rawbuf.end(), _same_chars.tmp.begin(), _same_chars.tmp.end());
		} while (--_same_chars.count);

	} else {
		char sz[32];
		int sz_len;
		if (_far2l_tty) {
			_rawbuf.insert(_rawbuf.end(), _same_chars.tmp.begin(), _same_chars.tmp.end());
			sz_len = sprintf(sz, // repeat last character <count-1> times
				ESC "[%ub", _same_chars.count - 1);
		} else {
			sz_len = sprintf(sz, // erase <count> chars and move cursor forward by <count>
				ESC "[%uX" ESC "[%uC", _same_chars.count, _same_chars.count);
		}
		if (sz_len >= 0) {
			assert(size_t(sz_len) <= ARRAYSIZE(sz));
			_rawbuf.insert(_rawbuf.end(), &sz[0], &sz[sz_len]);
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

void TTYOutput::ChangeCursorHeight(unsigned int height)
{
	if (_far2l_tty) {
		StackSerializer stk_ser;
		stk_ser.PushPOD(UCHAR(height));
		stk_ser.PushPOD(FARTTY_INTERRACT_SET_CURSOR_HEIGHT);
		stk_ser.PushPOD((uint8_t)0); // zero ID means not expecting reply
		SendFar2lInterract(stk_ser);

	} else if (_kernel_tty) {
		; // avoid printing 'q' on screen

	} else if (height < 30) {
		Format(ESC "[3 q"); // Blink Underline

	} else {
		Format(ESC "[0 q"); // Blink Block (Default)
	}
}

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
		if (x == 1) {
			Write(ESC "[G", 3);
		} else {
			Format(ESC "[%uG", x);
		}
		_cursor.x = x;

	} else if (y != _cursor.y) {
		if (y == 1) {
			Write(ESC "[d", 3);
		} else {
			Format(ESC "[%ud", y);
		}
		_cursor.y = y;
	}
}

int TTYOutput::WeightOfHorizontalMoveCursor(unsigned int y, unsigned int x) const
{
	if (_cursor.y != y) {
		return -1;
	}

	if (_cursor.x == x) {
		return 0;
	}

	if (x == 1) {
		return 3;// Write(ESC "[G", 3);
	}

	// Format(ESC "[%uG", x);
	if (x < 10) {
		return 4;
	}
	if (x < 100) {
		return 5;
	}
	return 6;
}

void TTYOutput::WriteLine(const CHAR_INFO *ci, unsigned int cnt)
{
	std::string tmp;
	for (;cnt; ++ci,--cnt) {
		const bool is_space = (ci->Char.UnicodeChar <= 0x20
			|| (ci->Char.UnicodeChar >= 0x7f && ci->Char.UnicodeChar < 0xa0)
			|| !WCHAR_IS_VALID(ci->Char.UnicodeChar));

		Attributes attr(ci->Attributes);
		if (_attr != attr && (!is_space || _attr.background != attr.background
				|| _attr.background_intensive != attr.background_intensive) ) {

			tmp = ESC "[";
// wikipedia claims that colors 90-97 are nonstandard, so in case of some
// terminal missing '90–97 Set bright foreground color' - use bold font
			if (_kernel_tty && _attr.foreground_intensive != attr.foreground_intensive)
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
