#include <stdarg.h>
#include <string>
#include <base64.h>
#include "TTYInputSequenceParser.h"
#include "ConsoleInput.h"
#include "WinPort.h"


//See:
// http://www.manmrk.net/tutorials/ISPF/XE/xehelp/html/HID00000579.htm
// http://www.leonerd.org.uk/hacks/fixterms/

extern ConsoleInput g_winport_con_in;

static bool IsEnhancedKey(WORD code)
{
	return (code==VK_LEFT || code==VK_RIGHT || code==VK_UP || code==VK_DOWN
		|| code==VK_HOME || code==VK_END || code==VK_NEXT || code==VK_PRIOR );
}



#if 1 // change to 1 to enable self-contradiction check on startup

template <typename Last> static void AssertNoConflictsBetween(const Last &last) { }

template <typename First, typename Second, class ... Types>
	static void AssertNoConflictsBetween(const First &first, const Second &second, Types ... others)
{
	for (const auto &f_i : first) {
		for (const auto &s_i : second) {
			if (memcmp(f_i.first.chars, s_i.first.chars,
				std::min(sizeof(f_i.first.chars), sizeof(s_i.first.chars)) ) == 0) {
				fprintf(stderr, "AssertNoConflictsBetween: '%s' vs '%s'\n",
					std::string(f_i.first.chars, sizeof(f_i.first.chars)).c_str(),
					std::string(s_i.first.chars, sizeof(s_i.first.chars)).c_str());
				abort();
			}
		}
	}

	AssertNoConflictsBetween(second, others...);
}


void TTYInputSequenceParser::AssertNoConflicts()
{
	AssertNoConflictsBetween(_nch2key1, _nch2key2, _nch2key3,
		_nch2key4, _nch2key5, _nch2key6, _nch2key7, _nch2key8);
}

#else
void TTYInputSequenceParser::AssertNoConflicts()
{
}
#endif


void TTYInputSequenceParser::AddStr(WORD vk, DWORD control_keys, const char *fmt, ...)
{
	char tmp[32];
	va_list va;
	va_start(va, fmt);
	int r = vsnprintf (&tmp[0], sizeof(tmp), fmt, va);
	va_end(va);

	fprintf(stderr, "TTYInputSequenceParser::AddStr(0x%x, 0x%x, '%s'): '%s' r=%d\n", vk, control_keys, fmt, tmp, r);

	TTYInputKey k = {vk, control_keys};
	switch (r) {
		case 1: _nch2key1.Add(tmp, k); break;
		case 2: _nch2key2.Add(tmp, k); break;
		case 3: _nch2key3.Add(tmp, k); break;
		case 4: _nch2key4.Add(tmp, k); break;
		case 5: _nch2key5.Add(tmp, k); break;
		case 6: _nch2key6.Add(tmp, k); break;
		case 7: _nch2key7.Add(tmp, k); break;
		case 8: _nch2key8.Add(tmp, k); break;
		default:
			fprintf(stderr,
				"TTYInputSequenceParser::AddStr(0x%x, 0x%x, '%s') - unexpected: %d\n",
				vk, control_keys, fmt, r);
			abort();
	}
}

void TTYInputSequenceParser::AddStrTilde(WORD vk, int code)
{
	AddStr(vk, 0, "[%d~", code);
	for (int i = 0; i <= 7; ++i) {
		DWORD control_keys = 0;
		if (i & 1) control_keys|= SHIFT_PRESSED;
		if (i & 2) control_keys|= LEFT_ALT_PRESSED;
		if (i & 4) control_keys|= LEFT_CTRL_PRESSED;
		AddStr(vk, control_keys, "[%d;%d~", code, 1 + i);
	}
}

void TTYInputSequenceParser::AddStr_ControlsThenCode(WORD vk, const char *fmt, const char *code)
{
	for (int i = 0; i <= 7; ++i) {
		DWORD control_keys = 0;
		if (i & 1) control_keys|= SHIFT_PRESSED;
		if (i & 2) control_keys|= LEFT_ALT_PRESSED;
		if (i & 4) control_keys|= LEFT_CTRL_PRESSED;
		AddStr(vk, control_keys, fmt, 1 + i, code);
	}
}

