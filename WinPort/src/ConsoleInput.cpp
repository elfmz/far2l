#include <assert.h>
#include "ConsoleInput.h"
#include "WinPort.h"
#include <UtfDefines.h>
#include <utils.h>

#define MAX_INPUT_BACKTRACE_SECONDS 10 // how max old events can be kept in backtrace
#define MAX_INPUT_BACKTRACE_CONUT   10 // how many events can be kept in backtrace regardless of their age

static const char* VirtualKeyNames[] = {
    "0x00",             // 0x00
    "VK_LBUTTON",       // 0x01
    "VK_RBUTTON",       // 0x02
    "VK_CANCEL",        // 0x03
    "VK_MBUTTON",       // 0x04
    "VK_XBUTTON1",      // 0x05
    "VK_XBUTTON2",      // 0x06
    "0x07",             // 0x07
    "VK_BACK",          // 0x08
    "VK_TAB",           // 0x09
    "0x0A",             // 0x0A
    "0x0B",             // 0x0B
    "VK_CLEAR",         // 0x0C
    "VK_RETURN",        // 0x0D
    "0x0E",             // 0x0E
    "0x0F",             // 0x0F
    "VK_SHIFT",         // 0x10
    "VK_CONTROL",       // 0x11
    "VK_MENU",          // 0x12
    "VK_PAUSE",         // 0x13
    "VK_CAPITAL",       // 0x14
    "VK_HANGUEL",       // 0x15
    "0x16",             // 0x16
    "VK_JUNJA",         // 0x17
    "VK_FINAL",         // 0x18
    "VK_HANJA",         // 0x19
    "0x1A",             // 0x1A
    "VK_ESCAPE",        // 0x1B
    "VK_CONVERT",       // 0x1C
    "VK_NONCONVERT",    // 0x1D
    "VK_ACCEPT",        // 0x1E
    "VK_MODECHANGE",    // 0x1F
    "VK_SPACE",         // 0x20
    "VK_PRIOR",         // 0x21
    "VK_NEXT",          // 0x22
    "VK_END",           // 0x23
    "VK_HOME",          // 0x24
    "VK_LEFT",          // 0x25
    "VK_UP",            // 0x26
    "VK_RIGHT",         // 0x27
    "VK_DOWN",          // 0x28
    "VK_SELECT",        // 0x29
    "VK_PRINT",         // 0x2A
    "VK_EXECUTE",       // 0x2B
    "VK_SNAPSHOT",      // 0x2C
    "VK_INSERT",        // 0x2D
    "VK_DELETE",        // 0x2E
    "VK_HELP",          // 0x2F
    "VK_0",             // 0x30
    "VK_1",             // 0x31
    "VK_2",             // 0x32
    "VK_3",             // 0x33
    "VK_4",             // 0x34
    "VK_5",             // 0x35
    "VK_6",             // 0x36
    "VK_7",             // 0x37
    "VK_8",             // 0x38
    "VK_9",             // 0x39
    "0x3A",             // 0x3A
    "0x3B",             // 0x3B
    "0x3C",             // 0x3C
    "0x3D",             // 0x3D
    "0x3E",             // 0x3E
    "0x3F",             // 0x3F
    "0x40",             // 0x40
    "VK_A",             // 0x41
    "VK_B",             // 0x42
    "VK_C",             // 0x43
    "VK_D",             // 0x44
    "VK_E",             // 0x45
    "VK_F",             // 0x46
    "VK_G",             // 0x47
    "VK_H",             // 0x48
    "VK_I",             // 0x49
    "VK_J",             // 0x4A
    "VK_K",             // 0x4B
    "VK_L",             // 0x4C
    "VK_M",             // 0x4D
    "VK_N",             // 0x4E
    "VK_O",             // 0x4F
    "VK_P",             // 0x50
    "VK_Q",             // 0x51
    "VK_R",             // 0x52
    "VK_S",             // 0x53
    "VK_T",             // 0x54
    "VK_U",             // 0x55
    "VK_V",             // 0x56
    "VK_W",             // 0x57
    "VK_X",             // 0x58
    "VK_Y",             // 0x59
    "VK_Z",             // 0x5A
    "VK_LWIN",          // 0x5B
    "VK_RWIN",          // 0x5C
    "VK_APPS",          // 0x5D
    "0x5E",             // 0x5E
    "VK_SLEEP",         // 0x5F
    "VK_NUMPAD0",       // 0x60
    "VK_NUMPAD1",       // 0x61
    "VK_NUMPAD2",       // 0x62
    "VK_NUMPAD3",       // 0x63
    "VK_NUMPAD4",       // 0x64
    "VK_NUMPAD5",       // 0x65
    "VK_NUMPAD6",       // 0x66
    "VK_NUMPAD7",       // 0x67
    "VK_NUMPAD8",       // 0x68
    "VK_NUMPAD9",       // 0x69
    "VK_MULTIPLY",      // 0x6A
    "VK_ADD",           // 0x6B
    "VK_SEPARATOR",     // 0x6C
    "VK_SUBTRACT",      // 0x6D
    "VK_DECIMAL",       // 0x6E
    "VK_DIVIDE",        // 0x6F
    "VK_F1",            // 0x70
    "VK_F2",            // 0x71
    "VK_F3",            // 0x72
    "VK_F4",            // 0x73
    "VK_F5",            // 0x74
    "VK_F6",            // 0x75
    "VK_F7",            // 0x76
    "VK_F8",            // 0x77
    "VK_F9",            // 0x78
    "VK_F10",           // 0x79
    "VK_F11",           // 0x7A
    "VK_F12",           // 0x7B
    "VK_F13",           // 0x7C
    "VK_F14",           // 0x7D
    "VK_F15",           // 0x7E
    "VK_F16",           // 0x7F
    "VK_F17",           // 0x80
    "VK_F18",           // 0x81
    "VK_F19",           // 0x82
    "VK_F20",           // 0x83
    "VK_F21",           // 0x84
    "VK_F22",           // 0x85
    "VK_F23",           // 0x86
    "VK_F24",           // 0x87
    "0x88",             // 0x88
    "0x89",             // 0x89
    "0x8A",             // 0x8A
    "0x8B",             // 0x8B
    "0x8C",             // 0x8C
    "0x8D",             // 0x8D
    "0x8E",             // 0x8E
    "0x8F",             // 0x8F
    "VK_NUMLOCK",       // 0x90
    "VK_SCROLL",        // 0x91
    "0x92",             // 0x92
    "0x93",             // 0x93
    "0x94",             // 0x94
    "0x95",             // 0x95
    "0x96",             // 0x96
    "0x97",             // 0x97
    "0x98",             // 0x98
    "0x99",             // 0x99
    "0x9A",             // 0x9A
    "0x9B",             // 0x9B
    "0x9C",             // 0x9C
    "0x9D",             // 0x9D
    "0x9E",             // 0x9E
    "0x9F",             // 0x9F
    "VK_LSHIFT",        // 0xA0
    "VK_RSHIFT",        // 0xA1
    "VK_LCONTROL",      // 0xA2
    "VK_RCONTROL",      // 0xA3
    "VK_LMENU",         // 0xA4
    "VK_RMENU",         // 0xA5
    "VK_BROWSER_BACK",         // 0xA6
    "VK_BROWSER_FORWARD",      // 0xA7
    "VK_BROWSER_REFRESH",      // 0xA8
    "VK_BROWSER_STOP",         // 0xA9
    "VK_BROWSER_SEARCH",       // 0xAA
    "VK_BROWSER_FAVORITES",    // 0xAB
    "VK_BROWSER_HOME",         // 0xAC
    "VK_VOLUME_MUTE",          // 0xAD
    "VK_VOLUME_DOWN",          // 0xAE
    "VK_VOLUME_UP",            // 0xAF
    "VK_MEDIA_NEXT_TRACK",     // 0xB0
    "VK_MEDIA_PREV_TRACK",     // 0xB1
    "VK_MEDIA_STOP",           // 0xB2
    "VK_MEDIA_PLAY_PAUSE",     // 0xB3
    "VK_LAUNCH_MAIL",          // 0xB4
    "VK_LAUNCH_MEDIA_SELECT",  // 0xB5
    "VK_LAUNCH_APP1",          // 0xB6
    "VK_LAUNCH_APP2",          // 0xB7
    "0xB8",             // 0xB8
    "0xB9",             // 0xB9
    "VK_OEM_1",         // 0xBA
    "VK_OEM_PLUS",      // 0xBB
    "VK_OEM_COMMA",     // 0xBC
    "VK_OEM_MINUS",     // 0xBD
    "VK_OEM_PERIOD",    // 0xBE
    "VK_OEM_2",         // 0xBF
    "VK_OEM_3",         // 0xC0
    "0xC1",             // 0xC1
    "0xC2",             // 0xC2
    "0xC3",             // 0xC3
    "0xC4",             // 0xC4
    "0xC5",             // 0xC5
    "0xC6",             // 0xC6
    "0xC7",             // 0xC7
    "0xC8",             // 0xC8
    "0xC9",             // 0xC9
    "0xCA",             // 0xCA
    "0xCB",             // 0xCB
    "0xCC",             // 0xCC
    "0xCD",             // 0xCD
    "0xCE",             // 0xCE
    "0xCF",             // 0xCF
    "0xD0",             // 0xD0
    "0xD1",             // 0xD1
    "0xD2",             // 0xD2
    "0xD3",             // 0xD3
    "0xD4",             // 0xD4
    "0xD5",             // 0xD5
    "0xD6",             // 0xD6
    "0xD7",             // 0xD7
    "0xD8",             // 0xD8
    "0xD9",             // 0xD9
    "0xDA",             // 0xDA
    "VK_OEM_4",         // 0xDB
    "VK_OEM_5",         // 0xDC
    "VK_OEM_6",         // 0xDD
    "VK_OEM_7",         // 0xDE
    "VK_OEM_8",         // 0xDF
    "0xE0",             // 0xE0
    "0xE1",             // 0xE1
    "VK_OEM_102",       // 0xE2
    "0xE3",             // 0xE3
    "0xE4",             // 0xE4
    "VK_PROCESSKEY",    // 0xE5
    "0xE6",             // 0xE6
    "VK_PACKET",        // 0xE7
    "0xE8",             // 0xE8
    "0xE9",             // 0xE9
    "0xEA",             // 0xEA
    "0xEB",             // 0xEB
    "0xEC",             // 0xEC
    "0xED",             // 0xED
    "0xEE",             // 0xEE
    "0xEF",             // 0xEF
    "0xF0",             // 0xF0
    "0xF1",             // 0xF1
    "0xF2",             // 0xF2
    "0xF3",             // 0xF3
    "0xF4",             // 0xF4
    "0xF5",             // 0xF5
    "VK_ATTN",          // 0xF6
    "VK_CRSEL",         // 0xF7
    "VK_EXSEL",         // 0xF8
    "VK_EREOF",         // 0xF9
    "VK_PLAY",          // 0xFA
    "VK_ZOOM",          // 0xFB
    "VK_NONAME",        // 0xFC
    "VK_PA1",           // 0xFD
    "VK_OEM_CLEAR",     // 0xFE
    "0xFF"              // 0xFF
};

