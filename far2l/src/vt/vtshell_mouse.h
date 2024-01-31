#pragma once
#include "IVTShell.h"

class VTMouse
{
	IVTShell *_vtshell;
	uint32_t _mex;
	unsigned int _sgr_prev_ibut{0};

public:
	VTMouse(IVTShell *vtshell, uint32_t mex);
	bool OnInputMouse(const MOUSE_EVENT_RECORD &MouseEvent);
};
