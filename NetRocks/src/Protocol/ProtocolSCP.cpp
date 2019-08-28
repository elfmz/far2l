#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/tcp.h>
#include <fcntl.h>
#include <signal.h>
#include <assert.h>
#include <string.h>
#include <utils.h>
#include <ScopeHelpers.h>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <deque>
#include <algorithm>
#include <libssh/libssh.h>
#include <libssh/ssh2.h>
#include <StringConfig.h>
#include <ScopeHelpers.h>
#include <Threaded.h>
#include "ProtocolSCP.h"
#include "SSHConnection.h"
#include "../Op/Utils/ExecCommandFIFO.hpp"


#if (LIBSSH_VERSION_INT >= SSH_VERSION_INT(0, 7, 7))
# define SSH_SCP_REQUEST_GET_SIZE ssh_scp_request_get_size64
#else
# define SSH_SCP_REQUEST_GET_SIZE ssh_scp_request_get_size
#endif

static ssh_scp SCPNewRequest(ssh_session session, int mode, const std::string &location)
{
	std::string location_quoted = "\"";
	location_quoted+= EscapeQuotas(location);
	location_quoted+= "\"";

	return ssh_scp_new(session, mode, location_quoted.c_str());
}

static void SCPRequestDeleter(ssh_scp  res)
{
	if (res) {
		ssh_scp_close(res);
		ssh_scp_free(res);
	}
}


RESOURCE_CONTAINER(SCPRequest, ssh_scp, SCPRequestDeleter);

std::shared_ptr<IProtocol> CreateProtocolSCP(const std::string &host, unsigned int port,
	const std::string &username, const std::string &password, const std::string &options) throw (std::runtime_error)
{
//	sleep(20);
	return std::make_shared<ProtocolSCP>(host, port, username, password, options);
}

ProtocolSCP::ProtocolSCP(const std::string &host, unsigned int port,
	const std::string &username, const std::string &password, const std::string &options) throw (std::runtime_error)
{
	StringConfig protocol_options(options);
	_conn = std::make_shared<SSHConnection>(host, port, username, password, protocol_options);
	clock_gettime(CLOCK_REALTIME, &_now);
}

ProtocolSCP::~ProtocolSCP()
{
	_conn->executed_command.reset();
}


mode_t ProtocolSCP::GetMode(const std::string &path, bool follow_symlink) throw (std::runtime_error)
{
#if SIMULATED_GETMODE_FAILS_RATE
	if ( (rand() % 100) + 1 <= SIMULATED_GETMODE_FAILS_RATE)
		throw ProtocolError("Simulated getmode error");
#endif

	SCPRequest scp (SCPNewRequest(_conn->ssh, SSH_SCP_READ, path));// | SSH_SCP_RECURSIVE
	int rc = ssh_scp_init(scp);
  	if (rc != SSH_OK){
		throw ProtocolError("SCP init error",  ssh_get_error(_conn->ssh), rc);
	}

	rc = ssh_scp_pull_request(scp);

	switch (rc) {
		case SSH_SCP_REQUEST_NEWFILE: {
			mode_t out = ssh_scp_request_get_permissions(scp);
			ssh_scp_deny_request(scp, "sorry");
			return out | S_IFREG;
		}
		case SSH_SCP_REQUEST_NEWDIR: {
			mode_t out = ssh_scp_request_get_permissions(scp);
			ssh_scp_deny_request(scp, "sorry");
			return out | S_IFDIR;
		}
		case SSH_SCP_REQUEST_ENDDIR: {
			fprintf(stderr, "SSH_SCP_REQUEST_ENDDIR\n");
			break;
		}
		case SSH_SCP_REQUEST_EOF: {
			fprintf(stderr, "SSH_SCP_REQUEST_EOF\n");
			break;
		}
		case SSH_ERROR: {
			fprintf(stderr, "SSH_ERROR\n");
			throw ProtocolError("Query mode error",  ssh_get_error(_conn->ssh), rc);
		}
	}

	throw ProtocolError("Query mode fault",  ssh_get_error(_conn->ssh), rc);
}

