#pragma once
#include <pthread.h>
#include <mutex>
#include <condition_variable>

class Threaded
{
	std::condition_variable _trd_cond;
	std::mutex _trd_mtx;
	bool _self_destruct = false;

	pthread_t _trd = 0;
	void *_trd_result = nullptr;
	bool _trd_exited = true, _trd_joined = true;

	static void *sThreadProc(void *p);

protected:
	virtual ~Threaded();

	virtual void *ThreadProc() = 0;

	void Detach();
	bool StartThread(bool self_destruct = false);
	bool WaitThread(unsigned int msec = (unsigned int)-1);
	void *GetThreadResult();
};

unsigned int BestThreadsCount();
