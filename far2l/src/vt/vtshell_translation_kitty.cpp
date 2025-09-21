#include "headers.hpp"
#include <string>
#include "vtshell_translation.h"

/**
References:

https://sw.kovidgoyal.net/kitty/keyboard-protocol/
https://learn.microsoft.com/en-us/windows/console/key-event-record-str
https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
*/


// todo: inspect all "Fixme" and "workaround" lines

// todo: flags stack

// todo: flags change modes 2 and 3

// todo: key tracking for WinKey modifier

// todo: correct keypad handling:
// - separate keycodes in different num lock modes
// - keypad + modifiers as CSI u in mode 1
// - Alt/Ctrl + keypad numbers in numeric mode generates nothing, but should generate CSI u
// (also problem in legacy code: should generate just numbers)
// Center keypad key in kitty:
// KP_5, 57404 u with numlock on
// KP_BEGIN, 1 E or 57427 ~ with numlock off
// in Wine:
// VK_NUMPAD5 with numlock on
// VK_CLEAR with numlock off

// todo: report other keys
// num lock, caps lock
// f13-f24
// fn, win key 2x, alt gr (iso level3 shift), app menu
// print screen, scroll lock, pause

// fixme: cursor keys mode switching not supported for now

// todo: constants instead of hardcoded numbers

// todo: "repeat" event unimplemented (do we really need it?)


