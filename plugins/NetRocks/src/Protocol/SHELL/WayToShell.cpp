#include "WayToShell.h"
#include "Parse.h"
#include <utils.h>
#include <MakePTYAndFork.h>
#include <time.h>
#include <termios.h>  // Для работы с терминальными атрибутами
#include <signal.h>
#include <os_call.hpp>
#include <Environment.h>
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

WayToShell::WayToShell(int fd_ipc_recv, const WayToShellConfig &cfg, const StringConfig &protocol_options)
	: _fd_ipc_recv(fd_ipc_recv)
{
	if (!cfg.command.empty()) {
		LaunchCommand(cfg, protocol_options);
	} else if (!cfg.serial.empty()) {
		OpenSerialPort(cfg, protocol_options);
	} else {
		throw ProtocolError("Command or port not specified");
	}
}

// That long echo after exit makes things work smoother, dunno why.
// And it really needs to be rather long
static const char s_exit_cmd[] = "\nexit\necho =================================\n";

WayToShell::~WayToShell()
{
	try {
		while (FinalRead(100)) {
			usleep(1000);
		}
		if (write(_master_fd, s_exit_cmd, sizeof(s_exit_cmd) - 1) == sizeof(s_exit_cmd) - 1) {
			FinalRead(1000);
			fprintf(stderr, "~WayToShell: exit delivered\n");
		} else {
			perror("~WayToShell: write exit");
		}

	} catch (...) {
		fprintf(stderr, "~WayToShell: exit exception\n");
	}

	CheckedCloseFD(_master_fd);
	CheckedCloseFDPair(_stderr_pipe);
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

bool WayToShell::FinalRead(int tmout)
{
	struct pollfd pfd{};
	pfd.fd = _master_fd;
	pfd.events = POLLIN;
	const int r = os_call_int(poll, &pfd, (nfds_t)1, tmout);
	return (r > 0 && RecvPolledFD(pfd, STDOUT));
}

void WayToShell::LaunchCommand(const WayToShellConfig &cfg, const StringConfig &protocol_options)
{
	std::string command = cfg.command;
	for (unsigned i = cfg.options.size(); i--;) { // reversed order for proper replacement
		Substitute(command, StrPrintf("$OPT%u", i).c_str(), cfg.OptionValue(i, protocol_options));
	}
	if (g_netrocks_verbosity > 0) {
		fprintf(stderr, "WayToShell::LaunchCommand: '%s'\n", command.c_str());
	}

	if (pipe(_stderr_pipe) < 0) {
		throw ProtocolError("pipe error", errno);
	}

	struct Argv : std::vector<char *>
	{
		~Argv()
		{
			for (auto &arg : *this) {
				free(arg);
			}
		}
	} argv;

	Environment::ExplodeCommandLine ecl(command);
	for (const auto &arg : ecl) {
		argv.emplace_back(strdup(arg.c_str()));
	}
	argv.emplace_back(nullptr);

	_pid = MakePTYAndFork(_master_fd);

	if (_pid == (pid_t)-1) {
		throw ProtocolError("PTY error", errno);
	}

	if (_pid == 0) {
		CheckedCloseFD(_stderr_pipe[0]); // close read end
		dup2(_stderr_pipe[1], STDERR_FILENO); // redirect stderr to the pipe
		CheckedCloseFD(_stderr_pipe[1]); // close write end, as it's now duplicated

		setenv("LANG", "C", 1);
		setenv("TERM", "xterm-mono", 1);
		unsetenv("COLORFGBG");
		// Child process
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
		// ssh should terminate if terminal closed
		signal(SIGINT, SIG_DFL);
		signal(SIGHUP, SIG_DFL);
		signal(SIGPIPE, SIG_DFL);

		execvp(*argv.data(), argv.data());
		ForkSafePrint("execlp failed\n");
		_exit(-1);
	}

	// Parent process
	CheckedCloseFD(_stderr_pipe[1]); // close write end

	// Now you can read from _stderr_pipe[0] to get the stderr output
	// and from _master_fd to get the stdout output

	MakeFDNonBlocking(_master_fd);
	MakeFDNonBlocking(_stderr_pipe[0]);
}

void WayToShell::OpenSerialPort(const WayToShellConfig &cfg, const StringConfig &protocol_options)
{
	const auto &opt_baudrate = cfg.OptionValue(0, protocol_options);
	const auto &opt_flowctrl = cfg.OptionValue(1, protocol_options);
	const auto &opt_databits = cfg.OptionValue(2, protocol_options);
	const auto &opt_stopbits = cfg.OptionValue(3, protocol_options);
	const auto &opt_parity =   cfg.OptionValue(4, protocol_options);

	speed_t baudrate;
	switch (atoi(opt_baudrate.c_str())) {
		case 50: baudrate = B50; break;
		case 150: baudrate = B150; break;
		case 300: baudrate = B300; break;
		case 600: baudrate = B600; break;
		case 1200: baudrate = B1200; break;
		case 2400: baudrate = B2400; break;
		case 4800: baudrate = B4800; break;
		case 9600: baudrate = B9600; break;
		case 19200: baudrate = B19200; break;
		case 38400: baudrate = B38400; break;
		case 57600: baudrate = B57600; break;
		case 115200: baudrate = B115200; break;
#ifdef B230400
		case 230400: baudrate = B230400; break;
#endif
#ifdef B460800
		case 460800: baudrate = B460800; break;
#endif
#ifdef B500000
		case 500000: baudrate = B500000; break;
#endif
#ifdef B576000
		case 576000: baudrate = B576000; break;
#endif
#ifdef B921600
		case 921600: baudrate = B921600; break;
#endif
#ifdef B1000000
		case 1000000: baudrate = B1000000; break;
#endif
#ifdef B1152000
		case 1152000: baudrate = B1152000; break;
#endif
#ifdef B1500000
		case 1500000: baudrate = B1500000; break;
#endif
#ifdef B2000000
		case 2000000: baudrate = B2000000; break;
#endif
#ifdef B2500000
		case 2500000: baudrate = B2500000; break;
#endif
#ifdef B3000000
		case 3000000: baudrate = B3000000; break;
#endif
#ifdef B3500000
		case 3500000: baudrate = B3500000; break;
#endif
#ifdef B4000000
		case 4000000: baudrate = B4000000; break;
#endif
		default:
			throw ProtocolError("Bad baudrate", opt_baudrate.c_str());
	}

	_master_fd = open(cfg.serial.c_str(), O_RDWR);
	if (_master_fd == -1) {
		throw ProtocolError("open port error", cfg.serial.c_str(), errno);
	}

	struct termios tios{};
	if (tcgetattr(_master_fd, &tios) != 0) {
		const int err = errno;
		close(_master_fd);
		throw ProtocolError("tcgetattr error", cfg.serial.c_str(), err);
	}

	tios.c_cflag&= ~PARENB;
	tios.c_cflag&= ~CSTOPB;
	tios.c_cflag&= ~CSIZE;

	tios.c_cflag&= ~CRTSCTS;
	tios.c_cflag|= CREAD | CLOCAL;

	tios.c_lflag&= ~ISIG;
	tios.c_lflag&= ~ICANON;
	tios.c_lflag&= ~ECHO;
	tios.c_lflag&= ~ECHOE;
	tios.c_lflag&= ~ECHONL;

	tios.c_iflag&= ~(IXON | IXOFF | IXANY);
	tios.c_iflag&= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);

	tios.c_oflag &= ~OPOST;
	tios.c_oflag &= ~ONLCR;

	tios.c_cc[VTIME] = 0;
	tios.c_cc[VMIN] = 1;

	switch (atoi(opt_databits.c_str())) {
		case 5: tios.c_cflag|= CS5; break;
		case 6: tios.c_cflag|= CS6; break;
		case 7: tios.c_cflag|= CS7; break;
		case 8: tios.c_cflag|= CS8; break;
		default:
			close(_master_fd);
			throw ProtocolError("wrong databits", opt_databits.c_str());
	}

	if (opt_flowctrl == "Xon/Xoff") {
		tios.c_iflag|= (IXON | IXOFF);
	} else if (opt_flowctrl == "RTS/CTS") {
		tios.c_cflag|= CRTSCTS;
	} else if (opt_flowctrl != "None") {
		close(_master_fd);
		throw ProtocolError("wrong flowctrl", opt_flowctrl.c_str());
	}

	if (opt_stopbits == "2") {
		tios.c_cflag|= CSTOPB;
	} else if (opt_stopbits != "1") {
		close(_master_fd);
		throw ProtocolError("wrong stopbits", opt_stopbits.c_str());
	}

	if (opt_parity == "Odd") {
		tios.c_cflag|= PARENB;
		tios.c_cflag|= PARODD;

	} else if (opt_parity == "Even") {
		tios.c_cflag|= PARENB;
		tios.c_cflag&= ~PARODD;

	} else if (opt_parity != "None") {
		close(_master_fd);
		throw ProtocolError("wrong parity", opt_parity.c_str());
	}

//	cfmakeraw(&tios);
	cfsetispeed(&tios, baudrate);
	cfsetospeed(&tios, baudrate);

	if (tcsetattr(_master_fd, TCSANOW, &tios) != 0) {
		const int err = errno;
		close(_master_fd);
		throw ProtocolError("tcsetattr error", cfg.serial.c_str(), err);
	}

	if (pipe(_stderr_pipe) < 0) { // create dummy pipe, just for genericity
		const int err = errno;
		close(_master_fd);
		throw ProtocolError("pipe error", err);
	}

	MakeFDNonBlocking(_master_fd);
	MakeFDNonBlocking(_stderr_pipe[0]);
}

