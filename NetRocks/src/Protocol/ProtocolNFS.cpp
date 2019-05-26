#include "ProtocolNFS.h"
#include <libsmbclient.h>
#include <StringConfig.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>


std::shared_ptr<IProtocol> CreateProtocolNFS(const std::string &host, unsigned int port,
	const std::string &username, const std::string &password, const std::string &options) throw (std::runtime_error)
{
	return std::make_shared<ProtocolNFS>(host, port, username, password, options);
}

////////////////////////////

ProtocolNFS::ProtocolNFS(const std::string &host, unsigned int port,
	const std::string &username, const std::string &password, const std::string &options) throw (std::runtime_error)
	:
	_nfs(std::make_shared<NFSConnection>())
{
	_nfs->ctx = nfs_init_context();
	if (!_nfs->ctx) {
		throw ProtocolError("Create context error", errno);
	}

	_nfs->host = host;

	StringConfig protocol_options(options);

	int i = protocol_options.GetInt("UID", -1);
	if (i != -1) {
		nfs_set_uid(_nfs->ctx, i);
	}
	i = protocol_options.GetInt("GID", -1);
	if (i != -1) {
		nfs_set_gid(_nfs->ctx, i);
	}

	if (!username.empty() || !password.empty() ) {
		;//TODO. How to use this??? nfs_set_auth(_nfs->ctx, struct AUTH *auth);
	}

}

ProtocolNFS::~ProtocolNFS()
{
}

std::string ProtocolNFS::InspectPath(const std::string &path)
{
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
}

bool ProtocolNFS::IsBroken()
{
	return false;//(!_conn || !_conn->ctx || !_conn->SMB || (ssh_get_status(_conn->ssh) & (SSH_CLOSED|SSH_CLOSED_ERROR)) != 0);
}

mode_t ProtocolNFS::GetMode(const std::string &path, bool follow_symlink) throw (std::runtime_error)
{
	struct stat s = {};
	int rc = smbc_stat(RootedPath(path).c_str(), &s);
	if (rc != 0)
		throw ProtocolError("Get mode error", errno);

	return s.st_mode;
}

unsigned long long ProtocolNFS::GetSize(const std::string &path, bool follow_symlink) throw (std::runtime_error)
{
	struct stat s = {};
	int rc = smbc_stat(RootedPath(path).c_str(), &s);
	if (rc != 0)
		throw ProtocolError("Get size error", errno);

	return s.st_size;
}

static int ProtocolNFS_GetInformationInternal(FileInformation &file_info, const std::string &path)
{
	struct stat s = {};
	int rc = smbc_stat(path.c_str(), &s);
	if (rc < 0)
		return rc;

	file_info.access_time = s.st_atim;
	file_info.modification_time = s.st_mtim;
	file_info.status_change_time = s.st_ctim;
	file_info.mode = s.st_mode;
	file_info.size = s.st_size;
	return 0;
}

void ProtocolNFS::GetInformation(FileInformation &file_info, const std::string &path, bool follow_symlink) throw (std::runtime_error)
{
	int rc = ProtocolNFS_GetInformationInternal(file_info, RootedPath(path));
	if (rc != 0)
		throw ProtocolError("Get info error", errno);
}

void ProtocolNFS::FileDelete(const std::string &path) throw (std::runtime_error)
{
	int rc = smbc_unlink(RootedPath(path).c_str());
	if (rc != 0)
		throw ProtocolError("Delete file error", errno);
}

void ProtocolNFS::DirectoryDelete(const std::string &path) throw (std::runtime_error)
{
	int rc = smbc_rmdir(RootedPath(path).c_str());
	if (rc != 0)
		throw ProtocolError("Delete directory error", errno);
}

void ProtocolNFS::DirectoryCreate(const std::string &path, mode_t mode) throw (std::runtime_error)
{
	int rc = smbc_mkdir(RootedPath(path).c_str(), mode);
	if (rc != 0)
		throw ProtocolError("Create directory error",  errno);
}

void ProtocolNFS::Rename(const std::string &path_old, const std::string &path_new) throw (std::runtime_error)
{
	int rc = smbc_rename(RootedPath(path_old).c_str(), RootedPath(path_new).c_str());
	if (rc != 0)
		throw ProtocolError("Rename error",  errno);
}


void ProtocolNFS::SetTimes(const std::string &path, const timespec &access_time, const timespec &modification_time) throw (std::runtime_error)
{
	struct timeval times[2] = {};
	times[0].tv_sec = access_time.tv_sec;
	times[0].tv_usec = access_time.tv_nsec / 1000;
	times[1].tv_sec = modification_time.tv_sec;
	times[1].tv_usec = modification_time.tv_nsec / 1000;

	int rc = smbc_utimes(RootedPath(path).c_str(), times);
	if (rc != 0)
		throw ProtocolError("Set times error",  errno);
}

void ProtocolNFS::SetMode(const std::string &path, mode_t mode) throw (std::runtime_error)
{
	int rc = smbc_chmod(RootedPath(path).c_str(), mode);
	if (rc != 0)
		throw ProtocolError("Set mode error",  errno);
}

