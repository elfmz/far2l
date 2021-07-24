#pragma once
#include <deque>
#include <list>
#include <map>
#include <vector>
#include <mutex>
#include <memory>
#include <condition_variable>
#include "Threaded.h"


struct IThreadedWorkItem
{
	/// Invoked within main thread after WorkProc completed or discared
	/// from Finalize(), Queue() or ThreadedWorkQueue's d-tor
	virtual ~IThreadedWorkItem() {}

	/// Invoked within worker thread to perform required work.
	virtual void WorkProc() = 0;
};

class ThreadedWorker;

////////////////////////////////////////////////////////////////////////////////////
/// Following class implements thread pool that processes work items asynchronously
/// Queued items processed by worker threads in arbitrary order but destroyed from
/// main thread in same order as they were queued.
class ThreadedWorkQueue
{
	friend class ThreadedWorker;
	std::mutex _mtx;
	std::condition_variable _cond;
	std::deque<IThreadedWorkItem *> _backlog;
	std::map<size_t, IThreadedWorkItem *> _done; // first is sequence number to keep ordering
	std::list<ThreadedWorker> _workers;
	const size_t _threads_count; // maximum parallel workers count
	volatile int _working = 0;
	volatile bool _stopping = false;
	volatile bool _notify_on_done = false;
	size_t _done_counter = 0;
	size_t _finalized_counter = 0;
	size_t _backlog_waits = 0;

	void WorkerThreadProc();

	struct OrderedItemsDestroyer : std::vector<IThreadedWorkItem *>
	{
		~OrderedItemsDestroyer();
	};

	void FetchOrderedDoneItems(OrderedItemsDestroyer &result);

public:
	/// If threads_count is positive then it specifies count of threads that will process work items
	/// If threads_count is == 0 then this count set to online CPU-s count
	ThreadedWorkQueue(size_t threads_count = 0);

	/// Beside of simple release, d-tor also discards all yet not processed items by
	/// invoking their DiscardProc() and destroys them afterwards.
	/// Use Finalize() to ensure that all items processed before d-tor.
	virtual ~ThreadedWorkQueue();

	/// Queues item for processing, don't use <twi> directly after it being queued.
	/// If pending items count >= than <backlog_limit> then <twi> is processed synchronously.
	/// <backlog_limit> defaulted to 2 * threads_count.
	/// Also it invokes CompleteProc() of already processed items and destroys them.
	void Queue(IThreadedWorkItem *twi, size_t backlog_limit = (size_t)-1);

	/// Waits for dispatch of all pending items, invoke before d-tor to make sure all items processed.
	/// Invokes CompleteProc() of finally processed items and destroys them.
	void Finalize();
};

/////////////////////////////////////////////////////////////////////////////////////////////////
/// Helper class that at c-tor inits std::unique_ptr<ThreadedWorkQueue> if its not yet inited
/// and at d-tor - it invokes Finalize() and resets std::unique_ptr if it was inited at c-tor
struct ThreadedWorkQueuePtrScope
{
	ThreadedWorkQueuePtrScope(std::unique_ptr<ThreadedWorkQueue> &pWQ);
	~ThreadedWorkQueuePtrScope();

private:
	std::unique_ptr<ThreadedWorkQueue> &_pWQ;
	bool _inited = false;
};

