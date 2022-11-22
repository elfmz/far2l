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
#include <VT256ColorTable.h>
#include "TTYOutput.h"
#include "FarTTY.h"
#include "WideMB.h"
#include "WinPort.h"

#define ESC "\x1b"

#define ATTRIBUTES_AFFECTING_BACKGROUND \
	(BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED | BACKGROUND_INTENSITY | BACKGROUND_TRUECOLOR)

void TTYOutput::TrueColors::AppendSuffix(std::string &out, DWORD rgb)
{
	// first try to deduce 256-color palette index...
	if (_colors256_lookup.empty()) {
		static_assert(VT_256COLOR_TABLE_COUNT <= 0x100, "Too big VT_256COLOR_TABLE_COUNT");
		for (size_t i = 0; i < VT_256COLOR_TABLE_COUNT; ++i) {
			_colors256_lookup[g_VT256ColorTable[i]] = i;
		}
	}
	char buf[64];
	const auto &it = _colors256_lookup.find(rgb);
	if (it != _colors256_lookup.end()) {
		sprintf(buf, "5;%u;", ((unsigned int)it->second) + 16);
	} else {
		sprintf(buf, "2;%u;%u;%u;", rgb & 0xff, (rgb >> 8) & 0xff, (rgb >> 16) & 0xff);
	}
	out+= buf;
}

template <DWORD64 R, DWORD64 G, DWORD64 B>
	static void AppendAnsiColorSuffix(std::string &out, DWORD64 attr)
{
	char ch = '0';
	if (attr & R) ch+= 1;
	if (attr & G) ch+= 2;
	if (attr & B) ch+= 4;
	out+= ch;
	out+= ';';
}

void TTYOutput::WriteUpdatedAttributes(DWORD64 attr, bool is_space)
{
	const DWORD64 xa = _prev_attr_valid ? attr ^ _prev_attr : (DWORD64)-1;
	if (xa == 0) {
		return;
	}
	if (is_space && (xa & ATTRIBUTES_AFFECTING_BACKGROUND) == 0) {
		if ((attr & BACKGROUND_TRUECOLOR) == 0 || GET_RGB_BACK(xa) == 0) {
			if ( ((attr | _prev_attr) & (COMMON_LVB_REVERSE_VIDEO | COMMON_LVB_UNDERSCORE | COMMON_LVB_STRIKEOUT)) == 0) {
				return;
			}
		}
	}

	_tmp_attrs = ESC "[";
// wikipedia claims that colors 90-97 are nonstandard, so in case of some
// terminal missing '90–97 Set bright foreground color' - use bold font
	if (_kernel_tty && (xa & FOREGROUND_INTENSITY) != 0) {
		_tmp_attrs+= (attr & FOREGROUND_INTENSITY) ? "1;" : "22;";
	}

	bool emit_tc_fore =
		((attr & FOREGROUND_TRUECOLOR) != 0 && (GET_RGB_FORE(xa) != 0 || (xa & FOREGROUND_TRUECOLOR) != 0));

	bool emit_tc_back =
		((attr & BACKGROUND_TRUECOLOR) != 0 && (GET_RGB_BACK(xa) != 0 || (xa & BACKGROUND_TRUECOLOR) != 0));

	if ( ((xa & (FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY)) != 0)
	  || ((_prev_attr & FOREGROUND_TRUECOLOR) != 0 && (attr & FOREGROUND_TRUECOLOR) == 0) ) {
		_tmp_attrs+= (attr & FOREGROUND_INTENSITY) ? '9' : '3';
		AppendAnsiColorSuffix<FOREGROUND_RED, FOREGROUND_GREEN, FOREGROUND_BLUE>(_tmp_attrs, attr);
		if ((attr & FOREGROUND_TRUECOLOR) != 0) {
			emit_tc_fore = true;
		}
	}

	if ( ((xa & (BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED | BACKGROUND_INTENSITY)) != 0)
	  || ((_prev_attr & BACKGROUND_TRUECOLOR) != 0 && (attr & BACKGROUND_TRUECOLOR) == 0) ) {
		if (attr & BACKGROUND_INTENSITY) {
			_tmp_attrs+= "10";
		} else {
			_tmp_attrs+= '4';
		}
		AppendAnsiColorSuffix<BACKGROUND_RED, BACKGROUND_GREEN, BACKGROUND_BLUE>(_tmp_attrs, attr);
		if ((attr & BACKGROUND_TRUECOLOR) != 0) {
			emit_tc_back = true;
		}
	}

	if (emit_tc_fore) {
		_tmp_attrs+= "38;";
		_true_colors.AppendSuffix(_tmp_attrs, GET_RGB_FORE(attr));
	}

	if (emit_tc_back) {
		_tmp_attrs+= "48;";
		_true_colors.AppendSuffix(_tmp_attrs, GET_RGB_BACK(attr));
	}

	if ( (xa & COMMON_LVB_STRIKEOUT) != 0) {
		_tmp_attrs+= (attr & COMMON_LVB_STRIKEOUT) ? "9;" : "29;";
	}

	if ( (xa & COMMON_LVB_UNDERSCORE) != 0) {
		_tmp_attrs+= (attr & COMMON_LVB_UNDERSCORE) ? "4;" : "24;";
	}

	if ( (xa & COMMON_LVB_REVERSE_VIDEO) != 0) {
		_tmp_attrs+= (attr & COMMON_LVB_REVERSE_VIDEO) ? "7;" : "27;";
	}

	assert(!_tmp_attrs.empty() && _tmp_attrs.back() == ';');
	_tmp_attrs.back() = 'm';
	_prev_attr = attr;
	_prev_attr_valid = true;

	Write(_tmp_attrs.c_str(), _tmp_attrs.size());
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
	for (;cnt; ++ci,--cnt, ++_cursor.x) if (ci->Char.UnicodeChar) {
		const bool is_space = !CI_USING_COMPOSITE_CHAR(*ci) && (
				ci->Char.UnicodeChar <= 0x20
			|| (ci->Char.UnicodeChar >= 0x7f && ci->Char.UnicodeChar < 0xa0)
			|| !WCHAR_IS_VALID(ci->Char.UnicodeChar));

		WriteUpdatedAttributes(ci->Attributes, is_space);

		if (is_space) {
			WriteWChar(L' ');

		} else if (CI_USING_COMPOSITE_CHAR(*ci)) {
			for (const WCHAR *pw = WINPORT(CompositeCharLookup)(ci->Char.UnicodeChar); *pw; ++pw) {
				WriteWChar(*pw);
			}

		} else {
			WriteWChar(ci->Char.UnicodeChar);
		}
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

void TTYOutput::SendOSC52ClipSet(const std::string &clip_data)
{
	std::string request = ESC "]52;;";
	base64_encode(request, (const unsigned char *)clip_data.data(), clip_data.size());
	request+= '\a';
	Write(request.c_str(), request.size());
}
