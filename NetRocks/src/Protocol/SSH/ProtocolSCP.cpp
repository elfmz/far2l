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
#include "../ShellParseUtils.h"
#include "../../Op/Utils/ExecCommandFIFO.hpp"

#define QUERY_BY_CMD

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
	out+= EscapeQuotes(s);
	out+= '\"';
	return out;
}

static void SCPRequestDeleter(ssh_scp res)
{
	if (res) {
		ssh_scp_close(res);
		ssh_scp_free(res);
	}
}

static std::string FilterError(const std::string &error)
{
	std::string out;
	// take very first unempty line from stderr
	std::vector<std::string> lines;
	StrExplode(lines, error, "\r\n");
	for (const auto &line : lines) if (!line.empty()) {
		out = line;
		break;
	}

	// leave only relevant text after colon here:
	// ls: invalid option -- 'f'
	size_t p = out.rfind(':');
	if (p != std::string::npos && p + 1 < out.size()) {
		out = out.substr(p + 1);
	}
	StrTrim(out);
	return out;
}


RESOURCE_CONTAINER(SCPRequest, ssh_scp, SCPRequestDeleter);

struct SCPRemoteCommand
{
	ExecCommandFIFO fifo;
	std::string output, error;

	void Execute()
	{
		_fd_err = open((fifo.FileName() + ".err").c_str(), O_RDONLY);
		_fd_out = open((fifo.FileName() + ".out").c_str(), O_RDONLY);
		_fd_in = open((fifo.FileName() + ".in").c_str(), O_WRONLY);
		_fd_ctl = open((fifo.FileName() + ".ctl").c_str(), O_WRONLY);

		if (!_fd_ctl.Valid() || !_fd_in.Valid() || !_fd_out.Valid() || !_fd_err.Valid()) {
			throw std::runtime_error("Can't open FIFO");
		}
		_alive_out = _alive_err = true;

		ExecFIFO_CtlMsg m = {};
		m.cmd = ExecFIFO_CtlMsg::CMD_PTY_SIZE;
		m.u.pty_size.cols = 80;
		m.u.pty_size.rows = 25;

		if (WriteAll(_fd_ctl, &m, sizeof(m)) != sizeof(m)) {
			throw std::runtime_error("Can't send PTY_SIZE");
		}
	}

	bool FetchOutput()
	{
		if (!_alive_out && !_alive_err) {
			return false;
		}

		FD_ZERO(&_fdr);
		FD_ZERO(&_fde);

		int maxfd = -1;
		if (_alive_out) {
			FD_SET(_fd_out, &_fdr);
			FD_SET(_fd_out, &_fde);
			maxfd = std::max(maxfd, (int)_fd_out);
		}

		if (_alive_err) {
			FD_SET(_fd_err, &_fdr);
			FD_SET(_fd_err, &_fde);
			maxfd = std::max(maxfd, (int)_fd_err);
		}

		int r = select(maxfd + 1, &_fdr, nullptr, &_fde, NULL);
		if ( r < 0) {
			if (errno == EAGAIN || errno == EINTR)
				return true;

			return false;
		}

		if (_alive_out && !FetchFD(output, _fd_out)) {
			_alive_out = false;
		}

		if (_alive_err && !FetchFD(error, _fd_err)) {
			_alive_err = false;
		}

		return (_alive_out || _alive_err);
	}

private:
	FDScope _fd_err;
	FDScope _fd_out;
	FDScope _fd_in;
	FDScope _fd_ctl;
	bool _alive_out{false}, _alive_err{false};

	fd_set _fdr, _fde;
	char _buf[0x10000];

	bool FetchFD(std::string &result, int fd)
	{
		if (FD_ISSET(fd, &_fdr) || FD_ISSET(fd, &_fde)) {
			ssize_t r = read(fd, _buf, sizeof(_buf));
			if (r == 0 || (r < 0 && errno != EAGAIN && errno != EINTR)) {
				return false;
			}

			if (r > 0) {
				result.append(_buf, r);
			}
		}
		return true;
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
		_conn->executed_command = std::make_shared<SSHExecutedCommand>(_conn, "", command_line, cmd.fifo.FileName(), false);

		cmd.Execute();
		while (cmd.FetchOutput()) {
			; // nothing
		}
		_output.swap(cmd.output);
		_error.swap(cmd.error);

		return cmd.fifo.ReadStatus();
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

	std::string FilteredError() const
	{
		return FilterError(_error);
	}
};


class SCPDirectoryEnumer : public IDirectoryEnumer
{
protected:
	std::shared_ptr<SSHConnection> _conn;
	bool _finished = false;
	SCPRemoteCommand _cmd;

