#pragma once
#include <pthread.h>

class Threaded
{
	pthread_t _trd = 0;
	static void *sThreadProc(void *p);

protected:
	virtual void *ThreadProc() = 0;

	bool StartThread();
	void *WaitThread();
};