static const char *FormatKeyName(unsigned vk)
{
	if (vk >= ARRAYSIZE(VirtualKeyNames)) {
		return "?";
	}
	return VirtualKeyNames[vk];
}

struct FormatControlKey
{
	char str[24];

	FormatControlKey(uint16_t state)
	{
		str[0]  = state & NUMLOCK_ON         ? 'N' : 'n';
		str[1]  = state & CAPSLOCK_ON        ? 'C' : 'c';
		str[2]  = state & SCROLLLOCK_ON      ? 'S' : 's';
		str[3]  = ' ';

		str[4]  = state & ENHANCED_KEY       ? 'E' : 'e';
		str[5]  = ' ';

		str[6]  = state & LEFT_ALT_PRESSED   ? 'L' : 'l';
		str[7]  = state & LEFT_ALT_PRESSED   ? 'A' : 'a';
		str[8]  = ' ';

		str[9]  = state & LEFT_CTRL_PRESSED  ? 'L' : 'l';
		str[10] = state & LEFT_CTRL_PRESSED  ? 'C' : 'c';
		str[11] = ' ';

		str[12] = state & SHIFT_PRESSED      ? 'S' : 's';
		str[13] = state & SHIFT_PRESSED      ? 'H' : 'h';
		str[14] = ' ';

		str[15] = state & RIGHT_ALT_PRESSED  ? 'R' : 'r';
		str[16] = state & RIGHT_ALT_PRESSED  ? 'A' : 'a';
		str[17] = ' ';

		str[18] = state & RIGHT_CTRL_PRESSED ? 'R' : 'r';
		str[19] = state & RIGHT_CTRL_PRESSED ? 'C' : 'c';

		str[20] = '\0';
	}
};

