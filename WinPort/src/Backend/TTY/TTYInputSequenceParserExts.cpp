#include <stdarg.h>
#include <string>
#include <base64.h>
#include <utils.h>
#include "TTYInputSequenceParser.h"
#include "Backend.h"


size_t TTYInputSequenceParser::ParseX10Mouse(const char *s, size_t l)//(char action, char col, char raw)
{
	/*
	`\x1B[M` has been recognized.
	mouse report: `\x1b[Mayx` where:
		* `a` - button number plus 32, can`t be greater than 223. Codes higher than 113 not used
		* `y` - column number (one-based) plus 32.
		* `x` - row number (one-based) plus 32.
	X button Encoding:
		|7|6|5|4|3|2|1|0|
		| |W|H|M|C|S|B|B|
		bits 0 and 1 are used for button:
			* 00 - MB1 pressed (left)
			* 01 - MB2 pressed (middle)
			* 10 - MB3 pressed (right)
			* 11 - released (none)
		Next three bits indicate modifier keys:
			* 0x04 - shift
			* 0x08 - alt (meta)
			* 0x10 - ctrl
		32 (x20) is added for drag events:
			For example, motion into cell x,y with button 1 down is reported as `\x1b[M@yx`.
				( @  = 32 + 0 (button 1) + 32 (motion indicator) ).
			Similarly, motion with button 3 down is reported as `\x1b[MByx`.
			    ( B  = 32 + 2 (button 3) + 32 (motion indicator) ).
		64 (x40) is added for wheel events.
		    so wheel up is 64, and wheel down is 65.
	valid lenght is 5
	*/
	if (l < 5) {
		return TTY_PARSED_WANTMORE;
	}

	int action = (int)s[2] - 32;
	//make coordinates zero-based
	int colunm = (int)s[3] - 33;
	int row    = (int)s[4] - 33;

	if (action > 223) {
		return TTY_PARSED_BADSEQUENCE;
	}

	AddPendingMouseEvent(action, colunm, row);

	return 5;
}

