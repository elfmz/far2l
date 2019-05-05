#include "Globals.h"
#include "PooledStrings.h"
#include <utils.h>

struct Globals G;

void Globals::Startup(const struct PluginStartupInfo *Info)
{
	info = *Info;
	fsf = *(Info->FSF);
	info.FSF = &fsf;
	command_prefix = L"sftp"; //ensure not longer than MAX_COMMAND_PREFIX
	config = InMyConfig("netrocks.ini", false);
	_started = true;
}

const wchar_t *Globals::GetMsgWide(int id)
{
	return info.GetMsg(info.ModuleNumber, id);
}

const char *Globals::GetMsgMB(int id)
{
	return PooledString(info.GetMsg(info.ModuleNumber, id));
}
