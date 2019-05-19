#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <utils.h>
#include <vector>
#include <algorithm>
#include <libssh/libssh.h>
#include <libssh/ssh2.h>
#include <libssh/sftp.h>
#include <StringConfig.h>
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

std::shared_ptr<IProtocol> CreateProtocolSFTP(const std::string &host, unsigned int port,
	const std::string &username, const std::string &password, const std::string &options) throw (std::runtime_error)
{
	return std::make_shared<ProtocolSFTP>(host, port, username, password, options);
}

// #define MAX_SFTP_IO_BLOCK_SIZE		((size_t)32768)	// googling shows than such block size compatible with at least most of servers

static void SSHSessionDeleter(ssh_session res)
{
	if (res) {
		ssh_disconnect(res);
		ssh_free(res);
	}
}

static void SFTPSessionDeleter(sftp_session res)
{
	if (res)
		sftp_free(res);
}

static void SFTPDirDeleter(sftp_dir res)
{
	if (res)
		sftp_closedir(res);
}

static void SFTPAttributesDeleter(sftp_attributes res)
{
	if (res)
		sftp_attributes_free(res);
}

static void SFTPFileDeleter(sftp_file res)
{
	if (res)
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
	size_t max_io_block = 32768; // default value

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
		throw ProtocolError("Pubkey", ssh_get_error(ssh), rc);

	unsigned char *hash = nullptr;
	size_t hlen = 0;
	rc = ssh_get_publickey_hash(pub_key, SSH_PUBLICKEY_HASH_SHA1, &hash, &hlen);
	ssh_key_free(pub_key);

	if (rc != SSH_OK)
		throw ProtocolError("Pubkey hash", ssh_get_error(ssh), rc);

	std::string out;
	for (size_t i = 0; i < hlen; ++i) {
		out+= StrPrintf("%02x", (unsigned int)hash[i]);
	}
	ssh_clean_pubkey_hash(&hash);

	return out;
}

ProtocolSFTP::ProtocolSFTP(const std::string &host, unsigned int port,
	const std::string &username, const std::string &password, const std::string &options) throw (std::runtime_error)
	: _conn(std::make_shared<SFTPConnection>())
{
	_conn->ssh = ssh_new();
	if (!_conn->ssh)
		throw ProtocolError("SSH session");

	ssh_options_set(_conn->ssh, SSH_OPTIONS_HOST, host.c_str());
	if (port > 0)
		ssh_options_set(_conn->ssh, SSH_OPTIONS_PORT, &port);	

	ssh_options_set(_conn->ssh, SSH_OPTIONS_USER, &port);

	StringConfig protocol_options(options);

	if (protocol_options.GetInt("TcpNoDelay") ) {
#if (LIBSSH_VERSION_INT >= SSH_VERSION_INT(0, 8, 0))
		int nodelay = 1;
		ssh_options_set(_conn->ssh, SSH_OPTIONS_NODELAY, &nodelay);
#else
		fprintf(stderr, "NetRocks::ProtocolSFTP: cannot set SSH_OPTIONS_NODELAY - too old libssh\n");
#endif
	}

	_conn->max_io_block = (size_t)std::max(protocol_options.GetInt("MaxIOBlock", _conn->max_io_block), 512);

	int verbosity = SSH_LOG_NOLOG;//SSH_LOG_WARNING;//SSH_LOG_PROTOCOL;
	ssh_options_set(_conn->ssh, SSH_OPTIONS_LOG_VERBOSITY, &verbosity);

	ssh_key priv_key {};
	std::string key_path;
	if (protocol_options.GetInt("PrivKeyEnable", 0) != 0) {
		key_path = protocol_options.GetString("PrivKeyPath");
		int key_import_result = ssh_pki_import_privkey_file(key_path.c_str(), password.c_str(), nullptr, nullptr, &priv_key);
		if (key_import_result == SSH_ERROR && password.empty()) {
			key_import_result = ssh_pki_import_privkey_file(key_path.c_str(), nullptr, nullptr, nullptr, &priv_key);
		}
		switch (key_import_result) {
			case SSH_EOF:
				throw std::runtime_error(StrPrintf("Cannot read key file \'%s\'", key_path.c_str()));

			case SSH_ERROR:
				throw ProtocolAuthFailedError();

			case SSH_OK:
				break;

			default:
				throw std::runtime_error(StrPrintf("Unexpected error %u while loading key from \'%s\'", key_import_result, key_path.c_str()));
		}
	}

	// TODO: seccomp: if (protocol_options.GetInt("Sandbox") ) ...

	int rc = ssh_connect(_conn->ssh);
	if (rc != SSH_OK)
		throw ProtocolError("Connection", ssh_get_error(_conn->ssh), rc);

	const std::string &pub_key_hash = GetSSHPubkeyHash(_conn->ssh);
	if (pub_key_hash != protocol_options.GetString("ServerIdentity"))
		throw ServerIdentityMismatchError(pub_key_hash);

	if (priv_key) {
		rc = ssh_userauth_publickey(_conn->ssh, username.empty() ? nullptr : username.c_str(), priv_key);
  		if (rc != SSH_AUTH_SUCCESS)
			throw std::runtime_error("Key file authentification failed");

	} else {
		rc = ssh_userauth_password(_conn->ssh, username.empty() ? nullptr : username.c_str(), password.c_str());
  		if (rc != SSH_AUTH_SUCCESS)
			throw ProtocolAuthFailedError();//"Authentification failed", ssh_get_error(_conn->ssh), rc);
	}

	_conn->sftp = sftp_new(_conn->ssh);
	if (_conn->sftp == nullptr)
		throw ProtocolError("SFTP session", ssh_get_error(_conn->ssh));

	rc = sftp_init(_conn->sftp);
	if (rc != SSH_OK)
		throw ProtocolError("SFTP init", ssh_get_error(_conn->ssh), rc);

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
		throw ProtocolError("stat", sftp_get_error(sftp));

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
		throw ProtocolError(ssh_get_error(_conn->ssh), rc);
}

