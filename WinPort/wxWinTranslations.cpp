#include "stdafx.h"
#include "wxWinTranslations.h"


wxColour ConsoleForeground2wxColorInternal(USHORT attributes)
{
	wxColour::ChannelType r = 0, g = 0, b = 0;
	if (attributes & FOREGROUND_RED)
		r = (attributes & FOREGROUND_INTENSITY) ? 0xff : 0xa0;
	if (attributes & FOREGROUND_BLUE)
		b = (attributes & FOREGROUND_INTENSITY) ? 0xff : 0xa0;
	if (attributes & FOREGROUND_GREEN)
		g = (attributes & FOREGROUND_INTENSITY) ? 0xff : 0xa0;
	return wxColour(r, g, b);
}

wxColour ConsoleBackground2wxColorInternal(USHORT attributes)
{
	wxColour::ChannelType r = 0, g = 0, b = 0;
	if (attributes & BACKGROUND_RED)
		r = (attributes & BACKGROUND_INTENSITY) ? 0xff : 0xa0;
	if (attributes & BACKGROUND_BLUE)
		b = (attributes & BACKGROUND_INTENSITY) ? 0xff : 0xa0;
	if (attributes & BACKGROUND_GREEN)
		g = (attributes & BACKGROUND_INTENSITY) ? 0xff : 0xa0;
	return wxColour(r, g, b);
}

wxColour ConsoleForeground2wxColor(USHORT attributes)
{
	return (attributes & COMMON_LVB_REVERSE_VIDEO) ?
		ConsoleBackground2wxColorInternal(attributes) :
		ConsoleForeground2wxColorInternal(attributes);
}

wxColour ConsoleBackground2wxColor(USHORT attributes)
{
	return (attributes & COMMON_LVB_REVERSE_VIDEO) ?
		ConsoleForeground2wxColorInternal(attributes) :
		ConsoleBackground2wxColorInternal(attributes);
}


////////////////////

