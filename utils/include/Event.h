#pragma once
#include <mutex>
#include <atomic>
#include <condition_variable>

class Event
{
	std::condition_variable _cond;
	std::mutex _mutex;
	bool _autoreset, _signaled;

public:
	Event(bool autoreset = false, bool signaled = false);

	bool TimedWait(unsigned int msec);
	void Wait();
	void Signal();
};