size_t TTYInputSequenceParser::ParseSGRMouse(const char *s, size_t l)
{
	/*
	`\x1B[<` has been recognized.
	https://invisible-island.net/xterm/ctlseqs/ctlseqs.html#h3-Extended-coordinates
	Sequence looks like "\x1B[<a;y;xM" or "\x1B[<a;y;xm", where:
		* `a` - is a sequence of digits representing the button number in decimal.
		* `y` - is a sequence of digits representing the column number (one-based) in decimal.
		* `x` - is a sequence of digits representing the row number (one-based) in decimal.
	button encoding same as the X10 variant does not add 32
	The sequence ends with `M` on button press and on `m` on button release.
	*/

	int params[3]  = {0};
	int params_cnt = 0;
	int action, colunm, row;
	bool pressed = false;

	size_t n;
	for (size_t i = n = 2;; ++i) {
		if (i == l) {
			return LIKELY(l < 32) ? TTY_PARSED_WANTMORE : TTY_PARSED_BADSEQUENCE;
		}
		if (s[i] == 'M' || s[i] == 'm' || s[i] == ';') {
			if (params_cnt == ARRAYSIZE(params)) {
				return TTY_PARSED_BADSEQUENCE;
			}
			if (i > n) {
				params[params_cnt] = atoi(&s[n]);
				++params_cnt;
			}
			n = i + 1;
			if (s[i] == 'M' || s[i] == 'm') {
				pressed = (s[i] == 'M');
				break;
			}

		} else if (s[i] < '0' || s[i] > '9') {
			return TTY_PARSED_BADSEQUENCE;
		}
	}

	//here we need to correct mouse move code science it conflict with button release
	if ((params[0] & ~(_shift_ind | _alt_ind | _ctrl_ind)) != 35) {
		action = pressed ? params[0] : 3;
	} else {
		action = params[0];
	}

	//make coordinates zero-based
	colunm = --params[1];
	row    = --params[2];

	AddPendingMouseEvent(action, colunm, row);

	return n;
}

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
	int params[first_limit][second_limit] = {{0}};
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
	fprintf(stderr, "%s \n", s);
	fprintf(stderr, "%i %i %i \n", params[0][0], params[0][1], params[0][2]);
	fprintf(stderr, "%i %i %i \n", params[1][0], params[1][1], params[1][2]);
	fprintf(stderr, "%i %i\n\n", first_count, second_count);
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
			_kitty_right_ctrl_down ? RIGHT_CTRL_PRESSED : LEFT_CTRL_PRESSED; } else {
			_kitty_right_ctrl_down = 0;
		}
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
				ir.Event.KeyEvent.wVirtualKeyCode = VK_F3; // workaround for wezterm's #3473
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
		case 8     : if (s[i] == 'u') ir.Event.KeyEvent.wVirtualKeyCode = VK_BACK; break; // workaround for wezterm's #3594
		case 11    : if (s[i] == '~') ir.Event.KeyEvent.wVirtualKeyCode = VK_F1; break; // workaround for wezterm's #3473
		case 12    : if (s[i] == '~') ir.Event.KeyEvent.wVirtualKeyCode = VK_F2; break; // workaround for wezterm's #3473
		case 14    : if (s[i] == '~') ir.Event.KeyEvent.wVirtualKeyCode = VK_F4; break; // workaround for wezterm's #3473
		case 15    : if (s[i] == '~') ir.Event.KeyEvent.wVirtualKeyCode = VK_F5; break;
		case 17    : if (s[i] == '~') ir.Event.KeyEvent.wVirtualKeyCode = VK_F6; break;
		case 18    : if (s[i] == '~') ir.Event.KeyEvent.wVirtualKeyCode = VK_F7; break;
		case 19    : if (s[i] == '~') ir.Event.KeyEvent.wVirtualKeyCode = VK_F8; break;
		case 20    : if (s[i] == '~') ir.Event.KeyEvent.wVirtualKeyCode = VK_F9; break;
		case 21    : if (s[i] == '~') ir.Event.KeyEvent.wVirtualKeyCode = VK_F10; break;
		case 23    : if (s[i] == '~') ir.Event.KeyEvent.wVirtualKeyCode = VK_F11; break;
		case 24    : if (s[i] == '~') ir.Event.KeyEvent.wVirtualKeyCode = VK_F12; break;
		case 32    : ir.Event.KeyEvent.wVirtualKeyCode = VK_SPACE; break;
		case 57399 : case 57425 : ir.Event.KeyEvent.wVirtualKeyCode = VK_NUMPAD0; break;
		case 57400 : case 57424 : ir.Event.KeyEvent.wVirtualKeyCode = VK_NUMPAD1; break;
		case 57401 : case 57420 : ir.Event.KeyEvent.wVirtualKeyCode = VK_NUMPAD2; break;
		case 57402 : case 57422 : ir.Event.KeyEvent.wVirtualKeyCode = VK_NUMPAD3; break;
		case 57403 : case 57417 : ir.Event.KeyEvent.wVirtualKeyCode = VK_NUMPAD4; break;
		case 57404 : case 57427 : ir.Event.KeyEvent.wVirtualKeyCode = VK_NUMPAD5; break; // "case 57427" is workaround for wezterm's #3478
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
				_kitty_right_ctrl_down = true;
				ir.Event.KeyEvent.dwControlKeyState |= RIGHT_CTRL_PRESSED;
			} else {
				_kitty_right_ctrl_down = false;
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
	if (ir.Event.KeyEvent.uChar.UnicodeChar && !ir.Event.KeyEvent.wVirtualKeyCode) {
		// Fixes non-latin chars under WezTerm (workaround for wezterm's #3479)
		ir.Event.KeyEvent.wVirtualKeyCode = VK_UNASSIGNED;
	}
	if ((modif_state & KITTY_MOD_CAPSLOCK) && !(modif_state & KITTY_MOD_SHIFT)) {
		// it's weird, but kitty can not give us uppercase utf8 in caps lock mode
		// ("text-as-codepoints" mode should solve it, but it is not working for cyrillic chars for unknown reason)
		ir.Event.KeyEvent.uChar.UnicodeChar = towupper(ir.Event.KeyEvent.uChar.UnicodeChar);
	}

	ir.Event.KeyEvent.bKeyDown = (event_type != KITTY_EVT_KEYUP) ? 1 : 0;

	ir.Event.KeyEvent.wRepeatCount = 0;

	_ir_pending.emplace_back(ir);

	if (!_using_extension) {
		fprintf(stderr, "TTYInputSequenceParser: using Kitty extension\n");
		_using_extension = 'k';
		_handler->OnUsingExtension(_using_extension);
	}

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

	size_t n = 0;
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
			} else {
				args[args_cnt] = 0;
			}
			++args_cnt;
			n = i + 1;
			if (s[i] == '_') {
				break;
			}

		} else if (s[i] < '0' || s[i] > '9') {
			return TTY_PARSED_BADSEQUENCE;
		}
	}

	INPUT_RECORD ir = {};
	ir.EventType = KEY_EVENT;
	ir.Event.KeyEvent.wVirtualKeyCode = args[0] ? args[0] : VK_UNASSIGNED;
	ir.Event.KeyEvent.wVirtualScanCode = args[1];
	ir.Event.KeyEvent.uChar.UnicodeChar = args[2];
	ir.Event.KeyEvent.bKeyDown = (args[3] ? TRUE : FALSE);
	ir.Event.KeyEvent.dwControlKeyState = args[4];
	ir.Event.KeyEvent.wRepeatCount = args[5];
	_ir_pending.emplace_back(ir);

	if (!_using_extension) {
		fprintf(stderr, "TTYInputSequenceParser: using WinTerm extension\n");
		_using_extension = 'w';
		_handler->OnUsingExtension(_using_extension);
	}
	return n;
}

