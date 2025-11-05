#include <sys/stat.h>
#include <fcntl.h>
#include <vector>
#include <string.h>
#include <errno.h>
#include "ProtocolNFS.h"
#include <StringConfig.h>
#include <utils.h>


std::shared_ptr<IProtocol> CreateProtocol(const std::string &protocol, const std::string &host, unsigned int port,
	const std::string &username, const std::string &password, const std::string &options, int fd_ipc_recv)
{
	return std::make_shared<ProtocolNFS>(host, port, username, password, options);
}

////////////////////////////

ProtocolNFS::ProtocolNFS(const std::string &host, unsigned int port,
	const std::string &username, const std::string &password, const std::string &options)
	:
	_nfs(std::make_shared<NFSConnection>()),
	_host(host)
{
	StringConfig protocol_options(options);
	_nfs->ctx = nfs_init_context();
	if (!_nfs->ctx) {
		throw ProtocolError("Create context error", errno);
	}

	if (protocol_options.GetInt("Override", 0) != 0) {
#ifdef LIBNFS_FEATURE_READAHEAD
		const std::string &host = protocol_options.GetString("Host");
		const std::string &groups_str = protocol_options.GetString("Groups");
		uint32_t uid = (uint32_t)protocol_options.GetInt("UID", 65534);
		uint32_t gid = (uint32_t)protocol_options.GetInt("GID", 65534);

		std::vector<uint32_t> groups;
		for (size_t i = 0, j = 0; i <= groups_str.size(); ++i) {
			if (i == groups_str.size() || !isdigit(groups_str[i])) {
				const std::string &grp = groups_str.substr(j, i - j);
				for (size_t k = 0; k < grp.size(); ++k) {
					if (isdigit(grp[k])) {
						groups.emplace_back(atoi(grp.c_str() + k));
						break;
					}
				}
				j = i + 1;
			}
		}

		struct AUTH *auth = libnfs_authunix_create(host.c_str(),
			uid, gid, groups.size(), groups.empty() ? nullptr : &groups[0]);
		if (auth) {
			nfs_set_auth(_nfs->ctx, auth);
			// owned by context: libnfs_auth_destroy(auth);
		}
#else
		fprintf(stderr, "Your libnfs doesnt support credentials override\n");
#endif
	}
}

ProtocolNFS::~ProtocolNFS()
{
}

static int RootedPathLevel(const std::string &rooted_path)
{
	// 0 on "nfs://"
	// 1 on "nfs://server" or "nfs://server/"
	// 2 on "nfs://server/export" or "nfs://server/export/"
	//etc
	if (rooted_path.size() < 6)
		return -1;
	if (rooted_path.size() == 6)
		return 0;

	int out = 1;
	for (size_t i = 6; i < rooted_path.size(); ++i) {
		if (rooted_path[i] == '/' && i + 1 < rooted_path.size() && rooted_path[i + 1] != '/') {
			++out;
		}
	}

	return out;
}

std::string ProtocolNFS::RootedPath(const std::string &path)
{
	std::string out = "nfs://";
	out+= _host;
	if (!_host.empty() && !_mount.empty()) {
		out+= '/';
		out+= _mount;
	}
	if (!path.empty() && path != "." && path[0] != '/' && (!_host.empty() || !_mount.empty())) {
		out+= '/';
	}
	if (path != "." && path != "/.") {
		out+= path;
//		out+= "?auto-traverse-mounts=1";
	}

	for (size_t i = 7; i < out.size();) {
		if (out[i] == '.' && out[i - 1] == '/' && (i + 1 == out.size() || out[i + 1] == '/')) {
			out.erase(i - 1, 2);
			--i;
		} else {
			++i;
		}
	}

	return out;
}

void ProtocolNFS::RootedPathToMounted(std::string &path)
{
	size_t p = path.find('/', 6);
	if (p == std::string::npos)
		throw ProtocolError("Cannot mount path without export", path.c_str());

	if (_nfs->mounted_path.empty() || path.size() < _nfs->mounted_path.size()
	 || memcmp(path.c_str(), _nfs->mounted_path.c_str(), _nfs->mounted_path.size()) != 0
	 || (path.size() > _nfs->mounted_path.size() && path[_nfs->mounted_path.size()] != '/')) {
		std::string server = path.substr(6, p - 6);
		std::string exported = path.substr(p);
		int rc = nfs_mount(_nfs->ctx, server.c_str(), exported.c_str());
		if (rc != 0)
			throw ProtocolError("Mount error", rc);

		_nfs->mounted_path = path;
	}

	path.erase(0, _nfs->mounted_path.size());
	if (path.empty())
		path = "/";
}