	virtual bool TryParseLine(std::string &name, std::string &owner, std::string &group, FileInformation &file_info) = 0;

public:
	SCPDirectoryEnumer(std::shared_ptr<SSHConnection> &conn)
		: _conn(conn)
	{
	}

	virtual ~SCPDirectoryEnumer()
	{
		_conn->executed_command.reset();
	}

	virtual bool Enum(std::string &name, std::string &owner, std::string &group, FileInformation &file_info)
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

	std::string FilteredError() const
	{
		return FilterError(_cmd.error);
	}
};

enum DirectoryEnumerMode
{
	DEM_LIST,
	DEM_QUERY,
	DEM_QUERY_FOLLOW_SYMLINKS
};

class SCPDirectoryEnumer_stat : public SCPDirectoryEnumer
{
	virtual bool TryParseLine(std::string &name, std::string &owner, std::string &group, FileInformation &file_info)
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

			const std::string &str_group = ShellParseUtils::ExtractStringTail(line, " ");
			const std::string &str_owner = ShellParseUtils::ExtractStringTail(line, " ");
			const std::string &str_change = ShellParseUtils::ExtractStringTail(line, " ");
			const std::string &str_modify = ShellParseUtils::ExtractStringTail(line, " ");
			const std::string &str_access = ShellParseUtils::ExtractStringTail(line, " ");
			const std::string &str_size = ShellParseUtils::ExtractStringTail(line, " ");
			const std::string &str_mode = ShellParseUtils::ExtractStringTail(line, " ");

			p = line.rfind('/');
			if (p != std::string::npos && line.size() > 1) {
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
			file_info.mode = HexToULong(str_mode.c_str(), str_mode.size());
			file_info.size = atol(str_size.c_str());

			owner = str_owner;
			group = str_group;
			return true;
		}
	}

public:
	SCPDirectoryEnumer_stat(std::shared_ptr<SSHConnection> &conn, DirectoryEnumerMode dem, size_t count, const std::string *paths)
		: SCPDirectoryEnumer(conn)
	{
		std::string command_line = "stat --format=\"%n %f %s %X %Y %Z %U %G\" ";
		if (dem == DEM_QUERY_FOLLOW_SYMLINKS)
			command_line+= "-L ";

		for (size_t i = 0; i < count; ++i) {
			const std::string &path_arg = QuotedArg(EnsureNoSlashAtNestedEnd(paths[i]));
			command_line+= path_arg;
			if (dem == DEM_LIST) {
				command_line+= "/.* ";
				command_line+= path_arg;
				command_line+= "/*";
			}
			command_line+= ' ';
		}
//		command_line+= "2>/dev/null";

		_conn->executed_command.reset();
		_conn->executed_command = std::make_shared<SSHExecutedCommand>(_conn, "", command_line, _cmd.fifo.FileName(), false);

		_cmd.Execute();
	}
};

