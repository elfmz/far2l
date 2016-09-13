#include "ConsoleInput.h"

#include <wx/wx.h>
#include <wx/display.h>

void ConsoleInput::Enqueue(const INPUT_RECORD *data, DWORD size)
{
	if (data->EventType == KEY_EVENT) {
		fprintf(stderr, "ConsoleInput::Enqueue: %lc %x %x %x %s\n", 
			data->Event.KeyEvent.uChar.UnicodeChar,
			data->Event.KeyEvent.uChar.UnicodeChar,
			data->Event.KeyEvent.wVirtualKeyCode,
			data->Event.KeyEvent.dwControlKeyState,
			data->Event.KeyEvent.bKeyDown ? "DOWN" : "UP");
	}
	std::unique_lock<std::mutex> lock(_mutex);
	for (DWORD i = 0; i < size; ++i)
		_pending.push_back(data[i]);
	if (size)
		_non_empty.notify_all();
}

DWORD ConsoleInput::Peek(INPUT_RECORD *data, DWORD size)
{
	DWORD i;
	std::unique_lock<std::mutex> lock(_mutex);
	for (i = 0; (i < size && i < _pending.size()); ++i)
		data[i] = _pending[i];
	return i;
}

DWORD ConsoleInput::Dequeue(INPUT_RECORD *data, DWORD size)
{
	DWORD i;
	std::unique_lock<std::mutex> lock(_mutex);
	for (i = 0; (i < size && !_pending.empty()); ++i) {
		data[i] = _pending.front();
		_pending.pop_front();
	}
	return i;
}

DWORD ConsoleInput::Count()
{
	std::unique_lock<std::mutex> lock(_mutex);
	return (DWORD)_pending.size();
}

DWORD ConsoleInput::Flush()
{
	std::unique_lock<std::mutex> lock(_mutex);
	DWORD rv = _pending.size();
	_pending.clear();
	return rv;
}

void ConsoleInput::WaitForNonEmpty()
{
	for (;;) {
		std::unique_lock<std::mutex> lock(_mutex);
		if (!_pending.empty()) break;
		_non_empty.wait(lock);
	}
}