int wxKeyCode2WinKeyCode(int code)
{
	switch (code) {
	case WXK_BACK: return  VK_BACK;
	case WXK_TAB: return VK_TAB;
	case WXK_RETURN: return VK_RETURN;
	case WXK_ESCAPE: return VK_ESCAPE;
	case WXK_SPACE: return VK_SPACE;
	case WXK_DELETE: return VK_DELETE;
	case WXK_START: return 0;
	case WXK_LBUTTON: return VK_LBUTTON;
    case WXK_RBUTTON: return VK_RBUTTON;
    case WXK_CANCEL: return VK_CANCEL;
    case WXK_MBUTTON: return VK_MBUTTON;
    case WXK_CLEAR: return VK_CLEAR;
    case WXK_SHIFT: return VK_SHIFT;
    case WXK_ALT: return VK_MENU;
    case WXK_CONTROL: return VK_CONTROL;
    case WXK_MENU: return VK_MENU;
    case WXK_PAUSE: return VK_PAUSE;
    case WXK_CAPITAL: return VK_CAPITAL;
    case WXK_END: return VK_END;
    case WXK_HOME: return VK_HOME;
    case WXK_LEFT: return VK_LEFT;
    case WXK_UP: return VK_UP;
    case WXK_RIGHT: return VK_RIGHT;
    case WXK_DOWN: return VK_DOWN;
    case WXK_SELECT: return VK_SELECT;
    case WXK_PRINT: return VK_PRINT;
    case WXK_EXECUTE: return VK_EXECUTE;
    case WXK_SNAPSHOT: return VK_SNAPSHOT;
    case WXK_INSERT: return VK_INSERT;
    case WXK_HELP: return VK_HELP;
    case WXK_NUMPAD0: return VK_NUMPAD0;
    case WXK_NUMPAD1: return VK_NUMPAD1;
    case WXK_NUMPAD2: return VK_NUMPAD2;
    case WXK_NUMPAD3: return VK_NUMPAD3;
    case WXK_NUMPAD4: return VK_NUMPAD4;
    case WXK_NUMPAD5: return VK_NUMPAD5;
    case WXK_NUMPAD6: return VK_NUMPAD6;
    case WXK_NUMPAD7: return VK_NUMPAD7;
    case WXK_NUMPAD8: return VK_NUMPAD8;
    case WXK_NUMPAD9: return VK_NUMPAD9;
    case WXK_MULTIPLY: return VK_MULTIPLY;
    case WXK_ADD: return VK_ADD;
    case WXK_SEPARATOR: return VK_SEPARATOR;
    case WXK_SUBTRACT: return VK_SUBTRACT;
    case WXK_DECIMAL: return VK_DECIMAL;
    case WXK_DIVIDE: return VK_DIVIDE;
    case WXK_F1: return VK_F1;
    case WXK_F2: return VK_F2;
    case WXK_F3: return VK_F3;
    case WXK_F4: return VK_F4;
    case WXK_F5: return VK_F5;
    case WXK_F6: return VK_F6;
    case WXK_F7: return VK_F7;
    case WXK_F8: return VK_F8;
    case WXK_F9: return VK_F9;
    case WXK_F10: return VK_F10;
    case WXK_F11: return VK_F11;
    case WXK_F12: return VK_F12;
    case WXK_F13: return VK_F13;
    case WXK_F14: return VK_F14;
    case WXK_F15: return VK_F15;
    case WXK_F16: return VK_F16;
    case WXK_F17: return VK_F17;
    case WXK_F18: return VK_F18;
    case WXK_F19: return VK_F19;
    case WXK_F20: return VK_F20;
    case WXK_F21: return VK_F21;
    case WXK_F22: return VK_F22;
    case WXK_F23: return VK_F23;
    case WXK_F24: return VK_F24;
    case WXK_NUMLOCK: return VK_NUMLOCK;
    case WXK_SCROLL: return VK_SCROLL;
    case WXK_PAGEUP: return VK_PRIOR;
    case WXK_PAGEDOWN: return VK_NEXT;
    case WXK_NUMPAD_SPACE: return VK_SPACE;
    case WXK_NUMPAD_TAB: return VK_TAB;
    case WXK_NUMPAD_ENTER: return VK_RETURN;
    case WXK_NUMPAD_F1: return VK_F1;
    case WXK_NUMPAD_F2: return VK_F2;
    case WXK_NUMPAD_F3: return VK_F3;
    case WXK_NUMPAD_F4: return VK_F4;
    case WXK_NUMPAD_HOME: return VK_HOME;
    case WXK_NUMPAD_LEFT: return VK_LEFT;
    case WXK_NUMPAD_UP: return VK_UP;
    case WXK_NUMPAD_RIGHT: return VK_RIGHT;
    case WXK_NUMPAD_DOWN: return VK_DOWN;
    case WXK_NUMPAD_PAGEUP: return VK_PRIOR;
    case WXK_NUMPAD_PAGEDOWN: return VK_NEXT;
    case WXK_NUMPAD_END: return VK_END;
    case WXK_NUMPAD_BEGIN: return VK_CLEAR;
    case WXK_NUMPAD_INSERT: return VK_INSERT;
    case WXK_NUMPAD_DELETE: return VK_DELETE;
    case WXK_NUMPAD_EQUAL: return VK_ADD;
    case WXK_NUMPAD_MULTIPLY: return VK_MULTIPLY;
    case WXK_NUMPAD_ADD: return VK_ADD;
    case WXK_NUMPAD_SEPARATOR: return VK_SEPARATOR;
    case WXK_NUMPAD_SUBTRACT: return VK_SUBTRACT;
    case WXK_NUMPAD_DECIMAL: return VK_DECIMAL;
    case WXK_NUMPAD_DIVIDE: return VK_DIVIDE;

    case WXK_WINDOWS_LEFT: return VK_LWIN;
    case WXK_WINDOWS_RIGHT: return VK_RWIN;
    case WXK_WINDOWS_MENU: return VK_MENU;
	case L'.': return VK_OEM_PERIOD;
	case L',': return VK_OEM_COMMA;
	case L'-': return VK_OEM_MINUS;
	case L'+': return VK_OEM_PLUS;
	case L'`': return VK_OEM_3;
	case L'[': return VK_OEM_4;
	case L'\\': return VK_OEM_5;
	case L']': return VK_OEM_6;
	case L'\'': return VK_OEM_7;
	case L'(': return '9';
	case L')': return '0';
	}
	//fprintf(stderr, "not translated %u %lc", code, code);
	return code;
}

/*int wxKeyIsEnhanced(int code)
{
	
	return false;
}*/

wx2INPUT_RECORD::wx2INPUT_RECORD(wxKeyEvent& event, BOOL KeyDown)
{
	EventType = KEY_EVENT;
	Event.KeyEvent.bKeyDown = KeyDown;
	Event.KeyEvent.wRepeatCount = 1;
	Event.KeyEvent.wVirtualKeyCode = wxKeyCode2WinKeyCode(event.GetKeyCode());
	Event.KeyEvent.wVirtualScanCode = 0;
	Event.KeyEvent.uChar.UnicodeChar = event.GetUnicodeKey();
	Event.KeyEvent.dwControlKeyState = 0;
	
//	if (wxKeyIsEnhanced(event.GetKeyCode()))
//		Event.KeyEvent.dwControlKeyState|= ENHANCED_KEY;
		
	if (event.ShiftDown())
		Event.KeyEvent.dwControlKeyState|= SHIFT_PRESSED;

	if (event.AltDown())
		Event.KeyEvent.dwControlKeyState|= LEFT_ALT_PRESSED;

	if (event.ControlDown())
		Event.KeyEvent.dwControlKeyState|= LEFT_CTRL_PRESSED;
}
