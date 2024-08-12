#include <stdarg.h>
#include <string>
#include <base64.h>
#include <utils.h>
#include "TTYInputSequenceParser.h"
#include "Backend.h"


//See:
// http://www.manmrk.net/tutorials/ISPF/XE/xehelp/html/HID00000579.htm
// http://www.leonerd.org.uk/hacks/fixterms/

#if 0 // change to 1 to enable self-contradiction check on startup

template <typename Last> static void AssertNoConflictsBetween(const Last &last) { }

template <typename First, typename Second, class ... Types>
	static void AssertNoConflictsBetween(const First &first, const Second &second, Types ... others)
{
	for (const auto &f_i : first) {
		for (const auto &s_i : second) {
			if (memcmp(f_i.first.chars, s_i.first.chars,
				std::min(sizeof(f_i.first.chars), sizeof(s_i.first.chars)) ) == 0) {
				ABORT_MSG("'%s' vs '%s'\n",
					std::string(f_i.first.chars, sizeof(f_i.first.chars)).c_str(),
					std::string(s_i.first.chars, sizeof(s_i.first.chars)).c_str());
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
			ABORT_MSG("unexpected r=%d for vk=0x%x control_keys=0x%x fmt='%s'", r, vk, control_keys, fmt);
	}
}

void TTYInputSequenceParser::AddStrTilde_Controls(WORD vk, int code)
{
	char tmp[32];
	AddStr_CodeThenControls(vk, "[%s;%d~", itoa(code, tmp, 10));
}

void TTYInputSequenceParser::AddStrTilde(WORD vk, int code)
{
	AddStr(vk, 0, "[%d~", code);
	AddStrTilde_Controls(vk, code);
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
	ASSERT(_handler != nullptr);
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
	AddStr(VK_RETURN, LEFT_ALT_PRESSED, "\x0d");

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

	// Custom ANSI codes to support Ctrl/Alt/Shift+0..9 combinations
	// for iTerm2 see https://gist.github.com/anteo/864c72f64c2e7863353d909bf076aed7
	AddStrTilde_Controls('0', 50);
	AddStrTilde_Controls('1', 51);
	AddStrTilde_Controls('2', 52);
	AddStrTilde_Controls('3', 53);
	AddStrTilde_Controls('4', 54);
	AddStrTilde_Controls('5', 55);
	AddStrTilde_Controls('6', 56);
	AddStrTilde_Controls('7', 57);
	AddStrTilde_Controls('8', 58);
	AddStrTilde_Controls('9', 59);
	// Custom ANSI code to support Ctrl/Alt/Shift+Enter
	AddStrTilde_Controls(VK_RETURN, 60);

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
				AddPendingKeyEvent(k);
				return 8;
			}

		case 7: if (_nch2key7.Lookup(s, k)) {
				AddPendingKeyEvent(k);
				return 7;
			}

		case 6: if (_nch2key6.Lookup(s, k)) {
				AddPendingKeyEvent(k);
				return 6;
			}

		case 5: if (_nch2key5.Lookup(s, k)) {
				AddPendingKeyEvent(k);
				return 5;
			}

		case 4: if (_nch2key4.Lookup(s, k)) {
				AddPendingKeyEvent(k);
				return 4;
			}

		case 3: if (_nch2key3.Lookup(s, k)) {
				AddPendingKeyEvent(k);
				return 3;
			}

		case 2: if (_nch2key2.Lookup(s, k)) {
				AddPendingKeyEvent(k);
				return 2;

			} else if ( (s[0] & 0b11000000) == 0b11000000) {
				// looks like alt + multibyte UTF8 sequence
				wchar_t wc;
				size_t l_used = l;
				MB2Wide_Unescaped(s, l_used, wc, true);
				if (l_used) {
					INPUT_RECORD ir = {};
					ir.EventType = KEY_EVENT;
					ir.Event.KeyEvent.wRepeatCount = 1;
					ir.Event.KeyEvent.uChar.UnicodeChar = wc;
					ir.Event.KeyEvent.dwControlKeyState|= LEFT_ALT_PRESSED;
					ir.Event.KeyEvent.bKeyDown = TRUE;
					_handler->OnInspectKeyEvent(ir.Event.KeyEvent);
					_ir_pending.emplace_back(ir); // g_winport_con_in->Enqueue(&ir, 1);
					ir.Event.KeyEvent.bKeyDown = FALSE;
					_ir_pending.emplace_back(ir); // g_winport_con_in->Enqueue(&ir, 1);
					return l_used;
				}
			}

		case 1: if (_nch2key1.Lookup(s, k)) {
				AddPendingKeyEvent(k);
				return 1;
			}

		default: ;
	}

	return 0;
}

