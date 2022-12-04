#pragma once
#include "IVTShell.h"

class VTMouse
{
	IVTShell *_vtshell;
	MouseExpectation _mex;

public:
	VTMouse(IVTShell *vtshell, MouseExpectation mex);
	bool OnInputMouse(const MOUSE_EVENT_RECORD &MouseEvent);
};
