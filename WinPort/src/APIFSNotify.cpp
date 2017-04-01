
#include <set>
#include <mutex>
#include <chrono>
#include <thread>
#include <condition_variable>
#ifndef __APPLE__
#include <sys/inotify.h>
#endif
#include <pthread.h>

#include <wx/wx.h>
#include <wx/display.h>

#include "WinPortHandle.h"
#include "WinCompat.h"
#include "WinPort.h"
#include "WinPortSynch.h"
#include <utils.h>


class WinPortFSNotify : public WinPortEvent
{
	std::vector<int> _watches;
	pthread_t _watcher;
	int _fd;
	DWORD _filter;
	bool _watching;
	int _pipe[2];


	void AddWatch(const char *path)
	{
		int w = -1;
#ifndef __APPLE__
		uint32_t mask = 0;
		
		//TODO: be smarter with filtering
		if (_filter & (FILE_NOTIFY_CHANGE_FILE_NAME|FILE_NOTIFY_CHANGE_DIR_NAME))
			mask|= IN_DELETE | IN_DELETE_SELF | IN_MOVE_SELF | IN_MOVED_FROM | IN_MOVED_TO | IN_CREATE;

		if (_filter & (FILE_NOTIFY_CHANGE_ATTRIBUTES | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_SECURITY))
			mask|= IN_MODIFY | IN_CREATE | IN_ATTRIB;

		w = inotify_add_watch(_fd, path, mask);
#endif
		if (w!=-1)
			_watches.push_back(w);
		else
			fprintf(stderr, "WinPortFSNotify::AddWatch('%s') - error %u\n", path, errno);
	}

	void AddWatchRecursive(const std::string &path, int level)
	{
		if (level==128) return;

		AddWatch(path.c_str());

		DIR *dir = opendir(path.c_str());
		if (!dir) return;

		std::string subpath;

		for (;;) {
			struct dirent *de = readdir(dir);
			if (!de) break;
			if (de->d_type == DT_DIR){
				 if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
					continue;

				subpath = path;
				if (!subpath.empty() && subpath[subpath.size()-1]!='/')
					subpath+= '/';
				subpath+= de->d_name;
				AddWatchRecursive(subpath, level + 1);
			}
		}
		closedir(dir);
	}

	static void *sWatcherProc(void *p)
	{
		((WinPortFSNotify *)p)->WatcherProc();
		return NULL;
	}

	void WatcherProc()
	{
#ifndef __APPLE__
		union {
			struct inotify_event ie;
			char space[ sizeof(struct inotify_event) + NAME_MAX + 10 ];
		} buf = { 0 };

		fd_set rfds;
		for (;;) {
			FD_ZERO(&rfds);
			FD_SET(_fd, &rfds);
			FD_SET(_pipe[0], &rfds);
			int r = select(std::max(_fd, _pipe[0]) + 1, &rfds, NULL, NULL, NULL);
			if (!_watching) break;

			if (FD_ISSET(_fd, &rfds)) {
				r = read(_fd, &buf, sizeof(buf) - 1);
				if (r > 0) {
					//fprintf(stderr, "WatcherProc: triggered by %s\n", buf.ie.name);
					Set();
				} else if (errno!=EAGAIN && errno!=EWOULDBLOCK) {
					fprintf(stderr, "WatcherProc: event read error %u\n", errno);
					break;
				}
			}
			if (FD_ISSET(_pipe[0], &rfds)) {
				if (read(_pipe[0], &buf, 1) <= 0) {
					if (errno!=EAGAIN && errno!=EWOULDBLOCK)  {
						fprintf(stderr, "WatcherProc: pipe read error %u\n", errno);
						break;
					}
				}
			}
		}
#endif
	}

	virtual void OnFinalizeApp()
	{
		fprintf(stderr, "WinPortFSNotify(%p)::OnFinalizeApp\n", this);
		StopWatching();
		WinPortEvent::OnFinalizeApp();
	}
	
	void StopWatching()
	{
#ifndef __APPLE__
		if (_fd!=-1) {
			if (_watching) {
				_watching = false;
				if (write(_pipe[1], &_watching, sizeof(_watching))!=sizeof(_watching))
					fprintf(stderr, "~WinPortFSNotify - pipe write error %u\n", errno);
					
				pthread_join(_watcher, NULL);
				close(_pipe[0]);
				close(_pipe[1]);
			}
			for (auto w : _watches)
				inotify_rm_watch(_fd, w);
			close(_fd);
			_fd = -1;
		}
#endif
	}
	
public:
	WinPortFSNotify(LPCWSTR lpPathName, BOOL bWatchSubtree, DWORD dwNotifyFilter)
		: WinPortEvent(true, false), _watcher(0),
		_filter(dwNotifyFilter), _watching(false)
	{
#ifndef __APPLE__
		_fd = inotify_init1(IN_CLOEXEC | IN_NONBLOCK);
		if (_fd==-1)
			return;

		if (bWatchSubtree) {
			AddWatchRecursive(Wide2MB(lpPathName), 0);
		} else
			AddWatch( Wide2MB(lpPathName).c_str() );

		if (!_watches.empty() && pipe2(_pipe, O_CLOEXEC)==0) {
			_watching = true;
			if (pthread_create(&_watcher, NULL, sWatcherProc, this)==0) {
				fprintf(stderr, "WinPortFSNotify('%ls') - watching\n", lpPathName);
			} else {
				fprintf(stderr, "WinPortFSNotify('%ls') - pthread error %u\n", lpPathName, errno);
				close(_pipe[0]);
				close(_pipe[1]);
				_watching = false;				
			}
		} else {
			fprintf(stderr, "WinPortFSNotify('%ls') - not watching\n", lpPathName);
		}
#endif
	}

	~WinPortFSNotify()
	{
		StopWatching();
	}

};

WINPORT_DECL(FindFirstChangeNotification, HANDLE, (LPCWSTR lpPathName, BOOL bWatchSubtree, DWORD dwNotifyFilter))
{
	WinPortFSNotify *wpn = new WinPortFSNotify(lpPathName,  bWatchSubtree, dwNotifyFilter);
	return WinPortHandle_Register(wpn);
}

WINPORT_DECL(FindNextChangeNotification, BOOL, (HANDLE hChangeHandle))
{
		AutoWinPortHandle<WinPortFSNotify> wph(hChangeHandle);
		if (!wph) {
			return FALSE;
		}
		
		wph->Reset();
		return TRUE;
}


WINPORT_DECL(FindCloseChangeNotification, BOOL, (HANDLE hChangeHandle))
{
	if (!WinPortHandle_Deregister(hChangeHandle)) {
		return FALSE;
	}
	
	return TRUE;
}

////01-04

