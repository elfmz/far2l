#pragma once
#include <windows.h>
#include <sudo.h>
#include <string>
#include <mutex>
#include <map>
#include <vector>
#include <memory>
#include <KeyFileHelper.h>
#include <farplug-wide.h>
#include "lng.h"


struct GlobalConfigWriter
{
	std::unique_ptr<KeyFileHelper> &_config;
	std::unique_lock<std::mutex> _lock;

	GlobalConfigWriter(const GlobalConfigWriter &) = delete;

public:
	GlobalConfigWriter(std::unique_ptr<KeyFileHelper> &config, std::mutex &mutex);
	GlobalConfigWriter(GlobalConfigWriter&& src) noexcept;

	~GlobalConfigWriter();

	void SetInt(const char *name, int value);
	void SetBool(const char *name, bool value);
};

extern struct Globals
{
	struct BackgroundTaskScope
	{
		BackgroundTaskScope();
		~BackgroundTaskScope();
	};

	std::wstring plugin_path;
	PluginStartupInfo info = {};
	FarStandardFunctions fsf = {};

	std::string tsocks_config;

	void Startup(const struct PluginStartupInfo *Info);

	inline bool IsStarted() const {return _started; }

	const wchar_t *GetMsgWide(int id);
	const char *GetMsgMB(int id);

	GlobalConfigWriter GetGlobalConfigWriter();
	int GetGlobalConfigInt(const char *name, int def = -1);
	bool GetGlobalConfigBool(const char *name, bool def = false);

private:
	std::unique_ptr<KeyFileHelper> _global_config;
	std::mutex _global_config_mtx;

	bool _started = false;
	std::map<int, std::string> _msg_mb;
	std::mutex _msg_mb_mtx;
} G;