void TTYInputSequenceParser::ParseAPC(const char *s, size_t l)
{
	if (strncmp(s, "f2l", 3) == 0) {
		_tmp_stk_ser.FromBase64(s + 3, l - 3);
		_handler->OnFar2lEvent(_tmp_stk_ser);

	} else if (strncmp(s, "far2l", 5) == 0) {
		_tmp_stk_ser.FromBase64(s + 5, l - 5);
		_handler->OnFar2lReply(_tmp_stk_ser);
	}
}

size_t TTYInputSequenceParser::ParseEscapeSequence(const char *s, size_t l)
{
	/*
	fprintf(stderr, "Parsing: ");
	for (size_t i = 0; i < l && s[i] != '\0'; i++) {
		fprintf(stderr, "%c", s[i]);
	}
	fprintf(stderr, "\n");
	*/

	if (l > 2 && s[0] == '[' && s[2] == 'n') {
		return 3;
	}

	if (l > 0 && s[0] == '_') {
		for (size_t i = 1; i < l; ++i) {
			if (s[i] == '\x07') {
				ParseAPC(s + 1, i - 1);
				return i + 1;
			}
		}
		return 0;
	}

	if (l > 4 && s[0] == '[' && s[1] == '2' && s[2] == '0' && (s[3] == '0' || s[3] == '1') && s[4] == '~') {
		OnBracketedPaste(s[3] == '0');
		return 5;
	}

	size_t r = 0;

	if (l > 1 && s[0] == '[' && s[1] == 'M') {
		// X10 mouse report: "\x1b[MAYX"
		r = ParseX10Mouse(s, l);
		if (r != TTY_PARSED_BADSEQUENCE) {
			return r;
		}
	}

	if (l > 1 && s[0] == '[' && s[1] == '<') {
		// SGR extended mouse report: "x1B[<a;b;cM" or "x1B[<a;b;cm"
		r = ParseSGRMouse(s, l);
		if (r != TTY_PARSED_BADSEQUENCE) {
			return r;
		}
	}

	if (l > 5 && s[0] == ']' && s[1] == '1' && s[2] == '3' && s[3] == '3' && s[4] == '7' && s[5] == ';') {
		r = TryParseAsITerm2EscapeSequence(s, l);
		if (r != TTY_PARSED_BADSEQUENCE) {
			return r;
		}
	}

	r = ParseNChars2Key(s, l);
	if (r != 0) {
		return r;
	}

	//win32-input-mode must be checked before kitty
	if (l > 1 && s[0] == '[') {
		r = TryParseAsWinTermEscapeSequence(s, l);
		if (r != TTY_PARSED_BADSEQUENCE) {
			return r;
		}
	}

	if (l > 1 && s[0] == '[') {
		r = TryParseAsKittyEscapeSequence(s, l);
		if (r != TTY_PARSED_BADSEQUENCE) {
			return r;
		}
	}


	// be well-responsive on panic-escaping
	for (size_t i = 0; (i + 1) < l; ++i) {
		if (s[i] == 0x1b && s[i + 1] == 0x1b) {
			return i;
		}
	}

	return (l >= 8) ? TTY_PARSED_BADSEQUENCE : TTY_PARSED_WANTMORE;
}

size_t TTYInputSequenceParser::ParseIntoPending(const char *s, size_t l)
{
	_extra_control_keys = 0;

	switch (*s) {
		case 0x1b: {
			if (l > 2 && s[1] == 0x1b) {
				_extra_control_keys = LEFT_ALT_PRESSED;
				size_t r = ParseEscapeSequence(s + 2, l - 2);
				_extra_control_keys = 0;
				if (r != TTY_PARSED_WANTMORE && r != TTY_PARSED_PLAINCHARS && r != TTY_PARSED_BADSEQUENCE) {
					return r + 2;
				}
			}

			size_t r = ParseEscapeSequence(s + 1, l - 1);
			if (r != TTY_PARSED_WANTMORE && r != TTY_PARSED_PLAINCHARS && r != TTY_PARSED_BADSEQUENCE) {
				return r + 1;
			}

			return r;
		}

		case 0x00:
			AddPendingKeyEvent(TTYInputKey{VK_SPACE, LEFT_CTRL_PRESSED});
			return 1;

		case 0x01: case 0x02: case 0x03: case 0x04: case 0x05: case 0x06: case 0x07: case 0x08:
		case 0x0a: case 0x0b: case 0x0c: case 0x0e: case 0x0f: case 0x10: case 0x11: case 0x12:
		case 0x13: case 0x14: case 0x15: case 0x16: case 0x17: case 0x18: case 0x19: case 0x1a:
			AddPendingKeyEvent(TTYInputKey{WORD('A' + (*s - 0x01)), LEFT_CTRL_PRESSED});
			return 1;

		case 0x09:
			AddPendingKeyEvent(TTYInputKey{VK_TAB, 0});
			return 1;

		case 0x0d:
			AddPendingKeyEvent(TTYInputKey{VK_RETURN, 0});
			return 1;

//		case 0x1b:
//			AddPendingKeyEvent(VK_OEM_4, 0);
//			return 1;

		case 0x1c:
			AddPendingKeyEvent(TTYInputKey{VK_OEM_5, 0});
			return 1;

		case 0x1d:
			AddPendingKeyEvent(TTYInputKey{VK_OEM_6, 0});
			return 1;

		case 0x1e:
			AddPendingKeyEvent(TTYInputKey{'6', 0});
			return 1;

		case 0x1f:
			AddPendingKeyEvent(TTYInputKey{'7', 0});
			return 1;

		case 0x7f:
			AddPendingKeyEvent(TTYInputKey{VK_BACK, 0});
			return 1;

		default:
			return (size_t)TTY_PARSED_PLAINCHARS;
	}

	ABORT();
}

