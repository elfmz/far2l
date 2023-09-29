/// NB: This file linked both into NetRocks plugin binary and into SHELL protocol broker executeable
#include "WayToShellConfig.h"
#include <KeyFileHelper.h>
#include <utils.h>

WaysToShell::WaysToShell(const std::string &ways_ini)
{
	std::vector<std::string>::operator =(KeyFileReadHelper(ways_ini).EnumSectionsAt(""));
}

WaysToShell::~WaysToShell()
{
}

//

WayToShellConfig::WayToShellConfig(const std::string &ways_ini, const std::string &way_name)
{
	KeyFileReadSection kf(ways_ini, way_name);
	command = kf.GetString("Command");
	serial = kf.GetString("Serial");
	std::string values;
	std::vector<std::string> exploded_values;
	for (unsigned i = 0;;++i) {
		values = kf.GetString(StrPrintf("OPT%u_Items", i));
		if (values.empty()) {
			break;
		}
		options.emplace_back();
		auto &opt = options.back();
		opt.name = kf.GetString(StrPrintf("OPT%u_Name", i));
		exploded_values.clear();
		StrExplode(exploded_values, values, "|");
		for (size_t v = 0; v < exploded_values.size(); ++v) {
			auto &ev = exploded_values[v];
			if (StrStartsFrom(ev, '{') && StrEndsBy(ev, '}')) {
				ev.resize(ev.size() - 1);
				ev.erase(0, 1);
				opt.def = v;
			}
			opt.items.emplace_back();
			auto &item = opt.items.back();
			const size_t p = ev.find(':');
			if (p != std::string::npos) {
				item.info = ev.substr(0, p);
				item.value = ev.substr(p + 1);
			} else {
				item.value = item.info = ev;
			}
		}
	}
}

WayToShellConfig::~WayToShellConfig()
{
}

std::string WayToShellConfig::OptionValue(unsigned index, const StringConfig &protocol_options) const
{
	ASSERT(index < options.size());
	const auto &opt = options[index];
	return protocol_options.GetString(StrPrintf("OPT%u", index).c_str(), opt.items[opt.def].value.c_str());
}
