#include <sys/stat.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <vector>
#include <map>
#include <string>
#include <fstream>
#include <utils.h>
#include <os_call.hpp>
#include <RandomString.h>
#include "ProtocolSHELL.h"
#include "Request.h"

// very poor for now

void ProtocolSHELL::ExecCmd::OnReadFDCtl(int fd)
{
	ExecFIFO_CtlMsg m;
	if (ReadAll(fd, &m, sizeof(m)) != sizeof(m)) {
		throw std::runtime_error("ctl read failed");
	}

	switch (m.cmd) {
		case ExecFIFO_CtlMsg::CMD_PTY_SIZE: {
			// m.u.pty_size.cols, m.u.pty_size.rows
		} break;

		case ExecFIFO_CtlMsg::CMD_SIGNAL: {
			if (m.u.signum == SIGKILL || m.u.signum == SIGINT || m.u.signum == SIGQUIT) {
				char c = 0;
				if (os_call_ssize(write, _kickass[1], (const void*)&c, sizeof(c)) != 1) {
					perror("~ExecFIFO_CtlMsg::CMD_SIGNAL: write kickass");
				}
			}
			// SendSignal(m.u.signum);
		} break;
	}
}

void ProtocolSHELL::ExecCmd::MarkerTrack::Inspect(const char *append, ssize_t &len)
{
	size_t tail_len_before = _tail.size();
	_tail.append(append, len);
	if (_matched) {
		len = 0;
	}
	for (;;) {
		size_t p = _tail.find(marker);
		if (p == std::string::npos) {
			break;
		}
		if (!_matched) {
			_matched = true;
			len = p - tail_len_before;
			ASSERT(len >= 0);
			status = atoi(_tail.c_str() + p + marker.size());
		} else {
			done = true;
			break;
		}
	}
	if (_tail.size() > marker.size()) {
		_tail.erase(0, _tail.size() - marker.size());
	}
}

static void WriteAllCRLF(int fd, char *data, size_t len)
{
	for (size_t i = 0, j = 0; ; ++i) {
		if (i == len || data[i] == '\n') {
			WriteAll(fd, &data[j], i - j);
			if (i != len) {
				WriteAll(fd, "\r\n", 2);
			}
			if (i == len) {
				break;
			}
			j = i + 1;
		}
	}
}

void ProtocolSHELL::ExecCmd::IOLoop()
{
	FDScope fd_err(open((_fifo + ".err").c_str(), O_WRONLY | O_CLOEXEC));
	FDScope fd_out(open((_fifo + ".out").c_str(), O_WRONLY | O_CLOEXEC));
	FDScope fd_in(open((_fifo + ".in").c_str(), O_RDONLY | O_CLOEXEC));
	FDScope fd_ctl(open((_fifo + ".ctl").c_str(), O_RDONLY | O_CLOEXEC));

	if (!fd_err.Valid() || !fd_out.Valid() || !fd_in.Valid() || !fd_ctl.Valid())
		throw ProtocolError("fifo");

	// get PTY size that is sent immediately
	OnReadFDCtl(fd_ctl);

	MakeFDNonBlocking(_fdinout);
	const int maxfd = std::max((int)fd_in, std::max((int)fd_ctl, std::max((int)_fdinout, std::max((int)_fderr, _kickass[0]))));
	for (unsigned int idle = 0; !_marker_track.done;) {
		char buf[0x1000];
		fd_set fdr, fde;
		FD_ZERO(&fdr);
		FD_ZERO(&fde);

		FD_SET(fd_in, &fdr);		// data from local side
		FD_SET(fd_ctl, &fdr);
		FD_SET(_fdinout, &fdr);		// data from remote side
		FD_SET(_fderr, &fdr);		// data from remote side
		FD_SET(_kickass[0], &fdr);

		FD_SET(fd_in, &fde);
		FD_SET(fd_ctl, &fde);
		FD_SET(_fdinout, &fde);
		FD_SET(_fderr, &fde);
		FD_SET(_kickass[0], &fdr);

		struct timeval tv = {0, (idle > 1000) ? 100000 : ((idle > 0) ? 10000 : 0)};
		if (idle < 100000) {
			++idle;
		}
		int r = select(maxfd + 1, &fdr, nullptr, &fde, &tv);
		if ( r < 0) {
			if (errno == EAGAIN || errno == EINTR)
				continue;

			throw std::runtime_error("select failed");
		}

		if (FD_ISSET(_kickass[0], &fde)) {
			throw std::runtime_error("kickass exception");
		}

		if (FD_ISSET(_kickass[0], &fdr)) {
			char c = 0;
			if (os_call_ssize(read, _kickass[0], (void*)&c, sizeof(c)) != 1) {
				throw std::runtime_error("kickass read failed");
			}
			if (c == 0) {
				throw std::runtime_error("kickass-driven exit");
			}
			// TODO: somehow keepalive
		}

		if (FD_ISSET(fd_in, &fdr)) {
			ssize_t rlen = read(fd_in, buf, sizeof(buf));
			if (rlen <= 0) {
				if (errno != EAGAIN) {
					throw std::runtime_error("fd_in read failed");
				}
			} else {
				WriteAll(_fdinout, buf, rlen);
				for (ssize_t i = 0; i < rlen; ++i) {
					if (buf[i] == 0x03 || buf[i] == 0x1c) {
						// hack: handle Ctrl+C by force exiting IO loop, this will leave whole connection in broken state
						// but its better than not to react at all leaving user with unresponsive command
						_marker_track.done = true;
						_marker_track.status = (buf[i] == 0x1c) ? 0 : SIGINT;
						_broken = true;
					}
				}
				idle = 0;
			}
		}
		if (FD_ISSET(fd_ctl, &fdr)) {
			OnReadFDCtl(fd_ctl);
			idle = 0;
		}

		if (FD_ISSET(_fderr, &fdr)) {
			ssize_t rlen = read(_fderr, buf, sizeof(buf));
			if (rlen <= 0) {
				if (errno != EAGAIN) {
					throw std::runtime_error("_fderr read failed");
				}
			} else {
				WriteAllCRLF(fd_err, buf, rlen);
				idle = 0;
			}
		}

		if (FD_ISSET(_fdinout, &fdr)) {
			ssize_t rlen = read(_fdinout, buf, sizeof(buf));
			if (rlen <= 0) {
				if (errno != EAGAIN) {
					throw std::runtime_error("_fdinout read failed");
				}
			} else {
				_marker_track.Inspect(buf, rlen);
				WriteAllCRLF(fd_out, buf, rlen);
				idle = 0;
			}
		}
	}

	FDScope fd_status(open((_fifo + ".status").c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0600));
	if (fd_status.Valid()) {
		WriteAll(fd_status, &_marker_track.status, sizeof(_marker_track.status));
	}
}

