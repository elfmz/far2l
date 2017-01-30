#pragma once
#include <mutex>

class InterThread
{
	int _pipe[2];
	std::mutex _mtx;

	public:
	InterThread();
	~InterThread();
	struct IRequest
	{
		int id;
		const void *argument;
		virtual void Done() = 0;
	};

	void SynchronousRequest(int id, const void *argument);
	void AsynchronousRequest(int id);
	IRequest *WaitForRequest();
};