std::string ProtocolNFS::MountedRootedPath(const std::string &path)
{
	std::string out = RootedPath(path);
	if (RootedPathLevel(out) <= 1) {
		throw ProtocolError("Path is not mountable");
	}
	RootedPathToMounted(out);
	return out;
}

mode_t ProtocolNFS::GetMode(const std::string &path, bool follow_symlink)
{
	std::string xpath = RootedPath(path);
	if (RootedPathLevel(xpath) <= 1) {
		return S_IFDIR | 0755;
	}
	fprintf(stderr, "ProtocolNFS::GetMode rooted_path='%s'\n", xpath.c_str());
	RootedPathToMounted(xpath);

#ifdef LIBNFS_FEATURE_READAHEAD
	struct nfs_stat_64 s = {};
	int rc = follow_symlink
		? nfs_stat64(_nfs->ctx, xpath.c_str(), &s)
		: nfs_lstat64(_nfs->ctx, xpath.c_str(), &s);
	auto out = s.nfs_mode;
#else
	struct stat s = {};
	int rc = nfs_stat(_nfs->ctx, xpath.c_str(), &s);
	auto out = s.st_mode;
#endif
	fprintf(stderr, "GetMode result %d\n", rc);
	if (rc != 0)
		throw ProtocolError("Get mode error", rc);
	return out;
}

unsigned long long ProtocolNFS::GetSize(const std::string &path, bool follow_symlink)
{
	std::string xpath = RootedPath(path);
	if (RootedPathLevel(xpath) <= 1) {
		return 0;
	}
	fprintf(stderr, "ProtocolNFS::GetSize rooted_path='%s'\n", xpath.c_str());
	RootedPathToMounted(xpath);

#ifdef LIBNFS_FEATURE_READAHEAD
	struct nfs_stat_64 s = {};
	int rc = follow_symlink
		? nfs_stat64(_nfs->ctx, xpath.c_str(), &s)
		: nfs_lstat64(_nfs->ctx, xpath.c_str(), &s);
	auto out = s.nfs_size;
#else
	struct stat s = {};
	int rc = nfs_stat(_nfs->ctx, xpath.c_str(), &s);
	auto out = s.st_size;
#endif
	if (rc != 0)
		throw ProtocolError("Get size error", rc);

	return out;
}

void ProtocolNFS::GetInformation(FileInformation &file_info, const std::string &path, bool follow_symlink)
{
	std::string xpath = RootedPath(path);
	if (RootedPathLevel(xpath) <= 1) {
		file_info = FileInformation();
		file_info.mode = S_IFDIR | 0755;
		return;
	}
	fprintf(stderr, "ProtocolNFS::GetInformation rooted_path='%s'\n", xpath.c_str());
	RootedPathToMounted(xpath);

#ifdef LIBNFS_FEATURE_READAHEAD
	struct nfs_stat_64 s = {};
	int rc = follow_symlink
		? nfs_stat64(_nfs->ctx, xpath.c_str(), &s)
		: nfs_lstat64(_nfs->ctx, xpath.c_str(), &s);
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
	int rc = nfs_stat(_nfs->ctx, xpath.c_str(), &s);
	if (rc != 0)
		throw ProtocolError("Get info error", rc);

	file_info.access_time = s.st_atim;
	file_info.modification_time = s.st_mtim;
	file_info.status_change_time = s.st_ctim;
	file_info.size = s.st_size;
	file_info.mode = s.st_mode;
#endif
}

void ProtocolNFS::FileDelete(const std::string &path)
{
	int rc = nfs_unlink(_nfs->ctx, MountedRootedPath(path).c_str());
	if (rc != 0)
		throw ProtocolError("Delete file error", rc);
}

void ProtocolNFS::DirectoryDelete(const std::string &path)
{
	int rc = nfs_rmdir(_nfs->ctx, MountedRootedPath(path).c_str());
	if (rc != 0)
		throw ProtocolError("Delete directory error", rc);
}