void TTYInputSequenceParser::AddStr_CodeThenControls(WORD vk, const char *fmt, const char *code)
{
	for (int i = 0; i <= 7; ++i) {
		DWORD control_keys = 0;
		if (i & 1) control_keys|= SHIFT_PRESSED;
		if (i & 2) control_keys|= LEFT_ALT_PRESSED;
		if (i & 4) control_keys|= LEFT_CTRL_PRESSED;
		AddStr(vk, control_keys, fmt, code, 1 + i);
	}
}

void TTYInputSequenceParser::AddStrF1F5(WORD vk, const char *code)
{
	AddStr(vk, 0, "O%s", code);
	AddStr_ControlsThenCode(vk, "O%d%s", code);
	AddStr_ControlsThenCode(vk, "[1;%d%s", code);
}

void TTYInputSequenceParser::AddStrCursors(WORD vk, const char *code)
{
	AddStr(vk, 0, "O%s", code);
	AddStr(vk, 0, "[%s", code);
	AddStr_ControlsThenCode(vk, "[%d%s", code);
	AddStr_ControlsThenCode(vk, "[1;%d%s", code);
}

TTYInputSequenceParser::TTYInputSequenceParser(ITTYInputSpecialSequenceHandler *handler)
	: _handler(handler)
{
	for (char c = 'A'; c <= 'Z'; ++c) {
		AddStr(c, LEFT_ALT_PRESSED, "%c", c + ('a' - 'A'));
		if (c != 'O') {
			AddStr(c, LEFT_ALT_PRESSED | SHIFT_PRESSED, "%c", c);
		}
	}
	for (char c = '0'; c <= '9'; ++c) {
		AddStr(c, LEFT_ALT_PRESSED, "%c", c);
	}
	AddStr('C', LEFT_CTRL_PRESSED | LEFT_ALT_PRESSED, "\x03");

	AddStr('0', LEFT_ALT_PRESSED | SHIFT_PRESSED, ")");
	AddStr('1', LEFT_ALT_PRESSED | SHIFT_PRESSED, "!");
	AddStr('2', LEFT_ALT_PRESSED | SHIFT_PRESSED, "@");
	AddStr('3', LEFT_ALT_PRESSED | SHIFT_PRESSED, "#");
	AddStr('4', LEFT_ALT_PRESSED | SHIFT_PRESSED, "$");
	AddStr('5', LEFT_ALT_PRESSED | SHIFT_PRESSED, "%%");
	AddStr('6', LEFT_ALT_PRESSED | SHIFT_PRESSED, "^");
	AddStr('7', LEFT_ALT_PRESSED | SHIFT_PRESSED, "&");
	AddStr('8', LEFT_ALT_PRESSED | SHIFT_PRESSED, "*");
	AddStr('9', LEFT_ALT_PRESSED | SHIFT_PRESSED, "(");
	AddStr(VK_OEM_PLUS, LEFT_ALT_PRESSED, "=");
	AddStr(VK_OEM_PLUS, LEFT_ALT_PRESSED | SHIFT_PRESSED, "+");
	AddStr(VK_OEM_MINUS, LEFT_ALT_PRESSED, "-");
	AddStr(VK_TAB, LEFT_ALT_PRESSED, "\x08");
	AddStr(VK_CLEAR, LEFT_ALT_PRESSED, "\x7f");
	AddStr(VK_DIVIDE, LEFT_ALT_PRESSED, "/");
	AddStr(VK_OEM_PERIOD, LEFT_ALT_PRESSED, ".");
	AddStr(VK_OEM_COMMA, LEFT_ALT_PRESSED, ",");

	AddStr(VK_OEM_1, LEFT_ALT_PRESSED, ";");
	AddStr(VK_OEM_3, LEFT_ALT_PRESSED, "`");
	//AddStr(VK_OEM_4, LEFT_ALT_PRESSED, "[");
	AddStr(VK_OEM_5, LEFT_ALT_PRESSED, "\\");
	AddStr(VK_OEM_6, LEFT_ALT_PRESSED, "]");
	AddStr(VK_OEM_7, LEFT_ALT_PRESSED, "\'");





	//AddStr(VK_DIVIDE, LEFT_ALT_PRESSED, "/");
	//AddStr(VK_ADD, LEFT_ALT_PRESSED | SHIFT_PRESSED, "_", c);


	AddStrCursors(VK_UP, "A");
	AddStrCursors(VK_DOWN, "B");
	AddStrCursors(VK_RIGHT, "C");
	AddStrCursors(VK_LEFT, "D");
	AddStrCursors(VK_END, "F");
	AddStrCursors(VK_HOME, "H");
	AddStrF1F5(VK_F1, "P"); AddStr(VK_F1, 0, "[[A");
	AddStrF1F5(VK_F2, "Q"); AddStr(VK_F2, 0, "[[B");
	AddStrF1F5(VK_F3, "R"); AddStr(VK_F3, 0, "[[C");
	AddStrF1F5(VK_F4, "S"); AddStr(VK_F4, 0, "[[D");
	AddStrF1F5(VK_F5, "E"); AddStr(VK_F5, 0, "[[E");

	AddStrTilde(VK_HOME, 1);
	AddStrTilde(VK_INSERT, 2);
	AddStrTilde(VK_DELETE, 3);
	AddStrTilde(VK_END, 4);
	AddStrTilde(VK_PRIOR, 5);
	AddStrTilde(VK_NEXT, 6);

	AddStrTilde(VK_F1, 11);	AddStr(VK_F1, SHIFT_PRESSED, "[25~");
	AddStrTilde(VK_F2, 12); AddStr(VK_F2, SHIFT_PRESSED, "[26~");
	AddStrTilde(VK_F3, 13); AddStr(VK_F3, SHIFT_PRESSED, "[28~");
	AddStrTilde(VK_F4, 14); AddStr(VK_F4, SHIFT_PRESSED, "[29~");
	AddStrTilde(VK_F5, 15); AddStr(VK_F5, SHIFT_PRESSED, "[31~");
	AddStrTilde(VK_F6, 17); AddStr(VK_F6, SHIFT_PRESSED, "[32~");
	AddStrTilde(VK_F7, 18); AddStr(VK_F7, SHIFT_PRESSED, "[33~");
	AddStrTilde(VK_F8, 19); AddStr(VK_F8, SHIFT_PRESSED, "[34~");
	AddStrTilde(VK_F9, 20);
	AddStrTilde(VK_F10, 21);
	AddStrTilde(VK_F11, 23);
	AddStrTilde(VK_F12, 24);

	AddStr(VK_ESCAPE, 0, "\x1b");

	AddStr(VK_SPACE, LEFT_ALT_PRESSED, " ");
	AddStr(VK_TAB, SHIFT_PRESSED, "[Z");

	AssertNoConflicts();
}

