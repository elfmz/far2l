#include "ProtocolSMB.h"
#include <libssh/ssh2.h>
#include <libssh/SMB.h>
#include <sys/stat.h>
#include <fcntl.h>


static void SMBSessionDeleter(ssh_session res)
{
	ssh_disconnect(res);
	ssh_free(res);
}

static void SMBSessionDeleter(SMB_session res)
{
	SMB_free(res);
}

static void SMBDirDeleter(SMB_dir res)
{
	SMB_closedir(res);
}

static void SMBAttributesDeleter(SMB_attributes res)
{
	SMB_attributes_free(res);
}

static void SMBFileDeleter(SMB_file res)
{
	SMB_close(res);
}


#define RESOURCE_CONTAINER(CONTAINER, RESOURCE, DELETER) 		\
class CONTAINER {						\
	RESOURCE _res = {};							\
	CONTAINER(const CONTAINER &) = delete;	\
public:									\
	operator RESOURCE() { return _res; }				\
	RESOURCE operator ->() { return _res; }				\
	CONTAINER(RESOURCE res) : _res(res) {}			\
	~CONTAINER() { DELETER(_res); }				\
};

RESOURCE_CONTAINER(SMBDir, SMB_dir, SMBDirDeleter);
RESOURCE_CONTAINER(SMBAttributes, SMB_attributes, SMBAttributesDeleter);
RESOURCE_CONTAINER(SMBFile, SMB_file, SMBFileDeleter);
//RESOURCE_CONTAINER(SSHSession, ssh_session, SSHSessionDeleter);
//RESOURCE_CONTAINER(SMBSession, SMB_session, SMBSessionDeleter);

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


ProtocolSMB::ProtocolSMB(const std::string &host, unsigned int port, const std::string &username, const std::string &password,
				const StringConfig &protocol_options) throw (ProtocolError)
	: _conn(new SMBConnection)
{
	_conn->ctx = create_smbctx();
	if (!_conn->ctx)
		throw ProtocolError("SMB context create failed");

	smb_username = username;
	smb_password = password;

	smbc_setFunctionAuthData(_conn->ctx, &ProtocolSMB_AuthFn);
	smbc_setOptionUseCCache(_conn->ctx, false);
	smbc_setOptionNoAutoAnonymousLogin(_conn->ctx, !password.empty());

	if (!smbc_init_context(_conn->ctx)){
		smbc_free_context(_conn->ctx,  1);
		_conn->ctx = nullptr;
		throw ProtocolError("SMB context init failed");
	}

	//TODO: smb_workgroup = ;
	_conn->path = "smb://";
	_conn->path+= host;
	if (directory.empty() || directory[0] != '/') _conn->path+= '/';
	_conn->path+= directory;
}

ProtocolSMB::~ProtocolSMB()
{
}

bool ProtocolSMB::IsBroken() const
{
	return (!_conn || !_conn->ctx || !_conn->SMB || (ssh_get_status(_conn->ssh) & (SSH_CLOSED|SSH_CLOSED_ERROR)) != 0);
}

static SMB_attributes SMBGetAttributes(SMB_session SMB, const std::string &path, bool follow_symlink) throw (ProtocolError)
{
	SMB_attributes out = follow_symlink ? SMB_stat(SMB, path.c_str()) : SMB_lstat(SMB, path.c_str());
	if (!out)
		throw ProtocolError("Stat error",  SMB_get_error(SMB));

	return out;
}

static mode_t SMBModeFromAttributes(SMB_attributes attributes)
{
	mode_t out = attributes->permissions;
	switch (attributes->type) {
		case SSH_FILEXFER_TYPE_REGULAR: out|= S_IFREG; break;
		case SSH_FILEXFER_TYPE_DIRECTORY: out|= S_IFDIR; break;
		case SSH_FILEXFER_TYPE_SYMLINK: out|= S_IFLNK; break;
		case SSH_FILEXFER_TYPE_SPECIAL: out|= S_IFBLK; break;
		case SSH_FILEXFER_TYPE_UNKNOWN:
		default:
			break;
	}
	return out;
}

static void SMBFileInfoFromAttributes(FileInformation &file_info, SMB_attributes attributes)
{
	file_info.access_time.tv_sec = attributes->atime64 ? attributes->atime64 : attributes->atime;
	file_info.access_time.tv_nsec = attributes->atime_nseconds;
	file_info.modification_time.tv_sec = attributes->mtime64 ? attributes->mtime64 : attributes->mtime;
	file_info.modification_time.tv_nsec = attributes->mtime_nseconds;
	file_info.status_change_time.tv_sec = attributes->createtime;
	file_info.status_change_time.tv_nsec = attributes->createtime_nseconds;
	file_info.size = attributes->size;
	file_info.mode = SMBModeFromAttributes(attributes);
}



mode_t ProtocolSMB::GetMode(const std::string &path, bool follow_symlink) throw (ProtocolError)
{
	SMBAttributes  attributes(SMBGetAttributes(_conn->SMB, path, follow_symlink));
	return SMBModeFromAttributes(attributes);
}

unsigned long long ProtocolSMB::GetSize(const std::string &path, bool follow_symlink) throw (ProtocolError)
{
	SMBAttributes  attributes(SMBGetAttributes(_conn->SMB, path, follow_symlink));
	return attributes->size;
}

void ProtocolSMB::FileDelete(const std::string &path) throw (ProtocolError)
{
	int rc = SMB_unlink(_conn->SMB, path.c_str());
	if (rc != 0)
		throw ProtocolError("Delete file error",  ssh_get_error(_conn->ssh), rc);
}

void ProtocolSMB::DirectoryDelete(const std::string &path) throw (ProtocolError)
{
	int rc = SMB_rmdir(_conn->SMB, path.c_str());
	if (rc != 0)
		throw ProtocolError("Delete directory error",  ssh_get_error(_conn->ssh), rc);
}

