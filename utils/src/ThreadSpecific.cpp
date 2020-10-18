#include "ThreadSpecific.h"
#include "utils.h"
#include <exception>
#include <stdexcept>

ThreadSpecific::ThreadSpecific(void (*destructor)(void*))
{
	int r = pthread_key_create(&_key, destructor);
	if (r != 0) {
		throw std::runtime_error(StrPrintf("ThreadSpecific: pthread_key_create error %d", r));
	}
}

ThreadSpecific::~ThreadSpecific()
{
}

void *ThreadSpecific::Get()
{
	return pthread_getspecific(_key);
}

void ThreadSpecific::Set(void *v)
{
	int r = pthread_setspecific(_key, v);
	if (r != 0) {
		throw std::runtime_error(StrPrintf("ThreadSpecific: pthread_setspecific error %d", r));
	}
}
