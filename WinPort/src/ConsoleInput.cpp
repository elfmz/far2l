#include <assert.h>
#include "ConsoleInput.h"

void ConsoleInput::Enqueue(const INPUT_RECORD *data, DWORD size)
{
	if (size) {
		for (DWORD i = 0; i < size; ++i) {
			if (data[i].EventType == KEY_EVENT) {
				fprintf(stderr, "ConsoleInput::Enqueue: %lc %x %x %x %s\n",
					data[i].Event.KeyEvent.uChar.UnicodeChar ? data[i].Event.KeyEvent.uChar.UnicodeChar : '#',
					data[i].Event.KeyEvent.uChar.UnicodeChar,
					data[i].Event.KeyEvent.wVirtualKeyCode,
					data[i].Event.KeyEvent.dwControlKeyState,
					data[i].Event.KeyEvent.bKeyDown ? "DOWN" : "UP");
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