void ProtocolSMB::DirectoryCreate(const std::string &path, mode_t mode) throw (ProtocolError)
{
	int rc = SMB_mkdir(_conn->SMB, path.c_str(), mode);
	if (rc != 0)
		throw ProtocolError("Create directory error",  ssh_get_error(_conn->ssh), rc);
}

void ProtocolSMB::Rename(const std::string &path_old, const std::string &path_new) throw (ProtocolError)
{
	int rc = SMB_rename(_conn->SMB, path_old.c_str(), path_new.c_str());
	if (rc != 0)
		throw ProtocolError("Rename error",  ssh_get_error(_conn->ssh), rc);
}

class SMBDirectoryEnumer : public IDirectoryEnumer
{
	std::shared_ptr<SMBConnection> _conn;
	SMBCFILE *_dir = nullptr;
	std::string _dir_path;

public:
	SMBDirectoryEnumer(std::shared_ptr<SMBConnection> &conn, const std::string &path)
		: _conn(conn)
	{
		_dir = smbc_getFunctionOpendir(_conn->ctx)(_conn->ctx, _conn->path.c_str());
		if (!_dir)
	        throw ProtocolError("Directory open error");

		_dir_path = path;
		if (_dir_path.empty() || _dir_path[_dir_path.size() - 1] != '/')
			_dir_path+= '/';
	}

	virtual ~SMBDirectoryEnumer()
	{
		if (_dir) {
			smbc_getFunctionClose(_conn->ctx)(_conn->ctx, _dir);
		}
	}

	virtual bool Enum(std::string &name, std::string &owner, std::string &group, FileInformation &file_info) throw (ProtocolError)
	{
		std::string subpath;
		for (;;) {
			struct smbc_dirent *dirent = smbc_getFunctionReaddir(_conn->ctx)(_conn->ctx, _dir);
			if (!dirent)
				return false;

			if (dirent->smbc_type == SMBC_PRINTER_SHARE || dirent->smbc_type == SMBC_COMMS_SHARE || dirent->smbc_type == SMBC_IPC_SHARE)
				continue;

			if (dirent->name[0] == 0 || strcmp(dirent->name, ".") == 0 || strcmp(dirent->name, "..") == 0)
				continue;

			name = dirent->name;
			owner.clear();
			group.clear();

			file_info = FileInformation();

			if (dirent->smbc_type == SMBC_WORKGROUP || dirent->smbc_type == SMBC_SERVER || dirent->smbc_type == SMBC_FILE_SHARE) {
				file_info.mode|= S_IFDIR;
				return true;
			}

			subpath = _conn->_dir_path;
			subpath+= name;
			struct stat s = {};
			if (smbc_stat(subpath.c_str(), &s) != 0) {
				if (dirent->smbc_type == SMBC_DIR) {
					file_info.mode|= S_IFDIR;
				}
				return true;
			}

			file_info.access_time = s.st_;
			file_info.modification_time;
			file_info.status_change_time;

			file_info.size = s.st_size;
			file_info.mode = s.st_mode;

			
		}
  	}
};

std::shared_ptr<IDirectoryEnumer> ProtocolSMB::DirectoryEnum(const std::string &path) throw (ProtocolError)
{
	return std::shared_ptr<IDirectoryEnumer>(new SMBDirectoryEnumer(_conn, path));
}
/*
class SMBFileIO : IFileReader, IFileWriter
{
	std::shared_ptr<SMBConnection> _conn;
	SMBFile _file;

public:
	SMBFileIO(std::shared_ptr<SMBConnection> &conn, const std::string &path, int flags, mode_t mode, unsigned long long resume_pos)
		: _conn(conn), _file(SMB_open(conn->SMB, path.c_str(), flags, mode))
	{
		if (!_file)
			throw ProtocolError("Failed to open file",  ssh_get_error(_conn->ssh));

		if (resume_pos) {
			int rc = SMB_seek64(_file, resume_pos);
			if (rc != 0)
				throw ProtocolError("Failed to seek file",  ssh_get_error(_conn->ssh), rc);
		}
	}

	virtual size_t Read(void *buf, size_t len) throw (ProtocolError)
	{
		const ssize_t rc = SMB_read(_file, buf, len);
		if (rc < 0)
			throw ProtocolError("Read file error",  ssh_get_error(_conn->ssh));
		// uncomment to simulate connection stuck if ( (rand()%100) == 0) sleep(60);

		return (size_t)rc;
	}

	virtual void Write(const void *buf, size_t len) throw (ProtocolError)
	{
		if (len > 0) for (;;) {
			const ssize_t rc = SMB_write(_file, buf, len);
			if (rc <= 0)
				throw ProtocolError("Write file error",  ssh_get_error(_conn->ssh));
			if ((size_t)rc >= len)
				break;

			len-= (size_t)len;
			buf = (const char *)buf + len;
		}
	}
};
*/

std::shared_ptr<IFileReader> ProtocolSMB::FileGet(const std::string &path, unsigned long long resume_pos) throw (ProtocolError)
{
	throw ProtocolError("Not implemented");
	// return std::shared_ptr<IFileReader>((IFileReader *)new SMBFileIO(_conn, path, O_RDONLY, 0, resume_pos));
}

std::shared_ptr<IFileWriter> ProtocolSMB::FilePut(const std::string &path, mode_t mode, unsigned long long resume_pos) throw (ProtocolError)
{
	throw ProtocolError("Not implemented");
	// return std::shared_ptr<IFileWriter>((IFileWriter *)new SMBFileIO(_conn, path, O_WRONLY | O_CREAT | (resume_pos ? 0 : O_TRUNC), mode, resume_pos));
}
