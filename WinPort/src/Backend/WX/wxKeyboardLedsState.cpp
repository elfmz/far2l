#include "wxKeyboardLedsState.h"
#include <wx/wx.h>
#include "WinPort.h"
#include "CallInMain.h"

wxKeyboardLedsState g_wx_keyboard_leds_state;

static unsigned int s_wx_assert_cached_bits = 0;
static unsigned int s_wx_assert_cache_bit = 0;

#define REMOTE_SLOWNESS_TRSH_MSEC		50

static unsigned int wxGetKeyboardLedsState(unsigned int prev_state)
{
	unsigned int out = 0;
	// Old non-GTK wxWidgets had missing support for this keys, and attempt
	// to use wxGetKeyState with unsupported key causes assert callback
	// to be invoked several times on each key event thats not good.
	// Avoid asserts all the time by 'caching' unsupported state.
#ifndef __WXOSX__ // under mac NumLock emulated externally with Clear button
	s_wx_assert_cache_bit = 1;
	if ((s_wx_assert_cached_bits & 1) == 0 && wxGetKeyState(WXK_NUMLOCK)) {
#else
 	if(prev_state & NUMLOCK_ON) { 
#endif
		out|= NUMLOCK_ON;
	}

	s_wx_assert_cache_bit = 2;
	if ((s_wx_assert_cached_bits & 2) == 0 && wxGetKeyState(WXK_SCROLL)) {
		out|= SCROLLLOCK_ON;
	}

	s_wx_assert_cache_bit = 4;
	if ((s_wx_assert_cached_bits & 4) == 0 && wxGetKeyState(WXK_CAPITAL)) {
		out|= CAPSLOCK_ON;
	}

	s_wx_assert_cache_bit = 0;

	return out;
}

void WinPortWxAssertHandler(const wxString& file, int line, const wxString& func, const wxString& cond, const wxString& msg)
{
	s_wx_assert_cached_bits|= s_wx_assert_cache_bit;

	fprintf(stderr, "%s: file='%ls' line=%d func='%ls' cond='%ls' msg='%ls'\n",
			__FUNCTION__,
			static_cast<const wchar_t*>(file.wc_str()), line,
			static_cast<const wchar_t*>(func.wc_str()),
			static_cast<const wchar_t*>(cond.wc_str()),
			static_cast<const wchar_t*>(msg.wc_str()));
}


wxKeyboardLedsState::~wxKeyboardLedsState()
{
	Shutdown();
}

void wxKeyboardLedsState::Startup()
{
	if (WaitThread(0)) {
		StartThread();
	}
}

void wxKeyboardLedsState::Shutdown()
{
	if (!WaitThread(0)) {
		{
			std::unique_lock<std::mutex> lock(_mtx);
			_id = 0;
			_cond.notify_all();
		}
		WaitThread();
	}
}


void *wxKeyboardLedsState::ThreadProc()
{
	std::unique_lock<std::mutex> lock(_mtx);
	for (unsigned int id = 0; _id != 0;) {
		if (id != _id) { // update requested
			id = _id;
			_long_wait = false;
			lock.unlock();
			_state = CallInMain<unsigned int>(std::bind(&wxGetKeyboardLedsState, _state));
			lock.lock();
		} else if (!_long_wait) {
			_cond.wait_for(lock, std::chrono::milliseconds(500));
			if (id == _id) {
				_long_wait = true;
			}
		} else {
			_cond.wait(lock);
		}
	}
	return nullptr;
}

void wxKeyboardLedsState::Toggle(unsigned int bits)
{
	std::lock_guard<std::mutex> lock(_mtx);
	_state^= (unsigned int)(bits);
}

void wxKeyboardLedsState::Set(unsigned int bits)
{
	std::lock_guard<std::mutex> lock(_mtx);
	_state|= (unsigned int)(bits);
}

void wxKeyboardLedsState::Clear(unsigned int bits)
{
	std::lock_guard<std::mutex> lock(_mtx);
	_state&= (unsigned int)(~bits);
}

unsigned int wxKeyboardLedsState::Current(bool trigger_update)
{
	std::lock_guard<std::mutex> lock(_mtx);
	do {
		++_id;
	} while (_id == 0);
	if (trigger_update || _long_wait) {
		_cond.notify_all();
	}
	return _state;
}