class SCPDirectoryEnumer_ls : public SCPDirectoryEnumer
{
	virtual bool TryParseLine(std::string &name, std::string &owner, std::string &group, FileInformation &file_info)
	{
/*
# ls -l -A -f /
total 25
drwxr-xr-x    2 root     root          2048 Sep 25  2021 bin
drwxr-xr-x    2 root     root          1024 Sep 25  2021 boot
drwxr-xr-x    8 root     root         12480 Jan  2 19:36 dev
drwxr-xr-x    9 root     root          1024 Jan  1 00:00 etc
drwxr-xr-x    3 root     root          1024 Sep 25  2021 lib
lrwxrwxrwx    1 root     root             3 Sep 24  2021 lib32 -> lib
lrwxrwxrwx    1 root     root            11 Sep 24  2021 linuxrc -> bin/busybox
drwx------    2 root     root         12288 Sep 25  2021 lost+found

devices - major&minor instead  of size:
brw-------    1 root     root      179,   0 Jan  1  1970 mmcblk0
crw-------    1 root     root        3, 144 Jan  1  1970 ttyy0
*/
		for (;;) {
			size_t p = _cmd.output.find_first_of("\r\n");
			if (p == std::string::npos) {
				return false;
			}

			std::string line = _cmd.output.substr(0, p);
			while (p < _cmd.output.size() && (_cmd.output[p] == '\r' || _cmd.output[p] == '\n')) {
				++p;
			}
			_cmd.output.erase(0, p);

			if (ShellParseUtils::ParseLineFromLS(line, name, owner, group,
					file_info.access_time, file_info.modification_time, file_info.status_change_time,
					file_info.size, file_info.mode)) {
				if (!name.empty() && FILENAME_ENUMERABLE(name)) {
					return true;
				}
				name.clear();
			}
		}
	}

public:
	SCPDirectoryEnumer_ls(std::shared_ptr<SSHConnection> &conn, const SCPQuirks &quirks, DirectoryEnumerMode dem, size_t count, const std::string *paths, const struct timespec &now)
		: SCPDirectoryEnumer(conn)
	{
		std::string command_line = "LC_TIME=C LS_COLORS= ls ";
		if (quirks.ls_supports_dash_f)
			command_line+= "-f ";

		command_line+= "-l -A ";

		if (dem != DEM_LIST)
			command_line+= "-d ";

		if (dem != DEM_QUERY)
			command_line+= "-H ";

		for (size_t i = 0; i < count; ++i) {
			command_line+= QuotedArg(EnsureNoSlashAtNestedEnd(paths[i]));
			command_line+= ' ';
		}
//		command_line+= "2>/dev/null";

		_conn->executed_command.reset();
		_conn->executed_command = std::make_shared<SSHExecutedCommand>(_conn, "", command_line, _cmd.fifo.FileName(), false);

		_cmd.Execute();
	}
};


std::shared_ptr<IProtocol> CreateProtocolSCP(const std::string &host, unsigned int port,
	const std::string &username, const std::string &password, const std::string &options)
{
//	sleep(20);
	return std::make_shared<ProtocolSCP>(host, port, username, password, options);
}

ProtocolSCP::ProtocolSCP(const std::string &host, unsigned int port,
	const std::string &username, const std::string &password, const std::string &options)
{
	_quirks.use_ls = false;
	_quirks.ls_supports_dash_f = true;
	_quirks.rm_file = "unlink";
	_quirks.rm_dir = "rmdir";

	StringConfig protocol_options(options);
	_conn = std::make_shared<SSHConnection>(host, port, username, password, protocol_options);
	_now.tv_sec = time(nullptr);

	SimpleCommand cmd(_conn);
	int rc = cmd.Execute("%s", "stat --format=\"%n %f %s %X %Y %Z %U %G\" -L .");
	if (rc != 0) {
		_quirks.use_ls = true;
		fprintf(stderr, "ProtocolSCP::ProtocolSCP: <stat .> result %d -> fallback to ls\n", rc);
	}

	int busybox_return_code = -1;
	switch (cmd.Execute("readlink /bin/sh")) {
		case 0:
			if (cmd.Output().find("busybox") == std::string::npos) {
				break;
			}
			// else: fallthrough
		default:
			// readlink not exists or /bin/sh not exists and also busybox exists?
			// Its enough arguments to assume that ls will be handled by busybox.
			busybox_return_code = cmd.Execute("busybox 2>&1");
	}

	if (busybox_return_code == 0 ||
		(busybox_return_code == 127 && cmd.Output().find("applet not found") != std::string::npos)) {
		if (busybox_return_code == 0) {
			// busybox found and returned list of available applets
			// some busybox systems may miss very usual things, analyze busybox command output
			// where it printed lit of supported commands
			std::vector<std::string> words;
			StrExplode(words, cmd.Output(), "\t ,");
			if (std::find(words.begin(), words.end(), _quirks.rm_file) == words.end()) {
				fprintf(stderr, "ProtocolSCP: '%s' unsupported\n", _quirks.rm_file);
				_quirks.rm_file = "rm -f";
			}
			if (std::find(words.begin(), words.end(), _quirks.rm_dir) == words.end()) {
				fprintf(stderr, "ProtocolSCP: '%s' unsupported\n", _quirks.rm_dir);
				_quirks.rm_dir = "rm -f -d";
			}
		} else { // busybox found but returned "busybox: applet not found"
			// some routers have rmdir, but have no unlink (and also no applets list from busybox w/o args)
			std::string probe_command = "ls /bin/"; probe_command += _quirks.rm_file;
			if (cmd.Execute(probe_command.c_str()) != 0) {
				fprintf(stderr, "ProtocolSCP: '%s' unsupported\n", _quirks.rm_file);
				_quirks.rm_file = "rm -f";
			}
		}

		_quirks.ls_supports_dash_f = false;
		fprintf(stderr, "ProtocolSCP: BusyBox detected, rc=%d rmd='%s' rmf='%s'\n",
			busybox_return_code, _quirks.rm_dir, _quirks.rm_file);
	}
}

