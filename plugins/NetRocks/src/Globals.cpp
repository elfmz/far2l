#include <utility>
#include "Globals.h"
#include "PooledStrings.h"
#include <utils.h>

struct Globals G;

//

GlobalConfigWriter::GlobalConfigWriter(std::unique_ptr<KeyFileHelper> &config, std::mutex &mutex)
	:
	_config(config),
	_lock(mutex)
{
}

GlobalConfigWriter::GlobalConfigWriter(GlobalConfigWriter&& src) noexcept
	:
	_config(src._config),
	_lock(std::move(src._lock))
{
}

GlobalConfigWriter::~GlobalConfigWriter()
{
	if (_config)
		_config->Save();
}

void GlobalConfigWriter::SetInt(const char *name, int value)
{
	if (_config)
		_config->SetInt("Options", name, value);
}

void GlobalConfigWriter::SetBool(const char *name, bool value)
{
	SetInt(name, value ? 1 : 0);
}


//

bool ImportFarFtpSites();

void Globals::Startup(const struct PluginStartupInfo *Info)
{
	if (!_started) {
		info = *Info;
		fsf = *(Info->FSF);
		info.FSF = &fsf;
		_global_config.reset(new KeyFileHelper(InMyConfig("plugins/NetRocks/options.cfg")));
		tsocks_config = InMyConfig("plugins/NetRocks/tsocks.cfg");
		if (!GetGlobalConfigBool("ImportFarFtpSitesDone", false)) {
			if (ImportFarFtpSites()) {
				GlobalConfigWriter w = GetGlobalConfigWriter();
				w.SetBool("ImportFarFtpSitesDone", true);
			}
		}
		_started = true;
	}
}

const wchar_t *Globals::GetMsgWide(int id)
{
	return info.GetMsg(info.ModuleNumber, id);
}

const char *Globals::GetMsgMB(int id)
{
	std::lock_guard<std::mutex> locker(_msg_mb_mtx);
	auto it = _msg_mb.find(id);
	if (it != _msg_mb.end())
		return it->second.c_str();

	auto ir = _msg_mb.emplace(id, Wide2MB(info.GetMsg(info.ModuleNumber, id)));
	return ir.first->second.c_str();
}

//

GlobalConfigWriter Globals::GetGlobalConfigWriter()
{
	return GlobalConfigWriter(_global_config, _global_config_mtx);
}

int Globals::GetGlobalConfigInt(const char *name, int def)
{
	std::lock_guard<std::mutex> locker(_global_config_mtx);

	if (!_global_config)
		return def;

	return _global_config->GetInt("Options", name, def);
}

bool Globals::GetGlobalConfigBool(const char *name, bool def)
{
	return GetGlobalConfigInt(name, def ? 1 : 0) != 0;
}

///////////

Globals::BackgroundTaskScope::BackgroundTaskScope()
{
	G.info.FSF->BackgroundTask(L"NR", TRUE);
}

Globals::BackgroundTaskScope::~BackgroundTaskScope()
{
	G.info.FSF->BackgroundTask(L"NR", FALSE);
}