static wchar_t FormatUnicodeChar(wchar_t uni)
{
	if (uni < 0x1f || !WCHAR_IS_VALID(uni)) {
		return L'?';
	}
	if (WCHAR_IS_COMBINING(uni)) {
		return L'!';
	}
	return uni;
}


void ConsoleInput::Enqueue(const INPUT_RECORD *data, DWORD size)
{
	if (size) {
		for (DWORD i = 0; i < size; ++i) {
			if (data[i].EventType == KEY_EVENT) {
				const auto kn = FormatKeyName(data[i].Event.KeyEvent.wVirtualKeyCode);
				FormatControlKey ks(data[i].Event.KeyEvent.dwControlKeyState);
				const auto uc = FormatUnicodeChar(data[i].Event.KeyEvent.uChar.UnicodeChar);
				fprintf(stderr, "ConsoleInput::Enqueue: %s %s \"%lc\" %s, %x %x %x %x\n",
					ks.str, kn, uc,
					data[i].Event.KeyEvent.bKeyDown ? "DOWN" : "UP",

					data[i].Event.KeyEvent.dwControlKeyState,
					data[i].Event.KeyEvent.wVirtualKeyCode,
					data[i].Event.KeyEvent.uChar.UnicodeChar,

					data[i].Event.KeyEvent.wVirtualScanCode
				);
			}
		}

		std::unique_lock<std::mutex> lock(_mutex);
		for (DWORD i = 0; i < size; ++i)
			_pending.push_back(data[i]);

		_non_empty.notify_all();
	}
}

