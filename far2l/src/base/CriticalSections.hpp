#pragma once

/*
CriticalSections.hpp

Критические секции
*/
/*
Copyright (c) 1996 Eugene Roshal
Copyright (c) 2000 Far Group
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "noncopyable.hpp"
#include <mutex>
#include <memory>

class CriticalSection
{
private:
	std::shared_ptr<std::recursive_mutex> _mutex;

public:
	CriticalSection()
		:
		_mutex(std::make_shared<std::recursive_mutex>())
	{}

public:
#if 1
	void Enter() { _mutex->lock(); }
	void Leave() { _mutex->unlock(); }
#else
	void Enter()
	{
		if (!_mutex->try_lock()) {
			fprintf(stderr, "CriticalSection:%p - locking by %p\n", this, pthread_self());
			_mutex->lock();
		}
		fprintf(stderr, "CriticalSection:%p - locked by %p\n", this, pthread_self());
	}
	void Leave()
	{
		fprintf(stderr, "CriticalSection:%p - unlocked by %p\n", this, pthread_self());
		_mutex->unlock();
	}
#endif
};

class CriticalSectionLock : public NonCopyable
{
private:
	CriticalSection &_object;

public:
	CriticalSectionLock(CriticalSection &object)
		:
		_object(object)
	{
		_object.Enter();
	}
	~CriticalSectionLock() { _object.Leave(); }
};
