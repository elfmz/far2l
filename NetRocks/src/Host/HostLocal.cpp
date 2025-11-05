#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <utime.h>
#include <dirent.h>
#include <limits.h>
#include "Erroring.h"

#ifdef __APPLE__
	#include <sys/mount.h>
#elif !defined(__FreeBSD__) && !defined(__DragonFly__) && !defined(__HAIKU__)
	#include <sys/statfs.h>
#endif

#include <string>
#include <map>
#include <ScopeHelpers.h>
#include <unistd.h>
#include <fcntl.h>
#include <pwd.h>
#include <grp.h>
#include <CheckedCast.hpp>
#include <utils.h>
#include "HostLocal.h"
#include "../../WinPort/WinCompat.h"

#ifdef NETROCKS_PROTOCOL
# include <utimens_compat.h>
# define API(x) x

#else
# include "../lng.h"
# include "../Globals.h"
# include <sudo.h>
# define API(x) sdc_##x
#endif


HostLocal::HostLocal()
{
}

HostLocal::~HostLocal()
{
}

std::string HostLocal::SiteName()
{
#ifdef NETROCKS_PROTOCOL
	return std::string();
#else
	return G.GetMsgMB(MHostLocalName);
#endif
}

void HostLocal::GetIdentity(Identity &identity)
{
	identity = Identity();
}

std::shared_ptr<IHost> HostLocal::Clone()
{
	return std::make_shared<HostLocal>();
}

void HostLocal::ReInitialize()
{
}

void HostLocal::Abort()
{
}

mode_t HostLocal::GetMode(const std::string &path, bool follow_symlink)
{
	struct stat s = {};
	int r = follow_symlink ? API(stat)(path.c_str(), &s) : API(lstat)(path.c_str(), &s);
	if (r == -1) {
		throw ProtocolError("stat failed", errno);
	}

	return s.st_mode;
}

unsigned long long HostLocal::GetSize(const std::string &path, bool follow_symlink)
{
	struct stat s = {};
	int r = follow_symlink ? API(stat)(path.c_str(), &s) : API(lstat)(path.c_str(), &s);
	if (r == -1) {
		throw ProtocolError("stat failed", errno);
	}

	return s.st_size;
}

void HostLocal::GetInformation(FileInformation &file_info, const std::string &path, bool follow_symlink)
{
	struct stat s = {};
	int r = follow_symlink ? API(stat)(path.c_str(), &s) : API(lstat)(path.c_str(), &s);
	if (r == -1) {
		throw ProtocolError("stat failed", errno);
	}
	file_info.access_time = s.st_atim;
	file_info.modification_time = s.st_mtim;
	file_info.status_change_time = s.st_ctim;
	file_info.size = s.st_size;
	file_info.mode = s.st_mode;
}

void HostLocal::FileDelete(const std::string &path)
{
	int r = API(unlink)(path.c_str());
	if (r == -1) {
		throw ProtocolError("unlink failed", errno);
	}
}

void HostLocal::DirectoryDelete(const std::string &path)
{
	int r = API(rmdir)(path.c_str());
	if (r == -1) {
		throw ProtocolError("rmdir failed", errno);
	}
}

void HostLocal::DirectoryCreate(const std::string &path, mode_t mode)
{
	int r = API(mkdir)(path.c_str(), mode);
	if (r == -1) {
		throw ProtocolError("mkdir failed", errno);
	}
}

void HostLocal::Rename(const std::string &path_old, const std::string &path_new)
{
	int r = API(rename)(path_old.c_str(), path_new.c_str());
	if (r == -1) {
		throw ProtocolError("rename failed", errno);
	}
}

void HostLocal::SetTimes(const std::string &path, const timespec &access_time, const timespec &modification_time)
{
	struct timespec times[2] = {access_time, modification_time};
	int r = API(utimens)(path.c_str(), times);
	if (r == -1) {
		throw ProtocolError("utimens failed", errno);
	}
}

void HostLocal::SetMode(const std::string &path, mode_t mode)
{
	int r = API(chmod)(path.c_str(), mode);
	if (r == -1) {
		throw ProtocolError("chmod failed", errno);
	}
}


void HostLocal::SymlinkCreate(const std::string &link_path, const std::string &link_target)
{
	int r = API(symlink)(link_target.c_str(), link_path.c_str());
	if (r == -1) {
		throw ProtocolError("symlink failed", errno);
	}
}

void HostLocal::SymlinkQuery(const std::string &link_path, std::string &link_target)
{
	char buf[PATH_MAX];
	ssize_t r = API(readlink)(link_path.c_str(), buf, sizeof(buf));
	if (r < 0 || (size_t)r >= sizeof(buf)) {
		throw ProtocolError("read failed", errno);
	}

	link_target.assign(buf, (size_t)r);
}

////////////////////////////////////////


class HostLocalDirectoryEnumer : public IDirectoryEnumer
{
	std::string _path, _subpath;
	DIR *_d = nullptr;
	std::map<uid_t, std::string> _cached_users;
	std::map<gid_t, std::string> _cached_groups;

