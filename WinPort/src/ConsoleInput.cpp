#include <assert.h>
#include "ConsoleInput.h"

void ConsoleInput::Enqueue(const INPUT_RECORD *data, DWORD size)
{
	if (data->EventType == KEY_EVENT) {
		fprintf(stderr, "ConsoleInput::Enqueue: %lc %x %x %x %s\n", 
			data->Event.KeyEvent.uChar.UnicodeChar ? data->Event.KeyEvent.uChar.UnicodeChar : '#',
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

DWORD ConsoleInput::Peek(INPUT_RECORD *data, DWORD size, unsigned int requestor_priority)
{
	DWORD i;
	std::unique_lock<std::mutex> lock(_mutex);
	if (requestor_priority < CurrentPriority())
		return 0;

	for (i = 0; (i < size && i < _pending.size()); ++i)
		data[i] = _pending[i];
	return i;
}

DWORD ConsoleInput::Dequeue(INPUT_RECORD *data, DWORD size, unsigned int requestor_priority)
{
	DWORD i;
	std::unique_lock<std::mutex> lock(_mutex);
	if (requestor_priority < CurrentPriority())
		return 0;

	for (i = 0; (i < size && !_pending.empty()); ++i) {
		data[i] = _pending.front();
		_pending.pop_front();
	}
	return i;
}

DWORD ConsoleInput::Count(unsigned int requestor_priority)
{
	std::unique_lock<std::mutex> lock(_mutex);
	return  (requestor_priority >= CurrentPriority()) ? (DWORD)_pending.size() : 0;
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

bool ConsoleInput::WaitForNonEmpty(unsigned int timeout_msec, unsigned int requestor_priority)
{
	std::unique_lock<std::mutex> lock(_mutex);

	if (timeout_msec == (unsigned int)-1) {
		for (;;) {
			if (!_pending.empty() && requestor_priority >= CurrentPriority())
				return true;

			_non_empty.wait(lock);
		}

	} else {
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
}

unsigned int ConsoleInput::RaiseRequestorPriority()
{
	std::unique_lock<std::mutex> lock(_mutex);
	unsigned int cur_priority = CurrentPriority();
	unsigned int new_priority = cur_priority + 1;
	assert( new_priority > cur_priority);
	_requestor_priorities.insert(new_priority);
	return new_priority;
}

void ConsoleInput::LowerRequestorPriority(unsigned int released_priority)
{
	std::unique_lock<std::mutex> lock(_mutex);
	assert(_requestor_priorities.erase(released_priority) > 0);
	if (!_pending.empty())
		_non_empty.notify_all();
}

unsigned int ConsoleInput::CurrentPriority() const
{
	if (_requestor_priorities.empty())
		return 0;

	auto last = _requestor_priorities.end();
	--last;
	return *last;
}

///
ConsoleInputPriority::ConsoleInputPriority(ConsoleInput &con_input)
	: _con_input(con_input),
	_my_priority(con_input.RaiseRequestorPriority())
{
}

ConsoleInputPriority::~ConsoleInputPriority()
{
	_con_input.LowerRequestorPriority(_my_priority);
}