size_t TTYInputSequenceParser::ParseNChars2Key(const char *s, size_t l)
{
	TTYInputKey k;
	switch (l >= 8 ? 8 : l) {
		case 8: if (_nch2key8.Lookup(s, k)) {
				PostKeyEvent(k);
				return 8;
			}

		case 7: if (_nch2key7.Lookup(s, k)) {
				PostKeyEvent(k);
				return 7;
			}

		case 6: if (_nch2key6.Lookup(s, k)) {
				PostKeyEvent(k);
				return 6;
			}

		case 5: if (_nch2key5.Lookup(s, k)) {
				PostKeyEvent(k);
				return 5;
			}

		case 4: if (_nch2key4.Lookup(s, k)) {
				PostKeyEvent(k);
				return 4;
			}

		case 3: if (_nch2key3.Lookup(s, k)) {
				PostKeyEvent(k);
				return 3;
			}

		case 2: if (_nch2key2.Lookup(s, k)) {
				PostKeyEvent(k);
				return 2;
			}

		case 1: if (_nch2key1.Lookup(s, k)) {
				PostKeyEvent(k);
				return 1;
			}

		default: ;
	}

	return 0;
}

void TTYInputSequenceParser::ParseAPC(const char *s, size_t l)
{
	if (!_handler)
		return;

	if (strncmp(s, "f2l", 3) == 0) {
		_tmp_stk_ser.FromBase64(s + 3, l - 3);
		_handler->OnFar2lEvent(_tmp_stk_ser);

	} else if (strncmp(s, "far2l", 5) == 0) {
		_tmp_stk_ser.FromBase64(s + 5, l - 5);
		_handler->OnFar2lReply(_tmp_stk_ser);
	}
}