	const std::string &UserByID(uid_t id)
	{
		auto it = _cached_users.find(id);
		if (it != _cached_users.end()) {
			return it->second;
		}

		struct passwd *pw = getpwuid(id);
		const auto &ir = _cached_users.emplace(id, (pw ? pw->pw_name : ""));
		return ir.first->second;
	}

	const std::string &GroupByID(gid_t id)
	{
		auto it = _cached_groups.find(id);
		if (it != _cached_groups.end()) {
			return it->second;
		}

		struct group *gr = getgrgid(id);
		const auto &ir = _cached_groups.emplace(id, (gr ? gr->gr_name : ""));
		return ir.first->second;
	}

public:
	HostLocalDirectoryEnumer(const std::string &path)
		: _path(path)
	{
		_d = API(opendir)(_path.c_str());
		if (!_d) {
			throw ProtocolError("opendir failed", errno);
		}
		if (!_path.empty() && _path[_path.size() - 1] != '/') {
			_path+= '/';
		}
	}

	virtual ~HostLocalDirectoryEnumer()
	{
		if (_d != nullptr) {
			API(closedir)(_d);
		}
	}

	virtual bool Enum(std::string &name, std::string &owner, std::string &group, FileInformation &file_info)
	{
		for (;;) {
			struct dirent *de = API(readdir)(_d);
			if (!de) {
				return false;
			}

			if (!FILENAME_ENUMERABLE(de->d_name)) {
				continue;
			}

			name = de->d_name;

			_subpath = _path;
			_subpath+= name;
			struct stat s = {};
			if (API(lstat)(_subpath.c_str(), &s) == -1) {
				owner.clear();
				group.clear();
				file_info = FileInformation();
				return true;
			}

			owner = UserByID(s.st_uid);
			group = GroupByID(s.st_gid);

			file_info.access_time = s.st_atim;
			file_info.modification_time = s.st_mtim;
			file_info.status_change_time = s.st_ctim;
			file_info.size = s.st_size;
			file_info.mode = s.st_mode;
			return true;
		}
  	}
};

std::shared_ptr<IDirectoryEnumer> HostLocal::DirectoryEnum(const std::string &path)
{
	return std::make_shared<HostLocalDirectoryEnumer>(path);
}


class HostLocalFileIO : public IFileReader, public IFileWriter
{
	int _fd = -1;

public:
	HostLocalFileIO(const std::string &path, unsigned long long resume_pos, int oflag, mode_t mode)
	{
		_fd = API(open)(path.c_str(), oflag, mode);
		if (_fd == -1) {
			throw ProtocolError("open failed", errno);
		}
		if (resume_pos) {
			if (API(lseek)(_fd, resume_pos, SEEK_SET) == -1) {
				CheckedCloseFD(_fd);
				throw ProtocolError("lseek failed", errno);
			}
		}
	}

	virtual ~HostLocalFileIO()
	{
		CheckedCloseFD(_fd);
	}

	virtual size_t Read(void *buf, size_t len)
	{
		ssize_t rv = API(read)(_fd, buf, len);
		if (rv < 0 && errno == EIO) {
			// workaround for SMB's read error when requested fragment overlaps end of file
			off_t pos = API(lseek)(_fd, 0, SEEK_CUR);
			struct stat st{};
			if (pos != -1 && API(fstat)(_fd, &st) == 0
					&& pos < (off_t)st.st_size && (off_t)(pos + len) > (off_t)st.st_size) {
				rv = API(read)(_fd, buf, (size_t)(st.st_size - pos));

			} else {
				errno = EIO;
			}
		}
		if (rv < 0) {
			throw ProtocolError("read failed", errno);
		}
		return (size_t)rv;
	}

	virtual void Write(const void *buf, size_t len)
	{
		while (len) {
			ssize_t rv = API(write)(_fd, buf, len);
			if (rv < 0 || (size_t)rv > len) {
				throw ProtocolError("write failed", errno);
			}
			len-= (size_t)rv;
			buf = (const char *)buf + rv;
		}
	}

	virtual void WriteComplete()
	{
/*
#ifdef __APPLE__
		if (fcntl(_fd, F_FULLFSYNC) == -1) {
#else
		if (fdatasync(_fd) == -1) {
#endif
			throw ProtocolError("sync failed", errno);
		}
*/
	}
};


std::shared_ptr<IFileReader> HostLocal::FileGet(const std::string &path, unsigned long long resume_pos)
{
	return std::make_shared<HostLocalFileIO>(path, resume_pos, O_RDONLY, 0 );
}

std::shared_ptr<IFileWriter> HostLocal::FilePut(const std::string &path, mode_t mode, unsigned long long size_hint, unsigned long long resume_pos)
{
	return std::make_shared<HostLocalFileIO>(path, resume_pos, (resume_pos == 0) ? O_CREAT | O_TRUNC | O_RDWR : O_RDWR, mode );
}

bool HostLocal::Alive()
{
	return true;
}
