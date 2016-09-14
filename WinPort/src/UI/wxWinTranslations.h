#pragma once

#include "WinCompat.h"

#include <wx/wx.h>
#include <wx/display.h>

struct wx2INPUT_RECORD : INPUT_RECORD
{
	wx2INPUT_RECORD(wxKeyEvent& event, BOOL KeyDown);
};

wxColour ConsoleForeground2wxColor(USHORT attributes);
wxColour ConsoleBackground2wxColor(USHORT attributes);

int wxKeyCode2WinKeyCode(int code);