static void InspectCallbacks(INPUT_RECORD *data, DWORD size, bool invoke)
{
	for (DWORD i = 0; i < size; ++i) {
		if (data[i].EventType == CALLBACK_EVENT) {
			data[i].EventType = NOOP_EVENT;
			if (invoke) {
				data[i].Event.CallbackEvent.Function(data[i].Event.CallbackEvent.Context);
			}
		}
	}
}

DWORD ConsoleInput::Peek(INPUT_RECORD *data, DWORD size, unsigned int requestor_priority)
{
	DWORD i;
	{
		std::unique_lock<std::mutex> lock(_mutex);
		if (requestor_priority < CurrentPriority()) {
			//fprintf(stderr,"%s: requestor_priority %u < %u\n", __FUNCTION__, requestor_priority, CurrentPriority());
			return 0;
		}

		for (i = 0; (i < size && i < _pending.size()); ++i)
			data[i] = _pending[i];
	}
	InspectCallbacks(data, i, false);

	//if (i) {
	//	fprintf(stderr,"%s: result %u\n", __FUNCTION__, i);
	//}

	return i;
}

static bool EventBacktraced(const INPUT_RECORD &evnt)
{
	if (evnt.EventType == MOUSE_EVENT) {
		if (evnt.Event.MouseEvent.dwButtonState == 0
				&& (evnt.Event.MouseEvent.dwEventFlags & (MOUSE_MOVED | MOUSE_HWHEELED | MOUSE_WHEELED)) != 0) {
			return false;
		}

	}

	return true;
}

