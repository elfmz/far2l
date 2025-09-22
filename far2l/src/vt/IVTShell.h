#pragma once
#include <string>

enum MouseMode
{
	MODE_X10_MOUSE             = 0x00001,
	MODE_VT200_MOUSE           = 0x00002,
//	MODE_VT200_HIGHLIGHT_MOUSE = 0x00004, not used
	MODE_BTN_EVENT_MOUSE       = 0x00008,
	MODE_ANY_EVENT_MOUSE       = 0x00010,
	MODE_SGR_EXT_MOUSE         = 0x00020
};

struct IVTShell
{
	virtual void OnMouseExpectation(MouseMode mex, bool enabled) = 0;
	virtual void OnBracketedPasteExpectation(bool enabled)              = 0;
	virtual void OnFocusChangeExpectation(bool enabled)                 = 0;
	virtual void OnWin32InputMode(bool enabled)                         = 0;
	virtual void SetKittyFlags(int flags)                               = 0;
	virtual int  GetKittyFlags()                                        = 0;
	virtual void OnApplicationProtocolCommand(const char *str)          = 0;
	virtual bool OnOSCommand(int id, std::string &str)                  = 0;
	virtual void InjectInput(const char *str)                           = 0;
	virtual void OnKeypadChange(unsigned char keypad)                   = 0;
	virtual void OnTerminalResized()                                    = 0;
	virtual void OnScreenModeChanged(bool alternate_mode)               = 0;
	virtual HANDLE ConsoleHandle()                                      = 0;
};
