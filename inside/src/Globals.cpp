#include "Globals.h"
#include <utils.h>

struct Globals G;

void Globals::Startup(const struct PluginStartupInfo *Info)
{
	info = *Info;
	command_prefix = "inside"; //ensure not longer than MAX_COMMAND_PREFIX

	std::string s = plugin_path;
	size_t p = s.rfind(GOOD_SLASH);
	if (p != std::string::npos) {
		s.resize(p + 1);
		s+= "config.ini";
	}
	configs.emplace_back(InMyConfig("plugins/inside/config.ini", false));
	configs.emplace_back(s);
	if (TranslateInstallPath_Lib2Share(s))
		configs.emplace_back(s);

	_started = true;
}

const char *Globals::GetMsg(int id)
{
	return info.GetMsg(info.ModuleNumber, id);
}
