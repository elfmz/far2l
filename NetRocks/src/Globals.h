#pragma once
#include <windows.h>
#include <sudo.h>
#include <string>
#include <mutex>
#include <map>
#include <vector>
#include <memory>
#include <KeyFileHelper.h>
#include <plugin.hpp>
#include "lng.h"


extern struct Globals
{
	std::wstring plugin_path;
	PluginStartupInfo info = {};
	FarStandardFunctions fsf = {};

	std::unique_ptr<KeyFileHelper> global_config;

	void Startup(const struct PluginStartupInfo *Info);

	inline bool IsStarted() const {return _started; }

	const wchar_t *GetMsgWide(int id);
	const char *GetMsgMB(int id);

	private:
	bool _started = false;
	std::map<int, std::string> _msg_mb;
	std::mutex _msg_mb_mtx;
} G;