void WayToShell::GetDescriptors(int &fdinout, int &fderr)
{
	fdinout = (_master_fd != -1) ? dup(_master_fd) : -1;
	fderr = (_stderr_pipe[0] != -1) ? dup(_stderr_pipe[0]) : -1;
}

void WayToShell::ThrowIfAppExited()
{
	if (_pid != (pid_t)-1) {
//		usleep(10000);
		if (waitpid(_pid, &_pid_status, WNOHANG) == _pid) {
			_pid = (pid_t)-1;
			throw ProtocolError("client app exited", _pid_status);
		}
	}
}

bool WayToShell::RecvPolledFD(struct pollfd &fd, enum OutputType output_type)
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
		fprintf(stderr, " !!! error reading pty n=%ld err=%d\n", n, errno);
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
unsigned WayToShell::Xfer(const char *data, size_t len)
{
	unsigned out = 0;
	while ((!data && out == 0) || len) {
		struct pollfd fds[3]{};
		fds[0].fd = _master_fd;
		fds[0].events = (data && len) ? POLLIN | POLLOUT : POLLIN;
		fds[1].fd = _stderr_pipe[0];
		fds[1].events = POLLIN;
		fds[2].fd = _fd_ipc_recv;
		fds[2].events = 0; // this poll'ed only for errors

		const int r = os_call_int(poll, &fds[0], (nfds_t)3, 10000);
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
			if (data && len && (fds[0].revents & POLLOUT) != 0) {
				const ssize_t wr = write(_master_fd, data, len);
				if (wr >= 0) {
					len-= wr;
					if (len != 0) {
						data+= wr;
					}

				} else if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
					throw ProtocolError("pty write error", errno);
				}
			}
			if ( (fds[0].revents & (POLLERR | POLLHUP)) != 0
					|| (fds[1].revents & (POLLERR | POLLHUP)) != 0) {
				throw ProtocolError("pty disrupted", errno);
			}
			if ((fds[2].revents & (POLLERR | POLLHUP)) != 0) {
				throw ProtocolError("ipc disrupted", errno);
			}
		}
	}
	return out;
}

