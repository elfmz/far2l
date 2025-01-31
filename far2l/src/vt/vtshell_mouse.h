#pragma once
#include "IVTShell.h"

class VTMouse
{
	//bit indicators for modifier keys in mouse input sequence
	const unsigned int
		_shift_ind = 0x04,
		_alt_ind   = 0x08,
		_ctrl_ind  = 0x10;

	IVTShell *_vtshell;
	uint32_t _mode;
	unsigned int _sgr_prev_ibut{0};


public:
	VTMouse(IVTShell *vtshell, uint32_t mex);
	bool OnInputMouse(const MOUSE_EVENT_RECORD &MouseEvent);
};