void ProtocolSFTP::DirectoryDelete(const std::string &path) throw (std::runtime_error)
{
#if SIMULATED_RMDIR_FAILS_RATE
	if ( (rand() % 100) + 1 <= SIMULATED_RMDIR_FAILS_RATE)
		throw ProtocolError("Simulated rmdir error");
#endif

	int rc = sftp_rmdir(_conn->sftp, path.c_str());
	if (rc != 0)
		throw ProtocolError(ssh_get_error(_conn->ssh), rc);
}

void ProtocolSFTP::DirectoryCreate(const std::string &path, mode_t mode) throw (std::runtime_error)
{
#if SIMULATED_MKDIR_FAILS_RATE
	if ( (rand() % 100) + 1 <= SIMULATED_MKDIR_FAILS_RATE)
		throw ProtocolError("Simulated mkdir error");
#endif

	int rc = sftp_mkdir(_conn->sftp, path.c_str(), mode);
	if (rc != 0)
		throw ProtocolError(ssh_get_error(_conn->ssh), rc);
}

void ProtocolSFTP::Rename(const std::string &path_old, const std::string &path_new) throw (std::runtime_error)
{
#if SIMULATED_RENAME_FAILS_RATE
	if ( (rand() % 100) + 1 <= SIMULATED_RENAME_FAILS_RATE)
		throw ProtocolError("Simulated rename error");
#endif

	int rc = sftp_rename(_conn->sftp, path_old.c_str(), path_new.c_str());
	if (rc != 0)
		throw ProtocolError(ssh_get_error(_conn->ssh), rc);
}

void ProtocolSFTP::SetTimes(const std::string &path, const timespec &access_time, const timespec &modification_time) throw (std::runtime_error)
{
	struct timeval times[2] = {};
	times[0].tv_sec = access_time.tv_sec;
	times[0].tv_usec = access_time.tv_nsec / 1000;
	times[1].tv_sec = modification_time.tv_sec;
	times[1].tv_usec = modification_time.tv_nsec / 1000;

	int rc = sftp_utimes(_conn->sftp, path.c_str(), times);
	if (rc != 0)
		throw ProtocolError(ssh_get_error(_conn->ssh), rc);
}

void ProtocolSFTP::SetMode(const std::string &path, mode_t mode) throw (std::runtime_error)
{
	int rc = sftp_chmod(_conn->sftp, path.c_str(), mode);
	if (rc != 0)
		throw ProtocolError(ssh_get_error(_conn->ssh), rc);
}


void ProtocolSFTP::SymlinkCreate(const std::string &link_path, const std::string &link_target) throw (std::runtime_error)
{
	int rc = sftp_symlink(_conn->sftp, link_target.c_str(), link_path.c_str());
	if (rc != 0)
		throw ProtocolError(ssh_get_error(_conn->ssh), rc);
}

void ProtocolSFTP::SymlinkQuery(const std::string &link_path, std::string &link_target) throw (std::runtime_error)
{
	char *target = sftp_readlink(_conn->sftp, link_path.c_str());
	if (target == NULL)
		throw ProtocolError(ssh_get_error(_conn->ssh));

	link_target = target;
	ssh_string_free_char(target);
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
			throw ProtocolError(ssh_get_error(_conn->ssh));
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
					throw ProtocolError(ssh_get_error(_conn->ssh));
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
	return std::make_shared<SFTPDirectoryEnumer>(_conn, path);
}

