#include "wxWinTranslations.h"
#include "INIReader.h"
#include "WinCompat.h"
#include "ConsoleOutput.h"

#include <wx/wx.h>
#include <wx/display.h>
#include <stdlib.h>
#include <wchar.h>
#include <iostream>
#include <fstream>

extern bool g_broadway;
extern ConsoleOutput g_wx_con_out;

/*
    Foreground/Background color palettes are 16 (r,g,b) values.
    More words here from Miotio...
*/
static int backgroundPalette[16*3];
static int foregroundPalette[16*3];

static void InitDefaultPalette() {
    for (int i=0; i<16; i++) {
        foregroundPalette[i*3] = i & FOREGROUND_RED? i & FOREGROUND_INTENSITY? 0xff: 0xa0: 0x00; 
        foregroundPalette[i*3+1] = i & FOREGROUND_GREEN? i & FOREGROUND_INTENSITY? 0xff: 0xa0: 0x00; 
        foregroundPalette[i*3+2] = i & FOREGROUND_BLUE? i & FOREGROUND_INTENSITY? 0xff: 0xa0: 0x00; 
        backgroundPalette[i*3] = (i<<4) & BACKGROUND_RED? i & BACKGROUND_INTENSITY? 0xff: 0xa0: 0x00; 
        backgroundPalette[i*3+1] = (i<<4) & BACKGROUND_GREEN? i & BACKGROUND_INTENSITY? 0xff: 0xa0: 0x00; 
        backgroundPalette[i*3+2] = (i<<4) & BACKGROUND_BLUE? i & BACKGROUND_INTENSITY? 0xff: 0xa0: 0x00; 
    }
}

static void SaveDefaultPalette(char* path) {
    std::ofstream palette_out(path);
    palette_out << "[foreground]" << std::endl;
    for (int i=0; i<16; i++) {
        char col_str[8];
        sprintf(col_str, "#%02X%02X%02X", foregroundPalette[i*3], foregroundPalette[i*3 + 1], foregroundPalette[i*3 + 2]);
        palette_out << i << " = " << col_str << std::endl;
    }
    palette_out << std::endl << "[background]" << std::endl;
    for (int i=0; i<16; i++) {
        char col_str[8];
        sprintf(col_str, "#%02X%02X%02X", backgroundPalette[i*3], backgroundPalette[i*3 + 1], backgroundPalette[i*3 + 2]);
        palette_out << i << " = " << col_str << std::endl;
    }
    palette_out.close();

}

static void LoadPalette(INIReader* reader) {
    const char* val;
    char i_str[3];
    int col;
    // foreground palette
    for (int i=0; i<16; i++) {
        itoa(i, i_str, 10);
        val = reader->Get("foreground", i_str, "#000000").c_str();
        sscanf(val, "#%x", &col);
        // r,g,b
        foregroundPalette[i*3] = (col & 0xff0000) >> 16;
        foregroundPalette[i*3+1] = (col & 0x00ff00) >> 8;
        foregroundPalette[i*3+2] = (col & 0x0000ff);
    }
    // background palette
    for (int i=0; i<16; i++) {
        itoa(i, i_str, 10);
        val = reader->Get("background", i_str, "#000000").c_str();
        sscanf(val, "#%x", &col);
        // r,g,b
        backgroundPalette[i*3] = (col & 0xff0000) >> 16;
        backgroundPalette[i*3 + 1] = (col & 0x00ff00) >> 8;
        backgroundPalette[i*3 + 2] = (col & 0x0000ff);
    }
}

void InitPalettes() {
	char palette_path[250];
    sprintf(palette_path, "%s/.config/far2l/palette.ini", getenv("HOME"));
    INIReader reader(palette_path);
    int err_code = reader.ParseError();
    switch (err_code) {
        case -1:  // New file
            InitDefaultPalette();
            SaveDefaultPalette(palette_path);
            break;
        case 0:  // OK
            LoadPalette(&reader);
            break;
        default:
            InitDefaultPalette();
            uint xc,yc;
            g_wx_con_out.GetSize(xc, yc);
            g_wx_con_out.SetCursor({(xc>>1) - 5, yc>>1});
            WCHAR msg[] = L"ERROR IN PALETTE FILE";
            g_wx_con_out.WriteString(msg, wcslen(msg));
            break;
    }
}


