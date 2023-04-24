#include "wxConsoleInputShim.h"
#include "Backend.h"
#include <vector>

namespace wxConsoleInputShim
{
	// no synch - as it all called within WX UI thread
	static std::vector<bool> s_pressed_keys(0xff);

	static inline bool KeyCodeIsOk(WORD vkey_code)
	{
		return vkey_code > 0 && vkey_code <= s_pressed_keys.size();
	}

	void Enqueue(const INPUT_RECORD *data, DWORD size)
	{
		for (DWORD i = 0; i < size; ++i) {
			if (data[i].EventType == KEY_EVENT) {
				const auto &krec = data[i].Event.KeyEvent;
				if (KeyCodeIsOk(krec.wVirtualKeyCode)) {
					s_pressed_keys[krec.wVirtualKeyCode - 1] = (krec.bKeyDown != FALSE);
				}
			}
		}
		g_winport_con_in->Enqueue(data, size);
	}

	bool IsKeyDowned(WORD vkey_code)
	{
		return KeyCodeIsOk(vkey_code) && s_pressed_keys[vkey_code - 1];
	}
}
