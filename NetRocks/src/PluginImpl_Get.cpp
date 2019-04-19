#include <mutex>
#include <thread>
#include <string>

#include "Globals.h"
#include <KeyFileHelper.h>
#include "PluginImpl.h"
#include "UI/SiteConnectionEditor.h"
#include "UI/XferConfirm.h"
#include "UI/XferProgress.h"

typedef std::map<std::string, FileInformation> ScanEntries;

struct GetFilesThread
{
	struct PluginPanelItem *_items;
	int _items_count;
	bool _move;
	const char *_dest_path;
	int _op_mode;
	std::shared_ptr<SiteConnection> _connection;
	std::string _site_dir;

	ScanEntries _entries;
	XferState _state;

	void CheckForUserInput()
	{
		for (;; usleep(1000000)) {
			std::lock_guard<std::mutex> locker(_state.lock);
			if (_state.aborting)
				throw std::runtime_error("Aborted");

			if (!_state.paused)
				break;
		}
	}

	void Scan(const std::string &path, unsigned int depth_limit = 256)
	{
		UnixFileList ufl;
		_connection->DirectoryEnum(path, ufl, 0);
		if (ufl.empty())
			return;

		std::string subpath;
		unsigned long long scanned_size = 0;
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
						fprintf(stderr, "NetRocks::Scan('%s'): depth limit exhausted\n", subpath.c_str());
						// _failed = true;
					}
				} else {
					scanned_size+= e.info.size;
				}
			}
		}

		CheckForUserInput();

		std::lock_guard<std::mutex> locker(_state.lock);
		_state.total_size+= scanned_size;
		_state.total_count = _entries.size();
	}

	void ScanItems()
	{
		std::string item_path;
		FileInformation file_info = {};
		for (int i = 0; i < _items_count; ++i) {
			if (strcmp(_items[i].FindData.cFileName, ".") == 0 || strcmp(_items[i].FindData.cFileName, "..") == 0) {
				continue;
			}

			item_path = _site_dir;
			item_path+= _items[i].FindData.cFileName;
			file_info.mode = _connection->GetMode(item_path, true);
			if (S_ISDIR(file_info.mode)) {
				file_info.size = 0;
				if (_entries.emplace(item_path, file_info).second) {
					Scan(item_path);
				}

			} else {
				file_info.size = _connection->GetSize(item_path, true);
				if (_entries.emplace(item_path, file_info).second) {
					std::lock_guard<std::mutex> locker(_state.lock);
					_state.total_size+= file_info.size;
					_state.total_count = _entries.size();
				}
			}

			CheckForUserInput();
		}
	}

	DWORD Proc()
	{
		DWORD out = 0;
		try {
			ScanItems();
			fprintf(stderr, "NetRocks::GetFiles: _dest_path='%s' --> count=%lu total_size=%llu\n", _dest_path, _entries.size(), _state.total_size);

		} catch (std::exception &e) {
			fprintf(stderr, "NetRocks::GetFilesThread('%s') ERROR: %s\n", _dest_path, e.what());
			out = -1;
		}
		std::lock_guard<std::mutex> locker(_state.lock);
		_state.finished = true;
		return out;
	}

	static void *sProc(void *p)
	{
		GetFilesThread *it = (GetFilesThread *)p;
		return (void *)(uintptr_t)it->Proc();
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
		fprintf(stderr, "NetRocks::GetFiles: cancel\n");
		return FALSE;
	}

	GetFilesThread gft{PanelItem, ItemsNumber, Move != 0, DestPath, OpMode, _connection, CurrentSiteDir(true)};
	XferProgress xpm(Move ? XK_MOVE : XK_COPY, XK_DOWNLOAD, destination, gft._state);

	pthread_t trd = 0;
	if (pthread_create(&trd, NULL, GetFilesThread::sProc, &gft) != 0) {
		return FALSE;
	}

	xpm.Show();

	void *trd_result = nullptr;
	pthread_join(trd, &trd_result);

	return FALSE;
}

