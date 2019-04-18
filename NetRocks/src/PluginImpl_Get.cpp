#include "Globals.h"
#include <KeyFileHelper.h>
#include "PluginImpl.h"
#include "UI/SiteConnectionEditor.h"
#include "UI/XferConfirm.h"


typedef std::map<std::string, FileInformation> ScanEntries;

class RemoteDirScanner
{
	std::shared_ptr<SiteConnection> _connection;
	ScanEntries &_entries;
	unsigned long scanned_size = 0;
	bool _failed = false;

public:
	RemoteDirScanner(std::shared_ptr<SiteConnection> &connection, ScanEntries &entries)
		: _connection(connection), _entries(entries)
	{
	}

	unsigned long long ScannedSize() const
	{
		return scanned_size;
	}

	bool IsFailed() const
	{
		return _failed;
	}

	void Scan(const std::string &path, unsigned int depth_limit = 256)
	{
		try {
			UnixFileList ufl;
			_connection->DirectoryEnum(path, ufl, 0);
			std::string subpath;
			for (const auto &e : ufl) {
				subpath = path;
				subpath+= '/';
				subpath+= e.name;
				if (_entries.emplace(subpath, e.info).second) {
					if (S_ISDIR(e.info.mode)) {
						if (depth_limit) {
							subpath+= '/';
							Scan(subpath, depth_limit - 1);
						} else {
							fprintf(stderr, "NetRocks::RemoteDirScanner('%s'): depth limit exhausted\n", subpath.c_str());
							_failed = true;
						}
					} else {
						scanned_size+= e.info.size;
					}
				}
			}

		} catch (std::exception &e) {
			fprintf(stderr, "NetRocks::RemoteDirScanner('%s') ERROR: %s\n", path.c_str(), e.what());
			_failed = true;
		}

	}
};

int PluginImpl::GetFiles(struct PluginPanelItem *PanelItem, int ItemsNumber, int Move, char *DestPath, int OpMode)
{
	fprintf(stderr, "NetRocks::GetFiles: _dir='%s' DestPath='%s' ItemsNumber=%d\n", _cur_dir, DestPath, ItemsNumber);
	if (ItemsNumber <= 0)
		return FALSE;

	std::string destination;
	if (DestPath) {
		destination = DestPath;
	} else
		destination = ".";

	XferDefaultOverwriteAction xdoa = XDOA_ASK;
	if (!XferConfirm(Move ? XK_MOVE : XK_COPY, XK_DOWNLOAD, destination).Ask(xdoa)) {
		return FALSE;
	}

	std::string site_dir = CurrentSiteDir(true), item_path;
	ScanEntries entries;
	std::string data_path;
	unsigned long long total_size = 0;
	FileInformation file_info = {};
	for (int i = 0; i < ItemsNumber; ++i) {
		item_path = site_dir;
		item_path+= PanelItem[i].FindData.cFileName;
		file_info.mode = _connection->GetMode(item_path, true);
		if (S_ISDIR(file_info.mode)) {
			file_info.size = 0;
			if (entries.emplace(item_path, file_info).second) {
				RemoteDirScanner rds(_connection, entries);
				rds.Scan(item_path);
				total_size+= rds.ScannedSize();
			}

		} else {
			file_info.size = _connection->GetSize(item_path, true);
			if (entries.emplace(item_path, file_info).second) {
				total_size+= file_info.size;
			}
		}
	}

	fprintf(stderr, "NetRocks::GetFiles: _dir='%s' DestPath='%s' --> count=%lu total_size=%llu\n", _cur_dir, DestPath, entries.size(), total_size);

	return FALSE;
}

