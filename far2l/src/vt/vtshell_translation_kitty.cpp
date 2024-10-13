#include "headers.hpp"
#include <string>

//const WORD key, bool ctrl, bool alt, bool shift, unsigned char keypad, WCHAR uc
std::string VT_TranslateKeyToKitty(const KEY_EVENT_RECORD &KeyEvent, int flags)
{
	const bool ctrl = (KeyEvent.dwControlKeyState & (LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED)) != 0;
	const bool alt = (KeyEvent.dwControlKeyState & (RIGHT_ALT_PRESSED|LEFT_ALT_PRESSED)) != 0;
	const bool shift = (KeyEvent.dwControlKeyState & (SHIFT_PRESSED)) != 0;
	// References:
	// https://sw.kovidgoyal.net/kitty/keyboard-protocol/
	// https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes

	// fixme: KEYPAD 5 работает как F5, а надо чтоб как F3

	int keycode = towlower(KeyEvent.uChar.UnicodeChar);

	int base = 0;
	if ((KeyEvent.wVirtualKeyCode >= 'A') && (KeyEvent.wVirtualKeyCode <= 'Z')) {
		base = towlower(KeyEvent.wVirtualKeyCode);
		if (base == keycode) base = 0;
	}

	// workaround for tty backend
	if (base && !keycode) keycode = base;

	int shifted = 0;

	// (KeyEvent.uChar.UnicodeChar && iswupper(KeyEvent.uChar.UnicodeChar))
	// is workaround for far2l wx as it is not sending Shift state for Char events
	// See
	// ir.Event.KeyEvent.wVirtualKeyCode = VK_OEM_PERIOD;
	// and below in wxMain.cpp: dwControlKeyState not set
	if (shift || (KeyEvent.uChar.UnicodeChar && iswupper(KeyEvent.uChar.UnicodeChar))) {
		shifted = KeyEvent.uChar.UnicodeChar;
	}

	int modifiers = 0;

	if (shift) modifiers |= 1;
	if (alt)   modifiers |= 2;
	if (ctrl)  modifiers |= 4;
	if (KeyEvent.dwControlKeyState & CAPSLOCK_ON) modifiers |= 64;
	if (KeyEvent.dwControlKeyState & NUMLOCK_ON)  modifiers |= 128;

	modifiers += 1; // as spec requres

	char suffix = 'u';

	// apply modifications for special keys

	switch (KeyEvent.wVirtualKeyCode) {
		case VK_ESCAPE:    keycode = 27;  break;
		case VK_RETURN:    keycode = 13;  break;

		case VK_TAB:       keycode = 9;   break;
		case VK_BACK:      keycode = 127; break;

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

	// avoid sending base char if it is equal to keycode
	if (base == keycode) { base = 0; }

	if (!(flags & 8)) { // "Report all keys as escape codes" disabled
		// do not sent modifiers themselfs
		if (!keycode && (modifiers > 1)) {
			return std::string();
		}
	}

	// Записываем ESC-последовательность
	// CSI unicode-key-code:shifted-key:base-layout-key ; modifiers:event-type ; text-as-codepoints u

	// Часть 1

	// We are not able to generate proper sequence for this key for now, sorry
	if (!keycode)
		return std::string();

	std::string out = "\x1B["; // Старт последовательности

	// Добавляем значение keycode
	out+= std::to_string(keycode);

	if ((flags & 4) && (shifted || base)) { // "report alternative keys" enabled
		out+= ':';
		if (shifted) {
			// Добавляем значение shifted
			out+= std::to_string(shifted);
		}
		if (base) {
			// Добавляем значение base
			out+= ':';
			out+= std::to_string(base);
		}
	}

	// Часть 2

	if ((modifiers > 1) || ((flags & 2) && !KeyEvent.bKeyDown)) {
		out+= ';';
		// Добавляем значение modifiers
		out+= std::to_string(modifiers);
		if ((flags & 2) && !KeyEvent.bKeyDown) {
			// Добавляем значение для типа события (1 для keydown, 2 для repeat, 3 для keyup)
			// fixme: repeat unimplemented
			out+= ":3";
		}
	}

	// Часть 3

	if ((flags & 16) && KeyEvent.uChar.UnicodeChar) { // "text as code points" enabled
		if (!((modifiers > 1) || ((flags & 2) && !KeyEvent.bKeyDown))) {
			// Если часть 2 пропущена, добавим ";", чтобы обозначить это
			out+= ';';
		}
		// Добавляем значение UnicodeChar
		out+= ';';
		out+= std::to_string((int)(unsigned int)KeyEvent.uChar.UnicodeChar);
	}

	// Финал

	// Добавляем значение suffix
	out+= suffix;

	if (!(flags & 8) && KeyEvent.uChar.UnicodeChar && !alt && !ctrl) { // "Report all keys as escape codes" disabled
		// just send text
		Wide2MB(&KeyEvent.uChar.UnicodeChar, 1, out, true);
	}

	return out;
}
