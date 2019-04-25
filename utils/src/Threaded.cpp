#include "Threaded.h"

// OSX has no pthread_timedjoin_np, so here goes bicycle for timed thread waiting

Threaded::~Threaded()
{
	std::unique_lock<std::mutex> lock(_mtx);
	if (!_exited || !_joined) {
		fprintf(stderr, "~Threaded: still busy\n");
		abort();
	}
}

void *Threaded::sThreadProc(void *p)
{
	Threaded *it = (Threaded *)p;
	void *result = it->ThreadProc();

	std::unique_lock<std::mutex> lock(it->_mtx);
	it->_exited = true;
	it->_cond.notify_all();

	return result;
}

bool Threaded::StartThread()
{
	_joined = _exited = false;
	if (pthread_create(&_trd, NULL, &sThreadProc, this) != 0) {
		_trd = 0;
		return false;
	}

	return true;
}

bool Threaded::WaitThread(unsigned int msec)
{
	std::unique_lock<std::mutex> lock(_mtx);
	for (;;) {
		if (_exited) {
			if (!_joined) {
				_joined = true;
				pthread_join(_trd, &_result);
			}
			return true;
		}
		if (msec == 0)
			return false;

		if (msec != (unsigned int)-1)  {
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

		} else
			_cond.wait(lock);
	}

}

void *Threaded::GetThreadResult()
{
	WaitThread();
	return _result;
}

