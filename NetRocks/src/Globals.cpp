#include "Globals.h"
#include <utils.h>

struct Globals G;

void Globals::Startup(const struct PluginStartupInfo *Info)
{
	info = *Info;
	fsf = *(Info->FSF);
	info.FSF = &fsf;
	command_prefix = "sftp"; //ensure not longer than MAX_COMMAND_PREFIX
	config = InMyConfig("netrocks.ini", false);
	_started = true;
}

const char *Globals::GetMsg(int id)
{
	return info.GetMsg(info.ModuleNumber, id);
}

std::chrono::milliseconds TimeMSNow()
{
	return std::chrono::duration_cast< std::chrono::milliseconds >( std::chrono::system_clock::now().time_since_epoch());
}

