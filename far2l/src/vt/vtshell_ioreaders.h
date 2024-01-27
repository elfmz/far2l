#pragma once
#include <list>
#include <mutex>
#include <atomic>
#include <string>
#include <pthread.h>

template <class T> 
	class StopAndStart
{
	T &_t;
public:
	StopAndStart(T &t) : _t(t)
	{
		_t.Stop();
	}
	~StopAndStart()
	{
		_t.Start();
	}	
};

class WithThread
{
public:
	WithThread();
	virtual ~WithThread();

protected:
	volatile bool  _started;

	bool Start();
	void Join();
	virtual void OnJoin();
	virtual void *ThreadProc() = 0;

private:
	pthread_t _thread;
	
	static void *sThreadProc(void *p);
};

class VTOutputReader : protected WithThread
{
public:
	struct IProcessor
	{
		virtual bool OnProcessOutput(const char *buf, int len) = 0;
	};
	
	VTOutputReader(IProcessor *processor) ;
	virtual ~VTOutputReader();
	void Start(int fd_out = -1);
	void Stop();
	inline bool IsDeactivated() const { return _deactivated; }
	void KickAss();


protected:
	virtual void OnJoin();

private:
	IProcessor *_processor;
	int _fd_out, _pipe[2];
	std::mutex _mutex;
	bool _deactivated;
	
	virtual void *ThreadProc();
};

class VTInputReader : protected WithThread
{
public:
	struct IProcessor
	{
		virtual void OnInputMouse(const MOUSE_EVENT_RECORD &MouseEvent) = 0;
		virtual void OnInputKey(const KEY_EVENT_RECORD &KeyEvent) = 0;
		virtual void OnInputResized(const INPUT_RECORD &ir) = 0;
		virtual void OnInputInjected(const std::string &str) = 0;
		virtual void OnBracketedPaste(bool start) = 0;
		virtual void OnRequestShutdown() = 0;
	};

	VTInputReader(IProcessor *processor);
	void Start(HANDLE con_hnd);
	void Stop();
	void InjectInput(const char *str, size_t len);

protected:
	virtual void OnJoin();

private:
	std::atomic<bool> _stop{false};
	IProcessor *_processor;
	HANDLE _con_hnd{NULL};
	std::list<std::string> _pending_injected_inputs;
	std::mutex _pending_injected_inputs_mutex;

	void KickInputThread();
	virtual void *ThreadProc();
};