unsigned long long ProtocolSCP::GetSize(const std::string &path, bool follow_symlink) throw (std::runtime_error)
{
#if SIMULATED_GETSIZE_FAILS_RATE
	if ( (rand() % 100) + 1 <= SIMULATED_GETSIZE_FAILS_RATE)
		throw ProtocolError("Simulated getsize error");
#endif

	SCPRequest scp(SCPNewRequest(_conn->ssh, SSH_SCP_READ, path));// | SSH_SCP_RECURSIVE
	int rc = ssh_scp_init(scp);
  	if (rc != SSH_OK){
		throw ProtocolError("SCP init error",  ssh_get_error(_conn->ssh), rc);
	}

	rc = ssh_scp_pull_request(scp);

	switch (rc) {
		case SSH_SCP_REQUEST_NEWFILE: case SSH_SCP_REQUEST_NEWDIR: {
			auto out = SSH_SCP_REQUEST_GET_SIZE(scp);
			ssh_scp_deny_request(scp, "sorry");
			return out;
		}
		case SSH_SCP_REQUEST_ENDDIR: {
			fprintf(stderr, "SSH_SCP_REQUEST_ENDDIR\n");
			break;
		}
		case SSH_SCP_REQUEST_EOF: {
			fprintf(stderr, "SSH_SCP_REQUEST_EOF\n");
			break;
		}
		case SSH_ERROR: {
			fprintf(stderr, "SSH_ERROR\n");
			throw ProtocolError("Query size error",  ssh_get_error(_conn->ssh), rc);
		}
	}

	throw ProtocolError("Query size fault",  ssh_get_error(_conn->ssh), rc);
}

void ProtocolSCP::GetInformation(FileInformation &file_info, const std::string &path, bool follow_symlink) throw (std::runtime_error)
{
#if SIMULATED_GETINFO_FAILS_RATE
	if ( (rand() % 100) + 1 <= SIMULATED_GETINFO_FAILS_RATE)
		throw ProtocolError("Simulated getinfo error");
#endif
	SCPRequest scp (SCPNewRequest(_conn->ssh, SSH_SCP_READ, path));// | SSH_SCP_RECURSIVE
	int rc = ssh_scp_init(scp);
  	if (rc != SSH_OK){
		throw ProtocolError("SCP init error",  ssh_get_error(_conn->ssh), rc);
	}

	rc = ssh_scp_pull_request(scp);

	switch (rc) {
		case SSH_SCP_REQUEST_NEWFILE: case SSH_SCP_REQUEST_NEWDIR: {
			file_info.size = SSH_SCP_REQUEST_GET_SIZE(scp);
			file_info.mode = ssh_scp_request_get_permissions(scp);
			file_info.mode|= (rc == SSH_SCP_REQUEST_NEWFILE) ? S_IFREG : S_IFDIR;
			file_info.access_time = file_info.modification_time = file_info.status_change_time = _now;
			ssh_scp_deny_request(scp, "sorry");
			return;
		}
		case SSH_SCP_REQUEST_ENDDIR: {
			fprintf(stderr, "SSH_SCP_REQUEST_ENDDIR\n");
			break;
		}
		case SSH_SCP_REQUEST_EOF: {
			fprintf(stderr, "SSH_SCP_REQUEST_EOF\n");
			break;
		}
		case SSH_ERROR: {
			fprintf(stderr, "SSH_ERROR\n");
			throw ProtocolError("Query info error",  ssh_get_error(_conn->ssh), rc);
		}
	}

	throw ProtocolError("Query info fault",  ssh_get_error(_conn->ssh), rc);
}

void ProtocolSCP::FileDelete(const std::string &path) throw (std::runtime_error)
{
	throw ProtocolUnsupportedError("SCP doesn't support delete");
}

void ProtocolSCP::DirectoryDelete(const std::string &path) throw (std::runtime_error)
{
	throw ProtocolUnsupportedError("SCP doesn't support rmdir");
}

void ProtocolSCP::DirectoryCreate(const std::string &path, mode_t mode) throw (std::runtime_error)
{
#if SIMULATED_MKDIR_FAILS_RATE
	if ( (rand() % 100) + 1 <= SIMULATED_MKDIR_FAILS_RATE)
		throw ProtocolError("Simulated mkdir error");
#endif

	SCPRequest scp (ssh_scp_new(_conn->ssh, SSH_SCP_WRITE, ExtractFilePath(path).c_str()));// | SSH_SCP_RECURSIVE
	int rc = ssh_scp_init(scp);
  	if (rc != SSH_OK){
		throw ProtocolError("SCP init error",  ssh_get_error(_conn->ssh), rc);
	}

	rc = ssh_scp_push_directory(scp, ExtractFileName(path).c_str(), mode);
  	if (rc != SSH_OK){
		throw ProtocolError("SCP push directory error",  ssh_get_error(_conn->ssh), rc);
	}
}

void ProtocolSCP::Rename(const std::string &path_old, const std::string &path_new) throw (std::runtime_error)
{
	throw ProtocolUnsupportedError("SCP doesn't support rename");
}

void ProtocolSCP::SetTimes(const std::string &path, const timespec &access_time, const timespec &modification_time) throw (std::runtime_error)
{
}

