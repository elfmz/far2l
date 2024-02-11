#pragma once
#include <string>

namespace VTLog
{
	void Start();
	void Stop();

	void Pause();
	void Resume();

	void ConsoleJoined(HANDLE con_hnd);

	void Reset(HANDLE con_hnd);

	std::string GetAsFile(HANDLE con_hnd, bool colored, bool append_screen_lines = true);
	
}
