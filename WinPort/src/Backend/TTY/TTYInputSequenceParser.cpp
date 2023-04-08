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
					if (_handler) {
						_handler->OnInspectKeyEvent(ir.Event.KeyEvent);
					}
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

bool right_ctrl_down = 0;

size_t TTYInputSequenceParser::TryParseAsKittyEscapeSequence(const char *s, size_t l)
{
    // kovidgoyal's kitty keyboard protocol (progressive enhancement flags 15) support
	// CSI [ XXX : XXX : XXX ; XXX : XXX [u~ABCDEFHPQRS]
	// some parts sometimes ommitted, see docs
	// https://sw.kovidgoyal.net/kitty/keyboard-protocol/

	// todo: enhanced key flag now set for essential keys only, should be set for more ones

	// todo: add more keys. all needed by far2l seem to be here, but kitty supports much more

    #define KITTY_MOD_SHIFT    1
    #define KITTY_MOD_ALT      2
    #define KITTY_MOD_CONTROL  4
    #define KITTY_MOD_CAPSLOCK 64
    #define KITTY_MOD_NUMLOCK  128
    #define KITTY_EVT_KEYUP    3

    /** 32 is enough without "text-as-code points" mode, but should be increased if this mode is enabled */
	const int max_kitty_esc_size = 32;

	/** first_limit should be set to 3 if "text-as-code points" mode is on */
	/** also second_limit should be increased to maximum # of code points per key in "text-as-code points" mode */
	const char first_limit = 2;
	const char second_limit = 3;
	int params[first_limit][second_limit] = {0};
	int first_count = 0;
	int second_count = 0;
	bool end_found = 0;
	size_t i;

	for (i = 1;; i++) {
		if (i >= l) {
			return LIKELY(l < max_kitty_esc_size) ? TTY_PARSED_WANTMORE : TTY_PARSED_BADSEQUENCE;
		}
		if (s[i] == ';') {
			second_count = 0;
			first_count++;
			if (first_count >= first_limit) {
				return TTY_PARSED_BADSEQUENCE;
			}
		} else if (s[i] == ':') {
			second_count++;
			if (second_count >= second_limit) {
				return TTY_PARSED_BADSEQUENCE;
			}
		} else if (!isdigit(s[i])) {
		    end_found = true;
			break;
		} else { // digit
			params[first_count][second_count] = atoi(&s[i]);
			while (i < l && isdigit(s[i])) {
				++i;
			}
			i--;
		}
	}

	// check for correct sequence ending
	end_found = end_found && (
			(s[i] == 'u') || (s[i] == '~') ||
			(s[i] == 'A') || (s[i] == 'B') ||
			(s[i] == 'C') || (s[i] == 'D') ||
			(s[i] == 'E') || (s[i] == 'F') ||
			(s[i] == 'H') || (s[i] == 'P') ||
			(s[i] == 'Q') ||
			(s[i] == 'R') || // "R" is still vaild here in old kitty versions
			(s[i] == 'S')
		);

	if (!end_found) {
		return TTY_PARSED_BADSEQUENCE;
	}

	/*
	fprintf(stderr, "%i %i %i %i %i \n", params[0][0], params[0][1], params[0][2], params[0][3], params[0][4]);
	fprintf(stderr, "%i %i %i %i %i \n", params[1][0], params[1][1], params[1][2], params[1][3], params[1][4]);
	fprintf(stderr, "%i %i %i %i %i \n", params[2][0], params[2][1], params[2][2], params[2][3], params[2][4]);
	fprintf(stderr, "%i %i\n", first_count, second_count);
	*/

	int event_type = params[1][1];
	int modif_state = params[1][0];

	INPUT_RECORD ir = {0};
	ir.EventType = KEY_EVENT;

	if (modif_state) {
	    modif_state -= 1;

		if (modif_state & KITTY_MOD_SHIFT)    { ir.Event.KeyEvent.dwControlKeyState |= SHIFT_PRESSED; }
		if (modif_state & KITTY_MOD_ALT)      { ir.Event.KeyEvent.dwControlKeyState |= LEFT_ALT_PRESSED; }
		if (modif_state & KITTY_MOD_CONTROL)  { ir.Event.KeyEvent.dwControlKeyState |=
			right_ctrl_down ? RIGHT_CTRL_PRESSED : LEFT_CTRL_PRESSED; }
		if (modif_state & KITTY_MOD_CAPSLOCK) { ir.Event.KeyEvent.dwControlKeyState |= CAPSLOCK_ON; }
		if (modif_state & KITTY_MOD_NUMLOCK)  { ir.Event.KeyEvent.dwControlKeyState |= NUMLOCK_ON; }
	}

	int base_char = params[0][2] ? params[0][2] : params[0][0];
	if (base_char <= UCHAR_MAX && isalpha(base_char)) {
		ir.Event.KeyEvent.wVirtualKeyCode = (base_char - 'a') + 0x41;
	}
	if (base_char <= UCHAR_MAX && isdigit(base_char)) {
		ir.Event.KeyEvent.wVirtualKeyCode = (base_char - '0') + 0x30;
	}
	switch (base_char) {
		case '`'   : ir.Event.KeyEvent.wVirtualKeyCode = VK_OEM_3; break;
		case '-'   : ir.Event.KeyEvent.wVirtualKeyCode = VK_OEM_MINUS; break;
		case '='   : ir.Event.KeyEvent.wVirtualKeyCode = VK_OEM_PLUS; break;
		case '['   : ir.Event.KeyEvent.wVirtualKeyCode = VK_OEM_4; break;
		case ']'   : ir.Event.KeyEvent.wVirtualKeyCode = VK_OEM_6; break;
		case '\\'  : ir.Event.KeyEvent.wVirtualKeyCode = VK_OEM_5; break;
		case ';'   : ir.Event.KeyEvent.wVirtualKeyCode = VK_OEM_1; break;
		case '\''  : ir.Event.KeyEvent.wVirtualKeyCode = VK_OEM_7; break;
		case ','   : ir.Event.KeyEvent.wVirtualKeyCode = VK_OEM_COMMA; break;
		case '.'   : ir.Event.KeyEvent.wVirtualKeyCode = VK_OEM_PERIOD; break;
		case '/'   : ir.Event.KeyEvent.wVirtualKeyCode = VK_OEM_2; break;
		case 9     : ir.Event.KeyEvent.wVirtualKeyCode = VK_TAB; break;
		case 27    : ir.Event.KeyEvent.wVirtualKeyCode = VK_ESCAPE; break;
		case 13    : if (s[i] == '~') {
				ir.Event.KeyEvent.wVirtualKeyCode = VK_F3;
			} else {
				ir.Event.KeyEvent.wVirtualKeyCode = VK_RETURN;
			}
			break;
		case 127   : ir.Event.KeyEvent.wVirtualKeyCode = VK_BACK; break;
		case 2     : if (s[i] == '~')   ir.Event.KeyEvent.wVirtualKeyCode = VK_INSERT; break;
		case 3     : if (s[i] == '~')   ir.Event.KeyEvent.wVirtualKeyCode = VK_DELETE; break;
		case 5     : if (s[i] == '~') { ir.Event.KeyEvent.wVirtualKeyCode = VK_PRIOR;
			ir.Event.KeyEvent.dwControlKeyState |= ENHANCED_KEY; } break;
		case 6     : if (s[i] == '~') { ir.Event.KeyEvent.wVirtualKeyCode = VK_NEXT;
			ir.Event.KeyEvent.dwControlKeyState |= ENHANCED_KEY; } break;
		case 15    : if (s[i] == '~') ir.Event.KeyEvent.wVirtualKeyCode = VK_F5; break;
		case 17    : if (s[i] == '~') ir.Event.KeyEvent.wVirtualKeyCode = VK_F6; break;
		case 18    : if (s[i] == '~') ir.Event.KeyEvent.wVirtualKeyCode = VK_F7; break;
		case 19    : if (s[i] == '~') ir.Event.KeyEvent.wVirtualKeyCode = VK_F8; break;
		case 20    : if (s[i] == '~') ir.Event.KeyEvent.wVirtualKeyCode = VK_F9; break;
		case 21    : if (s[i] == '~') ir.Event.KeyEvent.wVirtualKeyCode = VK_F10; break;
		case 23    : if (s[i] == '~') ir.Event.KeyEvent.wVirtualKeyCode = VK_F11; break;
		case 24    : if (s[i] == '~') ir.Event.KeyEvent.wVirtualKeyCode = VK_F12; break;
		case 57399 : case 57425 : ir.Event.KeyEvent.wVirtualKeyCode = VK_NUMPAD0; break;
		case 57400 : case 57424 : ir.Event.KeyEvent.wVirtualKeyCode = VK_NUMPAD1; break;
		case 57401 : case 57420 : ir.Event.KeyEvent.wVirtualKeyCode = VK_NUMPAD2; break;
		case 57402 : case 57422 : ir.Event.KeyEvent.wVirtualKeyCode = VK_NUMPAD3; break;
		case 57403 : case 57417 : ir.Event.KeyEvent.wVirtualKeyCode = VK_NUMPAD4; break;
		case 57404 : ir.Event.KeyEvent.wVirtualKeyCode = VK_NUMPAD5; break;
		case 57405 : case 57418 : ir.Event.KeyEvent.wVirtualKeyCode = VK_NUMPAD6; break;
		case 57406 : case 57423 : ir.Event.KeyEvent.wVirtualKeyCode = VK_NUMPAD7; break;
		case 57407 : case 57419 : ir.Event.KeyEvent.wVirtualKeyCode = VK_NUMPAD8; break;
		case 57408 : case 57421 : ir.Event.KeyEvent.wVirtualKeyCode = VK_NUMPAD9; break;
		case 57409 : case 57426 : ir.Event.KeyEvent.wVirtualKeyCode = VK_DECIMAL; break;
		
		case 57410 : ir.Event.KeyEvent.wVirtualKeyCode = VK_DIVIDE; break;
		case 57411 : ir.Event.KeyEvent.wVirtualKeyCode = VK_MULTIPLY; break;
		case 57412 : ir.Event.KeyEvent.wVirtualKeyCode = VK_SUBTRACT; break;
		case 57413 : ir.Event.KeyEvent.wVirtualKeyCode = VK_ADD; break;
		case 57414 : ir.Event.KeyEvent.wVirtualKeyCode = VK_RETURN; break;

		case 57444 : ir.Event.KeyEvent.wVirtualKeyCode = VK_LWIN; break;
		case 57450 : ir.Event.KeyEvent.wVirtualKeyCode = VK_RWIN; break;
		case 57363 : ir.Event.KeyEvent.wVirtualKeyCode = VK_APPS; break;

		case 57448 : ir.Event.KeyEvent.wVirtualKeyCode = VK_CONTROL;
		    if (event_type != KITTY_EVT_KEYUP) {
		    	right_ctrl_down = 1;
				ir.Event.KeyEvent.dwControlKeyState |= RIGHT_CTRL_PRESSED;
		    } else {
		    	right_ctrl_down = 0;
				ir.Event.KeyEvent.dwControlKeyState &= ~RIGHT_CTRL_PRESSED;
		    }
		    ir.Event.KeyEvent.dwControlKeyState |= ENHANCED_KEY;
			break;
		case 57442 : ir.Event.KeyEvent.wVirtualKeyCode = VK_CONTROL;
		    if (event_type != KITTY_EVT_KEYUP) {
				ir.Event.KeyEvent.dwControlKeyState |= LEFT_CTRL_PRESSED;
		    } else {
				ir.Event.KeyEvent.dwControlKeyState &= ~LEFT_CTRL_PRESSED;
		    }
			break;
		case 57443 : ir.Event.KeyEvent.wVirtualKeyCode = VK_MENU;
		    if (event_type != KITTY_EVT_KEYUP) {
				ir.Event.KeyEvent.dwControlKeyState |= LEFT_ALT_PRESSED;
		    } else {
				ir.Event.KeyEvent.dwControlKeyState &= ~LEFT_ALT_PRESSED;
		    }
			break;
		case 57449 : ir.Event.KeyEvent.wVirtualKeyCode = VK_MENU;
		    if (event_type != KITTY_EVT_KEYUP) {
				ir.Event.KeyEvent.dwControlKeyState |= RIGHT_ALT_PRESSED;
		    } else {
				ir.Event.KeyEvent.dwControlKeyState &= ~RIGHT_ALT_PRESSED;
		    }
		    ir.Event.KeyEvent.dwControlKeyState |= ENHANCED_KEY;
			break;
		case 57441 : ir.Event.KeyEvent.wVirtualKeyCode = VK_SHIFT;
		    // todo: add LEFT_SHIFT_PRESSED / RIGHT_SHIFT_PRESSED
		    // see https://github.com/microsoft/terminal/issues/337
		    if (event_type != KITTY_EVT_KEYUP) {
				ir.Event.KeyEvent.dwControlKeyState |= SHIFT_PRESSED;
		    } else {
				ir.Event.KeyEvent.dwControlKeyState &= ~SHIFT_PRESSED;
		    }
			break;
		case 57447 : ir.Event.KeyEvent.wVirtualKeyCode = VK_SHIFT;
		    // todo: add LEFT_SHIFT_PRESSED / RIGHT_SHIFT_PRESSED
		    // see https://github.com/microsoft/terminal/issues/337
		    if (event_type != KITTY_EVT_KEYUP) {
				ir.Event.KeyEvent.dwControlKeyState |= SHIFT_PRESSED;
		    } else {
				ir.Event.KeyEvent.dwControlKeyState &= ~SHIFT_PRESSED;
		    }
			ir.Event.KeyEvent.wVirtualScanCode = RIGHT_SHIFT_VSC;
			break;

		case 57360 : ir.Event.KeyEvent.wVirtualKeyCode = VK_NUMLOCK; break;
		case 57358 : ir.Event.KeyEvent.wVirtualKeyCode = VK_CAPITAL; break;
		
	}
	switch (s[i]) {
		case 'A': ir.Event.KeyEvent.wVirtualKeyCode = VK_UP;
			ir.Event.KeyEvent.dwControlKeyState |= ENHANCED_KEY; break;
		case 'B': ir.Event.KeyEvent.wVirtualKeyCode = VK_DOWN;
			ir.Event.KeyEvent.dwControlKeyState |= ENHANCED_KEY; break;
		case 'C': ir.Event.KeyEvent.wVirtualKeyCode = VK_RIGHT;
			ir.Event.KeyEvent.dwControlKeyState |= ENHANCED_KEY; break;
		case 'D': ir.Event.KeyEvent.wVirtualKeyCode = VK_LEFT;
			ir.Event.KeyEvent.dwControlKeyState |= ENHANCED_KEY; break;
		case 'E': ir.Event.KeyEvent.wVirtualKeyCode = VK_NUMPAD5; break;
		case 'H': ir.Event.KeyEvent.wVirtualKeyCode = VK_HOME; 
			ir.Event.KeyEvent.dwControlKeyState |= ENHANCED_KEY; break;
		case 'F': ir.Event.KeyEvent.wVirtualKeyCode = VK_END;
			ir.Event.KeyEvent.dwControlKeyState |= ENHANCED_KEY; break;
		case 'P': ir.Event.KeyEvent.wVirtualKeyCode = VK_F1; break;
		case 'Q': ir.Event.KeyEvent.wVirtualKeyCode = VK_F2; break;
		case 'R': ir.Event.KeyEvent.wVirtualKeyCode = VK_F3; break;
		case 'S': ir.Event.KeyEvent.wVirtualKeyCode = VK_F4; break;
	}

	if (ir.Event.KeyEvent.wVirtualScanCode == 0) {
		ir.Event.KeyEvent.wVirtualScanCode = 
			WINPORT(MapVirtualKey)(ir.Event.KeyEvent.wVirtualKeyCode, MAPVK_VK_TO_VSC);
	}

	ir.Event.KeyEvent.uChar.UnicodeChar = params[0][1] ? params[0][1] : params[0][0];
	if (
		(ir.Event.KeyEvent.uChar.UnicodeChar < 32) ||
		(ir.Event.KeyEvent.uChar.UnicodeChar == 127) ||
		((ir.Event.KeyEvent.uChar.UnicodeChar >= 57358) && (ir.Event.KeyEvent.uChar.UnicodeChar <= 57454)) ||
		!(WCHAR_IS_VALID(ir.Event.KeyEvent.uChar.UnicodeChar))
	) {
		// those are special values, should not be used as unicode char
		ir.Event.KeyEvent.uChar.UnicodeChar = 0;
	}
	if ((modif_state & KITTY_MOD_CAPSLOCK) && !(modif_state & KITTY_MOD_SHIFT)) {
	    // it's weird, but kitty can not give us uppercase utf8 in caps lock mode
	    // ("text-as-codepoints" mode should solve it, but it is not working for cyrillic chars for unknown reason)
		ir.Event.KeyEvent.uChar.UnicodeChar = towupper(ir.Event.KeyEvent.uChar.UnicodeChar);
	}

	ir.Event.KeyEvent.bKeyDown = (event_type != KITTY_EVT_KEYUP) ? 1 : 0;

	ir.Event.KeyEvent.wRepeatCount = 0;

	_ir_pending.emplace_back(ir);

	return i+1;
}

