#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include "ProtocolNFS.h"
#include <nfsc/libnfs-raw-nfs.h>
#include <StringConfig.h>
#include <utils.h>


std::shared_ptr<IProtocol> CreateProtocolNFS(const std::string &host, unsigned int port,
	const std::string &username, const std::string &password, const std::string &options) throw (std::runtime_error)
{
	return std::make_shared<ProtocolNFS>(host, port, username, password, options);
}

////////////////////////////

ProtocolNFS::ProtocolNFS(const std::string &host, unsigned int port,
	const std::string &username, const std::string &password, const std::string &options) throw (std::runtime_error)
	:
	_nfs(std::make_shared<NFSConnection>()),
	_host(host)
{
	_nfs->ctx = nfs_init_context();
	if (!_nfs->ctx) {
		throw ProtocolError("Create context error", errno);
	}

	StringConfig protocol_options(options);
#ifdef LIBNFS_FEATURE_READAHEAD
	int i = atoi(username.c_str());
	if (i != 0 || username == "0") {
		nfs_set_uid(_nfs->ctx, i);
	}
	i = atoi(password.c_str());
	if (i != 0 || password == "0") {
		nfs_set_gid(_nfs->ctx, i);
	}
#endif
}

ProtocolNFS::~ProtocolNFS()
{
}

std::string ProtocolNFS::RootedPath(const std::string &path)
{
	return path;
/*
	if (path.empty() || path == "/") {
		_ctx->mount.clear();
		return path;
	}

	size_t p = path.find('/', 1);
	if (p == std::string::npos)
		p = path.size();
	const std::string mount;
	if (path[0] == '/')
		mount = path.substr(1, p - 1);
	else
		mount = path.substr(0, p);

	if (mount == _nfs->mount) {
		return;
	}

	int rc = nfs_mount(_nfs->ctx, _nfs->host.c_str(), mount.c_str());
	if (rc != 0) {
		throw ProtocolError("Mount error", errno);
	}

	_nfs->mount.swap(mount);

	return path.substr(p);
*/
}

bool ProtocolNFS::IsBroken()
{
	return false;//(!_conn || !_conn->ctx || !_conn->SMB || (ssh_get_status(_conn->ssh) & (SSH_CLOSED|SSH_CLOSED_ERROR)) != 0);
}

mode_t ProtocolNFS::GetMode(const std::string &path, bool follow_symlink) throw (std::runtime_error)
{
#ifdef LIBNFS_FEATURE_READAHEAD
	struct nfs_stat_64 s = {};
	int rc = follow_symlink
		? nfs_stat64(_nfs->ctx, RootedPath(path).c_str(), &s)
		: nfs_lstat64(_nfs->ctx, RootedPath(path).c_str(), &s);
	auto out = s.nfs_mode;
#else
	struct stat s = {};
	int rc = nfs_stat(_nfs->ctx, RootedPath(path).c_str(), &s);
	auto out = s.st_mode;
#endif
	if (rc != 0)
		throw ProtocolError("Get mode error", rc);
	return out;
}

unsigned long long ProtocolNFS::GetSize(const std::string &path, bool follow_symlink) throw (std::runtime_error)
{
#ifdef LIBNFS_FEATURE_READAHEAD
	struct nfs_stat_64 s = {};
	int rc = follow_symlink
		? nfs_stat64(_nfs->ctx, RootedPath(path).c_str(), &s)
		: nfs_lstat64(_nfs->ctx, RootedPath(path).c_str(), &s);
	auto out = s.nfs_size;
#else
	struct stat s = {};
	int rc = nfs_stat(_nfs->ctx, RootedPath(path).c_str(), &s);
	auto out = s.st_size;
#endif
	if (rc != 0)
		throw ProtocolError("Get size error", rc);

	return out;
}

void ProtocolNFS::GetInformation(FileInformation &file_info, const std::string &path, bool follow_symlink) throw (std::runtime_error)
{
#ifdef LIBNFS_FEATURE_READAHEAD
	struct nfs_stat_64 s = {};
	int rc = follow_symlink
		? nfs_stat64(_nfs->ctx, RootedPath(path).c_str(), &s)
		: nfs_lstat64(_nfs->ctx, RootedPath(path).c_str(), &s);
	if (rc != 0)
		throw ProtocolError("Get info error", rc);

	file_info.access_time.tv_sec = s.nfs_atime;
	file_info.access_time.tv_nsec = s.nfs_atime_nsec;
	file_info.modification_time.tv_sec = s.nfs_mtime;
	file_info.modification_time.tv_nsec = s.nfs_mtime_nsec;
	file_info.status_change_time.tv_sec = s.nfs_ctime;
	file_info.status_change_time.tv_nsec = s.nfs_ctime_nsec;
	file_info.mode = s.nfs_mode;
	file_info.size = s.nfs_size;
#else
	struct stat s = {};
	int rc = nfs_stat(_nfs->ctx, RootedPath(path).c_str(), &s);
	if (rc != 0)
		throw ProtocolError("Get info error", rc);

	file_info.access_time = s.st_atim;
	file_info.modification_time = s.st_mtim;
	file_info.status_change_time = s.st_ctim;
	file_info.size = s.st_size;
	file_info.mode = s.st_mode;
#endif
}

