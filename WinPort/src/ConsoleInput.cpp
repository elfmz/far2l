#include <assert.h>
#include "ConsoleInput.h"

const char* VirtualKeyNames[] = {
    "ERR",              // 0x00
    "VK_LBUTTON",       // 0x01
    "VK_RBUTTON",       // 0x02
    "VK_CANCEL",        // 0x03
    "VK_MBUTTON",       // 0x04
    "VK_XBUTTON1",      // 0x05
    "VK_XBUTTON2",      // 0x06
    "ERR",              // 0x07
    "VK_BACK",          // 0x08
    "VK_TAB",           // 0x09
    "ERR",              // 0x0A
    "ERR",              // 0x0B
    "VK_CLEAR",         // 0x0C
    "VK_RETURN",        // 0x0D
    "ERR",              // 0x0E
    "ERR",              // 0x0F
    "VK_SHIFT",         // 0x10
    "VK_CONTROL",       // 0x11
    "VK_MENU",          // 0x12
    "VK_PAUSE",         // 0x13
    "VK_CAPITAL",       // 0x14
    "VK_HANGUEL",       // 0x15
    "ERR",              // 0x16
    "VK_JUNJA",         // 0x17
    "VK_FINAL",         // 0x18
    "VK_HANJA",         // 0x19
    "ERR",              // 0x1A
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
    "ERR",              // 0x3A
    "ERR",              // 0x3B
    "ERR",              // 0x3C
    "ERR",              // 0x3D
    "ERR",              // 0x3E
    "ERR",              // 0x3F
    "ERR",              // 0x40
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
    "ERR",              // 0x5E
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
    "ERR",              // 0x88
    "ERR",              // 0x89
    "ERR",              // 0x8A
    "ERR",              // 0x8B
    "ERR",              // 0x8C
    "ERR",              // 0x8D
    "ERR",              // 0x8E
    "ERR",              // 0x8F
    "VK_NUMLOCK",       // 0x90
    "VK_SCROLL",        // 0x91
    "ERR",              // 0x92
    "ERR",              // 0x93
    "ERR",              // 0x94
    "ERR",              // 0x95
    "ERR",              // 0x96
    "ERR",              // 0x97
    "ERR",              // 0x98
    "ERR",              // 0x99
    "ERR",              // 0x9A
    "ERR",              // 0x9B
    "ERR",              // 0x9C
    "ERR",              // 0x9D
    "ERR",              // 0x9E
    "ERR",              // 0x9F
    "VK_LSHIFT",        // 0xA0
    "VK_RSHIFT",        // 0xA1
    "VK_LCONTROL",      // 0xA2
    "VK_RCONTROL",      // 0xA3
    "VK_LMENU",         // 0xA4
    "VK_RMENU",         // 0xA5
    "VK_BROWSER_BACK",  // 0xA6
    "VK_BROWSER_FORWARD", // 0xA7
    "VK_BROWSER_REFRESH", // 0xA8
    "VK_BROWSER_STOP",  // 0xA9
    "VK_BROWSER_SEARCH", // 0xAA
    "VK_BROWSER_FAVORITES", // 0xAB
    "VK_BROWSER_HOME",  // 0xAC
    "VK_VOLUME_MUTE",   // 0xAD
    "VK_VOLUME_DOWN",   // 0xAE
    "VK_VOLUME_UP",     // 0xAF
    "VK_MEDIA_NEXT_TRACK", // 0xB0
    "VK_MEDIA_PREV_TRACK", // 0xB1
    "VK_MEDIA_STOP",    // 0xB2
    "VK_MEDIA_PLAY_PAUSE", // 0xB3
    "VK_LAUNCH_MAIL",   // 0xB4
    "VK_LAUNCH_MEDIA_SELECT", // 0xB5
    "VK_LAUNCH_APP1",   // 0xB6
    "VK_LAUNCH_APP2",   // 0xB7
    "ERR",              // 0xB8
    "ERR",              // 0xB9
    "VK_OEM_1",         // 0xBA
    "VK_OEM_PLUS",      // 0xBB
    "VK_OEM_COMMA",     // 0xBC
    "VK_OEM_MINUS",     // 0xBD
    "VK_OEM_PERIOD",    // 0xBE
    "VK_OEM_2",         // 0xBF
    "VK_OEM_3",         // 0xC0
    "ERR",              // 0xC1
    "ERR",              // 0xC2
    "ERR",              // 0xC3
    "ERR",              // 0xC4
    "ERR",              // 0xC5
    "ERR",              // 0xC6
    "ERR",              // 0xC7
    "ERR",              // 0xC8
    "ERR",              // 0xC9
    "ERR",              // 0xCA
    "ERR",              // 0xCB
    "ERR",              // 0xCC
    "ERR",              // 0xCD
    "ERR",              // 0xCE
    "ERR",              // 0xCF
    "ERR",              // 0xD0
    "ERR",              // 0xD1
    "ERR",              // 0xD2
    "ERR",              // 0xD3
    "ERR",              // 0xD4
    "ERR",              // 0xD5
    "ERR",              // 0xD6
    "ERR",              // 0xD7
    "ERR",              // 0xD8
    "ERR",              // 0xD9
    "ERR",              // 0xDA
    "VK_OEM_4",         // 0xDB
    "VK_OEM_5",         // 0xDC
    "VK_OEM_6",         // 0xDD
    "VK_OEM_7",         // 0xDE
    "VK_OEM_8",         // 0xDF
    "ERR",              // 0xE0
    "ERR",              // 0xE1
    "VK_OEM_102",       // 0xE2
    "ERR",              // 0xE3
    "ERR",              // 0xE4
    "VK_PROCESSKEY",    // 0xE5
    "ERR",              // 0xE6
    "VK_PACKET",        // 0xE7
    "ERR",              // 0xE8
    "ERR",              // 0xE9
    "ERR",              // 0xEA
    "ERR",              // 0xEB
    "ERR",              // 0xEC
    "ERR",              // 0xED
    "ERR",              // 0xEE
    "ERR",              // 0xEF
    "ERR",              // 0xF0
    "ERR",              // 0xF1
    "ERR",              // 0xF2
    "ERR",              // 0xF3
    "ERR",              // 0xF4
    "ERR",              // 0xF5
    "VK_ATTN",          // 0xF6
    "VK_CRSEL",         // 0xF7
    "VK_EXSEL",         // 0xF8
    "VK_EREOF",         // 0xF9
    "VK_PLAY",          // 0xFA
    "VK_ZOOM",          // 0xFB
    "VK_NONAME",        // 0xFC
    "VK_PA1",           // 0xFD
    "VK_OEM_CLEAR"      // 0xFE
};

