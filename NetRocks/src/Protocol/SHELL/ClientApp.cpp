#include "ClientApp.h"
#include <utils.h>
#include <MakePTYAndFork.h>
#include <time.h>
#include <termios.h>  // Для работы с терминальными атрибутами
#include <signal.h>
#include <os_call.hpp>
#include <MatchWildcard.hpp>
#include "../../Erroring.h"

// use 'export NETROCKS_VERBOSE=1' or '..=2' to see protocol dumps from DebugStr
static void DebugStr(const char *info, const char *str, size_t len)
{
	std::string tmp;
	for (size_t i = 0; i <= len; ++i) {
		if (i == len) {
			; // nothing
		} else if ((unsigned char)str[i] < 32) {
			tmp+= StrPrintf("{%02x}", (unsigned char)str[i]);
			if (str[i] == '\r' && i + 1 < len && str[i + 1] == '\n') {
				continue;
			}
		} else {
			tmp+= str[i];
		}
		if (!tmp.empty() && (i == len || str[i] == '\r' || str[i] == '\n')) {
			std::cerr << info << ": " << tmp << std::endl;
			tmp.clear();
		}
	}
}

static void DebugStr(const char *info, const std::string &str)
{
	DebugStr(info, str.c_str(), str.size());
}

static void ForkSafePrint(const char *str)
{
	if (write(STDOUT_FILENO, str, strlen(str)) <= 0) {
		perror("write");
	}
}

ClientApp::ClientApp()
{
}

ClientApp::~ClientApp()
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

bool ClientApp::Open(const char *app, char *const *argv)
{
	if (pipe(_stderr_pipe) < 0) {
		std::cerr << "pipe failed\n";
		return false;
	}

	_pid = MakePTYAndFork(_master_fd);

	if (_pid == (pid_t)-1) {
		std::cerr << "forkpty failed\n";
		return false;
	}

	if (_pid == 0) {
		CheckedCloseFD(_stderr_pipe[0]); // close read end
		dup2(_stderr_pipe[1], STDERR_FILENO); // redirect stderr to the pipe
		CheckedCloseFD(_stderr_pipe[1]); // close write end, as it's now duplicated

		setenv("LANG", "C", 1);
		setenv("TERM", "xterm-mono", 1);
		unsetenv("COLORFGBG");
		// Child process
		// ***
		// Получаем текущие атрибуты терминала
		struct termios tios{};
		if (tcgetattr(STDIN_FILENO, &tios) == -1) {
			ForkSafePrint("tcgetattr failed\n");
			_exit(-1);
		}

		// disable echo, CR/LF translation and other nasty things
		cfmakeraw(&tios);

		// Применяем новые атрибуты
		if (tcsetattr(STDIN_FILENO, TCSANOW, &tios) == -1) {
			ForkSafePrint("tcsetattr failed\n");
			_exit(-1);
		}
		// ***
		// ssh should terminate if terminal closed
		signal(SIGINT, SIG_DFL);
		signal(SIGHUP, SIG_DFL);
		signal(SIGPIPE, SIG_DFL);

		execvp(app, argv);
		ForkSafePrint("execlp failed\n");
		_exit(-1);
	}

	// Parent process
	CheckedCloseFD(_stderr_pipe[1]); // close write end

	// Now you can read from _stderr_pipe[0] to get the stderr output
	// and from _master_fd to get the stdout output

	MakeFDNonBlocking(_master_fd);
	MakeFDNonBlocking(_stderr_pipe[0]);
	return true;
}

void ClientApp::GetDescriptors(int &fdinout, int &fderr)
{
	fdinout = (_master_fd != -1) ? dup(_master_fd) : -1;
	fderr = (_stderr_pipe[0] != -1) ? dup(_stderr_pipe[0]) : -1;
}

void ClientApp::ThrowIfAppExited()
{
	if (_pid != (pid_t)-1) {
//		usleep(10000);
		if (waitpid(_pid, &_pid_status, WNOHANG) == _pid) {
			_pid = (pid_t)-1;
		}
	}

	if (_pid == (pid_t)-1) {
		throw ProtocolError("client app exited", _pid_status);
	}
}

