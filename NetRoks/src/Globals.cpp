#include "Globals.h"
#include <utils.h>

struct Globals G;

void Globals::Startup(const struct PluginStartupInfo *Info)
{
	info = *Info;
	command_prefix = "sftp"; //ensure not longer than MAX_COMMAND_PREFIX
	config = InMyConfig("netrox.ini", false);
	_started = true;
}

const char *Globals::GetMsg(int id)
{
	return info.GetMsg(info.ModuleNumber, id);
}
