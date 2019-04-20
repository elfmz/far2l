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
#include "../UI/XferConfirm.h"
#include "../UI/XferProgress.h"


class FilesGetter : protected Threaded
{
	struct Entries : std::map<std::string, FileInformation> {} _entries;

	std::set<std::string> _items;
	std::string _dst_dir;
	bool _mv;
	int _op_mode;
	XferDefaultOverwriteAction _xdoa = XDOA_ASK;

	std::shared_ptr<SiteConnection> _connection;
	std::string _site_dir;
	unsigned int _scan_depth_limit;

	XferState _state;

	void CheckForUserInput(std::unique_lock<std::mutex> &lock);
	void Scan(const std::string &path);
	void ScanItems();

	virtual void *ThreadProc();

public:
	FilesGetter(std::shared_ptr<SiteConnection> &connection);
	bool Do(const std::string &dst_dir, const std::string &src_dir, struct PluginPanelItem *items, int items_count, bool mv, int op_mode);
};