ProtocolSCP::~ProtocolSCP()
{
	_conn->executed_command.reset();
}


void ProtocolSCP::GetModes(bool follow_symlink, size_t count, const std::string *paths, mode_t *modes) noexcept
{
#ifdef QUERY_BY_CMD
	size_t j = 0;
	try {
		_conn->executed_command.reset();

		std::shared_ptr<SCPDirectoryEnumer> de;
		if (_quirks.use_ls) {
			de = std::make_shared<SCPDirectoryEnumer_ls>(_conn, _quirks, follow_symlink ? DEM_QUERY_FOLLOW_SYMLINKS : DEM_QUERY, count, paths, _now);
		} else {
			de = std::make_shared<SCPDirectoryEnumer_stat>(_conn, follow_symlink ? DEM_QUERY_FOLLOW_SYMLINKS : DEM_QUERY, count, paths);
		}

		std::string name, owner, group;
		FileInformation file_info;
		while (j < count && de->Enum(name, owner, group, file_info)) {
			while (j < count) {
				size_t p = paths[j].rfind('/');
				const char *expected_name = (p == std::string::npos) ? paths[j].c_str() : paths[j].c_str() + p + 1;

				++j;
				if (name == expected_name) {
					modes[j - 1] = file_info.mode;
					break;
				}
				modes[j - 1] = ~(mode_t)0;
			}
		}

	} catch (std::exception &e) {
		fprintf(stderr, "%s: %s\n", __FUNCTION__, e.what());
	} catch (...) {
		fprintf(stderr, "%s: ...\n", __FUNCTION__);
	}
	for (; j < count; ++j) {
		modes[j] = ~(mode_t)0;
	}
#else
	IProtocol::GetModes(follow_symlink, count, paths, modes);
#endif
}

mode_t ProtocolSCP::GetMode(const std::string &path, bool follow_symlink)
{
#if SIMULATED_GETMODE_FAILS_RATE
	if ( (rand() % 100) + 1 <= SIMULATED_GETMODE_FAILS_RATE)
		throw ProtocolError("Simulated getmode error");
#endif

#ifdef QUERY_BY_CMD
	FileInformation file_info;
	GetInformation(file_info, path, follow_symlink);
	return file_info.mode;
#else
	SCPRequest scp (ssh_scp_new(_conn->ssh, SSH_SCP_READ | SSH_SCP_RECURSIVE, path.c_str()));// |
	int rc = ssh_scp_init(scp);
	if (rc != SSH_OK) {
		throw ProtocolError("SCP init error", ssh_get_error(_conn->ssh), rc);
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
			throw ProtocolError("Query mode error", ssh_get_error(_conn->ssh), rc);
		}
	}

	throw ProtocolError("Query mode fault", ssh_get_error(_conn->ssh), rc);
#endif
}

