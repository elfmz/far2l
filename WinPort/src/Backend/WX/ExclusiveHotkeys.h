#pragma once
#include <wx/wx.h>
#include <cstdint>

class ExclusiveHotkeys
{
#ifdef __WXGTK__
	typedef int (*gdk_keyboard_grab_t)(void *widget, int, uint32_t);
	typedef int (*gdk_keyboard_ungrab_t)(uint32_t);

	gdk_keyboard_grab_t _gdk_grab;
	gdk_keyboard_ungrab_t _gdk_ungrab;
#endif
	bool _ctrl_left, _ctrl_right, _alt_left, _alt_right, _win_left, _win_right;
	bool _pending;

	bool IsTriggeringKeyEvent(wxKeyEvent& event);

	public:
	ExclusiveHotkeys();
	~ExclusiveHotkeys();

	bool Available() const;

	//following methods are not not MT-safe
	void SetTriggerKeys(bool ctrl_left, bool ctrl_right, bool alt_left, bool alt_right, bool win_left, bool win_right);
	void OnKeyDown(wxKeyEvent& event, wxWindow *win);
	void OnKeyUp(wxKeyEvent& event);
	void Reset();
};
