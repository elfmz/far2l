#pragma once
#include <memory>
#include <string>
#include <set>
#include <map>
#include <Threaded.h>
#include <StringConfig.h>
#include <libssh/libssh.h>
#include <libssh/ssh2.h>


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


#define RESOURCE_CONTAINER(CONTAINER, RESOURCE, DELETER) 		\
class CONTAINER {						\
	RESOURCE _res = {};							\
	CONTAINER(const CONTAINER &) = delete;	\
public:									\
	operator RESOURCE() { return _res; }				\
	RESOURCE operator ->() { return _res; }				\
	CONTAINER() = default;					\
	CONTAINER(RESOURCE res) : _res(res) {;}			\
	CONTAINER &operator= (RESOURCE res) { DELETER(_res); _res = res; return *this; } 	\
	~CONTAINER() { DELETER(_res); }				\
};

void SSHSessionDeleter(ssh_session res);
void SSHChannelDeleter(ssh_channel res);

RESOURCE_CONTAINER(SSHSession, ssh_session, SSHSessionDeleter);
RESOURCE_CONTAINER(SSHChannel, ssh_channel, SSHChannelDeleter);

struct SSHConnection;

class SSHExecutedCommand : protected Threaded
{
	std::shared_ptr<SSHConnection> _conn;
	std::string _working_dir;
	std::string _command_line;
	std::string _fifo;
	SSHChannel _channel;
	int _kickass[2] {-1, -1};
	bool _pty = false;
	bool _succeess = false;

	void OnReadFDIn(const char *buf, size_t len);
	virtual void SendSignal(int sig);
	void OnReadFDCtl(int fd);
	void IOLoop();
	virtual void *ThreadProc();

public:
	SSHExecutedCommand(std::shared_ptr<SSHConnection> conn, const std::string &working_dir, const std::string &command_line, const std::string &fifo, bool pty);
	virtual ~SSHExecutedCommand();

	void KeepAlive();
};


struct SSHConnection
{
	std::shared_ptr<SSHExecutedCommand> executed_command;
	std::map<std::string, std::string> env_set {{"TERM", "xterm"}};

	SSHSession ssh;

	SSHConnection(const SSHConnection&) = delete;

	SSHConnection(const std::string &host, unsigned int port, const std::string &username,
		const std::string &password, const StringConfig &protocol_options);
	virtual ~SSHConnection();
};
