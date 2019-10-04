
#include <set>
#include <mutex>
#include <vector>
#include <chrono>
#include <thread>
#include <condition_variable>
#if defined(__APPLE__) || defined(__FreeBSD__)
# include <sys/event.h>
# include <sys/time.h>
#elif !defined(__CYGWIN__)
# include <sys/inotify.h>
#endif
#include <pthread.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <os_call.hpp>
#include <fcntl.h>

#include "WinPortHandle.h"
#include "WinCompat.h"
#include "WinPort.h"
#include "WinPortSynch.h"
#include <utils.h>

#if !defined(__CYGWIN__)
class WinPortFSNotify : public WinPortEvent
{
#if defined(__APPLE__) || defined(__FreeBSD__)
	std::vector<struct kevent> _events;
#endif

	std::vector<int> _watches;
	pthread_t _watcher;
	int _fd;
	DWORD _filter;
	volatile bool _watching;
	int _pipe[2];


	void AddWatch(const char *path)
	{
		int w = -1;
#if defined(__APPLE__) || defined(__FreeBSD__)
		w = open(path, O_RDONLY);
		if (w != -1) {
			_events.emplace_back();
			EV_SET(&_events.back(), w, EVFILT_VNODE, EV_ADD | EV_ENABLE | EV_ONESHOT,
				NOTE_DELETE | NOTE_EXTEND | NOTE_WRITE | NOTE_ATTRIB | NOTE_LINK | NOTE_RENAME,
				0, 0);
		}
#else
		uint32_t mask = 0;
		
		//TODO: be smarter with filtering
		if (_filter & (FILE_NOTIFY_CHANGE_FILE_NAME|FILE_NOTIFY_CHANGE_DIR_NAME))
			mask|= IN_DELETE | IN_DELETE_SELF | IN_MOVE_SELF | IN_MOVED_FROM | IN_MOVED_TO | IN_CREATE;

		if (_filter & (FILE_NOTIFY_CHANGE_ATTRIBUTES | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_SECURITY))
			mask|= IN_MODIFY | IN_CREATE | IN_ATTRIB;

		w = inotify_add_watch(_fd, path, mask);
#endif
		if (w!=-1)
			_watches.emplace_back(w);
		else
			fprintf(stderr, "WinPortFSNotify::AddWatch('%s') - error %u\n", path, errno);
	}

	void AddWatchRecursive(const std::string &path, int level)
	{
		DIR *dir = opendir(path.c_str());
		if (!dir) return;

		std::vector<std::string> subdirs;
		for (;;) {
			struct dirent *de = readdir(dir);
			if (!de) break;
			if (de->d_type == DT_DIR && strcmp(de->d_name, ".") != 0 && strcmp(de->d_name, "..") != 0)
				subdirs.emplace_back(de->d_name);
		}
		closedir(dir);

		// first set watches on all at current level, and only then recurse deeper
		std::string subpath;
		for (const auto &subdir : subdirs) {
			subpath = path;
			if (!subpath.empty() && subpath[subpath.size()-1] != '/')
				subpath+= '/';
			subpath+= subdir;
			AddWatch(subpath.c_str());
		}

		if (level < 128 && _watches.size() < 0x100) {
			for (const auto &subdir : subdirs) {
				subpath = path;
				if (!subpath.empty() && subpath[subpath.size()-1] != '/')
					subpath+= '/';
				subpath+= subdir;
				AddWatchRecursive(subpath, level + 1);
			}
		}
	}

	static void *sWatcherProc(void *p)
	{
		((WinPortFSNotify *)p)->WatcherProc();
		return nullptr;
	}

	void WatcherProc()
	{
#if defined(__APPLE__) || defined(__FreeBSD__)
		struct kevent ev = {};
		int nev = kevent(_fd, &_events[0], _events.size(), &ev, 1, nullptr);
		if (nev > 0) {
			if (ev.ident != _pipe[0]) {
				Set();
			}
		}
#else
		union {
			struct inotify_event ie;
			char space[ sizeof(struct inotify_event) + NAME_MAX + 10 ];
		} buf = {};

		fd_set rfds;
		for (;;) {
			FD_ZERO(&rfds);
			FD_SET(_fd, &rfds);
			FD_SET(_pipe[0], &rfds);
			int r = select(std::max(_fd, _pipe[0]) + 1, &rfds, nullptr, nullptr, nullptr);
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
		fprintf(stderr, "WinPortFSNotify(%p)::OnFinalizeApp\n", (void*)this);
		StopWatching();
		WinPortEvent::OnFinalizeApp();
	}
	
	void StopWatching()
	{
		if (_watching) {
			_watching = false;
			if (os_call_ssize(write, _pipe[1], (const void*)&_pipe[1], (size_t)1) != 1)
				fprintf(stderr, "~WinPortFSNotify - pipe write error %u\n", errno);

			CheckedCloseFD(_pipe[1]);
			pthread_join(_watcher, nullptr);
			CheckedCloseFD(_pipe[0]);
		}

#if defined(__APPLE__) || defined(__FreeBSD__)
		for (auto w : _watches) {
			close(w);
		}
		_watches.clear();
		_events.clear();
		if (_fd != -1) {
			close(_fd);
			_fd = -1;
		}
#else
		if (_fd != -1) {
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
		_fd(-1), _filter(dwNotifyFilter), _watching(false)
	{
#if defined(__APPLE__) || defined(__FreeBSD__)
		_fd = kqueue();
		if (_fd == -1)
			return;
#else
		_fd = inotify_init1(IN_CLOEXEC | IN_NONBLOCK);
		if (_fd == -1)
			return;
#endif

		AddWatch( Wide2MB(lpPathName).c_str() );
		if (bWatchSubtree) {
			AddWatchRecursive(Wide2MB(lpPathName), 0);
		}

		if (!_watches.empty() && pipe_cloexec(_pipe)==0) {
#if defined(__APPLE__) || defined(__FreeBSD__)
			_events.emplace_back();
			EV_SET(&_events.back(), _pipe[0], EVFILT_READ, EV_ADD | EV_ENABLE | EV_ONESHOT, 0, 0, 0);
#endif
			_watching = true;
			if (pthread_create(&_watcher, nullptr, sWatcherProc, this)==0) {
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

#else

WINPORT_DECL(FindFirstChangeNotification, HANDLE, (LPCWSTR lpPathName, BOOL bWatchSubtree, DWORD dwNotifyFilter))
{
	return INVALID_HANDLE_VALUE;
}

WINPORT_DECL(FindNextChangeNotification, BOOL, (HANDLE hChangeHandle))
{
	return FALSE;
}


WINPORT_DECL(FindCloseChangeNotification, BOOL, (HANDLE hChangeHandle))
{
	return TRUE;
}

#endif

////01-04

