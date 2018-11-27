#pragma once
#include <mutex>
#include <atomic>
#include <condition_variable>

class Event
{
	std::condition_variable _cond;
	std::mutex _mutex;
	bool _done;

public:
	bool Wait();
	void Signal();
};
