#include <set>
#include <vector>
#include <atomic>
#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__DragonFly__)
# include <sys/types.h>
# include <sys/event.h>
# include <sys/time.h>
#elif defined(__HAIKU__)
#elif !defined(__CYGWIN__)
# include <sys/inotify.h>
#endif
#include <pthread.h>
#include <limits.h>
#include <dirent.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <os_call.hpp>
#include <fcntl.h>

#include "FSNotify.h"
#include <utils.h>
#include "../../WinPort/WinCompat.h"

#if defined(__CYGWIN__) || defined(__HAIKU__)

class FSNotify : public IFSNotify
{ // dummy implementation that doesnt watch for changes
	public:
		FSNotify(const std::string &pathname, bool watch_subtree, FSNotifyWhat what) {}
		virtual bool Check() const noexcept { return false; }
};

#else

class FSNotify : public IFSNotify
{
#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__DragonFly__)
	std::vector<struct kevent> _events;
#endif
	std::vector<int> _watches;
	pthread_t _watcher;
	int _fd;
	FSNotifyWhat _what;
	std::atomic<bool> _watching{false};
	std::atomic<bool> _change_notified{false};
	int _pipe[2];


	void AddWatch(const char *path)
	{
		int w;
#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__DragonFly__)
		w = open(path, O_RDONLY | O_CLOEXEC);
		if (w != -1) {
			u_int fflags = NOTE_DELETE | NOTE_LINK | NOTE_RENAME;
			if (_what == FSNW_NAMES_AND_STATS)
				fflags|= NOTE_EXTEND | NOTE_WRITE | NOTE_ATTRIB;
			_events.emplace_back();
			EV_SET(&_events.back(), w, EVFILT_VNODE, EV_ADD | EV_ENABLE | EV_ONESHOT, fflags, 0, 0);
		}
#else
		uint32_t mask = IN_DELETE | IN_DELETE_SELF | IN_MOVE_SELF | IN_MOVED_FROM | IN_MOVED_TO | IN_CREATE;
		if (_what == FSNW_NAMES_AND_STATS)
			mask|= IN_MODIFY | IN_ATTRIB;

		w = inotify_add_watch(_fd, path, mask);
#endif
		if (w != -1)
			_watches.emplace_back(w);
		else
			fprintf(stderr, "FSNotify::AddWatch('%s') - error %u\n", path, errno);
	}

	void AddWatchRecursive(const std::string &path, int level)
	{
		DIR *dir = opendir(path.c_str());
		if (!dir)
			return;

		std::vector<std::string> subdirs;
		for (;;) {
			struct dirent *de = readdir(dir);
			if (!de) break;
			if (de->d_type == DT_DIR && strcmp(de->d_name, ".") != 0 && strcmp(de->d_name, "..") != 0)
				subdirs.emplace_back(de->d_name);
		}
		closedir(dir);

		// first set watches on all at current level, and only then recurse deeper
		std::string subpath = path;
		if (!subpath.empty() && subpath.back() != '/')
			subpath+= '/';
		size_t subpath_base_len = subpath.size();

		for (const auto &subdir : subdirs) {
			subpath.resize(subpath_base_len);
			subpath+= subdir;
			AddWatch(subpath.c_str());
		}

		if (level < 128 && _watches.size() < 0x100) {
			for (const auto &subdir : subdirs) {
				subpath.resize(subpath_base_len);
				subpath+= subdir;
				AddWatchRecursive(subpath, level + 1);
			}
		}
	}

	static void *sWatcherProc(void *p)
	{
		((FSNotify *)p)->WatcherProc();
		return nullptr;
	}

	void WatcherProc()
	{
#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__DragonFly__)
		struct kevent ev = {};
		int nev = kevent(_fd, &_events[0], _events.size(), &ev, 1, nullptr);
		if (nev > 0) {
			if (ev.ident != _pipe[0]) {
				_change_notified = true;
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
			if (!_watching)
				break;

			if (FD_ISSET(_fd, &rfds)) {
				r = read(_fd, &buf, sizeof(buf) - 1);
				if (r > 0) {
					//fprintf(stderr, "WatcherProc: triggered by %s\n", buf.ie.name);
					_change_notified = true;

				} else if (errno != EAGAIN && errno != EINTR) {
					fprintf(stderr, "WatcherProc: event read error %u\n", errno);
					break;
				}
			}
			if (FD_ISSET(_pipe[0], &rfds)) {
				if (read(_pipe[0], &buf, 1) <= 0) {
					if (errno != EAGAIN && errno != EINTR)  {
						fprintf(stderr, "WatcherProc: pipe read error %u\n", errno);
						break;
					}
				}
			}
		}
#endif
	}

public:
	FSNotify(const std::string &pathname, bool watch_subtree, FSNotifyWhat what)
		:
		_watcher(0), _fd(-1), _what(what)
	{
#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__DragonFly__)
		_fd = kqueue();
		if (_fd == -1)
			return;
#else
		_fd = inotify_init1(IN_CLOEXEC | IN_NONBLOCK);
		if (_fd == -1)
			return;
#endif

		AddWatch(pathname.c_str());
		if (watch_subtree) {
			AddWatchRecursive(pathname, 0);
		}

		if (!_watches.empty() && pipe_cloexec(_pipe) == 0) {
#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__DragonFly__)
			_events.emplace_back();
			EV_SET(&_events.back(), _pipe[0], EVFILT_READ, EV_ADD | EV_ENABLE | EV_ONESHOT, 0, 0, 0);
#endif
			_watching = true;
			if (pthread_create(&_watcher, nullptr, sWatcherProc, this) == 0) {
				fprintf(stderr, "FSNotify: watching %lu entries for '%s'\n",
					(unsigned long)_watches.size(), pathname.c_str());

			} else {
				fprintf(stderr, "FSNotify: pthread error %u for '%s'\n", errno, pathname.c_str());
				close(_pipe[0]);
				close(_pipe[1]);
				_watching = false;
			}

		} else {
			fprintf(stderr, "FSNotify: not watching '%s'\n", pathname.c_str());
		}
	}

	virtual ~FSNotify()
	{
		if (_watching) {
			_watching = false;

			if (os_call_ssize(write, _pipe[1], (const void*)&_pipe[1], (size_t)1) != 1)
				fprintf(stderr, "~FSNotify: pipe write error %u\n", errno);

			CheckedCloseFD(_pipe[1]);
			pthread_join(_watcher, nullptr);
			CheckedCloseFD(_pipe[0]);
		}

#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__DragonFly__)
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
//		fprintf(stderr, "~FSNotify\n");
	}

	virtual bool Check() const noexcept
	{
		return _change_notified;
	}
};

#endif

IFSNotify *IFSNotify_Create(const std::string &pathname, bool watch_subtree, FSNotifyWhat what)
{
	return new FSNotify(pathname, watch_subtree, what);
}
