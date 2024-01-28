#pragma once
#include <string>

namespace VTLog
{
	void Start();
	void Stop();

	void Pause();
	void Resume();

	void ConsoleJoined(HANDLE con_hnd);

	void Reset();

	std::string GetAsFile(bool colored, bool append_screen_lines = true);
	
}