//work-around for double encoded mouse events in win32-input mode.
//maybe it will be better not copy/paste TryParseAsWinTermEscapeSequence here
//but passing yet another flag to it is less readable.
//so keep it this way until microsoft fix their stuff in the win32-input protocol
size_t TTYInputSequenceParser::TryUnwrappWinDoubleEscapeSequence(const char *s, size_t l)
{
	int args[6] = {0};
	int args_cnt = 0;

	size_t n = 0;
	for (size_t i = n = 1;; ++i) {
		if (i == l) {
			//fprintf(stderr, "\nwant mooore characters... \n");
			return LIKELY(l < 32) ? TTY_PARSED_WANTMORE : TTY_PARSED_BADSEQUENCE;
		}
		if (s[i] == '_' || s[i] == ';') {
 			if (args_cnt == ARRAYSIZE(args)) {
				return TTY_PARSED_BADSEQUENCE;
			}
			if (i > n) {
				args[args_cnt] = atoi(&s[n]);
			} else {
				args[args_cnt] = 0;
			}
			++args_cnt;
			n = i + 1;
			if (s[i] == '_') {
				break;
			}

		} else if (s[i] < '0' || s[i] > '9') {
			return TTY_PARSED_BADSEQUENCE;
		}
	}

	if(args[2] > 0 && args[3] == 1){ // only KeyDown and valid char should pass
		//fprintf(stderr, "Parsed: ==%c==\n", (unsigned char)(args[2]));
		_win_double_buffer.push_back((unsigned char)args[2]);
	}

	return n;
}

size_t TTYInputSequenceParser::ReadUTF8InHex(const char *s, wchar_t *uni_char)
{
	unsigned char bytes[4] = {0};
	int num_bytes = 0;
	size_t i;
	for (i = 0;; i += 2) {
		if (!isdigit(s[i]) && (s[i] < 'a' || s[i] > 'f')) break;
		if (!isdigit(s[i + 1]) && (s[i + 1] < 'a' || s[i + 1] > 'f')) break;
		sscanf(s + i, "%2hhx", &bytes[num_bytes]);
		num_bytes++;
	}

	if (num_bytes == 1) {
		*uni_char = bytes[0];
	} else if (num_bytes == 2) {
		*uni_char = ((bytes[0] & 0x1F) << 6) | (bytes[1] & 0x3F);
	} else if (num_bytes == 3) {
		*uni_char = ((bytes[0] & 0x0F) << 12) | ((bytes[1] & 0x3F) << 6) | (bytes[2] & 0x3F);
	} else if (num_bytes == 4) {
		*uni_char = ((bytes[0] & 0x07) << 18) | ((bytes[1] & 0x3F) << 12) | ((bytes[2] & 0x3F) << 6) | (bytes[3] & 0x3F);
	}

	return i;
}