unsigned long long ProtocolSCP::GetSize(const std::string &path, bool follow_symlink)
{
#if SIMULATED_GETSIZE_FAILS_RATE
	if ( (rand() % 100) + 1 <= SIMULATED_GETSIZE_FAILS_RATE)
		throw ProtocolError("Simulated getsize error");
#endif

#ifdef QUERY_BY_CMD
	FileInformation file_info;
	GetInformation(file_info, path, follow_symlink);
	return file_info.size;
#else
	SCPRequest scp(ssh_scp_new(_conn->ssh, SSH_SCP_READ | SSH_SCP_RECURSIVE, path.c_str()));// | SSH_SCP_RECURSIVE
	int rc = ssh_scp_init(scp);
	if (rc != SSH_OK) {
		throw ProtocolError("SCP init error", ssh_get_error(_conn->ssh), rc);
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
			throw ProtocolError("Query size error", ssh_get_error(_conn->ssh), rc);
		}
	}

	throw ProtocolError("Query size fault", ssh_get_error(_conn->ssh), rc);
#endif
}

void ProtocolSCP::GetInformation(FileInformation &file_info, const std::string &path, bool follow_symlink)
{
#if SIMULATED_GETINFO_FAILS_RATE
	if ( (rand() % 100) + 1 <= SIMULATED_GETINFO_FAILS_RATE)
		throw ProtocolError("Simulated getinfo error");
#endif

#ifdef QUERY_BY_CMD
	_conn->executed_command.reset();

	std::shared_ptr<SCPDirectoryEnumer> de;
	if (_quirks.use_ls) {
		de = std::make_shared<SCPDirectoryEnumer_ls>(_conn, _quirks, follow_symlink ? DEM_QUERY_FOLLOW_SYMLINKS : DEM_QUERY, 1, &path, _now);
	} else {
		de = std::make_shared<SCPDirectoryEnumer_stat>(_conn, follow_symlink ? DEM_QUERY_FOLLOW_SYMLINKS : DEM_QUERY, 1, &path);
	}

	std::string name, owner, group;
	if (!de->Enum(name, owner, group, file_info)) {
		throw ProtocolError("Query info fault", de->FilteredError().c_str());
	}

#else

	SCPRequest scp (ssh_scp_new(_conn->ssh, SSH_SCP_READ | SSH_SCP_RECURSIVE, path.c_str()));// | SSH_SCP_RECURSIVE
	int rc = ssh_scp_init(scp);
	if (rc != SSH_OK) {
		throw ProtocolError("SCP init error", ssh_get_error(_conn->ssh), rc);
	}

	rc = ssh_scp_pull_request(scp);

	switch (rc) {
		case SSH_SCP_REQUEST_NEWFILE: case SSH_SCP_REQUEST_NEWDIR: {
			file_info.size = SSH_SCP_REQUEST_GET_SIZE(scp);
			file_info.mode = ssh_scp_request_get_permissions(scp);
			file_info.mode|= (rc == SSH_SCP_REQUEST_NEWFILE) ? S_IFREG : S_IFDIR;
			file_info.access_time = file_info.modification_time = file_info.status_change_time = _now;
			ssh_scp_deny_request(scp, "sorry");
			break;
		}

		case SSH_SCP_REQUEST_ENDDIR:
			throw ProtocolError("Query info ENDDIR", ssh_get_error(_conn->ssh), rc);

		case SSH_SCP_REQUEST_EOF:
			throw ProtocolError("Query info EOF", ssh_get_error(_conn->ssh), rc);

		case SSH_ERROR:
			throw ProtocolError("Query info error", ssh_get_error(_conn->ssh), rc);

		default:
			throw ProtocolError("Query info fault", ssh_get_error(_conn->ssh), rc);
	}
#endif

	if (_conn->file_stats_override)
		_conn->file_stats_override->FilterFileInformation(path, file_info);
}

void ProtocolSCP::FileDelete(const std::string &path)
{
	SimpleCommand sc(_conn);
	int rc = sc.Execute("%s %s", _quirks.rm_file, QuotedArg(path).c_str());
	if (rc != 0) {
		throw ProtocolError(sc.FilteredError().c_str(), rc);
	}
	if (_conn->file_stats_override)
		_conn->file_stats_override->Cleanup(path);
}

void ProtocolSCP::DirectoryDelete(const std::string &path)
{
	SimpleCommand sc(_conn);
	int rc = sc.Execute("%s %s", _quirks.rm_dir, QuotedArg(path).c_str());
	if (rc != 0) {
		throw ProtocolError(sc.FilteredError().c_str(), rc);
	}
	if (_conn->file_stats_override)
		_conn->file_stats_override->Cleanup(path);
}