bool ClientApp::RecvPolledFD(struct pollfd &fd, enum OutputType output_type)
{
	if (!(fd.revents & POLLIN)) {
		return false;
	}

	std::vector<char> &data = (output_type == STDERR) ? _stderr_data : _stdout_data;
	const size_t ofs = data.size();
	const size_t piece = 4096;
	data.resize(data.size() + piece);
	ssize_t n = os_call_ssize(read, fd.fd, (void *)(data.data() + ofs), piece);
	if (n <= 0) {
		data.resize(ofs);
		throw ProtocolError("error reading pty", (output_type == STDERR) ? "stderr" : "stdout", errno);
	}
	data.resize(ofs + n);

//	if (g_netrocks_verbosity > 0) {
//		DebugStr((output_type == STDERR) ? "STDERR" : "STDOUT", data.data() + ofs, n);
//	}

	return true;
}

// if data is non-NULL then send that data, optionally recving incoming stuff
// if data is NULL then recv at least some incoming stuff
unsigned ClientApp::Xfer(const char *data, size_t len)
{
	unsigned out = 0;
	while ((!data && out == 0) || len) {
		struct pollfd fds[3]{};
		fds[0].fd = _master_fd;
		fds[0].events = (data && len) ? POLLIN | POLLOUT : POLLIN;
		fds[1].fd = _stderr_pipe[0];
		fds[1].events = POLLIN;
		const int r = os_call_int(poll, &fds[0], (nfds_t)2, 10000);
		if (r < 0) {
			throw ProtocolError("error polling pty", errno);
		}

		if (r == 0) {
			ThrowIfAppExited();

		} else {
			if (RecvPolledFD(fds[0], STDOUT)) {
				out|= STDOUT;
			}
			if (RecvPolledFD(fds[1], STDERR)) {
				out|= STDERR;
			}
			if (data && (fds[0].revents & POLLOUT) != 0) {
				const ssize_t wr = write(_master_fd, data, len);
				if (wr >= 0) {
					len-= wr;
					if (len != 0) {
						data+= wr;
					}

				} else if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
					throw ProtocolError("error writing pty", errno);
				}
			}
			if ( (fds[0].revents & (POLLERR | POLLHUP)) != 0
					|| (fds[1].revents & (POLLERR | POLLHUP)) != 0) {
				throw ProtocolError("pty endpoint error", errno);
			}
		}
	}
	return out;
}

void ClientApp::RecvSomeStdout()
{
	while ((Xfer() & STDOUT) == 0) {
		;
	}
}

static ssize_t DispatchExpectedReply(std::vector<std::string> &lines, std::vector<char> &incoming,
	const std::vector<const char *> &expected_replies)
{
	// fetch incoming data and pack it into lines only until found match with expected reply
	for (size_t left = 0, right = 0;;) {
		if (right == incoming.size() || incoming[right] == '\r' || incoming[right] == '\n' || incoming[right] == 0) {
			if (lines.empty() || lines.back().back() == '\n') {
				lines.emplace_back(incoming.data() + left, right - left);
			} else {
				lines.back().append(incoming.data() + left, right - left);
			}
			const bool append_lf = (right != incoming.size());
			if (right + 1 < incoming.size() && incoming[right] == '\r' && incoming[right + 1] == '\n') {
				right+= 2;
			} else if (right < incoming.size()) {
				right++;
			}
			left = right;
			if (!lines.back().empty()) {
				if (append_lf) {
					lines.back()+= '\n';
				}
				if (g_netrocks_verbosity > 1) {
					std::cerr << "Line '" << lines.back() << "' " << std::endl;
				}
				for (size_t index = 0; index != expected_replies.size(); ++index) {
					if (MatchWildcard(lines.back().c_str(), expected_replies[index])) {
						if (g_netrocks_verbosity > 2) {
							std::cerr << "Matched '" << expected_replies[index] << "' " << std::endl;
						}
						incoming.erase(incoming.begin(), incoming.begin() + right);
						return index;
					}
					if (g_netrocks_verbosity > 2) {
						std::cerr << "NOT Matched '" << expected_replies[index] << "' " << std::endl;
					}
				}
			} else {
				lines.pop_back();
			}
			if (right == incoming.size()) {
				incoming.clear();
				return -1;
			}
		} else {
			++right;
		}
	}
}

