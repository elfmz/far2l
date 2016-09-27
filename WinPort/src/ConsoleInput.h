#pragma once
#include <deque>
#include <mutex>
#include <condition_variable>
#include "WinCompat.h"

class ConsoleInput
{
	std::deque<INPUT_RECORD> _pending;
	std::mutex _mutex;
	std::condition_variable _non_empty;
public:
	void Enqueue(const INPUT_RECORD *data, DWORD size);
	DWORD Peek(INPUT_RECORD *data, DWORD size);
	DWORD Dequeue(INPUT_RECORD *data, DWORD size);
	DWORD Count();
	DWORD Flush();
	void WaitForNonEmpty();
};
