#pragma once
#include <string>

enum MouseExpectation
{
	MEX_X10_MOUSE             = 0x00001,
	MEX_VT200_MOUSE           = 0x00002,
//	MEX_VT200_HIGHLIGHT_MOUSE = 0x00004, irrelevant???
	MEX_BTN_EVENT_MOUSE       = 0x00008,
	MEX_ANY_EVENT_MOUSE       = 0x00010,
	MEX_SGR_EXT_MOUSE         = 0x00020
};

struct IVTShell
{
	virtual void OnMouseExpectation(MouseExpectation mex, bool enabled) = 0;
	virtual void OnBracketedPasteExpectation(bool enabled)              = 0;
	virtual void OnWin32InputMode(bool enabled)                         = 0;
	virtual void SetKittyFlags(int flags)                               = 0;
	virtual int  GetKittyFlags()                                        = 0;
	virtual void OnApplicationProtocolCommand(const char *str)          = 0;
	virtual bool OnOSCommand(int id, std::string &str)                  = 0;
	virtual void InjectInput(const char *str)                           = 0;
	virtual void OnKeypadChange(unsigned char keypad)                   = 0;
	virtual void OnTerminalResized()                                    = 0;
	virtual HANDLE ConsoleHandle()                                      = 0;
};
