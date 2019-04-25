#pragma once
#include <string>
#include <map>
#include <set>
#include <memory>
#include <Threaded.h>
#include <all_far.h>
#include <fstdlib.h>
#include "./Utils/EnumerRemote.h"
#include "./Utils/ProgressStateUpdate.h"
#include "../FileInformation.h"
#include "../SiteConnection.h"
#include "../UI/Confirm.h"
#include "../UI/Progress.h"


class Remove : protected Threaded
{
	RemoteEntries _entries;

	std::shared_ptr<EnumerRemote> _enumer;

	size_t _src_dir_len;
	int _op_mode;

	std::shared_ptr<SiteConnection> _connection;
	ProgressState _state;

	void Process();

	virtual void *ThreadProc();

public:
	Remove(std::shared_ptr<SiteConnection> &connection);
	bool Do(const std::string &src_dir, struct PluginPanelItem *items, int items_count, int op_mode);
};
