#pragma once

namespace VTHistory
{
	void Start();
	void Stop();
	
	string GetAsFile(unsigned short append_screen_lines = 0x7fff);
	
}