void ProtocolSCP::SetMode(const std::string &path, mode_t mode) throw (std::runtime_error)
{
}


void ProtocolSCP::SymlinkCreate(const std::string &link_path, const std::string &link_target) throw (std::runtime_error)
{
	throw ProtocolUnsupportedError("SCP doesn't support symlink");
}

void ProtocolSCP::SymlinkQuery(const std::string &link_path, std::string &link_target) throw (std::runtime_error)
{
	throw ProtocolUnsupportedError("SCP doesn't support symlink");
}

class SCPDirectoryEnumer : public IDirectoryEnumer
{
	std::shared_ptr<SSHConnection> _conn;
	ExecCommandFIFO _fifo;
	FDScope _fd_err;
	FDScope _fd_out;
	FDScope _fd_in;
	FDScope _fd_ctl;
	std::string _output;
	struct timespec _now;

	bool _finished = false;

	bool TryParseLine(std::string &name, std::string &owner, std::string &group, FileInformation &file_info) throw (std::runtime_error)
	{
/*
$ ls -lbA
total 104
-rw-rw-r-- 1 user user  1694 May 20 12:13 BackgroundTasks.cpp
-rw-rw-r-- 1 user user   811 May 20 12:13 BackgroundTasks.h
-rw-rw-r-- 1 user user  1953 Jun  4 12:28 Erroring.cpp
-rw-rw-r-- 1 user user   915 Jun  4 12:28 Erroring.h
-rw-rw-r-- 1 user user   341 Aug 21 23:55 FileInformation.h
-rw-rw-r-- 1 user user   629 May 20 12:13 Globals.cpp
-rw-rw-r-- 1 user user   549 May 20 12:13 Globals.h
drwxrwxr-x 2 user user  4096 Aug 21 23:55 Host
-rw-rw-r-- 1 user user  3497 Jun 30 00:35 lng.h
-rw-rw-r-- 1 user user  6862 Jun 14 14:49 NetRocks.cpp
drwxrwxr-x 3 user user  4096 Jul  8 23:36 Op
-rw-rw-r-- 1 user user 20225 Aug 21 23:55 PluginImpl.cpp
-rw-rw-r-- 1 user user  2030 Jun 26 11:20 PluginImpl.h
-rw-rw-r-- 1 user user  2144 May 20 12:13 PluginPanelItems.cpp
-rw-rw-r-- 1 user user   499 May 20 12:13 PluginPanelItems.h
-rw-rw-r-- 1 user user  1440 May 20 12:13 PooledStrings.cpp
-rw-rw-r-- 1 user user   280 May 20 12:13 PooledStrings.h
drwxrwxr-x 2 user user  4096 Aug 28 21:41 Protocol
-rw-rw-r-- 1 user user  4044 May 20 12:13 SitesConfig.cpp
-rw-rw-r-- 1 user user  1337 May 20 12:13 SitesConfig.h
drwxrwxr-x 4 user user  4096 Jun 30 00:35 UI
*/
		for (;;) {
			size_t p = _output.find_first_of("\r\n");
			if (p == std::string::npos) {
				return false;
			}

			std::string line = _output.substr(0, p), filename;
			while (p < _output.size() && (_output[p] == '\r' || _output[p] == '\n')) {
				++p;
			}
			_output.erase(0, p);

			
			for (size_t p = line.size();;) {
				p = line.rfind(' ', p);
				if (p == std::string::npos || p == 0) {
					break;
				}

				if (line[p - 1] != '\\') {
					name = line.substr(p + 1);
					line.resize(p);
					break;
				}

				--p;
			}
			


			std::vector<std::string> cols;
			StrExplode(cols, line, " \t");

			if (cols.size() < 6 || name.empty() || !FILENAME_ENUMERABLE(name)) {
				name.clear();
				continue;
			}

			while (cols[0].size() < 10) {
				cols[0]+= '-';
			}

			owner = cols[2];
			group = cols[3];
			file_info.size = atol(cols[4].c_str());

			switch (cols[0][0]) {
				case 'l': file_info.mode|= S_IFLNK; break;
				case 'd': file_info.mode|= S_IFDIR; break;
				case '-': file_info.mode|= S_IFREG; break;
				default: ; //...
			}
			if (cols[0][1] == 'r') file_info.mode|= 0200;
			if (cols[0][2] == 'w') file_info.mode|= 0400;
			if (cols[0][3] == 'x' || cols[0][3] == 's') file_info.mode|= 0100;
			if (cols[0][4] == 'r') file_info.mode|= 0020;
			if (cols[0][5] == 'w') file_info.mode|= 0040;
			if (cols[0][6] == 'x' || cols[0][6] == 's') file_info.mode|= 0010;
			if (cols[0][7] == 'r') file_info.mode|= 0002;
			if (cols[0][8] == 'w') file_info.mode|= 0004;
			if (cols[0][9] == 'x' || cols[0][9] == 's') file_info.mode|= 0001;

			file_info.access_time = file_info.modification_time = file_info.status_change_time = _now;

			for (size_t i = name.size(); i > 0;) {
				--i;
				if (name[i] == '\\' && i + 1 < name.size()) {
					switch (name[i + 1]) {
						case ' ': name.replace(i, 2, " "); break;
						case '\\': name.replace(i, 2, "\\"); break;
						case 't': name.replace(i, 2, "\t"); break;
						case 'r': name.replace(i, 2, "\r"); break;
						case 'n': name.replace(i, 2, "\n"); break;
					}
					if (i > 0) {
						--i;
					}
				}
			}
			return true;
		}
	}

public:
	SCPDirectoryEnumer(std::shared_ptr<SSHConnection> &conn, std::string path, const struct timespec &now)
		: _conn(conn), _now(now)
	{

		std::string command_line = "ls -lbA \"";
		command_line+= EscapeQuotas(path);
		command_line+= "\"";

		_conn->executed_command.reset();
		_conn->executed_command = std::make_shared<SSHExecutedCommand>(_conn, "", command_line, _fifo.FileName());

		_fd_err = open((_fifo.FileName() + ".err").c_str(), O_RDONLY);
		_fd_out = open((_fifo.FileName() + ".out").c_str(), O_RDONLY);
		_fd_in = open((_fifo.FileName() + ".in").c_str(), O_WRONLY);
		_fd_ctl = open((_fifo.FileName() + ".ctl").c_str(), O_WRONLY);

		if (!_fd_ctl.Valid() || !_fd_in.Valid() || !_fd_out.Valid() || !_fd_err.Valid()) {
			throw std::runtime_error("Can't open FIFO");
		}

		ExecFIFO_CtlMsg m = {};
		m.cmd = ExecFIFO_CtlMsg::CMD_PTY_SIZE;
		m.u.pty_size.cols = 80;
		m.u.pty_size.rows = 25;

		if (WriteAll(_fd_ctl, &m, sizeof(m)) != sizeof(m)) {
			throw std::runtime_error("Can't send PTY_SIZE");
		}
	}

