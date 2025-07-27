#pragma once
#include <string>

class VTAnsi;

namespace VTLog
{
	void Start();
	void Stop();

	void Pause();
	void Resume();

	void ConsoleJoined(HANDLE con_hnd);

	void Reset(HANDLE con_hnd);

	void SetCurrentVTA(VTAnsi* vta);
	VTAnsi* GetCurrentVTA();

	std::string GetAsFile(HANDLE con_hnd, bool colored, bool append_screen_lines = true, const char *wanted_path = nullptr);
}
