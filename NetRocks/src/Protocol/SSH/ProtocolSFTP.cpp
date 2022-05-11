#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/tcp.h>
#include <fcntl.h>
#include <signal.h>
#include <assert.h>
#include <string.h>
#include <utils.h>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <deque>
#include <algorithm>
#include <libssh/libssh.h>
#include <libssh/ssh2.h>
#include <libssh/sftp.h>
#include <StringConfig.h>
#include <ScopeHelpers.h>
#include <Threaded.h>
#include "ProtocolSFTP.h"
#include "SSHConnection.h"


std::shared_ptr<IProtocol> CreateProtocolSCP(const std::string &host, unsigned int port,
	const std::string &username, const std::string &password, const std::string &options);

std::shared_ptr<IProtocol> CreateProtocol(const std::string &protocol, const std::string &host, unsigned int port,
	const std::string &username, const std::string &password, const std::string &options)
{
	if (strcasecmp(protocol.c_str(), "scp") == 0) {
		return CreateProtocolSCP(host, port, username, password, options);
	}

	return std::make_shared<ProtocolSFTP>(host, port, username, password, options);
}

// #define MAX_SFTP_IO_BLOCK_SIZE		((size_t)32768)	// googling shows than such block size compatible with at least most of servers

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


RESOURCE_CONTAINER(SFTPDir, sftp_dir, SFTPDirDeleter);
RESOURCE_CONTAINER(SFTPAttributes, sftp_attributes, SFTPAttributesDeleter);
RESOURCE_CONTAINER(SFTPFile, sftp_file, SFTPFileDeleter);
RESOURCE_CONTAINER(SFTPSession, sftp_session, SFTPSessionDeleter);

class ExecutedCommand;


struct SFTPConnection : SSHConnection
{
	SFTPSession sftp;
	size_t max_read_block = 32768; // default value
	size_t max_write_block = 32768; // default value

	SFTPConnection(const std::string &host, unsigned int port, const std::string &username,
		const std::string &password, const StringConfig &protocol_options)
	:
		SSHConnection(host, port, username, password, protocol_options)
	{
		max_read_block = (size_t)std::max(protocol_options.GetInt("MaxReadBlock", max_read_block), 512);
		max_write_block = (size_t)std::max(protocol_options.GetInt("MaxWriteBlock", max_write_block), 512);

		const std::string &subsystem = protocol_options.GetString("CustomSubsystem");
		if (!subsystem.empty() && protocol_options.GetInt("UseCustomSubsystem", 0) != 0) {
			ssh_channel channel = ssh_channel_new(ssh);
			if (channel == nullptr)
				throw ProtocolError("SSH channel new", ssh_get_error(ssh));

			int rc = ssh_channel_open_session(channel);
			if (rc != SSH_OK) {
				ssh_channel_free(channel);
				throw ProtocolError("SFTP channel open", ssh_get_error(ssh));
			}
			if (subsystem.find('/') == std::string::npos) {
				rc = ssh_channel_request_subsystem(channel, subsystem.c_str());
			} else {
				rc = ssh_channel_request_exec(channel, subsystem.c_str());
			}

			if (rc != SSH_OK) {
				ssh_channel_free(channel);
				throw ProtocolError("SFTP custom subsystem", ssh_get_error(ssh));
			}

			sftp = sftp_new_channel(ssh, channel);
			if (!sftp) {
				ssh_channel_free(channel);
				throw ProtocolError("SFTP channel", ssh_get_error(ssh));
			}

#if (LIBSSH_VERSION_INT >= SSH_VERSION_INT(0, 8, 3))
			if (!sftp->read_packet) {
				sftp->read_packet = (struct sftp_packet_struct *)calloc(1, sizeof(struct sftp_packet_struct));
				sftp->read_packet->payload = ssh_buffer_new();
			}
#endif
		} else {
			sftp = sftp_new(ssh);
		}

		if (sftp == nullptr)
			throw ProtocolError("SFTP session", ssh_get_error(ssh));

		int rc = sftp_init(sftp);
		if (rc != SSH_OK)
			throw ProtocolError("SFTP init", ssh_get_error(ssh), rc);

		//_dir = directory;
	}