	virtual ~SCPDirectoryEnumer()
	{
		_conn->executed_command.reset();
	}

	virtual bool Enum(std::string &name, std::string &owner, std::string &group, FileInformation &file_info) throw (std::runtime_error)
	{
		name.clear();
		owner.clear();
		group.clear();
		file_info = FileInformation{};

#if SIMULATED_ENUM_FAILS_RATE
		if ( (rand() % 100) + 1 <= SIMULATED_ENUM_FAILS_RATE) {
			throw ProtocolError("Simulated enum dir error");
		}
#endif

		if (_finished) {
			return false;
		}

		fd_set fdr, fde;
		char buf[0x10000];
		for (;;) {
			if (TryParseLine(name, owner, group, file_info)) {
				return true;
			}

			FD_ZERO(&fdr);
			FD_ZERO(&fde);
			FD_SET(_fd_out, &fdr);
			FD_SET(_fd_err, &fdr);

			FD_SET(_fd_out, &fde);
			FD_SET(_fd_err, &fde);

			int r = select(std::max((int)_fd_out, (int)_fd_err) + 1, &fdr, nullptr, &fde, NULL);
			if ( r < 0) {
				if (errno == EAGAIN || errno == EINTR)
					continue;

				_finished = true;
				return false;
			}

			if (FD_ISSET(_fd_out, &fdr) || FD_ISSET(_fd_out, &fde)) {
				ssize_t r = read(_fd_out, buf, sizeof(buf));
				if (r == 0 || (r < 0 && errno != EAGAIN && errno != EINTR)) {
					_finished = true;
					return false;
				}

				if (r > 0) {
					_output.append(buf, r);
				}
			}

			if (FD_ISSET(_fd_err, &fdr) || FD_ISSET(_fd_err, &fde)) {
				ssize_t r = read(_fd_err, buf, sizeof(buf));
				if (r < 0 && errno != EAGAIN && errno != EINTR) {
					_finished = true;
					return false;
				}
			}
		}

  	}
};

std::shared_ptr<IDirectoryEnumer> ProtocolSCP::DirectoryEnum(const std::string &path) throw (std::runtime_error)
{
	_conn->executed_command.reset();

	return std::make_shared<SCPDirectoryEnumer>(_conn, path, _now);
}