void ProtocolNFS::DirectoryCreate(const std::string &path, mode_t mode)
{
	int rc = nfs_mkdir(_nfs->ctx, MountedRootedPath(path).c_str());//, mode);
	if (rc != 0)
		throw ProtocolError("Create directory error", rc);
}

void ProtocolNFS::Rename(const std::string &path_old, const std::string &path_new)
{
	int rc = nfs_rename(_nfs->ctx, MountedRootedPath(path_old).c_str(), MountedRootedPath(path_new).c_str());
	if (rc != 0)
		throw ProtocolError("Rename error", rc);
}


void ProtocolNFS::SetTimes(const std::string &path, const timespec &access_time, const timespec &modification_time)
{
	struct timeval times[2] = {};
	times[0].tv_sec = access_time.tv_sec;
	times[0].tv_usec = suseconds_t(access_time.tv_nsec / 1000);
	times[1].tv_sec = modification_time.tv_sec;
	times[1].tv_usec = suseconds_t(modification_time.tv_nsec / 1000);

	int rc = nfs_utimes(_nfs->ctx, MountedRootedPath(path).c_str(), times);
	if (rc != 0)
		throw ProtocolError("Set times error", rc);
}

void ProtocolNFS::SetMode(const std::string &path, mode_t mode)
{
	int rc = nfs_chmod(_nfs->ctx, MountedRootedPath(path).c_str(), mode);
	if (rc != 0)
		throw ProtocolError("Set mode error", rc);
}

void ProtocolNFS::SymlinkCreate(const std::string &link_path, const std::string &link_target)
{
	int rc = nfs_symlink(_nfs->ctx, MountedRootedPath(link_target).c_str(), MountedRootedPath(link_path).c_str());
	if (rc != 0)
		throw ProtocolError("Symlink create error", rc);
}

