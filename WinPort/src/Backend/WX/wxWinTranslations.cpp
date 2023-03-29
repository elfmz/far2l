#include "wxWinTranslations.h"
#include "KeyFileHelper.h"
#include "utils.h"
#include "WinPort.h"
#include "Backend.h"

#include <wx/wx.h>
#include <wx/display.h>

#if defined (__WXGTK__) && defined (__HASX11__)
#include <X11/Xlib.h>
#include <X11/keysym.h>
#endif

#if defined(wxHAS_RAW_KEY_CODES)
# ifdef __WXMAC__
#  include "Mac/touchbar.h"

/*
  kVK_Command                   = 0x37,
  kVK_Shift                     = 0x38,
  kVK_CapsLock                  = 0x39,
  kVK_Option                    = 0x3A,
  kVK_Control                   = 0x3B,
  kVK_RightShift                = 0x3C,
  kVK_RightOption               = 0x3D,
  kVK_RightControl              = 0x3E,
  kVK_Function                  = 0x3F,
*/
#  define RAW_FUNCTION 0x3F
#  define RAW_ALTGR    0x3D
// RAW_RCTRL is unneeded for macos cuz under macos VK_CONTROL
// represented by Command keys while VK_RCONTROL - by Control keys
# else
//#define GDK_KEY_Control_L 0xffe3
//#define GDK_KEY_Control_R 0xffe4
#  define RAW_ALTGR    0xffea
#  define RAW_RCTRL    0xffe4
# endif
#endif


extern bool g_broadway;
extern bool g_remote;

WinPortPalette g_wx_palette;


static const WinPortRGB &InternalConsoleForeground2RGB(USHORT attributes)
{
	return g_wx_palette.foreground[(attributes & 0x0f)];
}

static const WinPortRGB &InternalConsoleBackground2RGB(USHORT attributes)
{
	return g_wx_palette.background[(attributes & 0xf0) >> 4];
}

WinPortRGB ConsoleForeground2RGB(DWORD64 attributes)
{
	if ( (attributes & (FOREGROUND_TRUECOLOR | COMMON_LVB_REVERSE_VIDEO)) == FOREGROUND_TRUECOLOR) {
		return GET_RGB_FORE(attributes);
	}

	if ( (attributes & (BACKGROUND_TRUECOLOR | COMMON_LVB_REVERSE_VIDEO)) == (BACKGROUND_TRUECOLOR | COMMON_LVB_REVERSE_VIDEO)) {
		return GET_RGB_BACK(attributes);
	}

	return (attributes & COMMON_LVB_REVERSE_VIDEO)
		? InternalConsoleBackground2RGB((USHORT)attributes)
		: InternalConsoleForeground2RGB((USHORT)attributes);
}

WinPortRGB ConsoleBackground2RGB(DWORD64 attributes)
{
	if ( (attributes & (BACKGROUND_TRUECOLOR | COMMON_LVB_REVERSE_VIDEO)) == BACKGROUND_TRUECOLOR) {
		return GET_RGB_BACK(attributes);
	}

	if ( (attributes & (FOREGROUND_TRUECOLOR | COMMON_LVB_REVERSE_VIDEO)) == (FOREGROUND_TRUECOLOR | COMMON_LVB_REVERSE_VIDEO)) {
		return GET_RGB_FORE(attributes);
	}

	return (attributes & COMMON_LVB_REVERSE_VIDEO)
		? InternalConsoleForeground2RGB((USHORT)attributes)
		: InternalConsoleBackground2RGB((USHORT)attributes);
}


////////////////////