class SCPFileReader : public IFileReader
{
	std::shared_ptr<SSHConnection> _conn;
	SCPRequest _scp;
	unsigned long long _pending_size = 0;
public:
	SCPFileReader(std::shared_ptr<SSHConnection> &conn, const std::string &path)
	:
		_conn(conn),
		_scp(SCPNewRequest(conn->ssh, SSH_SCP_READ, path))
	{
		int rc = ssh_scp_init(_scp);
  		if (rc != SSH_OK){
			throw ProtocolError("SCP init error",  ssh_get_error(_conn->ssh), rc);
		}

		for (;;) {
			rc = ssh_scp_pull_request(_scp);
			switch (rc) {
				case SSH_SCP_REQUEST_NEWFILE: case SSH_SCP_REQUEST_NEWDIR: {
					_pending_size = SSH_SCP_REQUEST_GET_SIZE(_scp);
					ssh_scp_accept_request(_scp);
					return;
				}
				case SSH_SCP_REQUEST_ENDDIR: {
					fprintf(stderr, "SCPFileReader: SSH_SCP_REQUEST_ENDDIR on '%s'\n", path.c_str());
					_pending_size = 0; // ???
					break;
				}
				case SSH_SCP_REQUEST_EOF: {
					fprintf(stderr, "SCPFileReader: SSH_SCP_REQUEST_EOF on '%s'\n", path.c_str());
					_pending_size = 0; // ???
					break;
				}
				case SSH_ERROR: {
					fprintf(stderr, "SSH_ERROR\n");
					throw ProtocolError("Pull file error",  ssh_get_error(_conn->ssh), rc);
				}
			}
		}
	}

	virtual ~SCPFileReader()
	{
	}


	virtual size_t Read(void *buf, size_t len) throw (std::runtime_error)
	{
#if SIMULATED_READ_FAILS_RATE
		if ( (rand() % 100) + 1 <= SIMULATED_READ_FAILS_RATE)
			throw ProtocolError("Simulated read file error");
#endif

		if (_pending_size == 0) {
			return 0;
		}
		if (len > _pending_size) {
			len = _pending_size;
		}
		ssize_t rc = ssh_scp_read(_scp, buf, len);
		if (rc < 0) {
			throw ProtocolError("Read file error",  ssh_get_error(_conn->ssh), rc);

		} else if ((size_t)rc <= _pending_size) {
			_pending_size-= (size_t)rc;

		} else {
			rc = len;
			_pending_size = 0;
		}

		return rc;
	}
};

/*
class SFTPFileWriter : protected SCPFileIO, public IFileWriter
{
public:
	SFTPFileWriter(std::shared_ptr<SSHConnection> &conn, const std::string &path, int flags, mode_t mode, unsigned long long resume_pos)
		: SCPFileIO(conn, path, flags, mode, resume_pos)
	{
	}

	virtual void Write(const void *buf, size_t len) throw (std::runtime_error)
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

	virtual void WriteComplete() throw (std::runtime_error)
	{
#if SIMULATED_WRITE_COMPLETE_FAILS_RATE
		if ( (rand() % 100) + 1 <= SIMULATED_WRITE_COMPLETE_FAILS_RATE)
			throw ProtocolError("Simulated write-complete file error");
#endif
		// what?
	}
};
*/

std::shared_ptr<IFileReader> ProtocolSCP::FileGet(const std::string &path, unsigned long long resume_pos) throw (std::runtime_error)
{
	if (resume_pos) {
		throw ProtocolUnsupportedError("SCP doesn't support download resume");
	}

	_conn->executed_command.reset();
	return std::make_shared<SCPFileReader>(_conn, path);
}

std::shared_ptr<IFileWriter> ProtocolSCP::FilePut(const std::string &path, mode_t mode, unsigned long long resume_pos) throw (std::runtime_error)
{
	if (resume_pos) {
		throw ProtocolUnsupportedError("SCP doesn't support upload resume");
	}

	_conn->executed_command.reset();
	throw ProtocolUnsupportedError("TODO: ProtocolSCP::FilePut");
//	return std::make_shared<SFTPFileWriter>(_conn, path, O_WRONLY | O_CREAT | (resume_pos ? 0 : O_TRUNC), mode, resume_pos);
}


void ProtocolSCP::ExecuteCommand(const std::string &working_dir, const std::string &command_line, const std::string &fifo) throw (std::runtime_error)
{
	_conn->executed_command.reset();
	_conn->executed_command = std::make_shared<SSHExecutedCommand>(_conn, working_dir, command_line, fifo);
}