size_t TTYInputSequenceParser::Parse(const char *s, size_t l)
{
	switch (*s) {
		case 0x1b: {
			++s;
			--l;

			if (l > 2 && s[0] == '[' && s[2] == 'n') {
				return 4;
			}

			if (l > 0 && s[0] == '_') {
				for (size_t i = 1; i < l; ++i) {
					if (s[i] == '\x07') {
						ParseAPC(s + 1, i - 1);
						return i + 2;
					}
				}
				return 0;
			}

			if (l > 1 && s[0] == '[' && s[1] == 'M') { // mouse report: "\x1b[MAYX"
				if (l < 5)
					return 0;

				ParseMouse(s[2], s[3], s[4]);
				return 6;
			}

			size_t r = ParseNChars2Key(s, l);
			if (r != 0)
				return r + 1;

			// be well-responsive on panic-escaping
			for (size_t i = 0; (i + 1) < l; ++i) {
				if (s[i] == 0x1b && s[i + 1] == 0x1b) {
					return i + 1;
				}
			}

			return (l >= 8) ? (size_t)-2 : 0;
		}

		case 0x00:
			PostKeyEvent(TTYInputKey{VK_SPACE, LEFT_CTRL_PRESSED});
			return 1;

		case 0x01: case 0x02: case 0x03: case 0x04: case 0x05: case 0x06: case 0x07: case 0x08:
		case 0x0a: case 0x0b: case 0x0c: case 0x0e: case 0x0f: case 0x10: case 0x11: case 0x12:
		case 0x13: case 0x14: case 0x15: case 0x16: case 0x17: case 0x18: case 0x19: case 0x1a:
			PostKeyEvent(TTYInputKey{WORD('A' + (*s - 0x01)), LEFT_CTRL_PRESSED});
			return 1;

		case 0x09:
			PostKeyEvent(TTYInputKey{VK_TAB, 0});
			return 1;

		case 0x0d:
			PostKeyEvent(TTYInputKey{VK_RETURN, 0});
			return 1;

//		case 0x1b:
//			PostKeyEvent(VK_OEM_4, 0);
//			return 1;

		case 0x1c:
			PostKeyEvent(TTYInputKey{VK_OEM_5, 0});
			return 1;

		case 0x1d:
			PostKeyEvent(TTYInputKey{VK_OEM_6, 0});
			return 1;


		case 0x7f:
			PostKeyEvent(TTYInputKey{VK_BACK, 0});
			return 1;

		default:
			return (size_t)-1;
	}

	abort();
}


void TTYInputSequenceParser::PostKeyEvent(const TTYInputKey &k)
{
	INPUT_RECORD ir = {};
	ir.EventType = KEY_EVENT;
	ir.Event.KeyEvent.wRepeatCount = 1;
//	ir.Event.KeyEvent.uChar.UnicodeChar = i.second.unicode_char;
	if (k.vk == VK_SPACE)
	{
		ir.Event.KeyEvent.uChar.UnicodeChar = L' ';
	}
	ir.Event.KeyEvent.wVirtualKeyCode = k.vk;
	ir.Event.KeyEvent.dwControlKeyState = k.control_keys;
	ir.Event.KeyEvent.wVirtualScanCode = WINPORT(MapVirtualKey)(k.vk,MAPVK_VK_TO_VSC);
	if (IsEnhancedKey(k.vk))
		ir.Event.KeyEvent.dwControlKeyState|= ENHANCED_KEY;
	if (_handler)
		ir.Event.KeyEvent.dwControlKeyState|= _handler->OnQueryControlKeys();

	ir.Event.KeyEvent.bKeyDown = TRUE;
	g_winport_con_in.Enqueue(&ir, 1);
	ir.Event.KeyEvent.bKeyDown = FALSE;
	g_winport_con_in.Enqueue(&ir, 1);
}

