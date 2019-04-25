#pragma once
#include <string>
#include <map>
#include <set>
#include <all_far.h>
#include <fstdlib.h>

#include "EnumerBase.h"

#include "../../FileInformation.h"
#include "../../SiteConnection.h"


typedef std::map<std::string, FileInformation> RemoteEntries;

class EnumerRemote : public EnumerBase
{
	RemoteEntries &_entries;
	std::shared_ptr<SiteConnection> _connection;

	virtual void ScanItem(const std::string &path);
	virtual bool OnScanningPath(const std::string &path);

public:
	EnumerRemote(RemoteEntries &entries, ProgressState &state, const std::string src_dir,
		const struct PluginPanelItem *items, int items_count, bool no_special_files, std::shared_ptr<SiteConnection> &connection);
};
