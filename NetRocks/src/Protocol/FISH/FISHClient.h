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
#include <filesystem>

#include <poll.h>
#include <termios.h>  // Для работы с терминальными атрибутами
#include <algorithm>

enum OutputType
{
    STDOUT,
    STDERR
};

struct WaitResult
{
    int error_code;
    int index;
    OutputType output_type;
    std::string stdout_data;
    std::string stderr_data;
};

struct FileInfo
{
    int permissions;
    std::string owner;
    std::string group;
    long size;
//    std::filesystem::file_time_type modified_time;
//    std::filesystem::path path;
//    std::filesystem::path symlink_path;
    std::string path;
    std::string symlink_path;
    bool is_directory;
};

class FISHClient
{
    int _master_fd{(pid_t)-1};
    int _stderr_pipe[2]{-1, -1};
    pid_t _pid{(pid_t)-1};

public:
    virtual ~FISHClient();

	bool OpenApp(const char *app, const char *arg);
	WaitResult WaitFor(const std::vector<std::string>& commands, const std::vector<std::string>& expectedStrings);
	std::vector<FileInfo> ParseLs(const std::string& buffer);
};

