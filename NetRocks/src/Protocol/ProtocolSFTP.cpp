#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <utils.h>
#include <vector>
#include <libssh/ssh2.h>
#include <libssh/sftp.h>
#include "ProtocolSFTP.h"


//////////////////////////////////////////////////////////////////////////////////////////////////
#define SIMULATED_GETMODE_FAILS_RATE 0
#define SIMULATED_GETSIZE_FAILS_RATE 0
#define SIMULATED_GETINFO_FAILS_RATE 0
#define SIMULATED_UNLINK_FAILS_RATE 0
#define SIMULATED_RMDIR_FAILS_RATE 0
#define SIMULATED_MKDIR_FAILS_RATE 0
#define SIMULATED_RENAME_FAILS_RATE 0
#define SIMULATED_ENUM_FAILS_RATE 0
#define SIMULATED_OPEN_FAILS_RATE 0
#define SIMULATED_READ_FAILS_RATE 0
#define SIMULATED_WRITE_FAILS_RATE 0
#define SIMULATED_WRITE_COMPLETE_FAILS_RATE 0
///////////////////////////////////////////////////////////////////////////////////////////////////

static void SSHSessionDeleter(ssh_session res)
{
	ssh_disconnect(res);
	ssh_free(res);
}

static void SFTPSessionDeleter(sftp_session res)
{
	sftp_free(res);
}

static void SFTPDirDeleter(sftp_dir res)
{
	sftp_closedir(res);
}

static void SFTPAttributesDeleter(sftp_attributes res)
{
	sftp_attributes_free(res);
}

