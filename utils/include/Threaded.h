#pragma once
#include <pthread.h>
#include <mutex>
#include <condition_variable>

class Threaded
{
	std::condition_variable _cond;
	std::mutex _mtx;

	pthread_t _trd = 0;
	void *_result = nullptr;
	bool _exited = true, _joined = true;

	static void *sThreadProc(void *p);

protected:
	virtual ~Threaded();

	virtual void *ThreadProc() = 0;

	bool StartThread();
	bool WaitThread(unsigned int msec = (unsigned int)-1);
	void *GetThreadResult();
};
