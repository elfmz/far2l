#include <assert.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "utils.h"
#include "ExecAsync.h"
#include "ScopeHelpers.h"

ExecAsync::ExecAsync(const char *program) : _program(program)
{
	pipe_cloexec(_kill_fd);
}

ExecAsync::~ExecAsync()
{
	CheckedCloseFD(_kill_fd[1]);
	Wait();
	CheckedCloseFD(_kill_fd[0]);
}

void ExecAsync::AddArgument(const char *s)
{
	assert(!_started);
	_args.emplace_back(s);
}

void ExecAsync::DontCare()
{
	_dont_care = true;
}

void ExecAsync::Stdin(const std::vector<char> &v)
{
	assert(!_started);
	_stdin.insert(_stdin.end(), v.begin(), v.end());
}

bool ExecAsync::Start()
{
	if (_started) {
		fprintf(stderr, "ExecAsync: double start\n");
		return false;
	}
	_started = true;
	if (!StartThread()) {
		fprintf(stderr, "ExecAsync: start failed\n");
		_started = false;
		return false;
	}
	return true;
}

void ExecAsync::Kill(int sig)
{
	if (sig <= 0) {
		sig = SIGINT;
	}
	assert(_started);
	std::lock_guard<std::mutex> lock(_mtx);
	if (_pid != (pid_t)-1) {
		if (kill(_pid, sig) < 0) {
			fprintf(stderr, "ExecAsync: kill pid=%d sig=%d error %d\n", _pid, sig, errno);
		} else {
			fprintf(stderr, "ExecAsync: killed pid=%d sig=%d\n", _pid, sig);
		}
	}
}

void ExecAsync::KillSoftly()
{
	Kill(SIGINT);
}

void ExecAsync::KillHardly()
{
	Kill(SIGKILL);
	CheckedCloseFD(_kill_fd[1]);
}

bool ExecAsync::Wait(int timeout_msec)
{
	assert(_started);
	return WaitThread(timeout_msec);
}

int ExecAsync::ExecError()
{
	assert(_started);
	std::lock_guard<std::mutex> lock(_mtx);
	return _exec_error;
}

int ExecAsync::ExitSignal()
{
	assert(_started);
	std::lock_guard<std::mutex> lock(_mtx);
	return _exit_signal;
}

int ExecAsync::ExitCode()
{
	assert(_started);
	std::lock_guard<std::mutex> lock(_mtx);
	return _exit_code;
}

void ExecAsync::FetchStdout(std::vector<char> &content)
{
	assert(_started);
	std::lock_guard<std::mutex> lock(_mtx);
	content.swap(_stdout);
	_stdout.clear();
}

void ExecAsync::FetchStderr(std::vector<char> &content)
{
	assert(_started);
	std::lock_guard<std::mutex> lock(_mtx);
	content.swap(_stderr);
	_stderr.clear();
}

std::string ExecAsync::FetchStdout()
{
	assert(_started);
	std::lock_guard<std::mutex> lock(_mtx);
	std::string out(_stdout.data(), _stdout.size());
	_stdout.clear();
	return out;
}

std::string ExecAsync::FetchStderr()
{
	assert(_started);
	std::lock_guard<std::mutex> lock(_mtx);
	std::string out(_stderr.data(), _stderr.size());
	_stderr.clear();
	return out;
}


static size_t ReadFDIntoVector(std::vector<char> &v, int &fd, bool dont_care)
{
	if (dont_care) {
		v.clear();
	}
	size_t n = v.size();
	v.resize(n + 0x1000);
	ssize_t r = read(fd, v.data() + n, v.size() - n);
	if (r > 0) {
		v.resize(n + r);
		return r;
	}

	if (r == 0 || (r < 0 && errno != EINTR && errno != EAGAIN)) {
		CheckedCloseFD(fd);
	}
	v.resize(n);
	return 0;
}