//const WORD key, bool ctrl, bool alt, bool shift, unsigned char keypad, WCHAR uc
std::string VT_TranslateKeyToKitty(const KEY_EVENT_RECORD &KeyEvent, int flags, unsigned char keypad)
{
	std::string out;
	unsigned int shifted = 0;
	unsigned int modifiers = 0;
	char suffix = 'u';
	unsigned int keycode = 0;
	unsigned int base = 0;
	bool skipped = false;
	bool kitty;
	const char *legacy;
	bool nolegacy = false;

	// If key code is VK_NONAME, it is IME event. Just send text, as specified in
	// https://github.com/kovidgoyal/kitty/issues/8620#issuecomment-2869675283
	if (KeyEvent.wVirtualKeyCode == VK_NONAME) {
		if (KeyEvent.bKeyDown) {
			fprintf(stderr, "kitty kb: probably IME event keydown, sending as text\n");
			return Wide2MB(&KeyEvent.uChar.UnicodeChar);
		} else {
			fprintf(stderr, "kitty kb: probably IME event keyup, ignoring\n");
			return "";
		}
	}

	// initialization

	//fprintf(stderr, "kitty kb: generating ESC sequence\n");

	const bool ctrl = (KeyEvent.dwControlKeyState & (LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED)) != 0;
	const bool alt = (KeyEvent.dwControlKeyState & (RIGHT_ALT_PRESSED|LEFT_ALT_PRESSED)) != 0;
	const bool shift = (KeyEvent.dwControlKeyState & (SHIFT_PRESSED)) != 0;

	// See https://sw.kovidgoyal.net/kitty/keyboard-protocol/#disambiguate-escape-codes
	const bool disambiguate = (
		(ctrl && !alt && !shift) ||  // Ctrl+Key
		(!ctrl && alt && !shift) ||  // Alt+Key
		(ctrl && alt && !shift)  ||  // Ctrl+Alt+Key
		(!ctrl && alt && shift)      // Shift+Alt+Key
	);

	const bool is_text_key = (
		(KeyEvent.uChar.UnicodeChar > 0x1F) &&
		(KeyEvent.uChar.UnicodeChar != 0x7F) &&
		!(ctrl|alt)
	);

	// if mode 8 is not set, we should not report releases of some keys
	// see https://github.com/kovidgoyal/kitty/issues/8212

	if ((flags & 2) && !(flags & 8) && !KeyEvent.bKeyDown && (
		((KeyEvent.wVirtualKeyCode == VK_RETURN) && !(ctrl|alt|shift)) ||
		((KeyEvent.wVirtualKeyCode == VK_TAB)    && !(ctrl|alt|shift)) ||
		((KeyEvent.wVirtualKeyCode == VK_BACK)   && !(ctrl|alt|shift))
	)) {
		fprintf(stderr, "kitty kb: not reporting key release\n");
		return "";
	}

	if (!(flags & 2) && !KeyEvent.bKeyDown) {
		fprintf(stderr, "kitty kb: not reporting key release\n");
		return "";
	}

	// Get legacy representation. We will fall back to in some cases
	legacy = VT_TranslateSpecialKey(
		KeyEvent.wVirtualKeyCode, ctrl, alt, shift, keypad, KeyEvent.uChar.UnicodeChar);


	// check if we should fall back to legacy escape sequences generatinon

	kitty = (
		 (flags & 8) || // asked to report all keys as esc seqs
		((flags & 2) && !KeyEvent.bKeyDown) || // key release reporting are only possible as esc seqs
		((flags & 1) && ( // See https://sw.kovidgoyal.net/kitty/keyboard-protocol/#disambiguate-escape-codes
			 // Esc
			 (KeyEvent.wVirtualKeyCode == VK_ESCAPE)    ||
			((
			 // a-z 0-9 ` - = [ ] \ ; ' , . / with the modifiers only
			 (KeyEvent.wVirtualKeyCode >= 0x41 && KeyEvent.wVirtualKeyCode <= 0x5A) || // A-Z
			 (KeyEvent.wVirtualKeyCode >= 0x30 && KeyEvent.wVirtualKeyCode <= 0x39) || // 0-9
			 (KeyEvent.wVirtualKeyCode == VK_OEM_3)      ||  // `~
			 (KeyEvent.wVirtualKeyCode == VK_OEM_MINUS)  ||  // -_
			 (KeyEvent.wVirtualKeyCode == VK_OEM_PLUS)   ||  // +=
			 (KeyEvent.wVirtualKeyCode == VK_OEM_4)      ||  // [{
			 (KeyEvent.wVirtualKeyCode == VK_OEM_6)      ||  // ]}
			 (KeyEvent.wVirtualKeyCode == VK_OEM_5)      ||  // \|
			 (KeyEvent.wVirtualKeyCode == VK_OEM_1)      ||  // ;:
			 (KeyEvent.wVirtualKeyCode == VK_OEM_7)      ||  // '"
			 (KeyEvent.wVirtualKeyCode == VK_OEM_COMMA)  ||  // ,<
			 (KeyEvent.wVirtualKeyCode == VK_OEM_PERIOD) ||  // .>
			 (KeyEvent.wVirtualKeyCode == VK_OEM_2)          // /?
			) && disambiguate) ||
			 // Keypad
			 (KeyEvent.wVirtualKeyCode >= VK_NUMPAD0 &&
			  KeyEvent.wVirtualKeyCode <= VK_NUMPAD9 &&
			  KeyEvent.wVirtualKeyCode != VK_NUMPAD5)   || // See https://github.com/kovidgoyal/kitty/issues/8256
			 (KeyEvent.wVirtualKeyCode == VK_DECIMAL)   ||
			 (KeyEvent.wVirtualKeyCode == VK_SEPARATOR) ||
			 (KeyEvent.wVirtualKeyCode == VK_CLEAR)     || // Fixme: workaround; far2l is not sending VK_CLEAR in tty at all
			((KeyEvent.wVirtualKeyCode == VK_RETURN) && (KeyEvent.dwControlKeyState & ENHANCED_KEY)) // keypad Enter
		))
	);

	if ((flags & 1) && !kitty) {

		// See https://github.com/kovidgoyal/kitty/issues/8263

		if (!legacy && !is_text_key) {
			// Special key with no representation in legacy mode. Use kitty.
			kitty = true;
		} else if (
			// Text key. Use legacy.
			!is_text_key &&
			// Enter, Tab, Backspace without modifiers. Use legacy.
			!(
				((KeyEvent.wVirtualKeyCode == VK_RETURN) && !(ctrl|alt|shift)) ||
				((KeyEvent.wVirtualKeyCode == VK_TAB) && !(ctrl|alt|shift))  ||
				((KeyEvent.wVirtualKeyCode == VK_BACK) && !(ctrl|alt|shift))
			)
		) {
			kitty = true;
		}

		if (!kitty && KeyEvent.bKeyDown && legacy && legacy[0] && legacy[1]) { // [1] check for debug only

			fprintf(stderr, "kitty kb: legacy fallback ESC %s\n", legacy + 1);
			return legacy;
		}

		// We also fall back to legacy generation for non-CSIu function keys
		// if mode is not 8 to avoid affecting legacy apps by possible bugs here,
		// see "Prefer using legacy code" below

	}

	if (!kitty) {
		fprintf(stderr, "kitty kb: generated nothing (legacy generation should be used instead)\n");
		return "";
	}

	if (!(flags & 8)) {
		// do not send modifier presses without mode 8
		if (
			(KeyEvent.wVirtualKeyCode == VK_NUMLOCK) ||
			(KeyEvent.wVirtualKeyCode == VK_CAPITAL) ||
			(KeyEvent.wVirtualKeyCode == VK_SCROLL) ||
			(KeyEvent.wVirtualKeyCode == VK_LSHIFT) ||
			(KeyEvent.wVirtualKeyCode == VK_RSHIFT) ||
			(KeyEvent.wVirtualKeyCode == VK_LCONTROL) ||
			(KeyEvent.wVirtualKeyCode == VK_RCONTROL) ||
			(KeyEvent.wVirtualKeyCode == VK_LMENU) ||
			(KeyEvent.wVirtualKeyCode == VK_RMENU) ||
			(KeyEvent.wVirtualKeyCode == VK_LWIN) ||
			(KeyEvent.wVirtualKeyCode == VK_RWIN)
		) {
			fprintf(stderr, "kitty kb: do not sending modifier press without mode 8\n");
			return "";
		}
	}


	// generating modifiers value

	if (shift) modifiers |= 1;
	if (alt)   modifiers |= 2;
	if (ctrl)  modifiers |= 4;
	modifiers++; // bit mask + 1 as spec requres

	if (KeyEvent.dwControlKeyState & CAPSLOCK_ON) {
		modifiers |= 64;
		nolegacy = true;
	}
	if (KeyEvent.dwControlKeyState & NUMLOCK_ON) {
		modifiers |= 128;
		nolegacy = true;
	}


	// generating shifted value

	if (shift) {
		shifted = KeyEvent.uChar.UnicodeChar;
	}


	// generating key code and base layout key code
	keycode = towlower(KeyEvent.uChar.UnicodeChar);

	// Here we get VK_NONAME as wVirtualKeyCode for IME events,
	// so "base layout key" field can not be set correctly in such cases.
	// See also:
	// https://github.com/wxWidgets/wxWidgets/issues/25379

	if (
		((KeyEvent.wVirtualKeyCode >= 'A') && (KeyEvent.wVirtualKeyCode <= 'Z')) ||
		((KeyEvent.wVirtualKeyCode >= '0') && (KeyEvent.wVirtualKeyCode <= '9'))
	) {

		base = towlower(KeyEvent.wVirtualKeyCode);

		if (ctrl /* some legacy workaround, leaving behavior unchanged */ && isalpha(base)) {
			// Fixme: workaround for far2l wx sending unicode char with ctrl in wrong kb layout
			// See also: https://github.com/wxWidgets/wxWidgets/issues/25384
			keycode = base;
		}
	}

	switch (KeyEvent.wVirtualKeyCode) {

		// top row
		case VK_OEM_3:      base = '`'; break;
		// ...digits...
		case VK_OEM_MINUS:  base = '-'; break;
		case VK_OEM_PLUS:   base = '+'; break;

		// second row
		// ...letters...
		case VK_OEM_4:      base = '['; break;
		case VK_OEM_6:      base = ']'; break;

		// third row
		// ...letters...
		case VK_OEM_1:      base = ';'; break;
		case VK_OEM_7:      base = '\''; break;
		case VK_OEM_5:      base = '\\'; break;

		// forth row
		case 0xE1:          base = '/'; break;
		// ...letters...
		case VK_OEM_COMMA:  base = ','; break;
		case VK_OEM_PERIOD: base = '.'; break;
		case VK_OEM_2:      base = '/'; break;
	}

	// Fixme: workaround for far2l tty backend
	if (base && !keycode) keycode = base;


	// generate key codes for special keys

	switch (KeyEvent.wVirtualKeyCode) {

		// UnicodeChar for those keys is same, so no need to modify

		//case VK_ESCAPE:    keycode = 27;  break;
		//case VK_RETURN:    keycode = 13;  break;
		//case VK_TAB:       keycode = 9;   break;

		// leaving suffix 'u' unchanged
		case VK_BACK:      keycode = 127; break;

		// Fixme: WIP: make VK_CLEAR (numpad 5 without numlock) work until full keypad support is finished
		case VK_CLEAR:     keycode = 1; suffix = 'E'; break;

		// non-CSIu keys: keycode is not an unicode code point

		case VK_INSERT:    keycode = 2;   suffix = '~'; break;
		case VK_DELETE:    keycode = 3;   suffix = '~'; break;

		case VK_LEFT:      keycode = 1;   suffix = 'D'; break;
		case VK_RIGHT:     keycode = 1;   suffix = 'C'; break;
		case VK_UP:        keycode = 1;   suffix = 'A'; break;
		case VK_DOWN:      keycode = 1;   suffix = 'B'; break;

		case VK_PRIOR:     keycode = 5;   suffix = '~'; break;
		case VK_NEXT:      keycode = 6;   suffix = '~'; break;

		case VK_HOME:      keycode = 1;   suffix = 'H'; break;
		case VK_END:       keycode = 1;   suffix = 'F'; break;

		case VK_F1:        keycode = 11;  suffix = '~'; break;
		case VK_F2:        keycode = 12;  suffix = '~'; break;
		case VK_F3:        keycode = 13;  suffix = '~'; break;
		case VK_F4:        keycode = 14;  suffix = '~'; break;
		case VK_F5:        keycode = 15;  suffix = '~'; break;
		case VK_F6:        keycode = 17;  suffix = '~'; break;
		case VK_F7:        keycode = 18;  suffix = '~'; break;
		case VK_F8:        keycode = 19;  suffix = '~'; break;
		case VK_F9:        keycode = 20;  suffix = '~'; break;
		case VK_F10:       keycode = 21;  suffix = '~'; break;
		case VK_F11:       keycode = 23;  suffix = '~'; break;
		case VK_F12:       keycode = 24;  suffix = '~'; break;

		case VK_MENU:
		{
			if (!(flags & 8)) { // "Report all keys as escape codes" disabled - do not sent modifiers themselfs
				fprintf(stderr, "kitty kb: do not sending modifier press without mode 8\n");
				return "";
			}

			if (KeyEvent.dwControlKeyState & ENHANCED_KEY) {
				// right
				keycode = 57449; suffix = 'u';
			} else {
				// left
				keycode = 57443; suffix = 'u';
			}

			break;
		}

		case VK_CONTROL:
		{
			if (!(flags & 8)) { // "Report all keys as escape codes" disabled - do not sent modifiers themselfs
				fprintf(stderr, "kitty kb: do not sending modifier press without mode 8\n");
				return "";
			}

			if ((KeyEvent.dwControlKeyState & ENHANCED_KEY)) {
				// right
				keycode = 57448; suffix = 'u';
			} else {
				// left
				keycode = 57442; suffix = 'u';
			}

			break;
		}

		case VK_SHIFT:
		{
			if (!(flags & 8)) { // "Report all keys as escape codes" disabled - do not sent modifiers themselfs
				fprintf(stderr, "kitty kb: do not sending modifier press without mode 8\n");
				return "";
			}

			if (KeyEvent.wVirtualScanCode == RIGHT_SHIFT_VSC) {
				// right
				keycode = 57447; suffix = 'u';
			} else {
				// left
				keycode = 57441; suffix = 'u';
			}

			break;
		}

	}


	// Prefer using legacy code for all cases then kitty generation and legacy generation
	// in theory should be equal — to avoid bugs in new code affecting apps using legacy encoding.
	// So fall back to legacy if this is a non-CSIu function key,
	// flag 8 is not enabled and legacy generation is possible.
	if ((suffix != 'u') && KeyEvent.bKeyDown && !nolegacy && legacy && legacy[1] && !(flags & 8)) {
		fprintf(stderr, "kitty kb: function non-CSIu key, falling back to legacy generation\n");
		return legacy;
	}


	// According to the spec, the keycode should always be "unshifted", meaning it should contain the value
	// that this key would generate if Shift were not pressed. However, we have no knowledge of the user's
	// keyboard layout here, so the unshifted value for an arbitrary key cannot be reliably determined.
	// Therefore, if a char key is pressed together with Shift, and we can get its lowercase variant
	// with towlower(), we do. If it's not a char key, we can not detect unshifted value here
	// so we use special value 57610 (meaning "unknown", explained below).
	// Also it's unclear why the "unshifted" value would be needed in real-world applications:
	// for consistently working shortcuts, it's more reasonable to use the "base-layout-key" field instead.
	// As for 57610, it is just one more value from Unicode PUA. I took the largest value
	// used in the spec (57454) and added twice the number of values ​​used in the spec (78*2=156),
	// so there would be no collisions in case spec is extended. We can't just use 0 here,
	// since the spec requires unicode key code value to always be specified.
	// See also: https://github.com/elfmz/far2l/issues/2743
	// UPD: switching back to using shifted value in this case, as recommended by spec author here:
	// https://github.com/kovidgoyal/kitty/issues/8620#issuecomment-2878530117
	/*
	if (shifted && (shifted == keycode)) {
		fprintf(stderr, "kitty kb: unshifted key code undetectable, using 57610\n");
		keycode = 57610;
	}
	*/
	// base-layout-key also can not be trusted in our implementation
	// for non-latin keypresses w/o modifiers (except Shift) because of IM usage, see
	// https://github.com/wxWidgets/wxWidgets/issues/25379
	// But hot keys w/o modifiers is nonsence, so not a problem actually

	// avoid sending base char if it is equal to keycode
	if (base == keycode) { base = 0; }

	// check if we can finally generate escape sequence for this key
	if (!keycode && !shifted) { // one of two should present in any case
		fprintf(stderr, "kitty kb: no keycode and no shifted keycode, generating nothing\n");
		return "";
	}

	// generate final escape sequence
	// CSI unicode-key-code:shifted-key:base-layout-key ; modifiers:event-type ; text-as-codepoints u

	out = "\x1B[";

	// adding keycode
	out += std::to_string(keycode);

	if ((flags & 4) && (shifted || base)) { // "report alternative keys" enabled
		out+= ':';
		if (shifted) {
			// adding shifted
			out+= std::to_string(shifted);
		}
		if (base) {
			// adding base
			out+= ':';
			out+= std::to_string(base);
		}
	}

	if ((modifiers > 1) || ((flags & 2) && !KeyEvent.bKeyDown)) {
		out+= ';';
		// adding modifiers
		out+= std::to_string(modifiers);
		if ((flags & 2) && !KeyEvent.bKeyDown) {
			// adding event type (1 for keydown, 2 for repeat, 3 for keyup)
			out+= ":3";
		}
	} else {
		skipped = true; // middle part of sequence is skipped
	}

	if ((flags & 16) && KeyEvent.uChar.UnicodeChar) { // "text as code points" enabled
		if (skipped) {
			out+= ';';
		}
		// adding UnicodeChar
		out+= ';';
		out+= std::to_string((int)(unsigned int)KeyEvent.uChar.UnicodeChar);
	}

	out+= suffix;

	fprintf(stderr, "kitty kb: generated ESC%s\n", out.c_str() + 1);
	return out;
}
