#pragma once
#include <string>

namespace VTLog
{
	void Start();
	void Stop();

	void Pause();
	void Resume();

	void Reset();

	void GetAsString(std::string &s, bool append_screen_lines);
	std::string GetAsFile(bool append_screen_lines = true);
	
}