size_t TTYInputSequenceParser::TryParseAsWinTermEscapeSequence(const char *s, size_t l)
{
	// check for nasty win32-input-mode sequence: as described in
	// https://github.com/microsoft/terminal/blob/main/doc/specs/%234999%20-%20Improved%20keyboard%20handling%20in%20Conpty.md
	// [Vk;Sc;Uc;Kd;Cs;Rc_
	// First char assured to be [ by the caller so need to check that it followed
	// by up to 6 semicolon-separated integer values ended with underscore, keeping
	// in mind that some values can be omitted or be empty meaning they're set to zero

	int args[6] = {0};
	int args_cnt = 0;

	size_t n;
	for (size_t i = n = 1;; ++i) {
		if (i == l) {
			return LIKELY(l < 32) ? TTY_PARSED_WANTMORE : TTY_PARSED_BADSEQUENCE;
		}
		if (s[i] == '_' || s[i] == ';') {
			if (args_cnt == ARRAYSIZE(args)) {
				return TTY_PARSED_BADSEQUENCE;
			}
			if (i > n) {
				args[args_cnt] = atoi(&s[n]);
			}
			++args_cnt;
			n = i + 1;
			if (s[i] == '_') {
				break;
			}

		} else if (s[i] < '0' && s[i] > '9') {
			return TTY_PARSED_BADSEQUENCE;
		}
	}

	INPUT_RECORD ir = {};
	ir.EventType = KEY_EVENT;
	ir.Event.KeyEvent.wVirtualKeyCode = args[0];
	ir.Event.KeyEvent.wVirtualScanCode = args[1];
	ir.Event.KeyEvent.uChar.UnicodeChar = args[2];
	ir.Event.KeyEvent.bKeyDown = (args[3] ? TRUE : FALSE);
	ir.Event.KeyEvent.dwControlKeyState = args[4];
	ir.Event.KeyEvent.wRepeatCount = args[5];

	_ir_pending.emplace_back(ir);
	return n;
}