void ProtocolSCP::DirectoryCreate(const std::string &path, mode_t mode)
{
	fprintf(stderr, "ProtocolSCP::DirectoryCreate mode=%o path=%s\n", mode, path.c_str());
	if (path == "." || path == "./") //wtf
		return;

#if SIMULATED_MKDIR_FAILS_RATE
	if ( (rand() % 100) + 1 <= SIMULATED_MKDIR_FAILS_RATE)
		throw ProtocolError("Simulated mkdir error");
#endif

	SCPRequest scp(ssh_scp_new(_conn->ssh, SSH_SCP_WRITE | SSH_SCP_RECURSIVE, ExtractFilePath(path).c_str()));//
	int rc = ssh_scp_init(scp);
	if (rc != SSH_OK) {
		throw ProtocolError("SCP init error", ssh_get_error(_conn->ssh), rc);
	}

	rc = ssh_scp_push_directory(scp, ExtractFileName(path).c_str(), mode & 0777);
	if (rc != SSH_OK) {
		throw ProtocolError("SCP push directory error", ssh_get_error(_conn->ssh), rc);
	}
}

void ProtocolSCP::Rename(const std::string &path_old, const std::string &path_new)
{
	SimpleCommand sc(_conn);
	int rc = sc.Execute("mv %s %s", QuotedArg(path_old).c_str(), QuotedArg(path_new).c_str());
	if (rc != 0) {
		throw ProtocolError(sc.FilteredError().c_str(), rc);
	}
	if (_conn->file_stats_override)
		_conn->file_stats_override->Rename(path_old, path_new);
}

void ProtocolSCP::SetTimes(const std::string &path, const timespec &access_time, const timespec &modification_time)
{
	SimpleCommand sc(_conn);

	char str_mt[32]{};
	struct tm t;
	localtime_r(&modification_time.tv_sec, &t);
	strftime(str_mt, sizeof(str_mt) - 1, "%Y%m%d%H%M.%S", &t);
	int rc = 0;
	if (access_time.tv_sec != modification_time.tv_sec) {
		char str_at[32]{};
		localtime_r(&access_time.tv_sec, &t);
		strftime(str_at, sizeof(str_at) - 1, "%Y%m%d%H%M.%S", &t);
		rc = sc.Execute("touch -t %s %s && touch -a -t %s %s",
			str_mt, QuotedArg(path).c_str(), str_at, QuotedArg(path).c_str());
	} else {
		rc = sc.Execute("touch -t %s %s", str_mt, QuotedArg(path).c_str());
	}

	if (rc != 0) {
		fprintf(stderr, "%s(%s) ignored error %d\n", __FUNCTION__, path.c_str(), rc);
		//throw ProtocolError(sc.FilteredError().c_str(), rc);
		if (_conn->file_stats_override)
			_conn->file_stats_override->OverrideTimes(path, access_time, modification_time);
	}
}

void ProtocolSCP::SetMode(const std::string &path, mode_t mode)
{
	SimpleCommand sc(_conn);
	int rc = sc.Execute("chmod %o %s", mode, QuotedArg(path).c_str());
	if (rc != 0) {
		if (!_conn->file_stats_override)
			throw ProtocolError(sc.FilteredError().c_str(), rc);

		fprintf(stderr, "%s(%s) ignored error %d\n", __FUNCTION__, path.c_str(), rc);
		_conn->file_stats_override->OverrideMode(path, mode);
	}
}


void ProtocolSCP::SymlinkCreate(const std::string &link_path, const std::string &link_target)
{
	SimpleCommand sc(_conn);
	int rc = sc.Execute("ln -s %s %s", QuotedArg(link_target).c_str(), QuotedArg(link_path).c_str());
	if (rc != 0) {
		throw ProtocolError(sc.FilteredError().c_str(), rc);
	}
}

void ProtocolSCP::SymlinkQuery(const std::string &link_path, std::string &link_target)
{
	SimpleCommand sc(_conn);
	int rc = sc.Execute("readlink %s", QuotedArg(link_path).c_str());
// fprintf(stderr, "ProtocolSCP::SymlinkQuery('%s') -> '%s' %d\n", link_path.c_str(), link_target.c_str(), rc);
	if (rc != 0) {
		throw ProtocolError(sc.FilteredError().c_str(), rc);
	}
	link_target = sc.Output();
	while (!link_target.empty() && (link_target[link_target.size() - 1] == '\r' || link_target[link_target.size() - 1] == '\n')) {
		link_target.resize(link_target.size() - 1);
	}
}