void ProtocolNFS::SymlinkQuery(const std::string &link_path, std::string &link_target)
{
	char buf[0x1001] = {};
	int rc = nfs_readlink(_nfs->ctx, MountedRootedPath(link_path).c_str(), buf, sizeof(buf) - 1);
	if (rc != 0)
		throw ProtocolError("Symlink query error", rc);

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

	virtual bool Enum(std::string &name, std::string &owner, std::string &group, FileInformation &file_info)
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

struct NFSContainersEnumer : IDirectoryEnumer
{
protected:
	std::set<std::string> _names;

	virtual bool Enum(std::string &name, std::string &owner, std::string &group, FileInformation &file_info)
	{
		if (_names.empty()) {
			return false;
		}

		name = *_names.begin();
		owner.clear();
		group.clear();
		file_info = FileInformation();
		file_info.mode|= S_IFDIR | DEFAULT_ACCESS_MODE_DIRECTORY;
		_names.erase(_names.begin());
		return true;
	}
};

struct NFSServersEnumer : NFSContainersEnumer
{
	NFSServersEnumer(std::shared_ptr<NFSConnection> &nfs)
	{
		if (nfs->srv2exports.empty()) {
			struct nfs_server_list *servers = nfs_find_local_servers();
			fprintf(stderr, "NFSServersEnumer: %p\n", servers);
			if (servers != nullptr) {
				std::set<std::string> empty;
				struct nfs_server_list *server = servers;
				do {
					nfs->srv2exports.emplace(server->addr, empty);
					server = server->next;
				} while (server != nullptr);
				free_nfs_srvr_list(servers);
			}
		}

		for (const auto &i : nfs->srv2exports) {
			_names.emplace(i.first);
		}
	}

};

struct NFSExportsEnumer : NFSContainersEnumer
{
	NFSExportsEnumer(std::shared_ptr<NFSConnection> &nfs, const std::string &rooted_path)
	{
		if (rooted_path.size() <= 6) {
			fprintf(stderr, "NFSExportsEnumer('%s'): path too short\n", rooted_path.c_str());
			return;
		}

		std::string server = rooted_path.substr(6);

		while (!server.empty() && server[server.size() - 1] == '/') {
			server.resize(server.size() - 1);
		}
		if (server.empty()) {
			fprintf(stderr, "NFSExportsEnumer('%s'): no server in path\n", rooted_path.c_str());
			return;
		}

		auto &cached_exports = nfs->srv2exports[server];
		if (cached_exports.empty()) {
			struct exportnode *en_list = mount_getexports(server.c_str());
			fprintf(stderr, "NFSExportsEnumer('%s'): %p\n", server.c_str(), en_list);
			if (en_list != nullptr) {
				struct exportnode *en = en_list;
				do {
					cached_exports.emplace((*en->ex_dir == '/') ? en->ex_dir + 1 : en->ex_dir);
					en = en->ex_next;
				} while (en != nullptr);
				mount_free_export_list(en_list);
			}
		}

		_names = cached_exports;
	}
};

std::shared_ptr<IDirectoryEnumer> ProtocolNFS::DirectoryEnum(const std::string &path)
{
	std::string xpath = RootedPath(path);
	fprintf(stderr, "ProtocolNFS::DirectoryEnum: rooted_path='%s'\n", xpath.c_str());
	switch (RootedPathLevel(xpath)) {
		case 0: return std::shared_ptr<IDirectoryEnumer>(new NFSServersEnumer(_nfs));
		case 1: return std::shared_ptr<IDirectoryEnumer>(new NFSExportsEnumer(_nfs, xpath));
		default:
			RootedPathToMounted(xpath);
			return std::shared_ptr<IDirectoryEnumer>(new NFSDirectoryEnumer(_nfs, xpath));
	}
}


class NFSFileIO : public IFileReader, public IFileWriter
{
	std::shared_ptr<NFSConnection> _nfs;
	struct nfsfh *_file = nullptr;

public:
	NFSFileIO(std::shared_ptr<NFSConnection> &nfs, const std::string &path, int flags, mode_t mode, unsigned long long resume_pos)
		: _nfs(nfs)
	{
		fprintf(stderr, "TRying to open path: '%s'\n", path.c_str());
		int rc = (flags & O_CREAT)
			? nfs_creat(_nfs->ctx, path.c_str(), mode & (~O_CREAT), &_file)
			: nfs_open(_nfs->ctx, path.c_str(), flags, &_file);

		if (rc != 0 || _file == nullptr) {
			_file = nullptr;
			throw ProtocolError("Failed to open file", rc);
		}
		if (resume_pos) {
			uint64_t current_offset = resume_pos;
			int rc = nfs_lseek(_nfs->ctx, _file, resume_pos, SEEK_SET, &current_offset);
			if (rc < 0) {
				nfs_close(_nfs->ctx, _file);
				_file = nullptr;
				throw ProtocolError("Failed to seek file", rc);
			}
		}
	}

	virtual ~NFSFileIO()
	{
		if (_file != nullptr) {
			nfs_close(_nfs->ctx, _file);
		}
	}

	virtual size_t Read(void *buf, size_t len)
	{
#ifdef LIBNFS_API_V2
		const auto rc = nfs_read(_nfs->ctx, _file, (char *)buf, len);
#else
		const auto rc = nfs_read(_nfs->ctx, _file, len, (char *)buf);
#endif
		if (rc < 0)
			throw ProtocolError("Read file error", errno);
		// uncomment to simulate connection stuck if ( (rand()%100) == 0) sleep(60);

		return (size_t)rc;
	}

	virtual void Write(const void *buf, size_t len)
	{
		if (len > 0) for (;;) {
#ifdef LIBNFS_API_V2
			const auto rc = nfs_write(_nfs->ctx, _file, (const char *)buf, len);
#else
			const auto rc = nfs_write(_nfs->ctx, _file, len, (const char *)buf);
#endif
			if (rc <= 0)
				throw ProtocolError("Write file error", errno);
			if ((size_t)rc >= len)
				break;

			len-= (size_t)rc;
			buf = (const char *)buf + rc;
		}
	}

	virtual void WriteComplete()
	{
		// what?
	}
};


std::shared_ptr<IFileReader> ProtocolNFS::FileGet(const std::string &path, unsigned long long resume_pos)
{
	return std::make_shared<NFSFileIO>(_nfs, MountedRootedPath(path), O_RDONLY, 0, resume_pos);
}

std::shared_ptr<IFileWriter> ProtocolNFS::FilePut(const std::string &path, mode_t mode, unsigned long long size_hint, unsigned long long resume_pos)
{
	return std::make_shared<NFSFileIO>(_nfs, MountedRootedPath(path), O_WRONLY | O_CREAT | (resume_pos ? 0 : O_TRUNC), mode, resume_pos);
}
