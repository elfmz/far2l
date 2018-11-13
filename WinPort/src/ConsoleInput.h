#pragma once
#include <deque>
#include <mutex>
#include <set>
#include <condition_variable>
#include "WinCompat.h"

class ConsoleInput
{
	std::deque<INPUT_RECORD> _pending;
	std::mutex _mutex;
	std::condition_variable _non_empty;
	std::set<unsigned int> _requestor_priorities;

	unsigned int CurrentPriority() const;

public:
	void Enqueue(const INPUT_RECORD *data, DWORD size);
	DWORD Peek(INPUT_RECORD *data, DWORD size, unsigned int requestor_priority = 0);
	DWORD Dequeue(INPUT_RECORD *data, DWORD size, unsigned int requestor_priority = 0);
	DWORD Count(unsigned int requestor_priority = 0);
	DWORD Flush(unsigned int requestor_priority = 0);
	bool WaitForNonEmpty(unsigned int timeout_msec = (unsigned int)-1, unsigned int requestor_priority = 0);

	unsigned int RaiseRequestorPriority();
	void LowerRequestorPriority(unsigned int released_priority);
};

class ConsoleInputPriority
{
	ConsoleInput &_con_input;
	unsigned int _my_priority;

	public:
	ConsoleInputPriority(ConsoleInput &con_input);
	~ConsoleInputPriority();

	operator int() const {return _my_priority; }
};