void ProtocolNFS::SymlinkCreate(const std::string &link_path, const std::string &link_target) throw (std::runtime_error)
{
		throw ProtocolUnsupportedError("Symlink creation unsupported");
}

void ProtocolNFS::SymlinkQuery(const std::string &link_path, std::string &link_target) throw (std::runtime_error)
{
		throw ProtocolUnsupportedError("Symlink querying unsupported");
}

class SMBDirectoryEnumer : public IDirectoryEnumer
{
	std::shared_ptr<ProtocolNFS> _protocol;
	std::string _dir_path;
	int _dir = -1;
	char _buf[0x4000], *_entry = nullptr;
	int _remain = 0;

public:
	SMBDirectoryEnumer(std::shared_ptr<ProtocolNFS> protocol, const std::string &path)
		: _protocol(protocol),
		_dir_path(protocol->RootedPath(path))
	{
		if (_dir_path[_dir_path.size() - 1] == '.' && _dir_path[_dir_path.size() - 2] == '/') {
			_dir_path.resize(_dir_path.size() - 2);
		}
		_dir = smbc_opendir(_dir_path.c_str());
		if (_dir < 0) {
			throw ProtocolError("Directory open error", _dir_path.c_str(), errno);
		}

		if (_dir_path.empty() || _dir_path[_dir_path.size() - 1] != '/') {
			_dir_path+= '/';
		}
	}

	virtual ~SMBDirectoryEnumer()
	{
		if (_dir != -1) {
			smbc_closedir(_dir);
		}
	}

	virtual bool Enum(std::string &name, std::string &owner, std::string &group, FileInformation &file_info) throw (std::runtime_error)
	{
		std::string subpath;
		for (;;) {
			if (_remain > 0 && _entry != nullptr) {
				struct smbc_dirent *de = (struct smbc_dirent *)_entry;
				_remain-= de->dirlen;
				if (_remain > 0) {
					_entry+= de->dirlen;
				}
				if (FILENAME_ENUMERABLE(de->name)) {
					name = de->name;
					subpath = _dir_path;
					subpath+= name;

					file_info.size = 0;

					switch (de->smbc_type) {
						case SMBC_WORKGROUP: case SMBC_SERVER: case SMBC_FILE_SHARE: case SMBC_PRINTER_SHARE: case SMBC_DIR:
							ProtocolNFS_GetInformationInternal(file_info, subpath);
							file_info.mode = S_IFDIR;
							return true;

						case SMBC_FILE: case SMBC_LINK:
							ProtocolNFS_GetInformationInternal(file_info, subpath);
							file_info.mode = S_IFREG;
							return true;
					}
				}
			}


			if (_remain <= 0) {
				_entry = _buf;
				_remain = smbc_getdents(_dir, (struct smbc_dirent *)_buf, sizeof(_buf));
				if (_remain == 0)
					return false;
				if (_remain < 0)
					throw ProtocolError("Directory enum error", errno);
			}
		}
	}

};

std::shared_ptr<IDirectoryEnumer> ProtocolNFS::DirectoryEnum(const std::string &path) throw (std::runtime_error)
{
	return std::shared_ptr<IDirectoryEnumer>(new SMBDirectoryEnumer(shared_from_this(), path));
}


class NFSFileIO : public IFileReader, public IFileWriter
{
	std::shared_ptr<ProtocolNFS> _protocol;
	int _file = -1;

public:
	NFSFileIO(std::shared_ptr<ProtocolNFS> protocol, const std::string &path, int flags, mode_t mode, unsigned long long resume_pos)
		: _protocol(protocol)
	{
		_file = smbc_open(protocol->RootedPath(path).c_str(), flags, mode);
		if (_file == -1)
			throw ProtocolError("Failed to open file",  errno);

		if (resume_pos) {
			off_t rc = smbc_lseek(_file, resume_pos, SEEK_SET);
			if (rc == (off_t)-1) {
				smbc_close(_file);
				_file = -1;
				throw ProtocolError("Failed to seek file",  errno);
			}
		}
	}

	virtual ~NFSFileIO()
	{
		if (_file != -1) {
			smbc_close(_file);
		}
	}

	virtual size_t Read(void *buf, size_t len) throw (std::runtime_error)
	{
		const ssize_t rc = smbc_read(_file, buf, len);
		if (rc < 0)
			throw ProtocolError("Read file error",  errno);
		// uncomment to simulate connection stuck if ( (rand()%100) == 0) sleep(60);

		return (size_t)rc;
	}

	virtual void Write(const void *buf, size_t len) throw (std::runtime_error)
	{
		if (len > 0) for (;;) {
			const ssize_t rc = smbc_write(_file, buf, len);
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
	return std::make_shared<NFSFileIO>(shared_from_this(), path, O_RDONLY, 0, resume_pos);
}

std::shared_ptr<IFileWriter> ProtocolNFS::FilePut(const std::string &path, mode_t mode, unsigned long long resume_pos) throw (std::runtime_error)
{
	return std::make_shared<NFSFileIO>(shared_from_this(), path, O_WRONLY | O_CREAT | (resume_pos ? 0 : O_TRUNC), mode, resume_pos);
}