char* FormatKeyState(uint16_t state) {

	static char buffer[21];

	buffer[0]  = state & NUMLOCK_ON         ? 'N' : 'n';
	buffer[1]  = state & CAPSLOCK_ON        ? 'C' : 'c';
	buffer[2]  = state & SCROLLLOCK_ON      ? 'S' : 's';
	buffer[3]  = ' ';

	buffer[4]  = state & ENHANCED_KEY       ? 'E' : 'e';
	buffer[5]  = ' ';

	buffer[6]  = state & LEFT_ALT_PRESSED   ? 'L' : 'l';
	buffer[7]  = state & LEFT_ALT_PRESSED   ? 'A' : 'a';
	buffer[8]  = ' ';

	buffer[9]  = state & LEFT_CTRL_PRESSED  ? 'L' : 'l';
	buffer[10] = state & LEFT_CTRL_PRESSED  ? 'C' : 'c';
	buffer[11] = ' ';

	buffer[12] = state & SHIFT_PRESSED      ? 'S' : 's';
	buffer[13] = state & SHIFT_PRESSED      ? 'H' : 'h';
	buffer[14] = ' ';

	buffer[15] = state & RIGHT_ALT_PRESSED  ? 'R' : 'r';
	buffer[16] = state & RIGHT_ALT_PRESSED  ? 'A' : 'a';
	buffer[17] = ' ';

	buffer[18] = state & RIGHT_CTRL_PRESSED ? 'R' : 'r';
	buffer[19] = state & RIGHT_CTRL_PRESSED ? 'C' : 'c';

	buffer[20] = '\0';

	return buffer;
}


void ConsoleInput::Enqueue(const INPUT_RECORD *data, DWORD size)
{
	if (size) {
		for (DWORD i = 0; i < size; ++i) {
			if (data[i].EventType == KEY_EVENT) {
				fprintf(stderr, "ConsoleInput::Enqueue: %s %s \"%lc\" %s, %x %x %x %x\n",
					FormatKeyState(data[i].Event.KeyEvent.dwControlKeyState),
					VirtualKeyNames[data[i].Event.KeyEvent.wVirtualKeyCode],
					data[i].Event.KeyEvent.uChar.UnicodeChar ? data[i].Event.KeyEvent.uChar.UnicodeChar : '#',
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

DWORD ConsoleInput::Dequeue(INPUT_RECORD *data, DWORD size, unsigned int requestor_priority)
{
	DWORD i;
	{
		std::unique_lock<std::mutex> lock(_mutex);
		if (requestor_priority < CurrentPriority()) {
			// fprintf(stderr,"%s: requestor_priority %u < %u\n", __FUNCTION__, requestor_priority, CurrentPriority());
			return 0;
		}

		for (i = 0; (i < size && !_pending.empty()); ++i) {
			data[i] = _pending.front();
			_pending.pop_front();
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

void ConsoleInput::JoinConsoleInput(IConsoleInput *con_in)
{
	ConsoleInput *ci = (ConsoleInput *)con_in;
	if (!ci->_pending.empty()) {
		std::unique_lock<std::mutex> lock(_mutex);
		for (const auto &evnt : ci->_pending) {
			_pending.emplace_back(evnt);
		}
		_non_empty.notify_all();
	}
	delete ci;
}

///
