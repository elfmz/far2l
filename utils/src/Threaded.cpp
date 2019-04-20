#include "Threaded.h"


void *Threaded::sThreadProc(void *p)
{
	return ((Threaded *)p)->ThreadProc();
}

bool Threaded::StartThread()
{
	if (pthread_create(&_trd, NULL, &sThreadProc, this) != 0) {
		_trd = 0;
		return false;
	}

	return true;
}

void *Threaded::WaitThread()
{
	void *out = nullptr;
	pthread_join(_trd, &out);
	_trd = 0;
	return out;
}
