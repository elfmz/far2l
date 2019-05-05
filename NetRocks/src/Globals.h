#pragma once
#include <windows.h>
#include <sudo.h>
#include <string>
#include <vector>
#include <plugin.hpp>
#include "lng.h"

extern struct Globals
{
	enum {
		MAX_COMMAND_PREFIX = 50
	};
	std::wstring plugin_path;
	std::wstring command_prefix;
	std::string config;
	PluginStartupInfo info = {};
	FarStandardFunctions fsf = {};

	void Startup(const struct PluginStartupInfo *Info);

	inline bool IsStarted() const {return _started; }

	const wchar_t *GetMsgWide(int id);
	const char *GetMsgMB(int id);

	private:
	bool _started = false;
} G;