void WayToShell::RecvSomeStdout()
{
	try {
		while ((Xfer() & STDOUT) == 0) {
			;
		}
	} catch (std::exception &e) {
		if (_stderr_data.empty()) {
			throw;
		}
		std::string stderr_str(_stderr_data.data(), _stderr_data.size());
		throw ProtocolError(e.what(), stderr_str.c_str());
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

WaitResult WayToShell::SendAndWaitReplyInner(const std::string &send_str, const std::vector<const char *> &expected_replies)
{
	if (g_netrocks_verbosity > 2) {
		for (size_t index = 0; index != expected_replies.size(); ++index) {
			std::cerr << "Expect '" << expected_replies[index] << "' " << std::endl;
		}
	}
	WaitResult wr;
	try {
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

	} catch (std::exception &e) {
		std::string stderr_str;
		AppendTrimmedLines(stderr_str, wr.stderr_lines);
		if (!_stderr_data.empty()) {
			stderr_str+= '\n';
			stderr_str.append(_stderr_data.data(), _stderr_data.size());
		}
		if (stderr_str.empty()) {
			throw;
		}
		throw ProtocolError(e.what(), stderr_str.c_str());
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

WaitResult WayToShell::SendAndWaitReply(const std::string &send_str, const std::vector<const char *> &expected_replies, bool hide_in_log)
{
	if (g_netrocks_verbosity > 0) {
		if (hide_in_log) {
			DebugStr("SEND.HIDE", "???");
		} else {
			DebugStr("SEND", send_str);
		}
	}

	return SendAndWaitReplyInner(send_str, expected_replies);
}

WaitResult WayToShell::WaitReply(const std::vector<const char *> &expected_replies)
{
	return SendAndWaitReplyInner(std::string(), expected_replies);
}

void WayToShell::Send(const char *data, size_t len)
{
	if (g_netrocks_verbosity > 1) {
		DebugStr("SEND.BLOB", data, len);

	} else if (g_netrocks_verbosity > 0) {
		DebugStr("SEND.BLOB", StrPrintf("{%lu}", (unsigned long)len));
	}

	Xfer(data, len);
}

void WayToShell::Send(const char *data)
{
	const size_t len = strlen(data);
	if (g_netrocks_verbosity > 0) {
		DebugStr("SEND.PSZ", data, len);
	}

	Xfer(data, 	len);
}

void WayToShell::Send(const std::string &line)
{
	if (g_netrocks_verbosity > 0) {
		DebugStr("SEND.STR", line);
	}

	Xfer(line.c_str(), line.size());
}

void WayToShell::ReadStdout(void *buffer, size_t len)
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
