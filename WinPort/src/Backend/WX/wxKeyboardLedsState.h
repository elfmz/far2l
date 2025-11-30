#pragma once
#include <Threaded.h>
#include <mutex>
#include <condition_variable>
#include <memory>

class wxKeyboardLedsState : Threaded
{
	std::mutex _mtx;
	std::condition_variable _cond;
	unsigned int _id{1};
	unsigned int _state{0};
	bool _long_wait{false};

	virtual void *ThreadProc();

public:
	~wxKeyboardLedsState();

	void Startup();
	void Shutdown();

	void Toggle(unsigned int bits);
	void Set(unsigned int bits);
	void Clear(unsigned int bits);
	unsigned int Current(bool trigger_update = false);
};

extern wxKeyboardLedsState g_wx_keyboard_leds_state;
