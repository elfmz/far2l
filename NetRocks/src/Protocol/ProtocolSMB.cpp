#include "ProtocolSMB.h"
#include <libsmbclient.h>
#include <StringConfig.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>


std::shared_ptr<IProtocol> CreateProtocolSMB(const std::string &host, unsigned int port,
	const std::string &username, const std::string &password, const std::string &options) throw (std::runtime_error)
{
	return std::make_shared<ProtocolSMB>(host, port, username, password, options);
}

struct SMBConnection
{
	SMBCCTX	*ctx = nullptr;
	std::string path;

	~SMBConnection()
	{
		if (ctx != nullptr) {
			smbc_getFunctionPurgeCachedServers(ctx)(ctx);
    		smbc_free_context(ctx, 1);
		}
	}

private:
	SMBConnection(const SMBConnection &) = delete;
};

////////////////////////////
static std::string smb_username, smb_password, smb_workgroup;

static void ProtocolSMB_AuthFn(const char *server, const char *share, char *wrkgrp,
	int wrkgrplen, char *user, int userlen, char *passwd, int passwdlen)
{
    (void) server;
    (void) share;

    strncpy(wrkgrp, smb_workgroup.c_str(), wrkgrplen - 1);
	wrkgrp[wrkgrplen - 1] = 0;

    strncpy(user, smb_username.c_str(), userlen - 1);
	user[userlen - 1] = 0;

    strncpy(passwd, smb_password.c_str(), passwdlen - 1);
	passwd[passwdlen - 1] = 0;
}


ProtocolSMB::ProtocolSMB(const std::string &host, unsigned int port,
	const std::string &username, const std::string &password, const std::string &options) throw (std::runtime_error)
{
//	_conn->ctx = create_smbctx();
//	if (!_conn->ctx)
//		throw ProtocolError("SMB context create failed");

	smb_username = username;
	smb_password = password;

//	smbc_setFunctionAuthData(_conn->ctx, &ProtocolSMB_AuthFn);
//	smbc_setOptionUseCCache(_conn->ctx, false);
//	smbc_setOptionNoAutoAnonymousLogin(_conn->ctx, !password.empty());

	if (smbc_init(&ProtocolSMB_AuthFn, 0) < 0){
//		smbc_free_context(_conn->ctx,  1);
//		_conn->ctx = nullptr;
		throw ProtocolError("SMB context init failed", errno);
	}

	//TODO: smb_workgroup = ;
	_root = "smb://";
	_root+= host;
	_root+= '/';
}

ProtocolSMB::~ProtocolSMB()
{
}

std::string ProtocolSMB::RootedPath(const std::string &path)
{
	std::string out = _root;
	if (out[out.size() - 1] == '/' && !path.empty() && path[0] == '/') {
		out+= path.substr(1);
	} else {
		out+= path;
	}
	return out;
}

bool ProtocolSMB::IsBroken()
{
	return false;//(!_conn || !_conn->ctx || !_conn->SMB || (ssh_get_status(_conn->ssh) & (SSH_CLOSED|SSH_CLOSED_ERROR)) != 0);
}

mode_t ProtocolSMB::GetMode(const std::string &path, bool follow_symlink) throw (std::runtime_error)
{
	struct stat s = {};
	int rc = smbc_stat(RootedPath(path).c_str(), &s);
	if (rc != 0)
		throw ProtocolError("Get mode error", errno);

	return s.st_mode;
}

unsigned long long ProtocolSMB::GetSize(const std::string &path, bool follow_symlink) throw (std::runtime_error)
{
	struct stat s = {};
	int rc = smbc_stat(RootedPath(path).c_str(), &s);
	if (rc != 0)
		throw ProtocolError("Get size error", errno);

	return s.st_size;
}

static int ProtocolSMB_GetInformationInternal(FileInformation &file_info, const std::string &path)
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

