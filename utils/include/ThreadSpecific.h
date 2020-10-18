#pragma once
#include <pthread.h>

class ThreadSpecific
{
	pthread_key_t _key;

public:
	ThreadSpecific(void (*destructor)(void*) = nullptr);
	~ThreadSpecific();

	void *Get();
	void Set(void *v);
};
