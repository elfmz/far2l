#pragma once
#include <windows.h>

namespace wxConsoleInputShim
{
	void Enqueue(const INPUT_RECORD *data, DWORD size);
	bool IsKeyDowned(WORD vkey_code);
}
