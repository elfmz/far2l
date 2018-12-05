#include "Event.h"

void Event::Wait()
{
	std::unique_lock<std::mutex> lock(_mutex);
	while (!_done) {
		_cond.wait(lock);
	}
}

void Event::Signal()
{
	std::unique_lock<std::mutex> lock(_mutex);
	_done = true;
	_cond.notify_all();
}
