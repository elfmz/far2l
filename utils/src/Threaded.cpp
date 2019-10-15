#include "Threaded.h"
#include <stdlib.h>


// OSX has no pthread_timedjoin_np, so here goes bicycle for timed thread waiting

Threaded::~Threaded()
{
	std::lock_guard<std::mutex> lock(_trd_mtx);
	if (!_trd_exited || !_trd_joined) {
		fprintf(stderr, "~Threaded: still busy\n");
		abort();
	}
}

void *Threaded::sThreadProc(void *p)
{
	Threaded *it = (Threaded *)p;
	void *result = it->ThreadProc();

	std::lock_guard<std::mutex> lock(it->_trd_mtx);
	it->_trd_exited = true;
	it->_trd_cond.notify_all();

	return result;
}

bool Threaded::StartThread()
{
	std::lock_guard<std::mutex> lock(_trd_mtx);
	if (!_trd_joined) {
		if (!_trd_exited) {
			return false;
		}

		pthread_join(_trd, &_trd_result);
		_trd = 0;
	}

	_trd_joined = _trd_exited = false;
	if (pthread_create(&_trd, NULL, &sThreadProc, this) != 0) {
		_trd_joined = _trd_exited = true;
		_trd = 0;
		return false;
	}

	return true;
}

bool Threaded::WaitThread(unsigned int msec)
{
	std::unique_lock<std::mutex> lock(_trd_mtx);
	for (;;) {
		if (_trd_exited) {
			if (!_trd_joined) {
				_trd_joined = true;
				pthread_join(_trd, &_trd_result);
				_trd = 0;
			}
			return true;
		}
		if (msec == 0)
			return false;

		if (msec != (unsigned int)-1)  {
			std::chrono::milliseconds ms_before = std::chrono::duration_cast< std::chrono::milliseconds >
				(std::chrono::steady_clock::now().time_since_epoch());
			_trd_cond.wait_for(lock, std::chrono::milliseconds(msec));
			std::chrono::milliseconds ms = std::chrono::duration_cast< std::chrono::milliseconds >
				(std::chrono::steady_clock::now().time_since_epoch());
			ms-= ms_before;

			if (ms.count()  < msec)
				msec-= ms.count();
			else
				msec = 0;

		} else
			_trd_cond.wait(lock);
	}

}

void *Threaded::GetThreadResult()
{
	WaitThread();
	return _trd_result;
}

