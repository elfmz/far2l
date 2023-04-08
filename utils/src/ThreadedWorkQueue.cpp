#include "ThreadedWorkQueue.h"
#include "debug.h"
#include <unistd.h>
#include <stdio.h>
#include <stdexcept>

class ThreadedWorker : public Threaded
{
	ThreadedWorkQueue *_twq;

	virtual void *ThreadProc()
	{
		_twq->WorkerThreadProc();
		return nullptr;
	}

public:
	ThreadedWorker(ThreadedWorkQueue *twq) : _twq(twq)
	{
		if (!StartThread()) {
			throw std::runtime_error("StartThread failed");
		}
	}

	virtual ~ThreadedWorker()
	{
		WaitThread();
	}
};

ThreadedWorkQueue::OrderedItemsDestroyer::~OrderedItemsDestroyer()
{
	for (auto *twi : *this) {
		delete twi;
	}
}

ThreadedWorkQueue::ThreadedWorkQueue(size_t threads_count)
	:
	_threads_count(threads_count ? threads_count : BestThreadsCount())
{
}

ThreadedWorkQueue::~ThreadedWorkQueue()
{
	const size_t workers_count = _workers.size();
	if (workers_count != 0) {
		std::unique_lock<std::mutex> lock(_mtx);
		_stopping = true;
		_cond.notify_all();
	}
	_workers.clear(); // this also joins them
	ASSERT(_working == 0);

	fprintf(stderr,
		"%s: threads=%lu/%lu done=%lu finalized=%lu unprocessed_backlog=%lu unprocessed_done=%lu _backlog_waits=%lu\n",
			__FUNCTION__, (unsigned long)workers_count, (unsigned long)_threads_count,
			(unsigned long)_done_counter, (unsigned long)_finalized_counter,
			(unsigned long)_backlog.size(), (unsigned long)_done.size(), (unsigned long)_backlog_waits);

	for (const auto &it : _done) {
		delete it.second;
	}
	for (auto *twi: _backlog) {
		delete twi;
	}
}

// invoked by ThreadedWorker from its thread routine
void ThreadedWorkQueue::WorkerThreadProc()
{
	size_t seq = 0;
	IThreadedWorkItem *twi = nullptr;
	for (;;) {
		if (twi) try {
			twi->WorkProc();

		} catch (std::exception &e) {
			fprintf(stderr, "%s/WorkProc: %s", __FUNCTION__, e.what());
		}

		std::unique_lock<std::mutex> lock(_mtx);
		while (twi) try {
			_done.emplace(seq, twi);
			_working--;
			if (_notify_on_done) {
				_notify_on_done = false;
				_cond.notify_all();
			}
			twi = nullptr;

		} catch (std::exception &e) { // OOM? retry in one second til memory will appear
			fprintf(stderr, "%s: %s", __FUNCTION__, e.what());
			sleep(1);
		}

		if (_stopping) {
			break;
		}

		if (_backlog.empty()) {
			_cond.wait(lock);

		} else {
			seq = ++_done_counter;
			twi = _backlog.front();
			_backlog.pop_front();
			_working++;
		}
	}
}

void ThreadedWorkQueue::Queue(IThreadedWorkItem *twi, size_t backlog_limit)
{
	if (backlog_limit == (size_t)-1) {
		backlog_limit = 2 * _threads_count;
	}

	OrderedItemsDestroyer oid;

	{
		std::unique_lock<std::mutex> lock(_mtx);
		if (_workers.empty() || (!_backlog.empty() && _workers.size() < _threads_count)) {
			try {
				_workers.emplace_back(this);
			} catch (std::exception &e) {
				fprintf(stderr, "%s/WORKER: %s\n", __FUNCTION__, e.what());
			}
		}
		if (!_workers.empty()) {
			_backlog.emplace_back(twi);
			twi = nullptr;
			if (_backlog.size() == 1) {
				_cond.notify_one();
			} else {
				_cond.notify_all();
			}
			if (_backlog.size() > backlog_limit) {
				++_backlog_waits;
				do {
					_notify_on_done = true;
					_cond.wait(lock);
				} while (_backlog.size() > backlog_limit);
			}
		}
		FetchOrderedDoneItems(oid);
	}

	if (twi) {
		// no workers? fallback to synchronous processing
		try {
			twi->WorkProc();
		} catch (std::exception &e) {
			fprintf(stderr, "%s/WorkProc: %s", __FUNCTION__, e.what());
		}
		delete twi;
	}
}

void ThreadedWorkQueue::Finalize()
{
	OrderedItemsDestroyer oid;
	std::unique_lock<std::mutex> lock(_mtx);
	for (;;) {
		if (_backlog.empty() && _working == 0) {
			FetchOrderedDoneItems(oid);
			break;
		}
		_notify_on_done = true;
		_cond.wait(lock);
	}
}

// must be invoked under _mtx lock held
void ThreadedWorkQueue::FetchOrderedDoneItems(OrderedItemsDestroyer &oid)
{
	while (!_done.empty() && _done.begin()->first == _finalized_counter + 1) {
		++_finalized_counter;
		oid.emplace_back(_done.begin()->second);
		_done.erase(_done.begin());
	}
}

////////////////////////////////////////

ThreadedWorkQueuePtrScope::ThreadedWorkQueuePtrScope(std::unique_ptr<ThreadedWorkQueue> &pWQ)
	: _pWQ(pWQ)
{
	if (!_pWQ) {
		_inited = true;
		_pWQ.reset(new ThreadedWorkQueue);
	}
}

ThreadedWorkQueuePtrScope::~ThreadedWorkQueuePtrScope()
{
	try {
		_pWQ->Finalize();
	} catch (std::exception &e) {
		fprintf(stderr, "%s: %s\n", __FUNCTION__, e.what());
	}
	if (_inited) {
		_pWQ.reset();
	}
}