static int wxKeyCode2WinKeyCode(int code)
{
	switch (code) {
	case WXK_BACK: return VK_BACK;
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
	case WXK_NUMPAD_EQUAL: return VK_OEM_NEC_EQUAL;
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
	case L'_': case L'-': return VK_OEM_MINUS;
	case L'+': return VK_OEM_PLUS;
	case L';': case L':': return VK_OEM_1;
	case L'/': case L'?': return VK_OEM_2;
	case L'~': case L'`': return VK_OEM_3;
	case L'[': case L'{': return VK_OEM_4;
	case L'\\': case L'|': return VK_OEM_5;
	case L']': case '}': return VK_OEM_6;
	case L'\'': case '\"': return VK_OEM_7;
	case L'!': return '1';
	case L'@': return '2';
	case L'#': return '3';
	case L'$': return '4';
	case L'%': return '5';
	case L'^': return '6';
	case L'&': return '7';
	case L'*': return '8';
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

void KeyTracker::OnKeyDown(wxKeyEvent& event, DWORD ticks)
{
	_last_keydown = event;
	_last_keydown_ticks = ticks;

	const auto keycode = event.GetKeyCode();
	_pressed_keys.insert(keycode);

#if defined(wxHAS_RAW_KEY_CODES) && defined(__WXMAC__)
	if (event.GetKeyCode() == WXK_ALT && event.GetRawKeyCode() == RAW_ALTGR) {
		_composing = true;
	}

	if (event.GetRawKeyCode() == RAW_FUNCTION) {
		Touchbar_SetAlternate(true);

	} else if (event.GetKeyCode() != WXK_ALT
			&& event.GetKeyCode() != WXK_CONTROL
			&& event.GetKeyCode() != WXK_RAW_CONTROL
			&& event.GetKeyCode() != WXK_SHIFT) {
		Touchbar_SetAlternate(false);
	}
#endif

#if defined(wxHAS_RAW_KEY_CODES) && !defined(__WXMAC__)
	if (event.GetKeyCode() == WXK_CONTROL && event.GetRawKeyCode() == RAW_RCTRL) {
		_right_control = true;
	}
#endif
}

bool KeyTracker::OnKeyUp(wxKeyEvent& event)
{
#if defined(wxHAS_RAW_KEY_CODES) && defined(__WXMAC__)
	if (event.GetRawKeyCode() == RAW_FUNCTION
		|| ( event.GetKeyCode() != WXK_ALT
			&& event.GetKeyCode() != WXK_CONTROL
			&& event.GetKeyCode() != WXK_RAW_CONTROL
			&& event.GetKeyCode() != WXK_SHIFT)) {
		Touchbar_SetAlternate(false);
	}

	if (event.GetKeyCode() == WXK_ALT) {
		_composing = false;
	}
#endif

#if defined(wxHAS_RAW_KEY_CODES) && !defined(__WXMAC__)
	if (event.GetKeyCode() == WXK_CONTROL) {
		_right_control = false;
	}
#endif

	const auto keycode = event.GetKeyCode();
	return (_pressed_keys.erase(keycode) != 0);
}

bool KeyTracker::CheckForSuddenModifierUp(wxKeyCode keycode)
{
	auto it = _pressed_keys.find(keycode);
	if (it == _pressed_keys.end())
		return false;

	if (wxGetKeyState(keycode))
		return false;

	_pressed_keys.erase(it);

	INPUT_RECORD ir = {};
	ir.EventType = KEY_EVENT;
	ir.Event.KeyEvent.bKeyDown = FALSE;
	ir.Event.KeyEvent.wRepeatCount = 1;
	ir.Event.KeyEvent.wVirtualKeyCode = wxKeyCode2WinKeyCode(keycode);
	ir.Event.KeyEvent.wVirtualScanCode = WINPORT(MapVirtualKey)(ir.Event.KeyEvent.wVirtualKeyCode, MAPVK_VK_TO_VSC);
	ir.Event.KeyEvent.uChar.UnicodeChar = 0;
	ir.Event.KeyEvent.dwControlKeyState = 0;
#ifndef __WXMAC__
	if (keycode == WXK_CONTROL && _right_control) {
		_right_control = false;
		ir.Event.KeyEvent.wVirtualKeyCode = VK_RCONTROL;
	}
#endif
	if (ir.Event.KeyEvent.wVirtualKeyCode == VK_RCONTROL) {
		ir.Event.KeyEvent.wVirtualKeyCode = VK_CONTROL;
		ir.Event.KeyEvent.dwControlKeyState|= ENHANCED_KEY;
	}
	g_winport_con_in->Enqueue(&ir, 1);
	return true;
}

bool KeyTracker::CheckForSuddenModifiersUp()
{
	bool out = false;
	if (CheckForSuddenModifierUp(WXK_CONTROL)) {
		fprintf(stderr, "%s: CONTROL\n", __FUNCTION__);
		out = true;
	}
	if (CheckForSuddenModifierUp(WXK_ALT)) {
		fprintf(stderr, "%s: ALT\n", __FUNCTION__);
		out = true;
	}
	if (CheckForSuddenModifierUp(WXK_SHIFT)) {
		fprintf(stderr, "%s: SHIFT\n", __FUNCTION__);
		out = true;
	}
	return out;
}

void KeyTracker::ForceAllUp()
{
	for (auto kc : _pressed_keys) {
		INPUT_RECORD ir = {0};
		ir.EventType = KEY_EVENT;
		ir.Event.KeyEvent.wRepeatCount = 1;
		ir.Event.KeyEvent.wVirtualKeyCode = wxKeyCode2WinKeyCode(kc);
		if (ir.Event.KeyEvent.wVirtualKeyCode == VK_RCONTROL) {
			ir.Event.KeyEvent.wVirtualKeyCode = VK_CONTROL;
			ir.Event.KeyEvent.dwControlKeyState|= ENHANCED_KEY;
		}
		g_winport_con_in->Enqueue(&ir, 1);
#ifndef __WXMAC__
		if (ir.Event.KeyEvent.wVirtualKeyCode == VK_CONTROL && _right_control) {
			ir.Event.KeyEvent.dwControlKeyState|= ENHANCED_KEY;
			g_winport_con_in->Enqueue(&ir, 1);
		}
#endif
	}
	_pressed_keys.clear();
#ifndef __WXMAC__
	_right_control = false;
#endif
	_composing = false;
}

const wxKeyEvent& KeyTracker::LastKeydown() const
{
	return _last_keydown;
}

DWORD KeyTracker::LastKeydownTicks() const
{
	return _last_keydown_ticks;
}

bool KeyTracker::Alt() const
{
	return _pressed_keys.find(WXK_ALT) != _pressed_keys.end();
}

bool KeyTracker::Shift() const
{
	return _pressed_keys.find(WXK_SHIFT) != _pressed_keys.end();
}

bool KeyTracker::LeftControl() const
{
#ifndef __WXMAC__
	if (_right_control) {
		return false;
	}
#endif

	return _pressed_keys.find(WXK_CONTROL) != _pressed_keys.end();
}

bool KeyTracker::RightControl() const
{
#ifdef __WXMAC__
	return _pressed_keys.find(WXK_RAW_CONTROL) != _pressed_keys.end();
#else
	return _right_control;
#endif
}

//////////////////////

static DWORD s_cached_led_state = 0;

wx2INPUT_RECORD::wx2INPUT_RECORD(BOOL KeyDown, const wxKeyEvent& event, const KeyTracker &key_tracker)
{
	auto key_code = event.GetKeyCode();

#if defined (__WXGTK__) && defined (__HASX11__)
	if (!key_code) {
		Display *display;
		display = XOpenDisplay(NULL);
		KeySym *keymap;
		int keysyms_per_keycode;
		char* keysym;
		// GetRawKeyFlags() gives us raw x11 hardware key code under wxGTK
		keymap = XGetKeyboardMapping(display, event.GetRawKeyFlags(), 1, &keysyms_per_keycode);
		for (int i = 0; i < keysyms_per_keycode; i++) {
			if (keymap[i] && (keymap[i] != NoSymbol)) {
				keysym = XKeysymToString(keymap[i]);
				if (strlen(keysym) == 1) {
					// char key
					key_code = toupper(*keysym);
				}
				switch (keymap[i]) {
					case XK_minus:        key_code = VK_OEM_MINUS;   break;
					case XK_equal:        key_code = VK_OEM_PLUS;    break;
					case XK_bracketleft:  key_code = VK_OEM_4;       break;
					case XK_bracketright: key_code = VK_OEM_6;       break;
					case XK_semicolon:    key_code = VK_OEM_1;       break;
					case XK_apostrophe:   key_code = VK_OEM_7;       break;
					case XK_grave:        key_code = VK_OEM_3;       break;
					case XK_backslash:    key_code = VK_OEM_5;       break;
					case XK_comma:        key_code = VK_OEM_COMMA;   break;
					case XK_period:       key_code = VK_OEM_PERIOD;  break;
					case XK_slash:        key_code = VK_OEM_2;       break;
				}
			}
			if (key_code)
				break;
		}

		XFree(keymap);
		XCloseDisplay(display);
	}
#endif

#ifdef __linux__
	// Recent KDEs put into keycode non-latin characters in case of
	// non-latin input layout configured as first in the list of layouts.
	// FAR expects latin key codes there and GetRawKeyCode() gives them.
	// Why not using GetRawKeyCode() always? To avoid surprises, as
	// GetKeyCode() served well for a long time til this started to happen.
	// See https://github.com/elfmz/far2l/issues/1180
	if (key_code > 0x100) {
		auto raw_key_code = event.GetRawKeyCode();
		if (raw_key_code > 0x1f && raw_key_code <= 0x7f) {
			key_code = raw_key_code;
		}
	}
	if (key_code >= 'a' && key_code <= 'z') {
		key_code-= 'a' - 'A';
	}
#endif
	EventType = KEY_EVENT;
	Event.KeyEvent.bKeyDown = KeyDown;
	Event.KeyEvent.wRepeatCount = 1;
	Event.KeyEvent.wVirtualKeyCode = wxKeyCode2WinKeyCode(key_code);
	Event.KeyEvent.wVirtualScanCode = WINPORT(MapVirtualKey)(Event.KeyEvent.wVirtualKeyCode, MAPVK_VK_TO_VSC);
	Event.KeyEvent.uChar.UnicodeChar = event.GetUnicodeKey();
	Event.KeyEvent.dwControlKeyState = 0;

#if defined(wxHAS_RAW_KEY_CODES) && !defined(__WXMAC__)
	if (event.GetKeyCode() == WXK_CONTROL && event.GetRawKeyCode() == RAW_RCTRL) {
		Event.KeyEvent.wVirtualKeyCode = VK_RCONTROL;
	}
#endif

	if (IsEnhancedKey(event.GetKeyCode())) {
		Event.KeyEvent.dwControlKeyState|= ENHANCED_KEY;
	}

	if (Event.KeyEvent.wVirtualKeyCode == VK_RCONTROL) {
		//same on windows, otherwise far resets command line selection
		Event.KeyEvent.wVirtualKeyCode = VK_CONTROL;
		Event.KeyEvent.dwControlKeyState|= ENHANCED_KEY;
	}

	if (KeyDown || WINPORT(GetTickCount)() - key_tracker.LastKeydownTicks() > 500) {
		s_cached_led_state = WxKeyboardLedsState();
	}

	Event.KeyEvent.dwControlKeyState|= s_cached_led_state;

	// Keep in mind that key composing combinations with AltGr+.. arrive as keydown of Ctrl+Alt+..
	// so if event.ControlDown() and event.AltDown() are together then don't believe them and
	// use only state maintaned key_tracker. Unless under broadway, that may miss separate control
	// keys events.
	if (key_tracker.Alt() || (event.AltDown() && (!event.ControlDown() || g_broadway))) {
		Event.KeyEvent.dwControlKeyState|= LEFT_ALT_PRESSED;
	}

	if (key_tracker.Shift() || event.ShiftDown()) {
		Event.KeyEvent.dwControlKeyState|= SHIFT_PRESSED;
	}

	if (key_tracker.LeftControl()) {
		Event.KeyEvent.dwControlKeyState|= LEFT_CTRL_PRESSED;
	}

	if (key_tracker.RightControl()) {
		Event.KeyEvent.dwControlKeyState|= RIGHT_CTRL_PRESSED;

	} else if (event.ControlDown() && (!event.AltDown() || g_broadway)) {
		Event.KeyEvent.dwControlKeyState|= LEFT_CTRL_PRESSED;
	}
}

//////////////

static unsigned int s_wx_assert_cached_bits = 0;
static unsigned int s_wx_assert_cache_bit = 0;
static unsigned int s_remote_time_avg = 0;

#define REMOTE_SLOWNESS_TRSH_MSEC		50

DWORD WxKeyboardLedsState()
{
	// Getting LED modifiers requires 3 server roundtrips that
	// can be too time-expensive for remotely forwarded connections.
	clock_t stopwatch = 0;
	if (g_remote) {
		if (s_remote_time_avg > REMOTE_SLOWNESS_TRSH_MSEC) {
			return 0;
		}
		stopwatch = GetProcessUptimeMSec();
	}

	DWORD out = 0;
	// Old non-GTK wxWidgets had missing support for this keys, and attempt
	// to use wxGetKeyState with unsupported key causes assert callback
	// to be invoked several times on each key event thats not good.
	// Avoid asserts all the time by 'caching' unsupported state.
	s_wx_assert_cache_bit = 1;
	if ((s_wx_assert_cached_bits & 1) == 0 && wxGetKeyState(WXK_NUMLOCK)) {
		out|= NUMLOCK_ON;
	}

	s_wx_assert_cache_bit = 2;
	if ((s_wx_assert_cached_bits & 2) == 0 && wxGetKeyState(WXK_SCROLL)) {
		out|= SCROLLLOCK_ON;
	}

	s_wx_assert_cache_bit = 4;
	if ((s_wx_assert_cached_bits & 4) == 0 && wxGetKeyState(WXK_CAPITAL)) {
		out|= CAPSLOCK_ON;
	}

	s_wx_assert_cache_bit = 0;

	if (g_remote) {
		s_remote_time_avg+= (unsigned int)(GetProcessUptimeMSec() - stopwatch);
		s_remote_time_avg/= 2;
		if (s_remote_time_avg > REMOTE_SLOWNESS_TRSH_MSEC) {
			fprintf(stderr, "%s: remote is slow (%u)\n", __FUNCTION__, s_remote_time_avg);
		}
	}

	return out;
}

void WinPortWxAssertHandler(const wxString& file, int line, const wxString& func, const wxString& cond, const wxString& msg)
{
	s_wx_assert_cached_bits|= s_wx_assert_cache_bit;

	fprintf(stderr, "%s: file='%ls' line=%d func='%ls' cond='%ls' msg='%ls'\n",
			__FUNCTION__,
			static_cast<const wchar_t*>(file.wc_str()), line,
			static_cast<const wchar_t*>(func.wc_str()),
			static_cast<const wchar_t*>(cond.wc_str()),
			static_cast<const wchar_t*>(msg.wc_str()));
}