WaitResult ClientApp::SendAndWaitReplyInner(const std::string &send_str, const std::vector<const char *> &expected_replies)
{
	if (g_netrocks_verbosity > 2) {
		for (size_t index = 0; index != expected_replies.size(); ++index) {
			std::cerr << "Expect '" << expected_replies[index] << "' " << std::endl;
		}
	}
	WaitResult wr;
	for (bool first = true;;first = false) {
		unsigned xrv;
		if (first) {
			xrv = STDOUT | STDERR;
			if (!send_str.empty()) {
				xrv|= Xfer(send_str.c_str(), send_str.size());
			}
		} else {
			xrv = Xfer();
		}
		if (xrv & STDOUT) {
			wr.index = DispatchExpectedReply(wr.stdout_lines, _stdout_data, expected_replies);
			if (wr.index != -1) {
				wr.output_type = STDOUT;
				break;
			}
		}
		if (xrv & STDERR) {
			wr.index = DispatchExpectedReply(wr.stderr_lines, _stderr_data, expected_replies);
			if (wr.index != -1) {
				wr.output_type = STDERR;
				break;
			}
		}
	}

	if (g_netrocks_verbosity > 0 && !wr.stdout_lines.empty()) {
		for (const auto &line : wr.stdout_lines) {
			DebugStr("STDOUT.RPL", line);
		}
	}

	if (g_netrocks_verbosity > 0 && !wr.stderr_lines.empty()) {
		for (const auto &line : wr.stderr_lines) {
			DebugStr("STDERR.RPL", line);
		}
	}

	return wr;
}

WaitResult ClientApp::SendAndWaitReply(const std::string &send_str, const std::vector<const char *> &expected_replies, bool hide_in_log)
{
	if (g_netrocks_verbosity > 0) {
		if (hide_in_log) {
			DebugStr("SEND.HIDE", "***");
		} else {
			DebugStr("SEND", send_str);
		}
	}

	return SendAndWaitReplyInner(send_str, expected_replies);
}

WaitResult ClientApp::WaitReply(const std::vector<const char *> &expected_replies)
{
	return SendAndWaitReplyInner(std::string(), expected_replies);
}

void ClientApp::Send(const char *data, size_t len)
{
	if (g_netrocks_verbosity > 1) {
		DebugStr("SEND.BLOB", data, len);

	} else if (g_netrocks_verbosity > 0) {
		DebugStr("SEND.BLOB", StrPrintf("{%lu}", (unsigned long)len));
	}

	Xfer(data, len);
}

void ClientApp::Send(const char *data)
{
	const size_t len = strlen(data);
	if (g_netrocks_verbosity > 0) {
		DebugStr("SEND.PSZ", data, len);
	}

	Xfer(data, 	len);
}

void ClientApp::Send(const std::string &line)
{
	if (g_netrocks_verbosity > 0) {
		DebugStr("SEND.STR", line);
	}

	Xfer(line.c_str(), line.size());
}

void ClientApp::ReadStdout(void *buffer, size_t len)
{
	for (size_t ofs = 0;;) {
		size_t piece = std::min(len - ofs, _stdout_data.size());
		if (piece) {
			memcpy((char *)buffer + ofs, _stdout_data.data(), piece);
			_stdout_data.erase(_stdout_data.begin(), _stdout_data.begin() + piece);
			ofs+= piece;
		}
		if (ofs == len) {
			break;
		}
		RecvSomeStdout();
	}

	if (g_netrocks_verbosity > 1) {
		DebugStr("STDOUT.BLOB", (const char *)buffer, len);
	} else if (g_netrocks_verbosity > 0) {
		const auto &info = StrPrintf("{%lu}", (unsigned long)len);
		DebugStr("STDOUT.BLOB", info);
	}
}

std::string MultiLineRequest(std::string line)
{
	line+= '\n';
	return line;
}