void *ExecAsync::ThreadProc()
{
	FDPairScope in, out, err, xer;
	if (pipe_cloexec(in.fd) == -1 || pipe_cloexec(out.fd) == -1 || pipe_cloexec(err.fd) == -1 || pipe_cloexec(xer.fd) == -1) {
		return nullptr;
	}

	std::string print_str;
	if (!_program.empty()) {
		print_str = '[';
		print_str = _program;
		print_str+= ']';
	}
	std::vector<char *> argv(_args.size() + 1);
	for (size_t i = 0; i < _args.size(); ++i) {
		argv[i] = (char *)_args[i].c_str();
		if (!print_str.empty()) {
			print_str+= ' ';
		}
		print_str+= _args[i];
	}
	argv[_args.size()] = NULL;
	fprintf(stderr, "ExecAsync: %s\n", print_str.c_str());

	MakeFDNonCloexec(in.fd[0]);
	MakeFDNonCloexec(out.fd[1]);
	MakeFDNonCloexec(err.fd[1]);
	MakeFDNonCloexec(xer.fd[1]);

	fflush(stdout);
	fflush(stderr);

	pid_t pid = fork();
	if (pid == (pid_t)-1) {
		return nullptr;
	}

	if (!pid) {
		dup2(in.fd[0], STDIN_FILENO); close(in.fd[0]);
		dup2(out.fd[1], STDOUT_FILENO); close(out.fd[1]);
		dup2(err.fd[1], STDERR_FILENO); close(err.fd[1]);
		const char *program = _program.empty() ? argv[0] : _program.c_str();
		execvp(program, argv.data());
		int e = errno;
		fprintf(stderr, "%s trying to run [%s]\n", strerror(e), program);
		fflush(stderr);
		while (write(xer.fd[1], &e, sizeof(e)) < 0 && errno == EINTR) {
			usleep(1000);
		}
		close(xer.fd[1]);
		_exit(e);
	}

	{
		std::lock_guard<std::mutex> lock(_mtx);
		_pid = pid;
	}

	CheckedCloseFD(in.fd[0]);
	CheckedCloseFD(out.fd[1]);
	CheckedCloseFD(err.fd[1]);
	CheckedCloseFD(xer.fd[1]);

	MakeFDNonBlocking(in.fd[1]);
	MakeFDNonBlocking(out.fd[0]);
	MakeFDNonBlocking(err.fd[0]);
	MakeFDNonBlocking(xer.fd[0]);

	while (out.fd[0] != -1 || err.fd[0] != -1 || xer.fd[0] != -1) {
		fd_set read_fds, write_fds;
		FD_ZERO(&read_fds);
		FD_ZERO(&write_fds);
		int mfd = in.fd[1];
		if (in.fd[1] != -1) {
			FD_SET(in.fd[1], &write_fds);
		}
		if (_kill_fd[0] != -1) {
			FD_SET(_kill_fd[0], &read_fds);
			mfd = std::max(mfd, _kill_fd[0]);
		}
		if (out.fd[0] != -1) {
			FD_SET(out.fd[0], &read_fds);
			mfd = std::max(mfd, out.fd[0]);
		}
		if (err.fd[0] != -1) {
			FD_SET(err.fd[0], &read_fds);
			mfd = std::max(mfd, err.fd[0]);
		}
		if (xer.fd[0] != -1) {
			FD_SET(xer.fd[0], &read_fds);
			mfd = std::max(mfd, xer.fd[0]);
		}

		int r = select(mfd + 1, &read_fds, (in.fd[1] != -1) ? &write_fds : NULL, NULL, NULL);
		if (r < 0) {
			if (errno == EINTR || errno == EAGAIN) {
				continue;
			}
			fprintf(stderr, "ExecAsync: select error %d\n", errno);
			break;
		}
		if (r == 0) {
			fprintf(stderr, "ExecAsync: select timeout???\n");
			continue;
		}
        if (in.fd[1] != -1 && FD_ISSET(in.fd[1], &write_fds)) {
			if (_stdin.empty()) {
				CheckedCloseFD(in.fd[1]);
			} else {
				ssize_t r = write(in.fd[1], _stdin.data(), _stdin.size());
				if (r > 0) {
					_stdin.erase(_stdin.begin(), _stdin.begin() + r);
				}
			}
		}

        if (out.fd[0] != -1 && FD_ISSET(out.fd[0], &read_fds)) {
			std::lock_guard<std::mutex> lock(_mtx);
			ReadFDIntoVector(_stdout, out.fd[0], _dont_care);
		}

        if (err.fd[0] != -1 && FD_ISSET(err.fd[0], &read_fds)) {
			std::lock_guard<std::mutex> lock(_mtx);
			size_t r = ReadFDIntoVector(_stderr, err.fd[0], _dont_care);
			if (r > 0) {
				fwrite(&_stderr[_stderr.size() - r], 1, r, stderr);
			}
		}

        if (xer.fd[0] != -1 && FD_ISSET(xer.fd[0], &read_fds)) {
			std::lock_guard<std::mutex> lock(_mtx);
			ssize_t r = read(xer.fd[0], &_exec_error, sizeof(_exec_error));
			if (r == 0 || (r < 0 && errno != EINTR && errno != EAGAIN)) {
				CheckedCloseFD(xer.fd[0]);
			} else if (r > 0 && _exec_error == 0) {
				_exec_error = -1;
			}
		}

        if (_kill_fd[0] != -1 && FD_ISSET(_kill_fd[0], &read_fds)) {
			break; // hard exit
		}
	}

	for (;;) {
		int st{0};
		if (waitpid(pid, &st, 0) == pid) {
			if (WIFSIGNALED(st)) {
				std::lock_guard<std::mutex> lock(_mtx);
				_pid = -1;
				_exit_code = -1;
				_exit_signal = WTERMSIG(st);
				fprintf(stderr, "ExecAsync: pid=%u signal=%d (%s)\n", pid, _exit_signal, _program.c_str());
				if (!_exit_signal) {
					_exit_signal = -1;
				}
				break;
			}
			if (WIFEXITED(st)) {
				std::lock_guard<std::mutex> lock(_mtx);
				_pid = -1;
				_exit_code = WEXITSTATUS(st);
				_exit_signal = 0;
				fprintf(stderr, "ExecAsync: pid=%u exit=%d (%s)\n", pid, _exit_code, _program.c_str());
				break;
			}
		}
		usleep(1000);
	}

	return nullptr;
}