static WinPortRGB InternalConsoleForeground2RGB(USHORT attributes)
{
	WinPortRGB out;
    int color_index = (attributes & 0x0f);

    out.r = (char)foregroundPalette[color_index*3];
    out.g = (char)foregroundPalette[color_index*3 + 1];
    out.b = (char)foregroundPalette[color_index*3 + 2];
	return out;
}

static WinPortRGB InternalConsoleBackground2RGB(USHORT attributes)
{
	WinPortRGB out;
    int color_index = (attributes & 0xf0) >> 4;

    out.r = (char)backgroundPalette[color_index*3];
    out.g = (char)backgroundPalette[color_index*3 + 1];
    out.b = (char)backgroundPalette[color_index*3 + 2];
	return out;
}

WinPortRGB ConsoleForeground2RGB(USHORT attributes)
{
	return (attributes & COMMON_LVB_REVERSE_VIDEO) ?
		InternalConsoleBackground2RGB(attributes) :
		InternalConsoleForeground2RGB(attributes);
}

WinPortRGB ConsoleBackground2RGB(USHORT attributes)
{
	return (attributes & COMMON_LVB_REVERSE_VIDEO) ?
		InternalConsoleForeground2RGB(attributes) :
		InternalConsoleBackground2RGB(attributes);
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
#ifdef __WXMAC__
    case WXK_RAW_CONTROL: return VK_RCONTROL;
#endif
    case WXK_MENU: return VK_APPS;
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
#ifdef __WXOSX__
    case WXK_HELP: return VK_INSERT; //Or its hackintosh bug? If so - remove this ifdef.
#else
    case WXK_HELP: return VK_HELP;
#endif
    case WXK_INSERT: return VK_INSERT;
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

static int IsEnhancedKey(int code)
{
	return (code==WXK_LEFT || code==WXK_RIGHT || code==WXK_UP || code==WXK_DOWN
		|| code==WXK_HOME || code==WXK_END || code==WXK_PAGEDOWN || code==WXK_PAGEUP );
}

wx2INPUT_RECORD::wx2INPUT_RECORD(wxKeyEvent& event, BOOL KeyDown)
{
	EventType = KEY_EVENT;
	Event.KeyEvent.bKeyDown = KeyDown;
	Event.KeyEvent.wRepeatCount = 1;
	Event.KeyEvent.wVirtualKeyCode = wxKeyCode2WinKeyCode(event.GetKeyCode());
	Event.KeyEvent.wVirtualScanCode = 0;
	Event.KeyEvent.uChar.UnicodeChar = event.GetUnicodeKey();
	Event.KeyEvent.dwControlKeyState = 0;
	
#ifdef wxHAS_RAW_KEY_CODES 
#ifdef __APPLE__
//todo
#elif __FreeBSD__
//todo
#elif __linux__
//#define GDK_KEY_Control_L 0xffe3
//#define GDK_KEY_Control_R 0xffe4	
		if (Event.KeyEvent.wVirtualKeyCode == VK_CONTROL && event.GetRawKeyCode() == 0xffe4) {
			Event.KeyEvent.wVirtualKeyCode = VK_RCONTROL;
		}
#endif		
		
#endif	
	
	if (IsEnhancedKey(event.GetKeyCode()))
		Event.KeyEvent.dwControlKeyState|= ENHANCED_KEY;
	
	if (!g_broadway) {
		if (wxGetKeyState(WXK_NUMLOCK))
			Event.KeyEvent.dwControlKeyState|= NUMLOCK_ON;
		
		if (wxGetKeyState(WXK_SCROLL))
			Event.KeyEvent.dwControlKeyState|= SCROLLLOCK_ON;
		
		if (wxGetKeyState(WXK_CAPITAL))
			Event.KeyEvent.dwControlKeyState|= CAPSLOCK_ON;
	}
		
	if (event.ShiftDown())
		Event.KeyEvent.dwControlKeyState|= SHIFT_PRESSED;

	if (event.AltDown())// simulated outside cuz not reliable: || event.MetaDown()
		Event.KeyEvent.dwControlKeyState|= LEFT_ALT_PRESSED;

	if (event.ControlDown())
		Event.KeyEvent.dwControlKeyState|= LEFT_CTRL_PRESSED;
		
		
}
