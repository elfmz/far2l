#include "Globals.h"
#include "PooledStrings.h"
#include <utils.h>

struct Globals G;

void Globals::Startup(const struct PluginStartupInfo *Info)
{
	if (!_started) {
		info = *Info;
		fsf = *(Info->FSF);
		info.FSF = &fsf;
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
