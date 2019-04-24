#pragma once
#include <string>
#include <map>
#include <set>
#include <memory>
#include <Threaded.h>
#include <all_far.h>
#include <fstdlib.h>
#include "EnumerLocal.h"
#include "ProgressStateUpdate.h"
#include "../FileInformation.h"
#include "../SiteConnection.h"
#include "../UI/Confirm.h"
#include "../UI/Progress.h"


class Upload : protected Threaded, protected ProgressStateIOUpdater
{
	LocalEntries _entries;

	std::shared_ptr<EnumerLocal> _enumer;
	size_t _src_dir_len;
	bool _mv;
	int _op_mode;
	XferDefaultOverwriteAction _xdoa;

	std::shared_ptr<SiteConnection> _connection;
	std::string _dst_dir;
	ProgressState _state;

	void Transfer();

	virtual void *ThreadProc();

public:
	Upload(std::shared_ptr<SiteConnection> &connection);
	bool Do(const std::string &dst_dir, const std::string &src_dir, struct PluginPanelItem *items, int items_count, bool mv, int op_mode);
};