	virtual	~SFTPConnection()
	{
	}
};

struct SFTPFileNonblockinScope
{
	SFTPFile &_file;

	SFTPFileNonblockinScope(SFTPFile &file) : _file(file)
	{
		sftp_file_set_nonblocking(_file);
	}

	~SFTPFileNonblockinScope()
	{
		sftp_file_set_blocking(_file);
	}
};

////////////////////////////

ProtocolSFTP::ProtocolSFTP(const std::string &host, unsigned int port,
	const std::string &username, const std::string &password, const std::string &options)
{
	StringConfig protocol_options(options);
	_conn = std::make_shared<SFTPConnection>(host, port, username, password, protocol_options);
}

ProtocolSFTP::~ProtocolSFTP()
{
	_conn->executed_command.reset();
}

static sftp_attributes SFTPGetAttributes(sftp_session sftp, const std::string &path, bool follow_symlink)
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



mode_t ProtocolSFTP::GetMode(const std::string &path, bool follow_symlink)
{
#if SIMULATED_GETMODE_FAILS_RATE
	if ( (rand() % 100) + 1 <= SIMULATED_GETMODE_FAILS_RATE)
		throw ProtocolError("Simulated getmode error");
#endif

	_conn->executed_command.reset();

	SFTPAttributes  attributes(SFTPGetAttributes(_conn->sftp, path, follow_symlink));
	return SFTPModeFromAttributes(attributes);
}

unsigned long long ProtocolSFTP::GetSize(const std::string &path, bool follow_symlink)
{
#if SIMULATED_GETSIZE_FAILS_RATE
	if ( (rand() % 100) + 1 <= SIMULATED_GETSIZE_FAILS_RATE)
		throw ProtocolError("Simulated getsize error");
#endif

	_conn->executed_command.reset();

	SFTPAttributes  attributes(SFTPGetAttributes(_conn->sftp, path, follow_symlink));
	return attributes->size;
}

void ProtocolSFTP::GetInformation(FileInformation &file_info, const std::string &path, bool follow_symlink)
{
#if SIMULATED_GETINFO_FAILS_RATE
	if ( (rand() % 100) + 1 <= SIMULATED_GETINFO_FAILS_RATE)
		throw ProtocolError("Simulated getinfo error");
#endif

	_conn->executed_command.reset();

	SFTPAttributes  attributes(SFTPGetAttributes(_conn->sftp, path, follow_symlink));
	SftpFileInfoFromAttributes(file_info, attributes);
}

void ProtocolSFTP::FileDelete(const std::string &path)
{
#if SIMULATED_UNLINK_FAILS_RATE
	if ( (rand() % 100) + 1 <= SIMULATED_UNLINK_FAILS_RATE)
		throw ProtocolError("Simulated unlink error");
#endif

	_conn->executed_command.reset();

	int rc = sftp_unlink(_conn->sftp, path.c_str());
	if (rc != 0)
		throw ProtocolError(ssh_get_error(_conn->ssh), rc);
}

void ProtocolSFTP::DirectoryDelete(const std::string &path)
{
#if SIMULATED_RMDIR_FAILS_RATE
	if ( (rand() % 100) + 1 <= SIMULATED_RMDIR_FAILS_RATE)
		throw ProtocolError("Simulated rmdir error");
#endif

	_conn->executed_command.reset();

	int rc = sftp_rmdir(_conn->sftp, path.c_str());
	if (rc != 0)
		throw ProtocolError(ssh_get_error(_conn->ssh), rc);
}

void ProtocolSFTP::DirectoryCreate(const std::string &path, mode_t mode)
{
#if SIMULATED_MKDIR_FAILS_RATE
	if ( (rand() % 100) + 1 <= SIMULATED_MKDIR_FAILS_RATE)
		throw ProtocolError("Simulated mkdir error");
#endif

	_conn->executed_command.reset();

	int rc = sftp_mkdir(_conn->sftp, path.c_str(), mode);
	if (rc != 0)
		throw ProtocolError(ssh_get_error(_conn->ssh), rc);
}

