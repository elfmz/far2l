#pragma once
#include <string>

namespace VTLog
{
	void Register(HANDLE con_hnd);
	void Unregister(HANDLE con_hnd);

	void ConsoleJoined(HANDLE con_hnd);

	void Reset(HANDLE con_hnd);

	std::string GetAsFile(HANDLE con_hnd, bool colored, bool append_screen_lines = true, const char *wanted_path = nullptr);
}