std::shared_ptr<IDirectoryEnumer> ProtocolSCP::DirectoryEnum(const std::string &path)
{
	_conn->executed_command.reset();

	std::shared_ptr<IDirectoryEnumer> enumer;
	if (_quirks.use_ls) {
		enumer = std::make_shared<SCPDirectoryEnumer_ls>(_conn, _quirks, DEM_LIST, 1, &path, _now);
	} else {
		enumer = std::make_shared<SCPDirectoryEnumer_stat>(_conn, DEM_LIST, 1, &path);
	}

	if (_conn->file_stats_override && _conn->file_stats_override->NonEmpty()) {
		enumer = std::make_shared<DirectoryEnumerWithFileStatsOverride>(*_conn->file_stats_override, enumer, path);
	}

	return enumer;
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
		_scp(ssh_scp_new(conn->ssh, SSH_SCP_READ, path.c_str()))
	{
		int rc = ssh_scp_init(_scp);
		if (rc != SSH_OK) {
			throw ProtocolError("SCP init error", ssh_get_error(_conn->ssh), rc);
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
					throw ProtocolError("Pull file error", ssh_get_error(_conn->ssh), rc);
				}
			}
		}
	}

	virtual ~SCPFileReader()
	{
	}


	virtual size_t Read(void *buf, size_t len)
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
			throw ProtocolError("Read file error", ssh_get_error(_conn->ssh), rc);

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
		_scp(ssh_scp_new(conn->ssh, SSH_SCP_WRITE, ExtractFilePath(path).c_str())),
		_pending_size(size_hint)
	{
		mode&= 0777;
		int rc = ssh_scp_init(_scp);
		if (rc != SSH_OK) {
			throw ProtocolError("SCP init error", ssh_get_error(_conn->ssh), rc);
		}
		rc = SSH_SCP_PUSH_FILE(_scp, ExtractFileName(path).c_str(), size_hint, mode);
		if (rc != SSH_OK) {
			throw ProtocolError("SCP push error", ssh_get_error(_conn->ssh), rc);
		}
	}

	virtual ~SCPFileWriter()
	{
	}

	virtual void WriteComplete()
	{
#if SIMULATED_WRITE_COMPLETE_FAILS_RATE
		if ( (rand() % 100) + 1 <= SIMULATED_WRITE_COMPLETE_FAILS_RATE)
			throw ProtocolError("Simulated write-complete file error");
#endif
		if (_truncated)
			throw ProtocolError("Excessive data truncated");
	}



	virtual void Write(const void *buf, size_t len)
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
				throw ProtocolError("SCP write error", ssh_get_error(_conn->ssh), rc);
			}
		}
	}
};

std::shared_ptr<IFileReader> ProtocolSCP::FileGet(const std::string &path, unsigned long long resume_pos)
{
	if (resume_pos) {
		throw ProtocolUnsupportedError("SCP doesn't support download resume");
	}

	_conn->executed_command.reset();
	return std::make_shared<SCPFileReader>(_conn, path);
}

std::shared_ptr<IFileWriter> ProtocolSCP::FilePut(const std::string &path, mode_t mode, unsigned long long size_hint, unsigned long long resume_pos)
{
	if (resume_pos) {
		throw ProtocolUnsupportedError("SCP doesn't support upload resume");
	}

	_conn->executed_command.reset();
	return std::make_shared<SCPFileWriter>(_conn, path, mode, size_hint);
}


void ProtocolSCP::ExecuteCommand(const std::string &working_dir, const std::string &command_line, const std::string &fifo)
{
	_conn->executed_command.reset();
	_conn->executed_command = std::make_shared<SSHExecutedCommand>(_conn, working_dir, command_line, fifo, true);
}

void ProtocolSCP::KeepAlive(const std::string &path_to_check)
{
	if (_conn->executed_command) {
		_conn->executed_command->KeepAlive();
	} else {
		GetMode(path_to_check);
	}
}