size_t TTYInputSequenceParser::Parse(const char *s, size_t l, bool idle_expired)
{
	//work-around for double encoded events in win32-input mode
	//we encountered sequence \x1B[0;0;27;1;0;1_ it is \x1B encoded in win32 input
	//following codes are part of some double encoded input sequence and must be parsed in separate buffer
	if (
		(l > 8 && s[1] == '[' && s[2] == '0' && s[3] == ';' && s[4] == '0' && s[5] == ';' && s[6] == '2' && s[7] == '7')
		|| _win32_accumulate) {
		_win32_accumulate = true;
		return TryUnwrappWinDoubleEscapeSequence(s, l);
	}

	size_t r = ParseIntoPending(s, l);

	if ( (r == TTY_PARSED_WANTMORE || r == TTY_PARSED_BADSEQUENCE) && idle_expired && *s == 0x1b) {
		auto saved_pending = _ir_pending;
		auto saved_r = r;
		_ir_pending.clear();
		AddPendingKeyEvent(TTYInputKey{VK_ESCAPE, 0});
		if (l > 1) {
			r = ParseIntoPending(s + 1, l - 1);
			if (r != TTY_PARSED_WANTMORE && r != TTY_PARSED_BADSEQUENCE) {
				++r;
			}
		} else {
			r = 1;
		}

		if (r == TTY_PARSED_WANTMORE || r == TTY_PARSED_BADSEQUENCE) {
			_ir_pending.swap(saved_pending);
			r = saved_r;
		}
	}

	if (!_ir_pending.empty()) {
		g_winport_con_in->Enqueue(&_ir_pending[0], _ir_pending.size());
		_ir_pending.clear();
	}

	return r;
}


void TTYInputSequenceParser::AddPendingKeyEvent(const TTYInputKey &k)
{
	INPUT_RECORD ir = {};
	ir.EventType = KEY_EVENT;
	ir.Event.KeyEvent.bKeyDown = TRUE;
	ir.Event.KeyEvent.wRepeatCount = 1;
//	ir.Event.KeyEvent.uChar.UnicodeChar = i.second.unicode_char;
	if (k.vk == VK_SPACE) {
		ir.Event.KeyEvent.uChar.UnicodeChar = L' ';
	}
	ir.Event.KeyEvent.wVirtualKeyCode = k.vk;
	ir.Event.KeyEvent.dwControlKeyState = k.control_keys | _extra_control_keys;
	ir.Event.KeyEvent.wVirtualScanCode = WINPORT(MapVirtualKey)(k.vk,MAPVK_VK_TO_VSC);
	_handler->OnInspectKeyEvent(ir.Event.KeyEvent);
	_ir_pending.emplace_back(ir); // g_winport_con_in->Enqueue(&ir, 1);
	ir.Event.KeyEvent.bKeyDown = FALSE;
	_ir_pending.emplace_back(ir); // g_winport_con_in->Enqueue(&ir, 1);
}