void *ProtocolSHELL::ExecCmd::ThreadProc()
{
	try {
		fprintf(stderr, "[SHELL] ShellExecutedCommand: ENTERING [%s]\n", _command_line.c_str());
		IOLoop();
		fprintf(stderr, "[SHELL] ShellExecutedCommand: LEAVING\n");
	} catch (std::exception &ex) {
		fprintf(stderr, "[SHELL] ShellExecutedCommand: %s\n", ex.what());
	}

	return nullptr;
}

ProtocolSHELL::ExecCmd::ExecCmd(std::shared_ptr<WayToShell> &way, const std::string &working_dir, const std::string &command_line, const std::string &fifo)
	:
	_working_dir(working_dir),
	_command_line(command_line),
	_fifo(fifo)
{
	if (pipe_cloexec(_kickass) == -1) {
		throw ProtocolError("pipe", errno);
	}

	MakeFDNonBlocking(_kickass[1]);

	int fdinout, fderr;
	way->GetDescriptors(fdinout, fderr);
	_fdinout = fdinout;
	_fderr = fderr;

	RandomStringAppend(_marker_track.marker, 32, 32, RNDF_ALNUM);
	way->Send( Request("exec ").Add(command_line, '\n').Add(_marker_track.marker, ' ').Add(working_dir, '\n'));
	_marker_track.marker.insert(0, 1, '\n');
	_marker_track.marker.append(1, ':');

	if (!StartThread()) {
		CheckedCloseFDPair(_kickass);
		way->WaitReply({">(((^>\n"});
		throw std::runtime_error("start thread");
	}
}

ProtocolSHELL::ExecCmd::~ExecCmd()
{
	try {
		Abort();
	} catch(...) {
	}
}

void ProtocolSHELL::ExecCmd::Abort()
{
	if (!WaitThread(0)) {
		char c = 0;
		if (os_call_ssize(write, _kickass[1], (const void*)&c, sizeof(c)) != 1) {
			perror("~ShellExecutedCommand: write kickass");
		}
		WaitThread();
	}
}

bool ProtocolSHELL::ExecCmd::KeepAlive()
{
	return !WaitThread(0);
}

void ProtocolSHELL::ExecuteCommand(const std::string &working_dir, const std::string &command_line, const std::string &fifo)
{
	_exec_cmd.reset();
	_exec_cmd.reset(new ExecCmd(_way, working_dir, command_line, fifo));
}
