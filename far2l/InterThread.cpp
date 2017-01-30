#include "InterThread.h"

InterThread::InterThread()
{
	if (pipe_cloexec(_pipe) == -1) {
		_pipe[0] = _pipe[1] = -1;
	}
}

InterThread::~InterThread()
{
	close(_pipe[0]);
	close(_pipe[1]);
}

	int _pipe[2];
	std::mutex _mtx;

	public:
	struct IRequest
	{
		int id;
		const void *argument;
		virtual void Done() = 0;
	};

void InterThread::SynchronousRequest(int id, const void *argument)
{
	std::lock_guard<std::mutex> lock(_mtx);
}

void InterThread::AsynchronousRequest(int id)
{
}

InterThread::IRequest *InterThread::WaitForRequest()
{
}

