#include "headers.hpp"
#include "vtshell_mouse.h"

#define BUTTONS_PRESS_MASK (FROM_LEFT_1ST_BUTTON_PRESSED | FROM_LEFT_2ND_BUTTON_PRESSED | RIGHTMOST_BUTTON_PRESSED)

VTMouse::VTMouse(IVTShell *vtshell, MouseExpectation mex)
	: _vtshell(vtshell), _mex(mex)
{
}

static constexpr char sClickMatrix[4][4] { // [Button] [Mods: none, ctrl, alt, ctrl+alt]
	{'#', '3', '+', ';'}, // B_NONE (button released)
	{' ', '0', '(', '8'}, // B_LEFT
	{'!', '1', ')', '9'}, // B_MID
	{'"', '2', '*', ':'}, // B_RIGHT
};

static constexpr char sMoveMatrix[4][4] { // [Button] [Mods: none, ctrl, alt, ctrl+alt]
	{'C', 'S', 'K', '['}, // B_NONE (no button pressed)
	{'@', 'P', 'H', 'X'}, // B_LEFT
	{'A', 'Q', 'I', 'Y'}, // B_MID
	{'B', 'R', 'J', 'Z'}, // B_RIGHT
};

static constexpr char sWheelMatrix[2][2] { // [Direction] [Mods: none, ctrl]
	{'`', 'p'}, // UP
	{'a', 'q'}, // DOWN
};

bool VTMouse::OnInputMouse(const MOUSE_EVENT_RECORD &MouseEvent)
{
	if (MouseEvent.dwControlKeyState & SHIFT_PRESSED) {
		return false; // shift combinations reserved by VT
	}

	if (MouseEvent.dwEventFlags & MOUSE_MOVED) {
		if (_mex < MEX_VT200_HIGHLIGHT_MOUSE)
			return true;

		if (_mex == MEX_VT200_HIGHLIGHT_MOUSE) {
			if ( (MouseEvent.dwButtonState & BUTTONS_PRESS_MASK) == 0)
				return true;
		}
	}
	if (_mex == MEX_X10_MOUSE) {
		if ( (MouseEvent.dwButtonState & BUTTONS_PRESS_MASK) == 0)
			return true;
	}

	unsigned int imod = 0, ibut = 0;

	if (MouseEvent.dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) imod|= 1;
	if (MouseEvent.dwControlKeyState & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED)) imod|= 2;

	if (MouseEvent.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED) ibut = 1;
	else if (MouseEvent.dwButtonState & FROM_LEFT_2ND_BUTTON_PRESSED) ibut = 2;
	else if (MouseEvent.dwButtonState & RIGHTMOST_BUTTON_PRESSED) ibut = 3;

	char seq[] = {0x1b, '[', 'M', 0 /* action */,
		char('!' + std::min(MouseEvent.dwMousePosition.X, SHORT(0x7f - '!'))),
		char('!' + std::min(MouseEvent.dwMousePosition.Y, SHORT(0x7f - '!'))),
		0};

	if (MouseEvent.dwEventFlags & MOUSE_WHEELED) {
		seq[3] = sWheelMatrix[ (MouseEvent.dwButtonState & 0x00010000) ? 0 : 1 ][ (imod & 1) ? 1 : 0 ];

	} else if (MouseEvent.dwEventFlags & MOUSE_MOVED) {
		seq[3] = sMoveMatrix[ibut][imod];

	} else {
		seq[3] = sClickMatrix[ibut][imod];
	}

	_vtshell->InjectInput(seq);
	return true;
}
