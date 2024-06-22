#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <os_call.hpp>
#include "PipeIPC.h"

#ifdef __HAIKU__
	#include <posix/sys/select.h>
#endif

#if defined(__DragonFly__)
#include <sys/select.h>
#endif

static std::string FormatIPCError(const char *msg, unsigned int code)
{
	std::string s = msg;
	if (code != (unsigned int)-1) {
		char sz[32];
		snprintf(sz, sizeof(sz) - 1, " (%u)", code);
		s+= sz;
	}
	return s;
}

PipeIPCError::PipeIPCError(const char *msg, unsigned int code)
	: std::runtime_error(FormatIPCError(msg, code))
{
	fprintf(stderr, "PipeIPCError: %s\n", what());
}

//////////////

PipeIPCSender::PipeIPCSender(int fd) : _fd(fd)
{
}

PipeIPCSender::~PipeIPCSender()
{
	CheckedCloseFD(_fd);
}

void PipeIPCSender::SetFD(int fd)
{
	CheckedCloseFD(_fd);
	_fd = fd;
}

void PipeIPCSender::Send(const void *data, size_t len)
{
	if (len) for (;;) {
		ssize_t rv = os_call_ssize(write, _fd, data, len);
//		fprintf(stderr, "[%d] SENT: %lx/%lx {0x%x... }\n", getpid(), rv, len, *(const unsigned char *)data);
		if (rv <= 0)
			throw PipeIPCError("PipeIPCSender: write", errno);

		if ((size_t)rv == len)
			break;

		len-= (size_t)rv;
		data = (const char *)data + rv;
	}
}

void PipeIPCSender::SendString(const char *s)
{
	size_t len = strlen(s);
	SendPOD(len);
	if (len)
		Send(s, len);
}

void PipeIPCSender::SendString(const std::string &s)
{
	size_t len = s.size();
	SendPOD(len);
	if (len)
		Send(s.c_str(), len);
}

//////////////////


PipeIPCRecver::PipeIPCRecver(int fd) : _fd(fd), _aborted(false)
{
	if (pipe_cloexec(_kickass) != -1) {
		MakeFDNonBlocking(_kickass[1]);

	} else {
		_kickass[0] = _kickass[1] = -1;
	}
}

PipeIPCRecver::~PipeIPCRecver()
{
	CheckedCloseFD(_fd);
	CheckedCloseFDPair(_kickass);
}

void PipeIPCRecver::AbortReceiving()
{
	_aborted = true;
	char c = 0;
	if (os_call_ssize(write, _kickass[1], (const void*)&c, (size_t)1) != 1)
		perror("PipeIPCRecver - write kickass");
}

void PipeIPCRecver::SetFD(int fd)
{
	CheckedCloseFD(_fd);
	_aborted = false;
	_fd = fd;
}

bool PipeIPCRecver::WaitForRecv(int msec)
{
	fd_set fds, fde;
	timeval tv;
	for (;;) {
		if (_aborted)
			throw PipeIPCError("PipeIPCRecver: aborted", errno);

		int maxfd = (_kickass[0] > _fd) ? _kickass[0] : _fd;

		FD_ZERO(&fds);
		FD_ZERO(&fde);
		FD_SET(_kickass[0], &fds);
		FD_SET(_fd, &fds);
		FD_SET(_fd, &fde);

		if (msec != -1) {
			tv.tv_sec = msec / 1000;
			tv.tv_usec = (msec % 1000) * 1000;
		}
		int sv = select(maxfd + 1, &fds, nullptr, &fde, (msec != -1) ? &tv : nullptr);
		if (sv == -1) {
			if (errno != EAGAIN && errno != EINTR) {
				throw PipeIPCError("PipeIPCRecver: select", errno);
			}
			continue;
		}
		if (sv == 0) {
			return false;
		}

		if (FD_ISSET(_kickass[0], &fds)) {
			char c;
			if (os_call_ssize(read, _kickass[0], (void *)&c, (size_t)1) != 1)
				perror("PipeIPCRecver - read kickass");
		}

		if (FD_ISSET(_fd, &fde)) {
			throw PipeIPCError("PipeIPCRecver: exception", errno);
		}

		return FD_ISSET(_fd, &fds);
	}
}

void PipeIPCRecver::Recv(void *data, size_t len)
{
	while (len) {
		if (!WaitForRecv()) {
			usleep(100000);
			continue;
		}

		ssize_t rv = os_call_ssize(read, _fd, data, len);
//		fprintf(stderr, "[%d] RECV: %lx/%lx {0x%x... }\n", getpid(), rv, len, *(const unsigned char *)data);
		if (rv <= 0)
			throw PipeIPCError("PipeIPCRecver: read", errno);

		len-= (size_t)rv;
		data = (char *)data + rv;
	}
}

void PipeIPCRecver::RecvString(std::string &s)
{
	size_t len = 0;
	RecvPOD(len);
	s.resize(len);
	if (len)
		Recv(&s[0], len);
}

///////////

PipeIPCFD::PipeIPCFD()
{
	int r = pipe(master2broker);
	if (r != 0) {
		throw PipeIPCError("PipeIPCFD: pipe[master2broker]", errno);
	}

	r = pipe(broker2master);
	if (r != 0) {
		CheckedCloseFDPair(master2broker);
		throw PipeIPCError("PipeIPCFD: pipe[broker2master]", errno);
	}

	MakeFDCloexec(master2broker[1]);
	MakeFDCloexec(broker2master[0]);

	snprintf(broker_arg_r, sizeof(broker_arg_r), "%d", master2broker[0]);
	snprintf(broker_arg_w, sizeof(broker_arg_r), "%d", broker2master[1]);
}

void PipeIPCFD::Detach()
{
	CheckedCloseFD(master2broker[0]);
	master2broker[1] = -1;

	broker2master[0] = -1;
	CheckedCloseFD(broker2master[1]);
}

PipeIPCFD::~PipeIPCFD()
{
	CheckedCloseFDPair(master2broker);
	CheckedCloseFDPair(broker2master);
}
