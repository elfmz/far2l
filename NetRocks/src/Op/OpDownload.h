#pragma once
#include "OpBase.h"
#include "./Utils/EnumerRemote.h"

class OpDownload : protected OpBase, protected ProgressStateUpdaterCallback
{
	RemoteEntries _entries;

	std::shared_ptr<EnumerRemote> _enumer;

	bool _mv;
	XferOverwriteAction _default_xoa;

	std::string _dst_dir;

	void Transfer();

	virtual void Process();

public:
	OpDownload(std::shared_ptr<SiteConnection> &connection, int op_mode, const std::string &base_dir,
		const std::string &dst_dir, struct PluginPanelItem *items, int items_count, bool mv);
	bool Do();
};
