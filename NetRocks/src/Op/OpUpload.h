#pragma once
#include "OpBase.h"
#include "./Utils/EnumerLocal.h"

class OpUpload : protected OpBase, protected ProgressStateUpdaterCallback
{
	LocalEntries _entries;

	std::shared_ptr<EnumerLocal> _enumer;
	bool _mv;
	XferDefaultOverwriteAction _xdoa;

	std::string _dst_dir;

	void Transfer();

	virtual void Process();

public:
	OpUpload(std::shared_ptr<SiteConnection> &connection, int op_mode, const std::string &base_dir,
		const std::string &dst_dir, struct PluginPanelItem *items, int items_count, bool mv);
	bool Do();
};
