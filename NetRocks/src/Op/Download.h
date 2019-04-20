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


class Download : protected Threaded, protected SiteConnection::IOStatusCallback
{
	struct Entries : std::map<std::string, FileInformation> {} _entries;

	std::set<std::string> _items;
	std::string _dst_dir;
	size_t _src_dir_len;
	bool _mv;
	int _op_mode;
	XferDefaultOverwriteAction _xdoa = XDOA_ASK;

	std::shared_ptr<SiteConnection> _connection;
	std::string _site_dir;
	unsigned int _scan_depth_limit;

	XferState _state;

	void CheckForUserInput(std::unique_lock<std::mutex> &lock);
	void Scan();
	void ScanItem(const std::string &path);
	void Transfer();

	virtual bool OnIOStatus(unsigned long long transferred);
	virtual void *ThreadProc();

public:
	Download(std::shared_ptr<SiteConnection> &connection);
	bool Do(const std::string &dst_dir, const std::string &src_dir, struct PluginPanelItem *items, int items_count, bool mv, int op_mode);
};