size_t TTYInputSequenceParser::ParseEscapeSequence(const char *s, size_t l)
{

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

	if (l > 1 && s[0] == '[' && s[1] == 'M') { // mouse report: "\x1b[MAYX"
		if (l < 5)
			return 0;

		ParseMouse(s[2], s[3], s[4]);
		return 5;
	}

	size_t r = ParseNChars2Key(s, l);
	if (r != 0)
		return r;

	if (l > 1 && s[0] == '[') {
		r = TryParseAsKittyEscapeSequence(s, l);
		if (r != TTY_PARSED_BADSEQUENCE) {
			return r;
		}
	}	

	if (l > 1 && s[0] == '[') {
		r = TryParseAsWinTermEscapeSequence(s, l);
		if (r != TTY_PARSED_BADSEQUENCE)
			return r;
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
	if (_handler) {
		_handler->OnInspectKeyEvent(ir.Event.KeyEvent);
	}
	_ir_pending.emplace_back(ir); // g_winport_con_in->Enqueue(&ir, 1);
	ir.Event.KeyEvent.bKeyDown = FALSE;
	_ir_pending.emplace_back(ir); // g_winport_con_in->Enqueue(&ir, 1);
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

		case '2': // ctrl+right press
			ir.Event.MouseEvent.dwControlKeyState|= LEFT_CTRL_PRESSED;

		case '"': // right press
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

		case 'C': // mouse move
			break;

		case 'B': // RButon + mouse move
			_mouse.right = true;
			_mouse.right_ts = 0;
			break;

		case '@': // LButon + mouse move
			_mouse.left = true;
			_mouse.left_ts = 0;
			break;

		default:
			return;
	}

	if (_mouse.left)
			ir.Event.MouseEvent.dwButtonState|= FROM_LEFT_1ST_BUTTON_PRESSED;

	if (_mouse.middle)
			ir.Event.MouseEvent.dwButtonState|= FROM_LEFT_2ND_BUTTON_PRESSED;

	if (_mouse.right)
			ir.Event.MouseEvent.dwButtonState|= RIGHTMOST_BUTTON_PRESSED;

	_ir_pending.emplace_back(ir); // g_winport_con_in->Enqueue(&ir, 1);

}

void TTYInputSequenceParser::OnBracketedPaste(bool start)
{
	INPUT_RECORD ir = {};
	ir.EventType = BRACKETED_PASTE_EVENT;
	ir.Event.BracketedPaste.bStartPaste = start ? TRUE : FALSE;
	_ir_pending.emplace_back(ir);
}

//////////////////