class SFTPFileIO : public IFileReader, public IFileWriter
{
	std::shared_ptr<SFTPConnection> _conn;
	SFTPFile _file;
	bool _sync_fallback = false;

public:
	SFTPFileIO(std::shared_ptr<SFTPConnection> &conn, const std::string &path, int flags, mode_t mode, unsigned long long resume_pos)
		: _conn(conn), _file(sftp_open(conn->sftp, path.c_str(), flags, mode))
	{
#if SIMULATED_OPEN_FAILS_RATE
		if ( (rand() % 100) + 1 <= SIMULATED_OPEN_FAILS_RATE)
			throw ProtocolError("Simulated open file error");
#endif

		if (!_file)
			throw ProtocolError("open",  ssh_get_error(_conn->ssh));

		if (resume_pos) {
			int rc = sftp_seek64(_file, resume_pos);
			if (rc != 0)
				throw ProtocolError("seek",  ssh_get_error(_conn->ssh), rc);
		}
	}

	template <class BUF_T, class SYNC_IO_T>
		size_t SyncIO(BUF_T buf, size_t len, SYNC_IO_T p_sync_io) throw (std::runtime_error)
	{
			const ssize_t rc = p_sync_io(_file, buf, len);
			if (rc < 0)
				throw ProtocolError("io",  ssh_get_error(_conn->ssh));

			return (size_t)rc;
	}

	template <class BUF_T, class SYNC_IO_T, class ASYNC_IO_BEGIN_T, class ASYNC_IO_WAIT_T>
		size_t AsyncIO(BUF_T buf, size_t len, SYNC_IO_T p_sync_io, ASYNC_IO_BEGIN_T p_async_io_begin, ASYNC_IO_WAIT_T p_async_io_wait)
	{
		if (len <= _conn->max_io_block || _sync_fallback) {
			return SyncIO(buf, std::min(len, _conn->max_io_block), p_sync_io);
		}

		size_t pipeline_count = len / _conn->max_io_block;
		if (pipeline_count * _conn->max_io_block < len) {
			++pipeline_count;
		}

		int *pipeline = (int *)alloca(pipeline_count * sizeof(int));
		for (size_t i = 0; i != pipeline_count; ++i) {
			const size_t pipeline_piece = std::min(len - i * _conn->max_io_block, _conn->max_io_block);
			pipeline[i] = p_async_io_begin(_file, (uint32_t)pipeline_piece);
			if (pipeline[i] < 0) {
				if (i == 0) {
					size_t r = SyncIO(buf, std::min(len, _conn->max_io_block), p_sync_io);
					_sync_fallback = true;
					fprintf(stderr, "SFTPFileIO::IO(...0x%lx): sync fallback1\n", (unsigned long)len);
					return r;
				}
				pipeline_count = i;
				break;
			}
		}

		size_t done_length = 0;
		int first_failed = 0;

		for (size_t i = 0; i != pipeline_count; ++i) {
			const size_t pipeline_piece = std::min(len - i * _conn->max_io_block, _conn->max_io_block);
			int done_piece = p_async_io_wait(_file, (char *)buf + done_length, pipeline_piece, pipeline[i]);
			if (done_piece < 0) {
				if (i == 0) {
					first_failed = 1;
				}
			} else if (done_piece == 0) {
				;
			} else {
				done_length+= (size_t)(unsigned int)done_piece;
			}
		}

		if (done_length == 0 && first_failed) {
			done_length = SyncIO(buf, std::min(len, _conn->max_io_block), p_sync_io);
			_sync_fallback = true;
			fprintf(stderr, "SFTPFileIO::IO(...0x%lx): sync fallback2\n", (unsigned long)len);
		}

		return done_length;
	}


	virtual size_t Read(void *buf, size_t len) throw (std::runtime_error)
	{
#if SIMULATED_READ_FAILS_RATE
		if ( (rand() % 100) + 1 <= SIMULATED_READ_FAILS_RATE)
			throw ProtocolError("Simulated read file error");
#endif

		return AsyncIO(buf, len, sftp_read, sftp_async_read_begin, sftp_async_read);
	}

	virtual void Write(const void *buf, size_t len) throw (std::runtime_error)
	{
#if SIMULATED_WRITE_FAILS_RATE
		if ( (rand() % 100) + 1 <= SIMULATED_READ_FAILS_RATE)
			throw ProtocolError("Simulated write file error");
#endif
		// somewhy libssh doesnt have async write yet
		if (len > 0) for (;;) {
			size_t piece = (len >= _conn->max_io_block) ? _conn->max_io_block : len;

			piece = SyncIO(buf, piece, sftp_write);

			if (piece == 0)
				throw ProtocolError("zero write",  ssh_get_error(_conn->ssh));

			if (piece >= len)
				break;

			len-= (size_t)piece;
			buf = (const char *)buf + piece;
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
	return std::make_shared<SFTPFileIO>(_conn, path, O_RDONLY, 0, resume_pos);
}

std::shared_ptr<IFileWriter> ProtocolSFTP::FilePut(const std::string &path, mode_t mode, unsigned long long resume_pos) throw (std::runtime_error)
{
	return std::make_shared<SFTPFileIO>(_conn, path, O_WRONLY | O_CREAT | (resume_pos ? 0 : O_TRUNC), mode, resume_pos);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