static void SFTPFileDeleter(sftp_file res)
{
	sftp_close(res);
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

RESOURCE_CONTAINER(SFTPDir, sftp_dir, SFTPDirDeleter);
RESOURCE_CONTAINER(SFTPAttributes, sftp_attributes, SFTPAttributesDeleter);
RESOURCE_CONTAINER(SFTPFile, sftp_file, SFTPFileDeleter);
//RESOURCE_CONTAINER(SSHSession, ssh_session, SSHSessionDeleter);
//RESOURCE_CONTAINER(SFTPSession, sftp_session, SFTPSessionDeleter);

struct SFTPConnection
{
	ssh_session ssh = nullptr;
	sftp_session sftp = nullptr;

	~SFTPConnection()
	{
		if (sftp) {
			SFTPSessionDeleter(sftp);
		}

		if (ssh) {
			SSHSessionDeleter(ssh);
		}
	}
};

////////////////////////////

static std::string GetSSHPubkeyHash(ssh_session ssh)
{
	ssh_key pub_key = {};
	int rc = ssh_get_publickey(ssh, &pub_key);
	if (rc != SSH_OK)
		throw ProtocolError("Public key failed", ssh_get_error(ssh), rc);

	unsigned char *hash = nullptr;
	size_t hlen = 0;
	rc = ssh_get_publickey_hash(pub_key, SSH_PUBLICKEY_HASH_SHA1, &hash, &hlen);
	ssh_key_free(pub_key);

	if (rc != SSH_OK)
		throw ProtocolError("Public key hash failed", ssh_get_error(ssh), rc);

	std::string out;
	for (size_t i = 0; i < hlen; ++i) {
		out+= StrPrintf("%02x", (unsigned int)hash[i]);
	}
	ssh_clean_pubkey_hash(&hash);

	return out;
}

ProtocolSFTP::ProtocolSFTP(const std::string &host, unsigned int port, const std::string &username, const std::string &password,
				const StringConfig &options) throw (std::runtime_error)
	: _conn(new SFTPConnection)
{
	_conn->ssh = ssh_new();
	if (!_conn->ssh)
		throw ProtocolError("SSH session failed");

	ssh_options_set(_conn->ssh, SSH_OPTIONS_HOST, host.c_str());
	if (port > 0)
		ssh_options_set(_conn->ssh, SSH_OPTIONS_PORT, &port);	

	ssh_options_set(_conn->ssh, SSH_OPTIONS_USER, &port);

#if (LIBSSH_VERSION_INT >= SSH_VERSION_INT(0, 8, 0))
	int nodelay = 1;
	ssh_options_set(_conn->ssh, SSH_OPTIONS_NODELAY, &nodelay);
#endif

	int verbosity = SSH_LOG_NOLOG;//SSH_LOG_WARNING;//SSH_LOG_PROTOCOL;
	ssh_options_set(_conn->ssh, SSH_OPTIONS_LOG_VERBOSITY, &verbosity);

	int rc = ssh_connect(_conn->ssh);
	if (rc != SSH_OK)
		throw ProtocolError("Connection failed", ssh_get_error(_conn->ssh), rc);

	const std::string &pub_key_hash = GetSSHPubkeyHash(_conn->ssh);
	if (pub_key_hash != options.GetString("ServerIdentity"))
		throw ServerIdentityMismatchError(pub_key_hash);

	rc = ssh_userauth_password(_conn->ssh, username.empty() ? nullptr : username.c_str(), password.c_str());
  	if (rc != SSH_AUTH_SUCCESS)
		throw ProtocolAuthFailedError();//"Authentification failed", ssh_get_error(_conn->ssh), rc);

	_conn->sftp = sftp_new(_conn->ssh);
	if (_conn->sftp == nullptr)
		throw ProtocolError("SFTP session failed", ssh_get_error(_conn->ssh));

	rc = sftp_init(_conn->sftp);
	if (rc != SSH_OK)
		throw ProtocolError("SFTP initialization failed", ssh_get_error(_conn->ssh), rc);

	//_dir = directory;
}

ProtocolSFTP::~ProtocolSFTP()
{
}

bool ProtocolSFTP::IsBroken()
{
	return (!_conn || !_conn->ssh || !_conn->sftp || (ssh_get_status(_conn->ssh) & (SSH_CLOSED|SSH_CLOSED_ERROR)) != 0);
}

static sftp_attributes SFTPGetAttributes(sftp_session sftp, const std::string &path, bool follow_symlink) throw (std::runtime_error)
{
	sftp_attributes out = follow_symlink ? sftp_stat(sftp, path.c_str()) : sftp_lstat(sftp, path.c_str());
	if (!out)
		throw ProtocolError("Stat error",  sftp_get_error(sftp));

	return out;
}

static mode_t SFTPModeFromAttributes(sftp_attributes attributes)
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

static void SftpFileInfoFromAttributes(FileInformation &file_info, sftp_attributes attributes)
{
	file_info.access_time.tv_sec = attributes->atime64 ? attributes->atime64 : attributes->atime;
	file_info.access_time.tv_nsec = attributes->atime_nseconds;
	file_info.modification_time.tv_sec = attributes->mtime64 ? attributes->mtime64 : attributes->mtime;
	file_info.modification_time.tv_nsec = attributes->mtime_nseconds;
	file_info.status_change_time.tv_sec = attributes->createtime;
	file_info.status_change_time.tv_nsec = attributes->createtime_nseconds;
	file_info.size = attributes->size;
	file_info.mode = SFTPModeFromAttributes(attributes);
}



mode_t ProtocolSFTP::GetMode(const std::string &path, bool follow_symlink) throw (std::runtime_error)
{
#if SIMULATED_GETMODE_FAILS_RATE
	if ( (rand() % 100) + 1 <= SIMULATED_GETMODE_FAILS_RATE)
		throw ProtocolError("Simulated getmode error");
#endif

	SFTPAttributes  attributes(SFTPGetAttributes(_conn->sftp, path, follow_symlink));
	return SFTPModeFromAttributes(attributes);
}

unsigned long long ProtocolSFTP::GetSize(const std::string &path, bool follow_symlink) throw (std::runtime_error)
{
#if SIMULATED_GETSIZE_FAILS_RATE
	if ( (rand() % 100) + 1 <= SIMULATED_GETSIZE_FAILS_RATE)
		throw ProtocolError("Simulated getsize error");
#endif

	SFTPAttributes  attributes(SFTPGetAttributes(_conn->sftp, path, follow_symlink));
	return attributes->size;
}

void ProtocolSFTP::GetInformation(FileInformation &file_info, const std::string &path, bool follow_symlink) throw (std::runtime_error)
{
#if SIMULATED_GETINFO_FAILS_RATE
	if ( (rand() % 100) + 1 <= SIMULATED_GETINFO_FAILS_RATE)
		throw ProtocolError("Simulated getinfo error");
#endif

	SFTPAttributes  attributes(SFTPGetAttributes(_conn->sftp, path, follow_symlink));
	SftpFileInfoFromAttributes(file_info, attributes);
}

void ProtocolSFTP::FileDelete(const std::string &path) throw (std::runtime_error)
{
#if SIMULATED_UNLINK_FAILS_RATE
	if ( (rand() % 100) + 1 <= SIMULATED_UNLINK_FAILS_RATE)
		throw ProtocolError("Simulated unlink error");
#endif

	int rc = sftp_unlink(_conn->sftp, path.c_str());
	if (rc != 0)
		throw ProtocolError("Delete file error",  ssh_get_error(_conn->ssh), rc);
}

void ProtocolSFTP::DirectoryDelete(const std::string &path) throw (std::runtime_error)
{
#if SIMULATED_RMDIR_FAILS_RATE
	if ( (rand() % 100) + 1 <= SIMULATED_RMDIR_FAILS_RATE)
		throw ProtocolError("Simulated rmdir error");
#endif

	int rc = sftp_rmdir(_conn->sftp, path.c_str());
	if (rc != 0)
		throw ProtocolError("Delete directory error",  ssh_get_error(_conn->ssh), rc);
}

void ProtocolSFTP::DirectoryCreate(const std::string &path, mode_t mode) throw (std::runtime_error)
{
#if SIMULATED_MKDIR_FAILS_RATE
	if ( (rand() % 100) + 1 <= SIMULATED_MKDIR_FAILS_RATE)
		throw ProtocolError("Simulated mkdir error");
#endif

	int rc = sftp_mkdir(_conn->sftp, path.c_str(), mode);
	if (rc != 0)
		throw ProtocolError("Create directory error",  ssh_get_error(_conn->ssh), rc);
}

void ProtocolSFTP::Rename(const std::string &path_old, const std::string &path_new) throw (std::runtime_error)
{
#if SIMULATED_RENAME_FAILS_RATE
	if ( (rand() % 100) + 1 <= SIMULATED_RENAME_FAILS_RATE)
		throw ProtocolError("Simulated rename error");
#endif

	int rc = sftp_rename(_conn->sftp, path_old.c_str(), path_new.c_str());
	if (rc != 0)
		throw ProtocolError("Rename error",  ssh_get_error(_conn->ssh), rc);
}

class SFTPDirectoryEnumer : public IDirectoryEnumer
{
	std::shared_ptr<SFTPConnection> _conn;
	SFTPDir _dir;

public:
	SFTPDirectoryEnumer(std::shared_ptr<SFTPConnection> &conn, const std::string &path)
		: _conn(conn), _dir(sftp_opendir(conn->sftp, path.c_str()))
	{
		if (!_dir)
			throw ProtocolError("Directory open error",  ssh_get_error(_conn->ssh));
	}

	virtual bool Enum(std::string &name, std::string &owner, std::string &group, FileInformation &file_info) throw (std::runtime_error)
	{
		for (;;) {
#if SIMULATED_ENUM_FAILS_RATE
		if ( (rand() % 100) + 1 <= SIMULATED_ENUM_FAILS_RATE)
			throw ProtocolError("Simulated enum dir error");
#endif

			SFTPAttributes  attributes(sftp_readdir(_conn->sftp, _dir));
			if (!attributes) {
				if (!sftp_dir_eof(_dir))
					throw ProtocolError("Directory list error",  ssh_get_error(_conn->ssh));
				return false;
			}
			if (attributes->name == nullptr || attributes->name[0] == 0) {
				return false;
			}

			if (FILENAME_ENUMERABLE(attributes->name)) {
				name = attributes->name ? attributes->name : "";
				owner = attributes->owner ? attributes->owner : "";
				group = attributes->group ? attributes->group : "";

				SftpFileInfoFromAttributes(file_info, attributes);
				return true;
			}
		}
  	}
};

std::shared_ptr<IDirectoryEnumer> ProtocolSFTP::DirectoryEnum(const std::string &path) throw (std::runtime_error)
{
	return std::shared_ptr<IDirectoryEnumer>(new SFTPDirectoryEnumer(_conn, path));
}

class SFTPFileIO : IFileReader, IFileWriter
{
	std::shared_ptr<SFTPConnection> _conn;
	SFTPFile _file;

public:
	SFTPFileIO(std::shared_ptr<SFTPConnection> &conn, const std::string &path, int flags, mode_t mode, unsigned long long resume_pos)
		: _conn(conn), _file(sftp_open(conn->sftp, path.c_str(), flags, mode))
	{
#if SIMULATED_OPEN_FAILS_RATE
		if ( (rand() % 100) + 1 <= SIMULATED_OPEN_FAILS_RATE)
			throw ProtocolError("Simulated open file error");
#endif

		if (!_file)
			throw ProtocolError("Failed to open file",  ssh_get_error(_conn->ssh));

		if (resume_pos) {
			int rc = sftp_seek64(_file, resume_pos);
			if (rc != 0)
				throw ProtocolError("Failed to seek file",  ssh_get_error(_conn->ssh), rc);
		}
	}

	virtual size_t Read(void *buf, size_t len) throw (std::runtime_error)
	{
#if SIMULATED_READ_FAILS_RATE
		if ( (rand() % 100) + 1 <= SIMULATED_READ_FAILS_RATE)
			throw ProtocolError("Simulated read file error");
#endif
		const ssize_t rc = sftp_read(_file, buf, len);
		if (rc < 0)
			throw ProtocolError("Read file error",  ssh_get_error(_conn->ssh));
		// uncomment to simulate connection stuck if ( (rand()%100) == 0) sleep(60);

		return (size_t)rc;
	}

	virtual void Write(const void *buf, size_t len) throw (std::runtime_error)
	{
#if SIMULATED_WRITE_FAILS_RATE
		if ( (rand() % 100) + 1 <= SIMULATED_READ_FAILS_RATE)
			throw ProtocolError("Simulated write file error");
#endif
		if (len > 0) for (;;) {
			const ssize_t rc = sftp_write(_file, buf, len);
			if (rc <= 0)
				throw ProtocolError("Write file error",  ssh_get_error(_conn->ssh));
			if ((size_t)rc >= len)
				break;

			len-= (size_t)len;
			buf = (const char *)buf + len;
		}
	}

	virtual void WriteComplete() throw (std::runtime_error)
	{
#if SIMULATED_WRITE_COMPLETE_FAILS_RATE
		if ( (rand() % 100) + 1 <= SIMULATED_READ_FAILS_RATE)
			throw ProtocolError("Simulated write-complete file error");
#endif
		// what?
	}
};


std::shared_ptr<IFileReader> ProtocolSFTP::FileGet(const std::string &path, unsigned long long resume_pos) throw (std::runtime_error)
{
	return std::shared_ptr<IFileReader>((IFileReader *)new SFTPFileIO(_conn, path, O_RDONLY, 0, resume_pos));
}

std::shared_ptr<IFileWriter> ProtocolSFTP::FilePut(const std::string &path, mode_t mode, unsigned long long resume_pos) throw (std::runtime_error)
{
	return std::shared_ptr<IFileWriter>((IFileWriter *)new SFTPFileIO(_conn, path, O_WRONLY | O_CREAT | (resume_pos ? 0 : O_TRUNC), mode, resume_pos));
}
