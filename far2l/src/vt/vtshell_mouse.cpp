#include "headers.hpp"
#include "vtshell_mouse.h"

#define BUTTONS_PRESS_MASK (FROM_LEFT_1ST_BUTTON_PRESSED | FROM_LEFT_2ND_BUTTON_PRESSED | RIGHTMOST_BUTTON_PRESSED)

VTMouse::VTMouse(IVTShell *vtshell, uint32_t mode)
	: _vtshell(vtshell), _mode(mode)
{
}

bool VTMouse::OnInputMouse(const MOUSE_EVENT_RECORD &MouseEvent)
{
	//mode == 0 means that all mouse handling is disabled
	//shift combinations reserved by VT
	if (MouseEvent.dwControlKeyState & SHIFT_PRESSED || _mode == 0) {
		return false;
	}

	bool no_pressed = ((MouseEvent.dwButtonState & BUTTONS_PRESS_MASK) == 0) && !(MouseEvent.dwEventFlags & MOUSE_WHEELED);

	//send MOUSE_MOVED if only MODE_ANY_EVENT_MOUSE present
	if ((MouseEvent.dwEventFlags & MOUSE_MOVED) &&
		(no_pressed && (_mode & MODE_ANY_EVENT_MOUSE) == 0)) {
		return true;
	}

	// 3 means no button pressed
	unsigned int button = 3;
	if (MouseEvent.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED) button = 0;
	if (MouseEvent.dwButtonState & FROM_LEFT_2ND_BUTTON_PRESSED) button = 1;
	if (MouseEvent.dwButtonState & RIGHTMOST_BUTTON_PRESSED)     button = 2;
	if (MouseEvent.dwEventFlags & MOUSE_WHEELED)                 button = (SHORT(MouseEvent.dwButtonState >> 16) > 0) ? 0x40 : 0x41;

	//track previos button for proper released event in SGR
 	if (no_pressed && (_mode & MODE_SGR_EXT_MOUSE) && _sgr_prev_ibut != 0) {
		button = _sgr_prev_ibut;
		_sgr_prev_ibut = 0;
	} else {
		_sgr_prev_ibut = button;
	}

	if (MouseEvent.dwEventFlags & MOUSE_MOVED) button |= 0x20;

	if (MouseEvent.dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) button |= _ctrl_ind;
	if (MouseEvent.dwControlKeyState & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED))   button |= _alt_ind;
	if (MouseEvent.dwControlKeyState & SHIFT_PRESSED)							 button |= _shift_ind;


	char seq[64]; seq[sizeof(seq) - 1] = 0;
	if (_mode & MODE_SGR_EXT_MOUSE) {
		//in SGR pressed or released state is enocoded by suffix
		snprintf(seq, sizeof(seq) - 1, "\x1b[<%d;%d;%d%c",
			button,
			MouseEvent.dwMousePosition.X + 1,
			MouseEvent.dwMousePosition.Y + 1,
			(no_pressed ? 'm' : 'M')
		);
	} else {
		if(MouseEvent.dwMousePosition.X < SHORT(0xff - 33) && MouseEvent.dwMousePosition.Y < SHORT(0xff - 33)) {
			//in X10 Encoding button release and no button pressed are the same and 32 (0x20) is added to all values
			button |= 0x20;

			snprintf(seq, sizeof(seq) - 1, "\x1b[M%c%c%c",
				char(button),
				char(MouseEvent.dwMousePosition.X + 33),
				char(MouseEvent.dwMousePosition.Y + 33)
			);
		} else {
			// mouse out of encodeable region - skip events to avoid misclicks
			fprintf(stderr, "VTMouse: far away - %d:%d\n", MouseEvent.dwMousePosition.X, MouseEvent.dwMousePosition.Y);
			return true;
		}
	}

	_vtshell->InjectInput(seq);
	return true;
}