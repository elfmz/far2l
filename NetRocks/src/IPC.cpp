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
		ssize_t rv = write(_fd, data, len);
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


IPCRecver::IPCRecver(int fd) : _fd(fd)
{
}

IPCRecver::~IPCRecver()
{
	CheckedCloseFD(_fd);
}

void IPCRecver::SetFD(int fd)
{
	CheckedCloseFD(_fd);
	_fd = fd;
}

void IPCRecver::Recv(void *data, size_t len) throw(IPCError)
{
	if (len) for (;;) {
		ssize_t rv = read(_fd, data, len);
//		fprintf(stderr, "[%d] RECV: %lx/%lx {0x%x... }\n", getpid(), rv, len, *(const unsigned char *)data);
		if (rv <= 0)
			throw IPCError("IPCRecver: read", errno);

		if ((size_t)rv == len)
			break;

		len-= (size_t)rv;
		data = (char *)data + rv;
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