void TTYInputSequenceParser::AddPendingMouseEvent(int action, int col, int row)
{
	INPUT_RECORD ir = {};
	ir.EventType = MOUSE_EVENT;
	ir.Event.MouseEvent.dwMousePosition.X = col;
	ir.Event.MouseEvent.dwMousePosition.Y = row;
	DWORD now = WINPORT(GetTickCount)();

	//check bit indicators
	if (action & _shift_ind) ir.Event.MouseEvent.dwControlKeyState |= SHIFT_PRESSED;
	if (action & _alt_ind)   ir.Event.MouseEvent.dwControlKeyState |= LEFT_ALT_PRESSED;
	if (action & _ctrl_ind)  ir.Event.MouseEvent.dwControlKeyState |= LEFT_CTRL_PRESSED;

	//and remove them
	//it makes process soooo much easier
	action = action & ~(_shift_ind | _alt_ind | _ctrl_ind);
	switch (action) {
		case 0: // char ' ', left press
			if (now - _mouse.left_ts <= 500) {
				ir.Event.MouseEvent.dwEventFlags |= DOUBLE_CLICK;
				_mouse.left_ts = 0;
			} else {
				_mouse.left_ts = now;
			}

			_mouse.middle_ts = _mouse.right_ts = 0;
			_mouse.left = true;
			_mouse.left_pressed = true;
			break;

		case 1: //char '!', middle press
			if (now - _mouse.middle_ts <= 500) {
				ir.Event.MouseEvent.dwEventFlags |= DOUBLE_CLICK;
				_mouse.middle_ts = 0;
			} else {
				_mouse.middle_ts = now;
			}

			_mouse.left_ts = _mouse.right_ts = 0;
			_mouse.middle = true;
			_mouse.middle_pressed =true;
			break;

		case 2: // char '"', right press
			if (now - _mouse.right_ts <= 500) {
				ir.Event.MouseEvent.dwEventFlags |= DOUBLE_CLICK;
				_mouse.right_ts = 0;
			} else {
				_mouse.right_ts = now;
			}

			_mouse.left_ts = _mouse.middle_ts = 0;
			_mouse.right = true;
			_mouse.right_pressed = true;
			break;

		case 3: // char '#', release buttons
			//on release reset clicks and helds
			_mouse.left   = false;
			_mouse.middle = false;
			_mouse.right  = false;
			_mouse.left_pressed   = false;
			_mouse.middle_pressed = false;
			_mouse.right_pressed  = false;
			break;

		//wheel move
		case 64: // char '`', wheel up
			ir.Event.MouseEvent.dwEventFlags |= MOUSE_WHEELED;
			ir.Event.MouseEvent.dwButtonState |= (0x0001 <<16);
			break;

		case 65: // char 'a', wheel down
			ir.Event.MouseEvent.dwEventFlags |= MOUSE_WHEELED;
			ir.Event.MouseEvent.dwButtonState |= (0xffff << 16);
			break;

		//drag
		//while button already pressed we have to avoid sending another button event
		//otherwise FAR will be constantly change UI state under stream of clicks
		//just consider it mouse move
		case 32: // char '@', left button drag
			if (_mouse.left_pressed) {
				ir.Event.MouseEvent.dwEventFlags |= MOUSE_MOVED;
			} else {
				_mouse.left = true;
				_mouse.left_ts = 0;
			}
			break;

		case 33: // char 'A', middle button drag
			if (_mouse.middle_pressed) {
				ir.Event.MouseEvent.dwEventFlags |= MOUSE_MOVED;
			} else {
				_mouse.middle = true;
				_mouse.middle_ts = 0;
			}
			break;

		case 34: // char 'B', right button drag
			if (_mouse.right_pressed) {
				ir.Event.MouseEvent.dwEventFlags |= MOUSE_MOVED;
			} else {
				_mouse.right = true;
				_mouse.right_ts = 0;
			}
			break;

		case 35: // char 'C', mouse move
			ir.Event.MouseEvent.dwEventFlags |= MOUSE_MOVED;
			break;

		default:
			return;
	}

	if (_mouse.left) {
		ir.Event.MouseEvent.dwButtonState |= FROM_LEFT_1ST_BUTTON_PRESSED;
	}

	if (_mouse.middle) {
		ir.Event.MouseEvent.dwButtonState |= FROM_LEFT_2ND_BUTTON_PRESSED;
	}

	if (_mouse.right) {
		ir.Event.MouseEvent.dwButtonState |= RIGHTMOST_BUTTON_PRESSED;
	}

	_ir_pending.emplace_back(ir);
}

void TTYInputSequenceParser::OnBracketedPaste(bool start)
{
	INPUT_RECORD ir = {};
	ir.EventType = BRACKETED_PASTE_EVENT;
	ir.Event.BracketedPaste.bStartPaste = start ? TRUE : FALSE;
	_ir_pending.emplace_back(ir);
}

//work-around for double encoded events in win32-input mode
void TTYInputSequenceParser::ParseWinDoubleBuffer(bool idle)
{
	if (_win_double_buffer.size() > 1 && _win_double_buffer.back() == 0x1b) {
		// next sequence already started for some reason
		// this is not normal, we need to investigate why it happened
		_win32_accumulate = false;
		Parse(&_win_double_buffer[0], _win_double_buffer.size() - 1, idle);
		_win_double_buffer.erase(_win_double_buffer.begin(), _win_double_buffer.end() - 1);
		_win32_accumulate = true;
		return;
	}
	
	if (_win_double_buffer.size() > 2 && _win_double_buffer.back() >= '@' && _win_double_buffer.back() <= '~') {
		// end of sequence, whatever is it
		_win32_accumulate = false;
		Parse(&_win_double_buffer[0], _win_double_buffer.size(), idle);
		_win_double_buffer.clear();
	}
}

//////////////////
