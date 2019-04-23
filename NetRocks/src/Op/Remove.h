#pragma once
#include <string>
#include <map>
#include <set>
#include <memory>
#include <Threaded.h>
#include <all_far.h>
#include <fstdlib.h>
#include "../FileInformation.h"
#include "../SiteConnection.h"
#include "../UI/Confirm.h"
#include "../UI/Progress.h"


class Remove : protected Threaded
{
	struct Entries : std::map<std::string, FileInformation> {} _entries;

	std::set<std::string> _items;
	size_t _src_dir_len;
	int _op_mode;

	std::shared_ptr<SiteConnection> _connection;
	ProgressState _state;
	unsigned int _scan_depth_limit;

	void CheckForUserInput(std::unique_lock<std::mutex> &lock);
	void Scan();
	void ScanItem(const std::string &path);
	void Removal();

	virtual void *ThreadProc();

public:
	Remove(std::shared_ptr<SiteConnection> &connection);
	bool Do(const std::string &src_dir, struct PluginPanelItem *items, int items_count, int op_mode);
};
