#include "Event.h"

Event::Event(bool autoreset, bool signaled)
	: _autoreset(autoreset), _signaled(signaled)
{
}

bool Event::TimedWait(unsigned int msec)
{
	std::unique_lock<std::mutex> lock(_mutex);
	for (;;) {
		if (_signaled) {
			if (_autoreset) {
				_signaled = false;
			}
			return true;
		}

		if (!msec)
			return false;

		std::chrono::milliseconds ms_before = std::chrono::duration_cast< std::chrono::milliseconds >
			(std::chrono::steady_clock::now().time_since_epoch());
		_cond.wait_for(lock, std::chrono::milliseconds(msec));
		std::chrono::milliseconds ms = std::chrono::duration_cast< std::chrono::milliseconds >
			(std::chrono::steady_clock::now().time_since_epoch());
		ms-= ms_before;

		if (ms.count()  < msec)
			msec-= ms.count();
		else
			msec = 0;
	}
}

void Event::Wait()
{
	std::unique_lock<std::mutex> lock(_mutex);
	while (!_signaled) {
		_cond.wait(lock);
	}
	if (_autoreset) {
		_signaled = false;
	}
}

void Event::Signal()
{
	std::unique_lock<std::mutex> lock(_mutex);
	_signaled = true;
	_cond.notify_all();
}
