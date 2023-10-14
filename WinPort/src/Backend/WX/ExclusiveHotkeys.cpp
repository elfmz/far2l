#include "ExclusiveHotkeys.h"
#include <dlfcn.h>


ExclusiveHotkeys::ExclusiveHotkeys() :
	_ctrl_left(false), _ctrl_right(false), 
	_alt_left(false), _alt_right(false), 
	_win_left(false), _win_right(false),
	_pending(false)
{
#ifdef __WXGTK__
	_gdk_grab = (gdk_keyboard_grab_t)dlsym(RTLD_DEFAULT, "gdk_keyboard_grab");
	_gdk_ungrab = (gdk_keyboard_ungrab_t)dlsym(RTLD_DEFAULT, "gdk_keyboard_ungrab");
	if (!_gdk_grab || !_gdk_ungrab) {
		fprintf(stderr, 
			"ExclusiveHotkeys - API not found: _gdk_grab=%p _gdk_ungrab=%p\n", 
			_gdk_grab, _gdk_ungrab);
	}
#endif
}

ExclusiveHotkeys::~ExclusiveHotkeys()
{
	Reset();
}

bool ExclusiveHotkeys::Available() const
{
#ifdef __WXGTK__
	return (_gdk_grab != nullptr && _gdk_ungrab != nullptr);
#else
	return false;
#endif
}

bool ExclusiveHotkeys::IsTriggeringKeyEvent(wxKeyEvent& event)
{
	//event.GetRawKeyCode() is GDK_KEY_ constant
	return (_alt_left && event.GetRawKeyCode() == 0xffe9)
		|| (_alt_right && event.GetRawKeyCode() == 0xffea)
		|| (_ctrl_left && event.GetRawKeyCode() == 0xffe3)
		|| (_ctrl_right && event.GetRawKeyCode() == 0xffe4)
		|| (_win_left && event.GetRawKeyCode() == 0xffeb)
		|| (_win_right && event.GetRawKeyCode() == 0xffec);
}

void ExclusiveHotkeys::SetTriggerKeys(bool ctrl_left, bool ctrl_right, 
				bool alt_left, bool alt_right, bool win_left, bool win_right)
{
	fprintf(stderr, 
		"ExclusiveHotkeys::SetTriggerKeys: ctrl_left=%u ctrl_right=%u alt_left=%u alt_right=%u win_left=%u win_right=%u\n", 
		ctrl_left, ctrl_right, alt_left, alt_right, win_left, win_right);

	_ctrl_left = ctrl_left;
	_ctrl_right = ctrl_right;
	_alt_left = alt_left;
	_alt_right = alt_right;
	_win_left = win_left;
	_win_right = win_right;
}

void ExclusiveHotkeys::OnKeyDown(wxKeyEvent& event, wxWindow *win)
{
	if (!_pending && IsTriggeringKeyEvent(event) ) {
#ifdef __WXGTK__
		if (_gdk_grab && _gdk_ungrab) {
			_pending = (_gdk_grab(win->GTKGetDrawingWindow(), 0, 0) == 0);
		}
#else
		fprintf(stderr, "ExclusiveHotkeys::OnKeyDown - not implemented for this platform\n");
#endif
	}
}

void ExclusiveHotkeys::OnKeyUp(wxKeyEvent& event)
{
#ifdef __WXGTK__
	if (_pending && IsTriggeringKeyEvent(event) ) {
		_gdk_ungrab(0);
		_pending = false;
	}
#endif
}

void ExclusiveHotkeys::Reset()
{
#ifdef __WXGTK__
	if (_pending) {
		_gdk_ungrab(0);
		_pending = false;
	}
#endif
}