void TTYInputSequenceParser::ParseMouse(char action, char col, char raw)
{
	INPUT_RECORD ir = {};
	ir.EventType = MOUSE_EVENT;
	ir.Event.MouseEvent.dwMousePosition.X = ((unsigned char)col - (unsigned char)'!');
	ir.Event.MouseEvent.dwMousePosition.Y = ((unsigned char)raw - (unsigned char)'!');
	DWORD now = WINPORT(GetTickCount)();

	switch (action) {
		case '0': // ctrl+left press
			ir.Event.MouseEvent.dwControlKeyState|= LEFT_CTRL_PRESSED;

		case ' ': // left press
			if (now - _mouse.left_ts <= 500) {
				ir.Event.MouseEvent.dwEventFlags|= DOUBLE_CLICK;
				_mouse.left_ts = 0;
			} else
				_mouse.left_ts = now;

			_mouse.middle_ts = _mouse.right_ts = 0;
			_mouse.left = true;
			break;

		case '1': // ctrl+middle press
			ir.Event.MouseEvent.dwControlKeyState|= LEFT_CTRL_PRESSED;

		case '!': // middle press
			if (now - _mouse.middle_ts <= 500) {
				ir.Event.MouseEvent.dwEventFlags|= DOUBLE_CLICK;
				_mouse.middle_ts = 0;
			} else
				_mouse.middle_ts = now;

			_mouse.left_ts = _mouse.right_ts = 0;
			_mouse.middle = true;
			break;

		case '^': // right press
			if (now - _mouse.right_ts <= 500) {
				ir.Event.MouseEvent.dwEventFlags|= DOUBLE_CLICK;
				_mouse.right_ts = 0;
			} else
				_mouse.right_ts = now;

			_mouse.left_ts = _mouse.middle_ts = 0;
			_mouse.right = true;
			break;


		case '3': // ctrl+* depress
			ir.Event.MouseEvent.dwControlKeyState|= LEFT_CTRL_PRESSED;

		case '#': // * depress
			_mouse.left = _mouse.middle = _mouse.right = false;
			break;

		case 'p':
			ir.Event.MouseEvent.dwControlKeyState|= LEFT_CTRL_PRESSED;

		case '`':
			ir.Event.MouseEvent.dwEventFlags|= MOUSE_WHEELED;
			ir.Event.MouseEvent.dwButtonState|= (0x0001 <<16);
			break;

		case 'q':
			ir.Event.MouseEvent.dwControlKeyState|= LEFT_CTRL_PRESSED;

		case 'a':
			ir.Event.MouseEvent.dwEventFlags|= MOUSE_WHEELED;
			ir.Event.MouseEvent.dwButtonState|= (0xffff << 16);
			break;


		case 'P': // ctrl + mouse move
			ir.Event.MouseEvent.dwControlKeyState|= LEFT_CTRL_PRESSED;

		case '@': // mouse move
			break;

		default:
			return;
	}

	if (_mouse.left)
			ir.Event.MouseEvent.dwButtonState|= FROM_LEFT_1ST_BUTTON_PRESSED;

	if (_mouse.middle)
			ir.Event.MouseEvent.dwButtonState|= FROM_LEFT_2ND_BUTTON_PRESSED;

	if (_mouse.right)
			ir.Event.MouseEvent.dwButtonState|= FROM_LEFT_3RD_BUTTON_PRESSED;

	g_winport_con_in.Enqueue(&ir, 1);

}

//////////////////
