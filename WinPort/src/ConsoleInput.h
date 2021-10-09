#pragma once
#include <deque>
#include <mutex>
#include <set>
#include <condition_variable>
#include "WinCompat.h"
#include "Backend.h"

class ConsoleInput : public IConsoleInput
{
	std::deque<INPUT_RECORD> _pending;
	std::mutex _mutex;
	std::condition_variable _non_empty;
	std::set<unsigned int> _requestor_priorities;

	unsigned int CurrentPriority() const;

public:
	virtual ~ConsoleInput() {};

	virtual void Enqueue(const INPUT_RECORD *data, DWORD size);
	virtual DWORD Peek(INPUT_RECORD *data, DWORD size, unsigned int requestor_priority = 0);
	virtual DWORD Dequeue(INPUT_RECORD *data, DWORD size, unsigned int requestor_priority = 0);
	virtual DWORD Count(unsigned int requestor_priority = 0);
	virtual DWORD Flush(unsigned int requestor_priority = 0);
	virtual bool WaitForNonEmpty(unsigned int timeout_msec = (unsigned int)-1, unsigned int requestor_priority = 0);

	virtual unsigned int RaiseRequestorPriority();
	virtual void LowerRequestorPriority(unsigned int released_priority);
};