DWORD ConsoleInput::Dequeue(INPUT_RECORD *data, DWORD size, unsigned int requestor_priority)
{
	DWORD i;
	{
		clock_t now = GetProcessUptimeMSec();
		std::unique_lock<std::mutex> lock(_mutex);
		if (requestor_priority < CurrentPriority()) {
			// fprintf(stderr,"%s: requestor_priority %u < %u\n", __FUNCTION__, requestor_priority, CurrentPriority());
			return 0;
		}

		for (i = 0; (i < size && !_pending.empty()); ++i) {
			data[i] = _pending.front();
			_pending.pop_front();
			if (EventBacktraced(data[i])) {
				while (_backtrace.size() > MAX_INPUT_BACKTRACE_CONUT
						&& now - _backtrace.front().first > MAX_INPUT_BACKTRACE_SECONDS * 1000) {
					_backtrace.pop_front();
				}
				auto &back = _backtrace.emplace_back();
				back.first = now;
				back.second = data[i];
			}
		}
	}
	InspectCallbacks(data, i, true);

	// fprintf(stderr,"%s: result %u\n", __FUNCTION__, i);
	return i;
}

DWORD ConsoleInput::Count(unsigned int requestor_priority)
{
	std::unique_lock<std::mutex> lock(_mutex);
	return (requestor_priority >= CurrentPriority()) ? (DWORD)_pending.size() : 0;
}

DWORD ConsoleInput::Flush(unsigned int requestor_priority)
{
	std::unique_lock<std::mutex> lock(_mutex);
	if (requestor_priority < CurrentPriority())
		return 0;

	DWORD rv = _pending.size();
	_pending.clear();
	return rv;
}

void ConsoleInput::WaitForNonEmpty(unsigned int requestor_priority)
{
	std::unique_lock<std::mutex> lock(_mutex);
	while  (_pending.empty() || requestor_priority < CurrentPriority()) {
		_non_empty.wait(lock);
	}
}

bool ConsoleInput::WaitForNonEmptyWithTimeout(unsigned int timeout_msec, unsigned int requestor_priority)
{
	std::unique_lock<std::mutex> lock(_mutex);
	for (;;) {
		if (!_pending.empty() && requestor_priority >= CurrentPriority())
			return true;

		if (!timeout_msec)
			return false;

		std::chrono::milliseconds ms_before = std::chrono::duration_cast< std::chrono::milliseconds >
			(std::chrono::steady_clock::now().time_since_epoch());

		_non_empty.wait_for(lock, std::chrono::milliseconds(timeout_msec));

		std::chrono::milliseconds ms = std::chrono::duration_cast< std::chrono::milliseconds >
			(std::chrono::steady_clock::now().time_since_epoch());

		ms-= ms_before;

		if (ms.count() < timeout_msec)
			timeout_msec-= ms.count();
		else
			timeout_msec = 0;
	}
}

unsigned int ConsoleInput::RaiseRequestorPriority()
{
	std::unique_lock<std::mutex> lock(_mutex);
	unsigned int cur_priority = CurrentPriority();
	unsigned int new_priority = cur_priority + 1;
	assert( new_priority > cur_priority);
	_requestor_priorities.insert(new_priority);

	fprintf(stderr,"%s: new_priority=%u\n", __FUNCTION__, new_priority);
	return new_priority;
}

void ConsoleInput::LowerRequestorPriority(unsigned int released_priority)
{
	std::unique_lock<std::mutex> lock(_mutex);
	const size_t nerased = _requestor_priorities.erase(released_priority);
	assert(nerased != 0);
	if (!_pending.empty())
		_non_empty.notify_all();

	fprintf(stderr,"%s: released_priority=%u CurrentPriority()=%u nerased=%lu nremain=%lu\n",
		__FUNCTION__, released_priority, CurrentPriority(),
		(unsigned long)nerased, (unsigned long)_requestor_priorities.size());
}

unsigned int ConsoleInput::CurrentPriority() const
{
	if (_requestor_priorities.empty())
		return 0;

	return *_requestor_priorities.rbegin();
}


IConsoleInput *ConsoleInput::ForkConsoleInput(HANDLE con_handle)
{
	return new ConsoleInput;
}

