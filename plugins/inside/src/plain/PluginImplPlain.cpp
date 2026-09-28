#include "Globals.h"
#include "Commands.h"
#include "PluginImplPlain.h"


PluginImplPlain::PluginImplPlain(const char *name, const char *kind)
	: PluginImpl(name), _kind(kind)
{
	fprintf(stderr, "Inside::PluginImplPlain('%s', '%s')\n", name, kind);
}


bool PluginImplPlain::OnGetFindData(FP_SizeItemList &il, int OpMode)
{
	if (_dir.empty()) {
		std::set<std::string> names;
		Commands::Enum(_kind.c_str(), names);
		for (auto const &name : names) {
			AddUnsized(il, name.c_str(), 0);
		}
		return true;
	}

	return false;
}

bool PluginImplPlain::OnGetFile(const char *item_file, const char *data_path, uint64_t len)
{
	const std::string &cmd = Commands::Get(_kind.c_str(), item_file);
	if (cmd.empty())
		return false;

	Commands::Execute(cmd, _name, data_path);
	return true;
}


bool PluginImplPlain::OnPutFile(const char *item_file, const char *data_path)
{
	return false;
}

bool PluginImplPlain::OnDeleteFile(const char *item_file)
{
	return false;
}
