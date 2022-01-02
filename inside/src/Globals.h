#pragma once
#include <windows.h>
#include <sudo.h>
#include <string>
#include <vector>
#include <farplug-mb.h>
using namespace oldfar;
#include "lng.h"

extern struct Globals
{
	enum {
		MAX_COMMAND_PREFIX = 50
	};
	std::string plugin_path;
	std::string command_prefix;
	std::vector<std::string> configs;
	struct PluginStartupInfo info;

	void Startup(const struct PluginStartupInfo *Info);

	inline bool IsStarted() const {return _started; }

	const char *GetMsg(int id);

	private:
	bool _started = false;
} G;
