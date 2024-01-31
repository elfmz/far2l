#include "headers.hpp"
#include "vtshell_mouse.h"

#define BUTTONS_PRESS_MASK (FROM_LEFT_1ST_BUTTON_PRESSED | FROM_LEFT_2ND_BUTTON_PRESSED | RIGHTMOST_BUTTON_PRESSED)

VTMouse::VTMouse(IVTShell *vtshell, uint32_t mex)
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
		if ((_mex & (MEX_BTN_EVENT_MOUSE | MEX_ANY_EVENT_MOUSE)) == 0)
			return true;

		if ((MouseEvent.dwButtonState & BUTTONS_PRESS_MASK) == 0
				&& (_mex & MEX_ANY_EVENT_MOUSE) == 0) {
			return true;
		}
	}
	unsigned int imod = 0, ibut = 0;

	if (MouseEvent.dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) imod|= 1;
	if (MouseEvent.dwControlKeyState & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED)) imod|= 2;

	if (MouseEvent.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED) ibut = 1;
	else if (MouseEvent.dwButtonState & FROM_LEFT_2ND_BUTTON_PRESSED) ibut = 2;
	else if (MouseEvent.dwButtonState & RIGHTMOST_BUTTON_PRESSED) ibut = 3;

	if (_mex & MEX_SGR_EXT_MOUSE) {
		int action;
		char suffix = 'M';
		if (MouseEvent.dwEventFlags & MOUSE_MOVED) {
			action = 35;
			if (ibut) {
				action = (ibut == 1) ? 0 : ((ibut == 2) ? 1 : 2);
				action|= 32;
			}

		} else if (MouseEvent.dwEventFlags & MOUSE_WHEELED) {
			action = (SHORT(MouseEvent.dwButtonState >> 16) > 0) ? 64 : 65;

		} else {
			int abut = ibut;
			if (ibut == 0 && _sgr_prev_ibut != 0) {
				suffix = 'm';
				abut = _sgr_prev_ibut;
			}
			_sgr_prev_ibut = ibut;
			action = (abut == 1) ? 0 : ((abut == 2) ? 1 : 2);
		}
		if (imod & 1) action|= 16;
		if (imod & 2) action|= 8;
		if (MouseEvent.dwControlKeyState & SHIFT_PRESSED) action|= 4;

		char seq[64]; seq[sizeof(seq) - 1] = 0;
		snprintf(seq, sizeof(seq) - 1, "\x1b[<%d;%d;%d%c", action,
			MouseEvent.dwMousePosition.X + 1, MouseEvent.dwMousePosition.Y + 1,
			suffix);
		_vtshell->InjectInput(seq);
		return true;
	}

	if ( MouseEvent.dwMousePosition.X < 0 || MouseEvent.dwMousePosition.X > SHORT(0xff - '!')
		|| MouseEvent.dwMousePosition.Y < 0 || MouseEvent.dwMousePosition.Y > SHORT(0xff - '!') )
	{
		// mouse out of encodeable region - skip events to avoid misclicks
		fprintf(stderr, "VTMouse: far away - %d:%d\n", MouseEvent.dwMousePosition.X, MouseEvent.dwMousePosition.Y);
		return true;
	}

	char seq[] = {0x1b, '[', 'M', 0 /* action */,
		char('!' + MouseEvent.dwMousePosition.X),
		char('!' + MouseEvent.dwMousePosition.Y),
		0};

	if (MouseEvent.dwEventFlags & MOUSE_WHEELED) {
		seq[3] = sWheelMatrix[ (SHORT(MouseEvent.dwButtonState >> 16) > 0) ? 0 : 1 ][ (imod & 1) ? 1 : 0 ];

	} else if (MouseEvent.dwEventFlags & MOUSE_MOVED) {
		seq[3] = sMoveMatrix[ibut][imod];

	} else {
		seq[3] = sClickMatrix[ibut][imod];
	}

	_vtshell->InjectInput(seq);
	return true;
}
