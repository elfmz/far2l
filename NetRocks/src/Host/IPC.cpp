#include <fcntl.h>
#include <errno.h>
#include <os_call.hpp>
#include "IPC.h"

IPCSender::IPCSender(int fd) : _fd(fd)
{
}

IPCSender::~IPCSender()
{
	CheckedCloseFD(_fd);
}

void IPCSender::SetFD(int fd)
{
	CheckedCloseFD(_fd);
	_fd = fd;
}

void IPCSender::Send(const void *data, size_t len) throw(IPCError)
{
	if (len) for (;;) {
		ssize_t rv = os_call_ssize(write, _fd, data, len);
//		fprintf(stderr, "[%d] SENT: %lx/%lx {0x%x... }\n", getpid(), rv, len, *(const unsigned char *)data);
		if (rv <= 0)
			throw IPCError("IPCSender: write", errno);

		if ((size_t)rv == len)
			break;

		len-= (size_t)rv;
		data = (const char *)data + rv;
	}
}

void IPCSender::SendString(const char *s) throw(IPCError)
{
	size_t len = strlen(s);
	SendPOD(len);
	if (len)
		Send(s, len);
}

void IPCSender::SendString(const std::string &s) throw(IPCError)
{
	size_t len = s.size();
	SendPOD(len);
	if (len)
		Send(s.c_str(), len);
}

//////////////////


IPCRecver::IPCRecver(int fd) : _fd(fd), _aborted(false)
{
	if (pipe_cloexec(_kickass) != -1) {
		fcntl(_kickass[1], F_SETFL, fcntl(_kickass[1], F_GETFL, 0) | O_NONBLOCK);
	} else {
		_kickass[0] = _kickass[1] = -1;
	}
}

IPCRecver::~IPCRecver()
{
	CheckedCloseFD(_fd);
	CheckedCloseFDPair(_kickass);
}

void IPCRecver::AbortReceiving()
{
	_aborted = true;
	char c = 0;
	if (os_call_ssize(write, _kickass[1], (const void*)&c, (size_t)1) != 1)
		perror("IPCRecver - write kickass");
}

void IPCRecver::SetFD(int fd)
{
	CheckedCloseFD(_fd);
	_aborted = false;
	_fd = fd;
}

void IPCRecver::Recv(void *data, size_t len) throw(IPCError)
{
	fd_set fds, fde;
	if (len) for (;;) {
		if (_aborted)
			throw IPCError("IPCRecver: aborted", errno);

		int maxfd = (_kickass[0] > _fd) ? _kickass[0] : _fd;

		FD_ZERO(&fds);
		FD_ZERO(&fde);
		FD_SET(_kickass[0], &fds);
		FD_SET(_fd, &fds);
		FD_SET(_fd, &fde);

		int sv = select(maxfd + 1, &fds, nullptr, &fde, nullptr);
		if (sv == -1) {
			if (errno != EAGAIN && errno != EINTR) {
				throw IPCError("IPCRecver: select", errno);
			}
			continue;

		}
		if (sv == 0) {
			sleep(1);
			continue;
		}

		if (FD_ISSET(_kickass[0], &fds)) {
			char c;
			if (os_call_ssize(read, _kickass[0], (void *)&c, (size_t)1) != 1)
				perror("IPCRecver - read kickass");
		}

		if (FD_ISSET(_fd, &fde)) {
			throw IPCError("IPCRecver: exception", errno);
		}

		if (FD_ISSET(_fd, &fds)) {
			ssize_t rv = os_call_ssize(read, _fd, data, len);
//			fprintf(stderr, "[%d] RECV: %lx/%lx {0x%x... }\n", getpid(), rv, len, *(const unsigned char *)data);
			if (rv <= 0)
				throw IPCError("IPCRecver: read", errno);

			if ((size_t)rv == len)
				break;

			len-= (size_t)rv;
			data = (char *)data + rv;
		}
	}
}

void IPCRecver::RecvString(std::string &s) throw(IPCError)
{
	size_t len = 0;
	RecvPOD(len);
	s.resize(len);
	if (len)
		Recv(&s[0], len);
}

///////////

IPCEndpoint::IPCEndpoint(int fd_recv, int fd_send)
	: IPCRecver(fd_recv), IPCSender(fd_send)
{
}