size_t TTYInputSequenceParser::TryParseAsITerm2EscapeSequence(const char *s, size_t l)
{
	/*
	fprintf(stderr, "iTerm2 parsing: ");
	for (size_t i = 0; i < l && s[i] != '\0'; i++) {
		fprintf(stderr, "%c", s[i]);
	}
	fprintf(stderr, "\n");
	*/

	// protocol spec:
	// https://gitlab.com/gnachman/iterm2/-/issues/7440#note_129307021

	size_t len = 0;
	while (1) {
		if (len >= l)
			return TTY_PARSED_WANTMORE;
		if (s[len] == 7)
			break;
		len++;
	}
	len++;

	unsigned int flags = 0;
	unsigned int flags_length = 0;
	sscanf(s + 8, "%i%n", &flags, &flags_length); // 8 is a fixed length of "]1337;d;"

	flags -= 1;
	unsigned int flags_win = 0;

	// Flags changed event: esc ] 1337 ; f ; flags ^G
	if (s[6] == 'f') {

		bool go = false;
		int vkc = 0, vsc = 0, kd = 0, cks = 0;

		if ((flags  & 1) && !(_iterm_last_flags & 1)) { go = 1; vkc = VK_SHIFT; kd = 1; cks |= SHIFT_PRESSED; }
		if ((flags  & 1) &&  (_iterm_last_flags & 1)) { cks |= SHIFT_PRESSED; }
		if (!(flags & 1) &&  (_iterm_last_flags & 1)) { go = 1; vkc = VK_SHIFT; kd = 0; }

		if ((flags  & 2) && !(_iterm_last_flags & 2)) { go = 1; vkc = VK_SHIFT; kd = 1; cks |= SHIFT_PRESSED;
			vsc = RIGHT_SHIFT_VSC; }
		if ((flags  & 2) &&  (_iterm_last_flags & 2)) { cks |= SHIFT_PRESSED; }
		if (!(flags & 2) &&  (_iterm_last_flags & 2)) { go = 1; vkc = VK_SHIFT; kd = 0; vsc = RIGHT_SHIFT_VSC; }

		if ((flags  & 4) && !(_iterm_last_flags & 4)) { go = 1; vkc = VK_MENU; kd = 1; cks |= LEFT_ALT_PRESSED; }
		if ((flags  & 4) &&  (_iterm_last_flags & 4)) { cks |= LEFT_ALT_PRESSED; }
		if (!(flags & 4) &&  (_iterm_last_flags & 4)) { go = 1; vkc = VK_MENU; kd = 0; }

		/*
		if ((flags  & 8) && !(_iterm_last_flags & 8)) { go = 1; vkc = VK_MENU; kd = 1; cks |= RIGHT_ALT_PRESSED;
			cks |= ENHANCED_KEY; }
		if ((flags  & 8) &&  (_iterm_last_flags & 8)) { cks |= RIGHT_ALT_PRESSED; }
		if (!(flags & 8) &&  (_iterm_last_flags & 8)) { go = 1; vkc = VK_MENU; kd = 0; cks |= ENHANCED_KEY; }
		*/

		if ((flags  & 16) && !(_iterm_last_flags & 16)) { go = 1; vkc = VK_CONTROL; kd = 1; cks |= LEFT_CTRL_PRESSED; }
		if ((flags  & 16) &&  (_iterm_last_flags & 16)) { cks |= LEFT_CTRL_PRESSED; }
		if (!(flags & 16) &&  (_iterm_last_flags & 16)) { go = 1; vkc = VK_CONTROL; kd = 0; }

		if ((flags  & 32) && !(_iterm_last_flags & 32)) { go = 1; vkc = VK_CONTROL; kd = 1; cks |= RIGHT_CTRL_PRESSED;
			cks |= ENHANCED_KEY; }
		if ((flags  & 32) &&  (_iterm_last_flags & 32)) { cks |= RIGHT_CTRL_PRESSED; }
		if (!(flags & 32) &&  (_iterm_last_flags & 32)) { go = 1; vkc = VK_CONTROL; kd = 0; cks |= ENHANCED_KEY; }

		// map right Option to right Control
		if ((flags  & 8) && !(_iterm_last_flags & 8)) { go = 1; vkc = VK_CONTROL; kd = 1; cks |= RIGHT_CTRL_PRESSED;
			cks |= ENHANCED_KEY; }
		if ((flags  & 8) &&  (_iterm_last_flags & 8)) { cks |= RIGHT_CTRL_PRESSED; }
		if (!(flags & 8) &&  (_iterm_last_flags & 8)) { go = 1; vkc = VK_CONTROL; kd = 0; cks |= ENHANCED_KEY; }

		// iTerm2 cmd+v workaround
		if (((flags  & 64) && !(_iterm_last_flags & 64)) ||
				((flags  & 128) && !(_iterm_last_flags & 128))) {
			_iterm2_cmd_state = 1;
			_iterm2_cmd_ts = time(NULL);
		}

		if (go) {
			INPUT_RECORD ir = {0};
			ir.EventType = KEY_EVENT;
			ir.Event.KeyEvent.wVirtualKeyCode = vkc;
			ir.Event.KeyEvent.wVirtualScanCode = vsc ? vsc :
				WINPORT(MapVirtualKey)(ir.Event.KeyEvent.wVirtualKeyCode, MAPVK_VK_TO_VSC);
			ir.Event.KeyEvent.bKeyDown = kd;
			ir.Event.KeyEvent.dwControlKeyState = cks;
			ir.Event.KeyEvent.wRepeatCount = 1;

			_ir_pending.emplace_back(ir);
		}

		if (!_using_extension) {
			fprintf(stderr, "TTYInputSequenceParser: using Apple ITerm2 extension\n");
			_using_extension = 'a';
			_handler->OnUsingExtension(_using_extension);
		}
		_iterm_last_flags = flags;
		return len;
	}

	wchar_t uni_char;
	size_t i = ReadUTF8InHex(s + 8 + flags_length + 1, &uni_char); // 8 is a fixed length of "]1337;d;"

	unsigned int keycode = 0;
	unsigned int keycode_length = 0;
	sscanf(s + 8 + flags_length + 1 + i + 1, "%i%n", &keycode, &keycode_length);

	unsigned int vkc = 0;

	// On MacOS, characters from the third level layout are entered while Option is pressed.
	// So workaround needed for Alt+letters quick search to work
	if (flags  & 4) { // Left Option is pressed? (right Option is mapped to right Control)
		// read unicode char value from "ignoring-modifiers-except-shift"
		ReadUTF8InHex(s + 8 + flags_length + 1 + i + 1 + keycode_length + 1, &uni_char);
		vkc = VK_UNASSIGNED;
	}

	// MacOS key codes:
	// https://github.com/phracker/MacOSX-SDKs/blob/master/MacOSX10.6.sdk/System/Library/Frameworks/Carbon.framework/Versions/A/Frameworks/HIToolbox.framework/Versions/A/Headers/Events.h

	switch (keycode) {
		case 0x00: vkc = 0x41; break; // A
		case 0x01: vkc = 0x53; break; // S
		case 0x02: vkc = 0x44; break; // D
		case 0x03: vkc = 0x46; break; // F
		case 0x04: vkc = 0x48; break; // H
		case 0x05: vkc = 0x47; break; // G
		case 0x06: vkc = 0x5A; break; // Z
		case 0x07: vkc = 0x58; break; // X
		case 0x08: vkc = 0x43; break; // C
		case 0x09: vkc = 0x56; break; // V
		case 0x0B: vkc = 0x42; break; // B
		case 0x0C: vkc = 0x51; break; // Q
		case 0x0D: vkc = 0x57; break; // W
		case 0x0E: vkc = 0x45; break; // E
		case 0x0F: vkc = 0x52; break; // R
		case 0x10: vkc = 0x59; break; // Y
		case 0x11: vkc = 0x54; break; // T
		case 0x12: vkc = 0x31; break; // 1
		case 0x13: vkc = 0x32; break; // 2
		case 0x14: vkc = 0x33; break; // 3
		case 0x15: vkc = 0x34; break; // 4
		case 0x16: vkc = 0x36; break; // 6
		case 0x17: vkc = 0x35; break; // 5
		case 0x18: vkc = VK_OEM_PLUS; break; // =
		case 0x19: vkc = 0x39; break; // 9
		case 0x1A: vkc = 0x37; break; // 7
		case 0x1B: vkc = VK_OEM_MINUS; break; // -
		case 0x1C: vkc = 0x38; break; // 8
		case 0x1D: vkc = 0x30; break; // 0
		case 0x1E: vkc = VK_OEM_6; break; // ]
		case 0x1F: vkc = 0x4F; break; // O
		case 0x20: vkc = 0x55; break; // U
		case 0x21: vkc = VK_OEM_4; break; // [
		case 0x22: vkc = 0x49; break; // I
		case 0x23: vkc = 0x50; break; // P
		case 0x25: vkc = 0x4C; break; // L
		case 0x26: vkc = 0x4A; break; // J
		case 0x27: vkc = VK_OEM_7; break; // '
		case 0x28: vkc = 0x4B; break; // K
		case 0x29: vkc = VK_OEM_1; break; // ;
		case 0x2A: vkc = VK_OEM_5; break; // Back Slash
		case 0x2B: vkc = VK_OEM_COMMA; break; // ,
		case 0x2C: vkc = VK_OEM_2; break; // Slash
		case 0x2D: vkc = 0x4E; break; // N
		case 0x2E: vkc = 0x4D; break; // M
		case 0x2F: vkc = VK_OEM_PERIOD; break; // .
		case 0x32: vkc = VK_OEM_3; break; // `
		case 0x41: vkc = VK_DECIMAL; break; // .
		case 0x43: vkc = VK_MULTIPLY; break; // *
		case 0x45: vkc = VK_ADD; break; // +
		//case 0x47: vkc = ; break; // keypad "clear"
		case 0x4B: vkc = VK_DIVIDE; break; // /
		case 0x4C: vkc = VK_RETURN; break; // Enter
		case 0x4E: vkc = VK_SUBTRACT; break; // -
		//case 0x51: vkc = ; break; // keypad "equals"
		case 0x52: vkc = 0x60; break; // 0
		case 0x53: vkc = 0x61; break; // 1
		case 0x54: vkc = 0x62; break; // 2
		case 0x55: vkc = 0x63; break; // 3
		case 0x56: vkc = 0x64; break; // 4
		case 0x57: vkc = 0x65; break; // 5
		case 0x58: vkc = 0x66; break; // 6
		case 0x59: vkc = 0x67; break; // 7
		case 0x5B: vkc = 0x68; break; // 8
		case 0x5C: vkc = 0x69; break; // 9

		case 0x24: vkc = VK_RETURN; break; // Enter
		case 0x30: vkc = VK_TAB; break; // Tab
		case 0x31: vkc = VK_SPACE; break; // Space
		case 0x33: vkc = VK_BACK; break; // Del https://discussions.apple.com/thread/4072343?answerId=18799493022#18799493022
		case 0x35: vkc = VK_ESCAPE; break; // Esc
		case 0x37: vkc = VK_LWIN; // Command
			// iTerm2 cmd+v workaround
			_iterm2_cmd_state = 1;
			_iterm2_cmd_ts = time(NULL);
			break;
		case 0x38: vkc = VK_SHIFT; break; // Shift
		case 0x39: vkc = VK_CAPITAL; break; // CapsLock
		case 0x3A: vkc = VK_MENU; break; // Option
		case 0x3B: vkc = VK_CONTROL; break; // Control
		case 0x3C: vkc = VK_SHIFT; break; // RightShift
		case 0x3D: vkc = VK_CONTROL; break; // RightOption // map right Option to right Control
		case 0x3E: vkc = VK_CONTROL; break; // RightControl
		//case 0x3F: vkc = ; break; // Function
		//case 0x40: vkc = ; break; // F17
		//case 0x48: vkc = ; break; // VolumeUp
		//case 0x49: vkc = ; break; // VolumeDown
		//case 0x4A: vkc = ; break; // Mute
		//case 0x4F: vkc = ; break; // F18
		//case 0x50: vkc = ; break; // F19
		//case 0x5A: vkc = ; break; // F20
		case 0x60: vkc = VK_F5; break; // F5
		case 0x61: vkc = VK_F6; break; // F6
		case 0x62: vkc = VK_F7; break; // F7
		case 0x63: vkc = VK_F3; break; // F3
		case 0x64: vkc = VK_F8; break; // F8
		case 0x65: vkc = VK_F9; break; // F9
		case 0x67: vkc = VK_F11; break; // F11
		//case 0x69: vkc = ; break; // F13
		//case 0x6A: vkc = ; break; // F16
		//case 0x6B: vkc = ; break; // F14
		case 0x6D: vkc = VK_F10; break; // F10
		case 0x6F: vkc = VK_F12; break; // F12
		//case 0x71: vkc = ; break; // F15
		//case 0x72: vkc = ; break; // Help
		case 0x73: vkc = VK_HOME; break; // Home
		case 0x74: vkc = VK_PRIOR; break; // PageUp
		case 0x75: vkc = VK_DELETE; break; // ForwardDelete https://discussions.apple.com/thread/4072343?answerId=18799493022#18799493022
		case 0x76: vkc = VK_F4; break; // F4
		case 0x77: vkc = VK_END; break; // End
		case 0x78: vkc = VK_F2; break; // F2
		case 0x79: vkc = VK_NEXT; break; // PageDown
		case 0x7A: vkc = VK_F1; break; // F1
		case 0x7B: vkc = VK_LEFT; break; // Left
		case 0x7C: vkc = VK_RIGHT; break; // Right
		case 0x7D: vkc = VK_DOWN; break; // Down
		case 0x7E: vkc = VK_UP; break; // Up
	}

	if (!vkc && uni_char) { vkc = VK_UNASSIGNED; }

	if (keycode == 0x3D) flags_win |= ENHANCED_KEY; // RightOption
	if (keycode == 0x3E) flags_win |= ENHANCED_KEY; // RightControl
	if (keycode == 0x4C) flags_win |= ENHANCED_KEY; // Numpad Enter

	bool has_ctrl = 0;
	if (flags & 1) { flags_win |= SHIFT_PRESSED; } // todo: LEFT_SHIFT_PRESSED
	if (flags & 2) { flags_win |= SHIFT_PRESSED; } // todo: RIGHT_SHIFT_PRESSED
	if (flags & 4) { flags_win |= LEFT_ALT_PRESSED; }
	// if (flags & 8) { flags_win |= RIGHT_ALT_PRESSED; }
	if (flags & 16) { flags_win |= LEFT_CTRL_PRESSED; has_ctrl = 1; }
	if (flags & 32) { flags_win |= RIGHT_CTRL_PRESSED; has_ctrl = 1; }

	// map right Option to right Control
	if (flags & 8) { flags_win |= RIGHT_CTRL_PRESSED; }

	if (has_ctrl && vkc >= 0x30 && vkc <= 0x39) {
		// special handling of Ctrl+numbers
		uni_char = vkc - 0x30 + '0';

		if (s[6] == 'u' && vkc > 0x31 && vkc < 0x39) {
			// iterm generates only keyup events in such cases
			// lets emulate keydown also
			INPUT_RECORD ir = {};
			ir.EventType = KEY_EVENT;
			ir.Event.KeyEvent.wVirtualKeyCode = vkc;
			ir.Event.KeyEvent.wVirtualScanCode =
				WINPORT(MapVirtualKey)(ir.Event.KeyEvent.wVirtualKeyCode, MAPVK_VK_TO_VSC);
			ir.Event.KeyEvent.uChar.UnicodeChar = uni_char;
			ir.Event.KeyEvent.bKeyDown = TRUE;
			ir.Event.KeyEvent.dwControlKeyState = flags_win;
			ir.Event.KeyEvent.wRepeatCount = 1;
			if (keycode == 0x3C) ir.Event.KeyEvent.wVirtualScanCode = RIGHT_SHIFT_VSC; // RightShift
			_ir_pending.emplace_back(ir);
		}
	}

	INPUT_RECORD ir = {};
	ir.EventType = KEY_EVENT;
	ir.Event.KeyEvent.wVirtualKeyCode = vkc;
	ir.Event.KeyEvent.wVirtualScanCode =
		WINPORT(MapVirtualKey)(ir.Event.KeyEvent.wVirtualKeyCode, MAPVK_VK_TO_VSC);
	ir.Event.KeyEvent.uChar.UnicodeChar = uni_char;
	ir.Event.KeyEvent.bKeyDown = (s[6] == 'd' ? TRUE : FALSE);
	ir.Event.KeyEvent.dwControlKeyState = flags_win;
	ir.Event.KeyEvent.wRepeatCount = 1;
	if (keycode == 0x3C) ir.Event.KeyEvent.wVirtualScanCode = RIGHT_SHIFT_VSC; // RightShift
	_ir_pending.emplace_back(ir);

	if (!_using_extension) {
		fprintf(stderr, "TTYInputSequenceParser: using Apple ITerm2 extension\n");
		_using_extension = 'a';
		_handler->OnUsingExtension(_using_extension);
	}
	_iterm_last_flags = flags;
	return len;
}
