#pragma once
#include <string>

enum MouseExpectation
{
	MEX_NONE                  = 0,
	MEX_X10_MOUSE             = 9,
	MEX_VT200_MOUSE           = 1000,
	MEX_VT200_HIGHLIGHT_MOUSE = 1001,
	MEX_BTN_EVENT_MOUSE       = 1002,
	MEX_ANY_EVENT_MOUSE       = 1003
};

struct IVTShell
{
	virtual void OnMouseExpectation(MouseExpectation mex)      = 0;
	virtual void OnBracketedPasteExpectation(bool enabled)     = 0;
	virtual void OnWin32InputMode(bool enabled)                = 0;
	virtual void OnApplicationProtocolCommand(const char *str) = 0;
	virtual bool OnOSCommand(int id, std::string &str)         = 0;
	virtual void InjectInput(const char *str)                  = 0;
	virtual void OnKeypadChange(unsigned char keypad)          = 0;
	virtual void OnTerminalResized()                           = 0;
	virtual HANDLE ConsoleHandle()                             = 0;
};
