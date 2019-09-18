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
# define SSH_SCP_PUSH_FILE ssh_scp_push_file64

#else
# define SSH_SCP_REQUEST_GET_SIZE ssh_scp_request_get_size
# define SSH_SCP_PUSH_FILE ssh_scp_push_file

#endif

static std::string QuotedArg(const std::string &s)
{
	if (s.find_first_of("\"\r\n\t' ") == std::string::npos) {
		return s;
	}

	std::string out = "\"";
	out+= EscapeQuotas(s);
	out+= '\"';
	return out;
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


struct SCPRemoteCommand
{
	ExecCommandFIFO fifo;
	FDScope fd_err;
	FDScope fd_out;
	FDScope fd_in;
	FDScope fd_ctl;

	std::string output, error;

	void Execute()
	{
		fd_err = open((fifo.FileName() + ".err").c_str(), O_RDONLY);
		fd_out = open((fifo.FileName() + ".out").c_str(), O_RDONLY);
		fd_in = open((fifo.FileName() + ".in").c_str(), O_WRONLY);
		fd_ctl = open((fifo.FileName() + ".ctl").c_str(), O_WRONLY);

		if (!fd_ctl.Valid() || !fd_in.Valid() || !fd_out.Valid() || !fd_err.Valid()) {
			throw std::runtime_error("Can't open FIFO");
		}

		ExecFIFO_CtlMsg m = {};
		m.cmd = ExecFIFO_CtlMsg::CMD_PTY_SIZE;
		m.u.pty_size.cols = 80;
		m.u.pty_size.rows = 25;

		if (WriteAll(fd_ctl, &m, sizeof(m)) != sizeof(m)) {
			throw std::runtime_error("Can't send PTY_SIZE");
		}
	}

	bool FetchOutput()
	{
		fd_set fdr, fde;
		char buf[0x10000];

		FD_ZERO(&fdr);
		FD_ZERO(&fde);
		FD_SET(fd_out, &fdr);
		FD_SET(fd_err, &fdr);

		FD_SET(fd_out, &fde);
		FD_SET(fd_err, &fde);

		int r = select(std::max((int)fd_out, (int)fd_err) + 1, &fdr, nullptr, &fde, NULL);
		if ( r < 0) {
			if (errno == EAGAIN || errno == EINTR)
				return true;

			return false;
		}

		if (FD_ISSET(fd_out, &fdr) || FD_ISSET(fd_out, &fde)) {
			ssize_t r = read(fd_out, buf, sizeof(buf));
			if (r == 0 || (r < 0 && errno != EAGAIN && errno != EINTR)) {
				return false;
			}

			if (r > 0) {
				output.append(buf, r);
			}
		}

		if (FD_ISSET(fd_err, &fdr) || FD_ISSET(fd_err, &fde)) {
			ssize_t r = read(fd_err, buf, sizeof(buf));
			if (r < 0 && errno != EAGAIN && errno != EINTR) {
				return false;
			}

			if (r > 0) {
				error.append(buf, r);
			}
		}

		return true;
	}

	int ReadStatus()
	{
		return fifo.ReadStatus();
	}
};


class SimpleCommand
{
	std::shared_ptr<SSHConnection> _conn;
	std::string _output, _error;

public:
	SimpleCommand(std::shared_ptr<SSHConnection> &conn)
		: _conn(conn)
	{
	}

	virtual ~SimpleCommand()
	{
		_conn->executed_command.reset();
	}

	int Execute(const std::string &command_line)
	{
		SCPRemoteCommand cmd;

		_conn->executed_command.reset();
		_conn->executed_command = std::make_shared<SSHExecutedCommand>(_conn, "", command_line, cmd.fifo.FileName());

		cmd.Execute();
		while (cmd.FetchOutput()) {
			cmd.output.clear();
			cmd.error.clear();
		}

		_output.swap(cmd.output);
		_error.swap(cmd.error);

		return cmd.ReadStatus();
	}

	int Execute(const char *cmdline_fmt, ...)
	{
		va_list args;
		va_start(args, cmdline_fmt);
		const std::string &command_line = StrPrintfV(cmdline_fmt, args);
		va_end(args);

		return Execute(command_line);
	}

	const std::string &Output() const { return _output; }
	const std::string &Error() const { return _error; }
};

mode_t ProtocolSCP::GetMode(const std::string &path, bool follow_symlink) throw (std::runtime_error)
{
#if SIMULATED_GETMODE_FAILS_RATE
	if ( (rand() % 100) + 1 <= SIMULATED_GETMODE_FAILS_RATE)
		throw ProtocolError("Simulated getmode error");
#endif

	SCPRequest scp (ssh_scp_new(_conn->ssh, SSH_SCP_READ | SSH_SCP_RECURSIVE, QuotedArg(path).c_str()));// | 
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

	SCPRequest scp(ssh_scp_new(_conn->ssh, SSH_SCP_READ | SSH_SCP_RECURSIVE, QuotedArg(path).c_str()));// | SSH_SCP_RECURSIVE
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
	SCPRequest scp (ssh_scp_new(_conn->ssh, SSH_SCP_READ | SSH_SCP_RECURSIVE, QuotedArg(path).c_str()));// | SSH_SCP_RECURSIVE
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
	SimpleCommand sc(_conn);
	int rc = sc.Execute("unlink %s", QuotedArg(path).c_str());
	if (rc != 0) {
		throw ProtocolError(sc.Error().c_str(), rc);
	}
}

void ProtocolSCP::DirectoryDelete(const std::string &path) throw (std::runtime_error)
{
	SimpleCommand sc(_conn);
	int rc = sc.Execute("rmdir %s", QuotedArg(path).c_str());
	if (rc != 0) {
		throw ProtocolError(sc.Error().c_str(), rc);
	}
}

void ProtocolSCP::DirectoryCreate(const std::string &path, mode_t mode) throw (std::runtime_error)
{
	fprintf(stderr, "ProtocolSCP::DirectoryCreate mode=%o path=%s\n", mode, path.c_str());
	if (path == "." || path == "./") //wtf
		return;

#if SIMULATED_MKDIR_FAILS_RATE
	if ( (rand() % 100) + 1 <= SIMULATED_MKDIR_FAILS_RATE)
		throw ProtocolError("Simulated mkdir error");
#endif

	SCPRequest scp(ssh_scp_new(_conn->ssh, SSH_SCP_WRITE | SSH_SCP_RECURSIVE, QuotedArg(ExtractFilePath(path)).c_str()));// 
	int rc = ssh_scp_init(scp);
  	if (rc != SSH_OK){
		throw ProtocolError("SCP init error",  ssh_get_error(_conn->ssh), rc);
	}

	rc = ssh_scp_push_directory(scp, ExtractFileName(path).c_str(), mode & 0777);
  	if (rc != SSH_OK){
		throw ProtocolError("SCP push directory error",  ssh_get_error(_conn->ssh), rc);
	}
}

void ProtocolSCP::Rename(const std::string &path_old, const std::string &path_new) throw (std::runtime_error)
{
	SimpleCommand sc(_conn);
	int rc = sc.Execute("rename %s %s", QuotedArg(path_old).c_str(), QuotedArg(path_new).c_str());
	if (rc != 0) {
		throw ProtocolError(sc.Error().c_str(), rc);
	}
}

void ProtocolSCP::SetTimes(const std::string &path, const timespec &access_time, const timespec &modification_time) throw (std::runtime_error)
{
}

void ProtocolSCP::SetMode(const std::string &path, mode_t mode) throw (std::runtime_error)
{
	SimpleCommand sc(_conn);
	int rc = sc.Execute("chmod %o %s", mode, QuotedArg(path).c_str());
	if (rc != 0) {
		throw ProtocolError(sc.Error().c_str(), rc);
	}
}


void ProtocolSCP::SymlinkCreate(const std::string &link_path, const std::string &link_target) throw (std::runtime_error)
{
	SimpleCommand sc(_conn);
	int rc = sc.Execute("ln -s %s %s", QuotedArg(link_target).c_str(), QuotedArg(link_path).c_str());
	if (rc != 0) {
		throw ProtocolError(sc.Error().c_str(), rc);
	}
}

void ProtocolSCP::SymlinkQuery(const std::string &link_path, std::string &link_target) throw (std::runtime_error)
{
	SimpleCommand sc(_conn);
	int rc = sc.Execute("readlink %s", QuotedArg(link_path).c_str());
// fprintf(stderr, "ProtocolSCP::SymlinkQuery('%s') -> '%s' %d\n", link_path.c_str(), link_target.c_str(), rc);
	if (rc != 0) {
		throw ProtocolError(sc.Error().c_str(), rc);
	}
	link_target = sc.Output();
	while (!link_target.empty() && (link_target[link_target.size() - 1] == '\r' || link_target[link_target.size() - 1] == '\n')) {
		link_target.resize(link_target.size() - 1);
	}
}

static std::string ExtractStringTail(std::string &line)
{
	std::string out;
	size_t p = line.rfind(' ');
	if (p != std::string::npos) {
		out = line.substr(p + 1);
		line.resize(p);
	} else {
		out.swap(line);
	}

	return out;
}

class SCPDirectoryEnumer : public IDirectoryEnumer
{
	std::shared_ptr<SSHConnection> _conn;
	struct timespec _now;

	bool _finished = false;
	SCPRemoteCommand _cmd;

	bool TryParseLine(std::string &name, std::string &owner, std::string &group, FileInformation &file_info) throw (std::runtime_error)
	{
// PATH/NAME MODE SIZE ACCESS MODIFY CHANGE USER GROUP
// /bin/bash 81ed 1037528 1568672221 1494938995 1523911947 root root
// /bin/bunzip2 81ed 31352 1568065896 1562243762 1562383716 root root
// /bin/busybox 81ed 1984584 1558035456 1551971578 1554350429 root root
// /bin/bzcat 81ed 31352 1568065896 1562243762 1562383716 root root
// /bin/bzcmp a1ff 6 1568754149 1562243761 1562383716 root root

		for (;;) {
			size_t p = _cmd.output.find_first_of("\r\n");
			if (p == std::string::npos) {
				return false;
			}

			std::string line = _cmd.output.substr(0, p), filename;
			while (p < _cmd.output.size() && (_cmd.output[p] == '\r' || _cmd.output[p] == '\n')) {
				++p;
			}
			_cmd.output.erase(0, p);

			const std::string &str_group = ExtractStringTail(line);
			const std::string &str_owner = ExtractStringTail(line);
			const std::string &str_change = ExtractStringTail(line);
			const std::string &str_modify = ExtractStringTail(line);
			const std::string &str_access = ExtractStringTail(line);
			const std::string &str_size = ExtractStringTail(line);
			const std::string &str_mode = ExtractStringTail(line);

			p = line.rfind('/');
			if (p != std::string::npos) {
				name = line.substr(p + 1);
			} else {
				name.swap(line);
			}

			if (name.empty() || !FILENAME_ENUMERABLE(name)) {
				name.clear();
				continue;
			}

			file_info.access_time.tv_sec = atol(str_access.c_str());
			file_info.modification_time.tv_sec = atol(str_modify.c_str());
			file_info.status_change_time.tv_sec = atol(str_change.c_str());
			file_info.mode = htoul(str_mode.c_str());
			file_info.size = atol(str_size.c_str());

			owner = str_owner;
			group = str_group;
			return true;
		}
	}

public:
	SCPDirectoryEnumer(std::shared_ptr<SSHConnection> &conn, std::string path, const struct timespec &now)
		: _conn(conn), _now(now)
	{
		std::string command_line = "stat --format=\"%n %f %s %X %Y %Z %U %G\" ";
		command_line+= QuotedArg(path);
		command_line+= "/.* ";
		command_line+= QuotedArg(path);
		command_line+= "/*";
		command_line+= " 2>/dev/null";

		_conn->executed_command.reset();
		_conn->executed_command = std::make_shared<SSHExecutedCommand>(_conn, "", command_line, _cmd.fifo.FileName());

		_cmd.Execute();
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

		for (;;) {
			if (TryParseLine(name, owner, group, file_info)) {
				return true;
			}

			if (!_cmd.FetchOutput()) {
				_finished = true;
				return false;
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
		_scp(ssh_scp_new(conn->ssh, SSH_SCP_READ, QuotedArg(path).c_str()))
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

class SCPFileWriter : public IFileWriter
{
	std::shared_ptr<SSHConnection> _conn;
	SCPRequest _scp;
	unsigned long long _pending_size;
	bool _truncated = false;

public:
	SCPFileWriter(std::shared_ptr<SSHConnection> &conn, const std::string &path, mode_t mode, unsigned long long size_hint)
	:
		_conn(conn),
		_scp(ssh_scp_new(conn->ssh, SSH_SCP_WRITE, QuotedArg(ExtractFilePath(path)).c_str())),
		_pending_size(size_hint)
	{
		mode&= 0777;
		int rc = ssh_scp_init(_scp);
  		if (rc != SSH_OK){
			throw ProtocolError("SCP init error",  ssh_get_error(_conn->ssh), rc);
		}
		rc = SSH_SCP_PUSH_FILE(_scp, ExtractFileName(path).c_str(), size_hint, mode);
  		if (rc != SSH_OK){
			throw ProtocolError("SCP push error",  ssh_get_error(_conn->ssh), rc);
		}
	}

	virtual ~SCPFileWriter()
	{
	}

	virtual void WriteComplete() throw (std::runtime_error)
	{
#if SIMULATED_WRITE_COMPLETE_FAILS_RATE
		if ( (rand() % 100) + 1 <= SIMULATED_WRITE_COMPLETE_FAILS_RATE)
			throw ProtocolError("Simulated write-complete file error");
#endif
		if (_truncated)
			throw ProtocolError("Excessive data truncated");
	}



	virtual void Write(const void *buf, size_t len) throw (std::runtime_error)
	{
#if SIMULATED_WRITE_FAILS_RATE
		if ( (rand() % 100) + 1 <= SIMULATED_READ_FAILS_RATE)
			throw ProtocolError("Simulated write file error");
#endif

		if (len > _pending_size) {
			len = _pending_size;
			_truncated = true;
		}

		if (len) {
			int rc = ssh_scp_write(_scp, buf, len);
			if (rc != SSH_OK) {
				throw ProtocolError("SCP write error",  ssh_get_error(_conn->ssh), rc);
			}
		}
	}
};

std::shared_ptr<IFileReader> ProtocolSCP::FileGet(const std::string &path, unsigned long long resume_pos) throw (std::runtime_error)
{
	if (resume_pos) {
		throw ProtocolUnsupportedError("SCP doesn't support download resume");
	}

	_conn->executed_command.reset();
	return std::make_shared<SCPFileReader>(_conn, path);
}

std::shared_ptr<IFileWriter> ProtocolSCP::FilePut(const std::string &path, mode_t mode, unsigned long long size_hint, unsigned long long resume_pos) throw (std::runtime_error)
{
	if (resume_pos) {
		throw ProtocolUnsupportedError("SCP doesn't support upload resume");
	}

	_conn->executed_command.reset();
	return std::make_shared<SCPFileWriter>(_conn, path, mode, size_hint);
}


void ProtocolSCP::ExecuteCommand(const std::string &working_dir, const std::string &command_line, const std::string &fifo) throw (std::runtime_error)
{
	_conn->executed_command.reset();
	_conn->executed_command = std::make_shared<SSHExecutedCommand>(_conn, working_dir, command_line, fifo);
}