void ProtocolSFTP::Rename(const std::string &path_old, const std::string &path_new)
{
#if SIMULATED_RENAME_FAILS_RATE
	if ( (rand() % 100) + 1 <= SIMULATED_RENAME_FAILS_RATE)
		throw ProtocolError("Simulated rename error");
#endif

	_conn->executed_command.reset();

	int rc = sftp_rename(_conn->sftp, path_old.c_str(), path_new.c_str());
	if (rc != 0)
		throw ProtocolError(ssh_get_error(_conn->ssh), rc);
}

void ProtocolSFTP::SetTimes(const std::string &path, const timespec &access_time, const timespec &modification_time)
{
	_conn->executed_command.reset();

	struct timeval times[2] = {};
	times[0].tv_sec = access_time.tv_sec;
	times[0].tv_usec = suseconds_t(access_time.tv_nsec / 1000);
	times[1].tv_sec = modification_time.tv_sec;
	times[1].tv_usec = suseconds_t(modification_time.tv_nsec / 1000);

	int rc = sftp_utimes(_conn->sftp, path.c_str(), times);
	if (rc != 0)
		throw ProtocolError(ssh_get_error(_conn->ssh), rc);
}

void ProtocolSFTP::SetMode(const std::string &path, mode_t mode)
{
	_conn->executed_command.reset();

	int rc = sftp_chmod(_conn->sftp, path.c_str(), mode);
	if (rc != 0)
		throw ProtocolError(ssh_get_error(_conn->ssh), rc);
}


void ProtocolSFTP::SymlinkCreate(const std::string &link_path, const std::string &link_target)
{
	_conn->executed_command.reset();

	int rc = sftp_symlink(_conn->sftp, link_target.c_str(), link_path.c_str());
	if (rc != 0)
		throw ProtocolError(ssh_get_error(_conn->ssh), rc);
}