void ProtocolNFS::FileDelete(const std::string &path) throw (std::runtime_error)
{
	int rc = nfs_unlink(_nfs->ctx, RootedPath(path).c_str());
	if (rc != 0)
		throw ProtocolError("Delete file error", rc);
}

void ProtocolNFS::DirectoryDelete(const std::string &path) throw (std::runtime_error)
{
	int rc = nfs_rmdir(_nfs->ctx, RootedPath(path).c_str());
	if (rc != 0)
		throw ProtocolError("Delete directory error", rc);
}

void ProtocolNFS::DirectoryCreate(const std::string &path, mode_t mode) throw (std::runtime_error)
{
	int rc = nfs_mkdir(_nfs->ctx, RootedPath(path).c_str());//, mode);
	if (rc != 0)
		throw ProtocolError("Create directory error", rc);
}

void ProtocolNFS::Rename(const std::string &path_old, const std::string &path_new) throw (std::runtime_error)
{
	int rc = nfs_rename(_nfs->ctx, RootedPath(path_old).c_str(), RootedPath(path_new).c_str());
	if (rc != 0)
		throw ProtocolError("Rename error", rc);
}


void ProtocolNFS::SetTimes(const std::string &path, const timespec &access_time, const timespec &modification_time) throw (std::runtime_error)
{
	struct timeval times[2] = {};
	times[0].tv_sec = access_time.tv_sec;
	times[0].tv_usec = access_time.tv_nsec / 1000;
	times[1].tv_sec = modification_time.tv_sec;
	times[1].tv_usec = modification_time.tv_nsec / 1000;

	int rc = nfs_utimes(_nfs->ctx, RootedPath(path).c_str(), times);
	if (rc != 0)
		throw ProtocolError("Set times error",  rc);
}

void ProtocolNFS::SetMode(const std::string &path, mode_t mode) throw (std::runtime_error)
{
	int rc = nfs_chmod(_nfs->ctx, RootedPath(path).c_str(), mode);
	if (rc != 0)
		throw ProtocolError("Set mode error",  rc);
}

void ProtocolNFS::SymlinkCreate(const std::string &link_path, const std::string &link_target) throw (std::runtime_error)
{
	int rc = nfs_symlink(_nfs->ctx, RootedPath(link_target).c_str(), RootedPath(link_path).c_str());
	if (rc != 0)
		throw ProtocolError("Symlink create error",  rc);
}

void ProtocolNFS::SymlinkQuery(const std::string &link_path, std::string &link_target) throw (std::runtime_error)
{
	char buf[0x1001] = {};
	int rc = nfs_readlink(_nfs->ctx, RootedPath(link_path).c_str(), buf, sizeof(buf) - 1);
	if (rc != 0)
		throw ProtocolError("Symlink query error",  rc);

	link_target = buf;
}

class NFSDirectoryEnumer : public IDirectoryEnumer
{
	std::shared_ptr<NFSConnection> _nfs;
	struct nfsdir *_dir = nullptr;

public:
	NFSDirectoryEnumer(std::shared_ptr<NFSConnection> &nfs, const std::string &path)
		: _nfs(nfs)
	{
		//const std::string &dir_path = protocol->RootedPath(path)
		int rc = nfs_opendir(_nfs->ctx, path.c_str(), &_dir);
		if (rc != 0 || _dir == nullptr) {
			_dir = nullptr;
			throw ProtocolError("Directory open error", rc);
		}
	}

	virtual ~NFSDirectoryEnumer()
	{
		if (_dir != nullptr) {
			nfs_closedir(_nfs->ctx, _dir);
		}
	}