void ProtocolSMB::GetInformation(FileInformation &file_info, const std::string &path, bool follow_symlink) throw (std::runtime_error)
{
	int rc = ProtocolSMB_GetInformationInternal(file_info, RootedPath(path));
	if (rc != 0)
		throw ProtocolError("Get info error", errno);
}

void ProtocolSMB::FileDelete(const std::string &path) throw (std::runtime_error)
{
	int rc = smbc_unlink(RootedPath(path).c_str());
	if (rc != 0)
		throw ProtocolError("Delete file error", errno);
}

void ProtocolSMB::DirectoryDelete(const std::string &path) throw (std::runtime_error)
{
	int rc = smbc_rmdir(RootedPath(path).c_str());
	if (rc != 0)
		throw ProtocolError("Delete directory error", errno);
}

void ProtocolSMB::DirectoryCreate(const std::string &path, mode_t mode) throw (std::runtime_error)
{
	int rc = smbc_mkdir(RootedPath(path).c_str(), mode);
	if (rc != 0)
		throw ProtocolError("Create directory error",  errno);
}

void ProtocolSMB::Rename(const std::string &path_old, const std::string &path_new) throw (std::runtime_error)
{
	int rc = smbc_rename(RootedPath(path_old).c_str(), RootedPath(path_new).c_str());
	if (rc != 0)
		throw ProtocolError("Rename error",  errno);
}


void ProtocolSMB::SetTimes(const std::string &path, const timespec &access_time, const timespec &modification_time) throw (std::runtime_error)
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

void ProtocolSMB::SetMode(const std::string &path, mode_t mode) throw (std::runtime_error)
{
	int rc = smbc_chmod(RootedPath(path).c_str(), mode);
	if (rc != 0)
		throw ProtocolError("Set mode error",  errno);
}

void ProtocolSMB::SymlinkCreate(const std::string &link_path, const std::string &link_target) throw (std::runtime_error)
{
		throw ProtocolUnsupportedError("Symlink creation unsupported");
}

void ProtocolSMB::SymlinkQuery(const std::string &link_path, std::string &link_target) throw (std::runtime_error)
{
		throw ProtocolUnsupportedError("Symlink querying unsupported");
}

class SMBDirectoryEnumer : public IDirectoryEnumer
{
	std::shared_ptr<ProtocolSMB> _protocol;
	std::string _dir_path;
	int _dir = -1;
	char _buf[0x4000], *_entry = nullptr;
	int _remain = 0;

public:
	SMBDirectoryEnumer(std::shared_ptr<ProtocolSMB> protocol, const std::string &path)
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
							ProtocolSMB_GetInformationInternal(file_info, subpath);
							file_info.mode = S_IFDIR;
							return true;

						case SMBC_FILE: case SMBC_LINK:
							ProtocolSMB_GetInformationInternal(file_info, subpath);
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

std::shared_ptr<IDirectoryEnumer> ProtocolSMB::DirectoryEnum(const std::string &path) throw (std::runtime_error)
{
	return std::shared_ptr<IDirectoryEnumer>(new SMBDirectoryEnumer(shared_from_this(), path));
}


class SMBFileIO : public IFileReader, public IFileWriter
{
	std::shared_ptr<ProtocolSMB> _protocol;
	int _file = -1;

public:
	SMBFileIO(std::shared_ptr<ProtocolSMB> protocol, const std::string &path, int flags, mode_t mode, unsigned long long resume_pos)
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

	virtual ~SMBFileIO()
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


std::shared_ptr<IFileReader> ProtocolSMB::FileGet(const std::string &path, unsigned long long resume_pos) throw (std::runtime_error)
{
	return std::make_shared<SMBFileIO>(shared_from_this(), path, O_RDONLY, 0, resume_pos);
}

std::shared_ptr<IFileWriter> ProtocolSMB::FilePut(const std::string &path, mode_t mode, unsigned long long resume_pos) throw (std::runtime_error)
{
	return std::make_shared<SMBFileIO>(shared_from_this(), path, O_WRONLY | O_CREAT | (resume_pos ? 0 : O_TRUNC), mode, resume_pos);
}
