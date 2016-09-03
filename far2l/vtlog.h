#pragma once
#include <string>

namespace VTHistory
{
	void Start();
	void Stop();
	
	void Reset();
	std::string GetAsFile(bool append_screen_lines = true);
	
}
