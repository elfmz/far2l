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
		std::cerr << "Child exited with status " << status << '\n';
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

WaitResult FISHClient::SendHelperAndWaitReply(const char *helper, const std::vector<const char *> &expected_replies)
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
		send_str+= '\r';
	}

	return SendAndWaitReply(send_str, expected_replies);
}

ssize_t FISHClient::ReadStdout(void *buffer, size_t len)
{
	return os_call_ssize(read, _master_fd, (void *)buffer, len);
}

static bool RecvPollFD(WaitResult &wr, const std::vector<const char *> &expected_replies, struct pollfd &fd, enum OutputType output_type)
{
	if (!(fd.revents & POLLIN)) {
		return false;
	}
	fd.revents&= ~POLLIN;
	std::string &data = (output_type == STDERR) ? wr.stderr_data : wr.stdout_data;

	char buffer[2048];
	ssize_t n = read(fd.fd, buffer, sizeof(buffer));
	if (n <= 0) {
		fprintf(stderr, "%s: read error %d\n", __FUNCTION__, errno);
		return false;
	}

#if 0 // 1 for debug output
	std::string dbgstr(buffer, n);
	for  (size_t i = dbgstr.size(); i--; ) {
		if (dbgstr[i] < 32) {
			dbgstr.replace(i, 1, StrPrintf("{%02x}", (unsigned char)dbgstr[i]));
		}
	}
	std::cerr << ((output_type == STDERR) ? "STDERR: " : "STDOUT: ") << dbgstr << std::endl;
#endif

	// tricky code below here to convert lineendings until found any expected reply match and
	// DONT convert lineendings after match cause it may be heading part of file data being got
	size_t line_start_pos = data.rfind('\n');
	if (line_start_pos != std::string::npos) {
		++line_start_pos;
	} else {
		line_start_pos = 0;
	}
	data.append(buffer, n);

	do { // loop line by line, converting CR to LF until found matching line
		size_t line_end_pos = data.find_first_of("\r\n", line_start_pos);
		if (line_end_pos == std::string::npos) {
			line_end_pos = data.size();

		} else if (data[line_end_pos] == '\r') {
			if (line_end_pos + 1 < data.size() && data[line_end_pos + 1] == '\n') {
				data.erase(line_end_pos, 1);
			} else {
				data[line_end_pos] = '\n';
			}
		}
		// capture line to check against expected replies including bounding LFs if any
		if (line_start_pos) {
			--line_start_pos;
		}
		const auto &line = data.substr(line_start_pos, std::min(line_end_pos + 1, data.size()) - line_start_pos);

//		std::cerr << "Line '" << line << "' " << line_start_pos << " .. " << line_end_pos<< std::endl;
		for (wr.index = 0; wr.index != expected_replies.size(); ++wr.index) {
			const size_t pos = line.find(expected_replies[wr.index]);
			if (pos != std::string::npos) {
//				std::cerr << "Matched '" << expected_reply << "' " << std::endl;
				wr.pos = pos + line_start_pos;
				wr.output_type = output_type;
				return true;
			}
//			std::cerr << "NOT Matched '" << expected_reply << "' " << std::endl;
		}

		line_start_pos = line_end_pos + 1;

	} while (line_start_pos < data.size());

	return false;
}

WaitResult FISHClient::SendAndWaitReply(const std::string &send_str, const std::vector<const char *> &expected_replies)
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
		if (RecvPollFD(wr, expected_replies, fds[0], STDOUT)
		 || RecvPollFD(wr, expected_replies, fds[1], STDERR)) {
			return wr;
		}
	}

	wr.error_code = errno ? errno : -2;
	return wr;
}
