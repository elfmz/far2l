#include <fcntl.h>
#include <pty.h>
#include <unistd.h>
#include <sys/wait.h>
#include <iostream>
#include <cstring>
#include <vector>
#include <sstream>
#include <iterator>
#include <iomanip>
#include <ctime>
#include <chrono>
#include <map>
#include <filesystem>

#include <poll.h>
#include <termios.h>  // Для работы с терминальными атрибутами
#include <algorithm>

enum OutputType
{
	STDBAD = 0,
    STDOUT,
    STDERR
};

struct WaitResult
{
    int error_code{0};
    int index{-1};
    OutputType output_type{STDBAD};
    std::string stdout_data;
    std::string stderr_data;
};

class FISHClient
{
    int _master_fd{(pid_t)-1};
    int _stderr_pipe[2]{-1, -1};
    pid_t _pid{(pid_t)-1};
	std::map<std::string, std::string> _substs;

	void ApplySubstitutions(std::string &str);

public:
	FISHClient();
    virtual ~FISHClient();

	bool OpenApp(const char *app, const char *arg);
	void SetSubstitution(const char *key, const std::string &value);
	WaitResult SendAndWaitReply(const std::string &send_str, const std::vector<std::string> &expected_replies);
	WaitResult SendHelperAndWaitReply(const char *helper, const std::vector<std::string> &expected_replies);
};