void ConsoleInput::ReleaseConsoleInput(IConsoleInput *con_in, bool join)
{
	ConsoleInput *ci = (ConsoleInput *)con_in;
	if (join && !ci->_pending.empty()) {
		std::unique_lock<std::mutex> lock(_mutex);
		for (const auto &evnt : ci->_pending) {
			_pending.emplace_back(evnt);
		}
		_non_empty.notify_all();
	}
	delete ci;
}

DWORD ConsoleInput::GetBacktrace(CHAR *buf, DWORD size)
{
	std::string s;
	const clock_t now = GetProcessUptimeMSec();
	std::unique_lock<std::mutex> lock(_mutex);
	DWORD i = 0;
	for (auto it = _backtrace.rbegin(); i < size && it != _backtrace.rend(); ++i, ++it) {
		switch (it->second.EventType) {
			case KEY_EVENT: {
				const auto kn = FormatKeyName(it->second.Event.KeyEvent.wVirtualKeyCode);
				FormatControlKey ks(it->second.Event.KeyEvent.dwControlKeyState);
				const auto uc = FormatUnicodeChar(it->second.Event.KeyEvent.uChar.UnicodeChar);
				s+= StrPrintf("%04u KEY_%s: vkc=%u vsc=%u ctl=0x%x wc=%u %s %s '%lc'\n", (unsigned int)(now - it->first),
						it->second.Event.KeyEvent.bKeyDown ? "DOWN" : "UP",
						(unsigned int)it->second.Event.KeyEvent.wVirtualKeyCode,
						(unsigned int)it->second.Event.KeyEvent.wVirtualScanCode,
						(unsigned int)it->second.Event.KeyEvent.dwControlKeyState,
						(unsigned int)it->second.Event.KeyEvent.uChar.UnicodeChar,
						ks.str, kn, uc);
				} break;

				case FOCUS_EVENT:
					s+= StrPrintf("%04u FOCUS: %s\n", (unsigned int)(now - it->first),
						it->second.Event.FocusEvent.bSetFocus ? "set" : "unset");
					break;

				case MOUSE_EVENT:
					s+= StrPrintf("%04u MOUSE: btn=0x%x ctl=0x%x flg=0x%x pos={%d.%d}\n", (unsigned int)(now - it->first),
						it->second.Event.MouseEvent.dwButtonState,
						it->second.Event.MouseEvent.dwControlKeyState,
						it->second.Event.MouseEvent.dwEventFlags,
						(int)it->second.Event.MouseEvent.dwMousePosition.X, (int)it->second.Event.MouseEvent.dwMousePosition.Y);
					break;

				case WINDOW_BUFFER_SIZE_EVENT:
					s+= StrPrintf("%04u WINSIZE: sz={%d.%d} dmg=%d\n", (unsigned int)(now - it->first),
						(int)it->second.Event.WindowBufferSizeEvent.dwSize.X, (int)it->second.Event.WindowBufferSizeEvent.dwSize.Y,
						it->second.Event.WindowBufferSizeEvent.bDamaged);
					break;

				case CALLBACK_EVENT:
					s+= StrPrintf("%04u CALLBACK: fn=%p ctx=%p\n", (unsigned int)(now - it->first),
						it->second.Event.CallbackEvent.Function,
						it->second.Event.CallbackEvent.Context);
					break;

				case BRACKETED_PASTE_EVENT:
					s+= StrPrintf("%04u BRACKETED_PASTE: %s\n", (unsigned int)(now - it->first),
						it->second.Event.BracketedPaste.bStartPaste ? "start" : "stop");
					break;

				case NOOP_EVENT:
					s+= StrPrintf("%04u NOOP\n", (unsigned int)(now - it->first));
					break;

				default:
					s+= StrPrintf("%04u OTHER: type=0x%x\n", (unsigned int)(now - it->first), it->second.EventType);
			}
	}
	if (size) {
		if (size > s.size() + 1) {
			size = s.size() + 1;
		}
		memcpy(buf, s.c_str(), size);
		buf[size - 1] = 0;
	}
	return s.size() + 1;
}

///