	virtual bool Enum(std::string &name, std::string &owner, std::string &group, FileInformation &file_info) throw (std::runtime_error)
	{
		for (;;) {
			struct nfsdirent *de = nfs_readdir(_nfs->ctx, _dir);
			if (de == nullptr) {
				return false;
			}
			if (!de->name || !FILENAME_ENUMERABLE(de->name)) {
				continue;
			}

			name = de->name;

			file_info.access_time.tv_sec = de->atime.tv_sec;
			file_info.modification_time.tv_sec = de->mtime.tv_sec;
			file_info.status_change_time.tv_sec = de->ctime.tv_sec;
#ifdef LIBNFS_FEATURE_READAHEAD
			owner = StrPrintf("UID:%u", de->uid);
			group = StrPrintf("GID:%u", de->gid);
			file_info.access_time.tv_nsec = de->atime_nsec;
			file_info.modification_time.tv_nsec = de->mtime_nsec;
			file_info.status_change_time.tv_nsec = de->ctime_nsec;
#else
			owner.clear();
			group.clear();
#endif
			file_info.mode = de->mode;
			file_info.size = de->size;
			switch (de->type) {
				case NF3REG: file_info.mode|= S_IFREG; break;
				case NF3DIR: file_info.mode|= S_IFDIR; break;
				case NF3BLK: file_info.mode|= S_IFBLK; break;
				case NF3CHR: file_info.mode|= S_IFCHR; break;
				case NF3LNK: file_info.mode|= S_IFLNK; break;
				case NF3SOCK: file_info.mode|= S_IFSOCK; break;
				case NF3FIFO: file_info.mode|= S_IFIFO; break;
			}
			return true;
		}
	}
};

std::shared_ptr<IDirectoryEnumer> ProtocolNFS::DirectoryEnum(const std::string &path) throw (std::runtime_error)
{
	return std::shared_ptr<IDirectoryEnumer>(new NFSDirectoryEnumer(_nfs, RootedPath(path)));
}


class NFSFileIO : public IFileReader, public IFileWriter
{
	std::shared_ptr<NFSConnection> _nfs;
	struct nfsfh *_file = nullptr;

public:
	NFSFileIO(std::shared_ptr<NFSConnection> &nfs, const std::string &path, int flags, mode_t mode, unsigned long long resume_pos)
		: _nfs(nfs)
	{
		int rc = (flags & O_CREAT)
			? nfs_creat(_nfs->ctx, path.c_str(), mode & (~O_CREAT), &_file)
			: nfs_open(_nfs->ctx, path.c_str(), flags, &_file);
		
		if (rc != 0 || _file != nullptr) {
			_file = nullptr;
			throw ProtocolError("Failed to open file",  rc);
		}
		if (resume_pos) {
			uint64_t current_offset = resume_pos;
			int rc = nfs_lseek(_nfs->ctx, _file, resume_pos, SEEK_SET, &current_offset);
			if (rc < 0) {
				nfs_close(_nfs->ctx, _file);
				_file = nullptr;
				throw ProtocolError("Failed to seek file",  rc);
			}
		}
	}

	virtual ~NFSFileIO()
	{
		if (_file != nullptr) {
			nfs_close(_nfs->ctx, _file);
		}
	}

	virtual size_t Read(void *buf, size_t len) throw (std::runtime_error)
	{
		const auto rc = nfs_read(_nfs->ctx, _file, len, (char *)buf);
		if (rc < 0)
			throw ProtocolError("Read file error",  errno);
		// uncomment to simulate connection stuck if ( (rand()%100) == 0) sleep(60);

		return (size_t)rc;
	}

	virtual void Write(const void *buf, size_t len) throw (std::runtime_error)
	{
		if (len > 0) for (;;) {
			const auto rc = nfs_write(_nfs->ctx, _file, len, (char *)buf);
			if (rc <= 0)
				throw ProtocolError("Write file error",  errno);
			if ((size_t)rc >= len)
				break;

			len-= (size_t)rc;
			buf = (const char *)buf + rc;
		}
	}

	virtual void WriteComplete() throw (std::runtime_error)
	{
		// what?
	}
};


std::shared_ptr<IFileReader> ProtocolNFS::FileGet(const std::string &path, unsigned long long resume_pos) throw (std::runtime_error)
{
	return std::make_shared<NFSFileIO>(_nfs, path, O_RDONLY, 0, resume_pos);
}

std::shared_ptr<IFileWriter> ProtocolNFS::FilePut(const std::string &path, mode_t mode, unsigned long long resume_pos) throw (std::runtime_error)
{
	return std::make_shared<NFSFileIO>(_nfs, path, O_WRONLY | O_CREAT | (resume_pos ? 0 : O_TRUNC), mode, resume_pos);
}
