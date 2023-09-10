#include "FISHClient.h"
#include <fstream>
#include <utils.h>
#include <os_call.hpp>

static void ForkSafePrint(const char *str)
{
	if (write(STDOUT_FILENO, str, strlen(str)) <= 0) {
		perror("write");
	}
}

FISHClient::FISHClient()
{
}

FISHClient::~FISHClient()
{
	// Закрыть файловые дескрипторы
	CheckedCloseFD(_master_fd);
	CheckedCloseFD(_stderr_pipe[0]);
	if (_pid != (pid_t)-1) {
		// Проверяем, завершен ли дочерний процесс
		int status;
		if (waitpid(_pid, &status, WNOHANG) == 0) {
			// Если нет, отправляем сигнал для завершения
			kill(_pid, SIGTERM);
			waitpid(_pid, &status, 0); // Ожидаем завершения
		}
		std::cout << "Child exited with status " << status << '\n';
	}
}

void FISHClient::SetSubstitution(const char *key, const std::string &value)
{
	_substs[key] = value;
}

void FISHClient::ApplySubstitutions(std::string &str)
{
	for (const auto &s : _substs) {
		for (size_t ofs = 0;;) {
			size_t p = str.find(s.first, ofs);
			if (p == std::string::npos) {
				break;
			}
			str.replace(p, s.first.size(), s.second);
			ofs = p + s.second.size();
		}
	}
}

bool FISHClient::OpenApp(const char *app, const char *arg)
{
	if (pipe(_stderr_pipe) < 0) {
		std::cerr << "pipe failed\n";
		return false;
	}

	_pid = forkpty(&_master_fd, nullptr, nullptr, nullptr);

	if (_pid == (pid_t)-1) {
		std::cerr << "forkpty failed\n";
		return false;
	}

	if (_pid == 0) {
		// Child process
		// ***
		// Получаем текущие атрибуты терминала
		struct termios term;
		if (tcgetattr(STDIN_FILENO, &term) == -1) {
			ForkSafePrint("tcgetattr failed\n");
			_exit(-1);
		}

		// disable echo, CR/LF translation and other nasty things
		cfmakeraw(&term);

		// Применяем новые атрибуты
		if (tcsetattr(STDIN_FILENO, TCSANOW, &term) == -1) {
			ForkSafePrint("tcsetattr failed\n");
			_exit(-1);
		}
		// ***

		CheckedCloseFD(_stderr_pipe[0]); // close read end
		dup2(_stderr_pipe[1], STDERR_FILENO); // redirect stderr to the pipe
		CheckedCloseFD(_stderr_pipe[1]); // close write end, as it's now duplicated
		execlp(app, app, arg, (char*) nullptr);
		ForkSafePrint("execlp failed\n");
		_exit(-1);
	}

	// Parent process
	CheckedCloseFD(_stderr_pipe[1]); // close write end

	// Now you can read from _stderr_pipe[0] to get the stderr output
	// and from _master_fd to get the stdout output

	// Учтём, что в fish_pipeopen, на основе которого написана эта функция, stderr
	// просто отбрасывается.
	// Так что мы, по идее, могли бы использовать и предыдущий (проверенный) вариант openApp(),
	// где stdout и stderr смешиваются:
	/*
		bool openApp(const char* app, const char* arg) {
			_pid = forkpty(&_master_fd, nullptr, nullptr, nullptr);

			if (_pid < 0) {
				std::cerr << "forkpty failed\n";
				return false;
			} else if (_pid == 0) {
				// Child process
				execlp(app, app, arg, (char*) nullptr);
				std::cerr << "execlp failed\n";
				return false;
			}
			return true;
		}
		*/
	return true;
}

WaitResult FISHClient::SendHelperAndWaitReply(const char *helper, const std::vector<std::string> &expected_replies)
{
	std::ifstream helper_ifs;
	helper_ifs.open(helper);
	std::string send_str, tmp_str;
	if (!helper_ifs.is_open() ) {
		fprintf(stderr, "Can't open helper '%s'\n", helper);
		WaitResult result;
		result.error_code = -1;
		return result;
	}

	while (std::getline(helper_ifs, tmp_str)) {
		ApplySubstitutions(tmp_str);
		send_str+= tmp_str;
		send_str+= '\n';
	}

	return SendAndWaitReply(send_str, expected_replies);
}

static void SanitizeEOL(std::string &s, size_t ofs)
{
	for (;;) {
		size_t p = s.find('\r', ofs);
		if (p == std::string::npos) {
			break;
		}
		if (p + 1 < s.size() && s[p + 1] == '\n') {
			s.erase(p, 1);
			ofs = p;
		} else {
			s[p] = '\n';
			ofs = p + 1;
		}
	}
}

ssize_t FISHClient::ReadStdout(void *buffer, size_t len)
{
	return os_call_ssize(read, _master_fd, (void *)buffer, len);
}

WaitResult FISHClient::SendAndWaitReply(const std::string &send_str, const std::vector<std::string> &expected_replies)
{
	WaitResult wr;

	if (WriteAll(_master_fd, send_str.c_str(), send_str.length()) != send_str.length()) {
		wr.error_code = errno ? errno : -1;
		return wr;
	}

	struct pollfd fds[2];
	fds[0].fd = _master_fd;
	fds[0].events = POLLIN;
	fds[1].fd = _stderr_pipe[0];
	fds[1].events = POLLIN;

	while (os_call_int(poll, &fds[0], (nfds_t)2, -1) >= 0) {
		char buffer[2048];

		if (fds[0].revents & POLLIN) {
			int n = read(_master_fd, buffer, sizeof(buffer));
			if (n > 0) {
				wr.stdout_data.append(buffer, n);
				SanitizeEOL(wr.stdout_data, wr.stdout_data.size() - n);
				//std::cout << "Debug (stdout): " << buffer << std::endl;  // ***
			}
			fds[0].revents &= ~POLLIN;
		}

		if (fds[1].revents & POLLIN) {
			int n = read(_stderr_pipe[0], buffer, sizeof(buffer));
			if (n > 0) {
				wr.stderr_data.append(buffer, n);
				SanitizeEOL(wr.stderr_data, wr.stderr_data.size() - n);
				//std::cout << "Debug (stderr): " << buffer << std::endl;  // ***
			}
			fds[1].revents &= ~POLLIN;
		}

		for (wr.index = 0; wr.index != (int)expected_replies.size(); ++wr.index) {
			wr.pos = wr.stdout_data.find(expected_replies[wr.index]);
			if (wr.pos != std::string::npos) {
				wr.output_type = STDOUT;
				return wr;
			}

			wr.pos = wr.stderr_data.find(expected_replies[wr.index]);
			if (wr.pos != std::string::npos) {
				wr.output_type = STDERR;
				return wr;
			}
		}
	}

	wr.error_code = errno ? errno : -2;
	return wr;
}
