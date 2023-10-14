#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <iostream>
#include <cstring>
#include <vector>
#include <sstream>
#include <iterator>
#include <ctime>
#include <map>

#include <poll.h>
#include <algorithm>

#include "WayToShellConfig.h"
#include "Request.h"

enum OutputType
{
	STDBAD = 0,
	STDOUT = 1,
	STDERR = 2
};

struct WaitResult
{
	std::vector<std::string> stdout_lines;
	std::vector<std::string> stderr_lines;
	ssize_t index{-1};
	OutputType output_type{STDBAD};
};

class WayToShell
{
	int _fd_ipc_recv;
	int _master_fd{-1};
	int _stderr_pipe[2]{-1, -1};
	pid_t _pid{(pid_t)-1};
	int _pid_status{0};

	std::vector<char> _stdout_data, _stderr_data;

	void ThrowIfAppExited();

	bool RecvPolledFD(struct pollfd &fd, enum OutputType output_type);
	unsigned Xfer(const char *data = nullptr, size_t len = 0);
	void RecvSomeStdout();

	WaitResult SendAndWaitReplyInner(const std::string &send_str, const std::vector<const char *> &expected_replies);

	bool FinalRead(int tmout);

	void LaunchCommand(const WayToShellConfig &cfg, const StringConfig &protocol_options);
	void OpenSerialPort(const WayToShellConfig &cfg, const StringConfig &protocol_options);

public:
	WayToShell(int fd_ipc_recv, const WayToShellConfig &cfg, const StringConfig &protocol_options);
	virtual ~WayToShell();

	void GetDescriptors(int &fdinout, int &fderr);

	void Send(const char *data, size_t len);
	void Send(const char *data);
	void Send(const std::string &line);

	void ReadStdout(void *buffer, size_t len);

	WaitResult WaitReply(const std::vector<const char *> &expected_replies);
	WaitResult SendAndWaitReply(const std::string &send_str, const std::vector<const char *> &expected_replies, bool hide_in_log = false);
};