void ProtocolSFTP::SymlinkQuery(const std::string &link_path, std::string &link_target)
{
	_conn->executed_command.reset();

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

	virtual bool Enum(std::string &name, std::string &owner, std::string &group, FileInformation &file_info)
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

std::shared_ptr<IDirectoryEnumer> ProtocolSFTP::DirectoryEnum(const std::string &path)
{
	_conn->executed_command.reset();

	return std::make_shared<SFTPDirectoryEnumer>(_conn, path);
}

class SFTPFileIO
{
protected:
	std::shared_ptr<SFTPConnection> _conn;
	SFTPFile _file;

public:
	SFTPFileIO(std::shared_ptr<SFTPConnection> &conn, const std::string &path, int flags, mode_t mode, unsigned long long resume_pos)
		: _conn(conn), _file(sftp_open(conn->sftp, path.c_str(), flags, mode))
	{
		if (!_file)
			throw ProtocolError("open",  ssh_get_error(_conn->ssh));

		if (resume_pos) {
			int rc = sftp_seek64(_file, resume_pos);
			if (rc != 0)
				throw ProtocolError("seek",  ssh_get_error(_conn->ssh), rc);
		}
	}
};

class SFTPFileReader : protected SFTPFileIO, public IFileReader
{
	// pipeline of read requests for which async_begin had been called but not yet replied
	std::deque<uint32_t> _pipeline;

	// partial bytes remained by last previous read that were not fit into caller's buffer
	std::vector<char> _partial;

	// count of blocks of size _conn->max_read_block that must be requested til reaching guessed EOF
	unsigned long long _unrequested_count = 0;

	// true if previous reply contained less bytes than requests
	bool _truncated_reply = false;

	int AsyncReadComplete(void *data)
	{
		int r = sftp_async_read(_file, data, _conn->max_read_block, _pipeline.front());
		if (r != SSH_AGAIN) {
			_pipeline.pop_front();
		}

		if (r >= 0) {
			// Fail if on some block server returned less data than was requested
			// and that truncated block appeared not to be last in the file
			// if this situation will be ignored then position of file's blocks
			// pipelined after truncated block was guessed incorrectly that will
			// lead to data corruption
			// It could be possile to silently recover from such situation by
			// discarding wrong blocks, adjusting position and reducing block size
			// however such automation would lead to performance degradation so just
			// let user know about that he needs to reduce selected read block size.
			if (r > 0 && _truncated_reply) {
				throw ProtocolError("Read block too big");
			}
			if ((size_t)r < _conn->max_read_block) {
				_truncated_reply = true;
			}
		}

		return r;
	}

	void EnsurePipelinedRequestsInner(size_t pipeline_size_limit)
	{
		while (_pipeline.size() < pipeline_size_limit
		 && (_unrequested_count || _pipeline.empty())) {
			_pipeline.emplace_back();
			int id = sftp_async_read_begin(_file, _conn->max_read_block);
			if (id < 0)  {
				_pipeline.pop_back();
				throw ProtocolError("sftp_async_read_begin", ssh_get_error(_conn->ssh));
			}
			_pipeline.back() = (uint32_t)id;
			if (_unrequested_count) {
				--_unrequested_count;
			}
		}
	}

	// - ensures that pipeline contains at least one request
	// - adds more requests to pipeline if its size less then limit and _unrequested_count not zero
	// - reduces_unrequested_count by amount of requested blocks
	void EnsurePipelinedRequests(size_t pipeline_size_limit)
	{
		for (;;) try {
			if (pipeline_size_limit > 16) {
				EnsurePipelinedRequestsInner(16);
				SFTPFileNonblockinScope file_nonblock_scope(_file);
				EnsurePipelinedRequestsInner(pipeline_size_limit);
			} else {
				EnsurePipelinedRequestsInner(pipeline_size_limit);
			}
			break;

		} catch (std::exception &ex) {
			fprintf(stderr, "SFTPFileReader::EnqueueReadRequests: [%ld] %s\n", (unsigned long)_pipeline.size(), ex.what());
			if (!_pipeline.empty()) {
				break;
			}
			usleep(1000);
		}
	}

public:
	SFTPFileReader(std::shared_ptr<SFTPConnection> &conn, const std::string &path, unsigned long long resume_pos)
		: SFTPFileIO(conn, path, O_RDONLY, 0640, resume_pos)
	{
		SFTPAttributes  attributes(sftp_fstat(_file));
		if (attributes && attributes->size >= resume_pos) {
			_unrequested_count = attributes->size - resume_pos;
			_unrequested_count = (_unrequested_count / _conn->max_read_block) + ((_unrequested_count % _conn->max_read_block) ? 1 : 0);
		} else  {
			_unrequested_count = std::numeric_limits<unsigned long long>::max();
		}
		EnsurePipelinedRequests( (_unrequested_count >= 32) ? 32 : (size_t)_unrequested_count);
	}

	~SFTPFileReader()
	{
		if (!_pipeline.empty()) try {
			if (g_netrocks_verbosity > 0) {
				fprintf(stderr, "~SFTPFileReader: still pipelined %u\n", (unsigned int)_pipeline.size());
			}
			_partial.resize(_conn->max_read_block);
			do {
				AsyncReadComplete(&_partial[0]);
			} while (!_pipeline.empty());

		} catch (std::exception &ex) {
			fprintf(stderr, "~SFTPFileReader: %s\n", ex.what());
		}
	}


	virtual size_t Read(void *buf, size_t len)
	{
#if SIMULATED_READ_FAILS_RATE
		if ( (rand() % 100) + 1 <= SIMULATED_READ_FAILS_RATE)
			throw ProtocolError("Simulated read file error");
#endif

		EnsurePipelinedRequests( (len / _conn->max_read_block) + ((len % _conn->max_read_block) ? 1 : 0) );

		size_t out = _partial.size();
		if (out) {
			out = std::min(len, out);
			memcpy(buf, &_partial[0], out);
			if (out < _partial.size()) {
				_partial.erase(_partial.begin(), _partial.begin() + out);
			} else {
				_partial.clear();
			}

			len-= out;
			buf = (char *)buf + out;
			if (len < _conn->max_read_block) {
				return out;
			}
		}

		assert(_partial.empty());

		bool failed = false;
		if (len < _conn->max_read_block) {
			_partial.resize(_conn->max_read_block);
			int r = AsyncReadComplete(&_partial[0]);
			if (r > 0) {
				_partial.resize((size_t)r);
				size_t rfit = std::min(_partial.size(), len);
				memcpy(buf, &_partial[0], rfit);
				_partial.erase(_partial.begin(), _partial.begin() + rfit);
				out+= rfit;
				len-= rfit;
				buf = (char *)buf + rfit;
			} else {
				_partial.clear();
				if (r < 0) {
					failed = true;
				}
			}

		} else {
			int r = AsyncReadComplete(buf);
			if (r < 0) {
				failed = true;

			} else if (r == 0) {
				;

			} else {
				out+= r;
				len-= r;
				buf = (char *)buf + r;

				if (!_pipeline.empty() && len >= _conn->max_read_block) {
					SFTPFileNonblockinScope file_nonblock_scope(_file);
					do {
						r = AsyncReadComplete(buf);
						if (r <= 0) {
							if (r != SSH_AGAIN) {
								if (r < 0) {
									failed = true;
								}
							}
							break;
						}

						out+= r;
						len-= r;
						buf = (char *)buf + r;
					} while (!_pipeline.empty() && len >= _conn->max_read_block);
				}
			}
		}

		if (failed && out == 0) {
			throw ProtocolError("read error",  ssh_get_error(_conn->ssh));
		}

		return out;
	}
};


class SFTPFileWriter : protected SFTPFileIO, public IFileWriter
{
public:
	SFTPFileWriter(std::shared_ptr<SFTPConnection> &conn, const std::string &path, int flags, mode_t mode, unsigned long long resume_pos)
		: SFTPFileIO(conn, path, flags, mode, resume_pos)
	{
	}

	virtual void Write(const void *buf, size_t len)
	{
#if SIMULATED_WRITE_FAILS_RATE
		if ( (rand() % 100) + 1 <= SIMULATED_WRITE_FAILS_RATE)
			throw ProtocolError("Simulated write file error");
#endif
		// somewhy libssh doesnt have async write yet
		if (len > 0) for (;;) {
			size_t piece = (len >= _conn->max_write_block) ? _conn->max_write_block : len;
			ssize_t written = sftp_write(_file, buf, piece);

			if (written <= 0)
				throw ProtocolError("write error",  ssh_get_error(_conn->ssh));

			if ((size_t)written >= len)
				break;

			len-= (size_t)written;
			buf = (const char *)buf + written;
		}
	}

	virtual void WriteComplete()
	{
#if SIMULATED_WRITE_COMPLETE_FAILS_RATE
		if ( (rand() % 100) + 1 <= SIMULATED_WRITE_COMPLETE_FAILS_RATE)
			throw ProtocolError("Simulated write-complete file error");
#endif
		// what?
	}
};


std::shared_ptr<IFileReader> ProtocolSFTP::FileGet(const std::string &path, unsigned long long resume_pos)
{
	_conn->executed_command.reset();

	return std::make_shared<SFTPFileReader>(_conn, path, resume_pos);
}

std::shared_ptr<IFileWriter> ProtocolSFTP::FilePut(const std::string &path, mode_t mode, unsigned long long size_hint, unsigned long long resume_pos)
{
	_conn->executed_command.reset();

	return std::make_shared<SFTPFileWriter>(_conn, path, O_WRONLY | O_CREAT | (resume_pos ? 0 : O_TRUNC), mode, resume_pos);
}

void ProtocolSFTP::ExecuteCommand(const std::string &working_dir, const std::string &command_line, const std::string &fifo)
{
	_conn->executed_command.reset();
	_conn->executed_command = std::make_shared<SSHExecutedCommand>(_conn, working_dir, command_line, fifo, true);
}

void ProtocolSFTP::KeepAlive(const std::string &path_to_check)
{
	if (_conn->executed_command) {
		_conn->executed_command->KeepAlive();
	} else {
		GetMode(path_to_check);
	}
}
