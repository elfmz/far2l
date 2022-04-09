#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "os_call.hpp"
#include "LocalSocket.h"

template <class IO_ERROR>
	static size_t CheckIOResult(ssize_t r)
{
	if (r < 0) {
		throw IO_ERROR();
	}
	if (r == 0) {
		throw LocalSocketDisconnected();
	}

	return (size_t)r;
}

size_t LocalSocket::Send(const void *data, size_t len)
{
	if (!len) {
		return 0;
	}

	for (int attempt = 0;; ++attempt) {
		ssize_t r = os_call_ssize(send, (int)_sock, data, len, 0);
		if (r >= 0 || errno != ENOBUFS || attempt >= 100) {
			return CheckIOResult<LocalSocketSendError>(r);
		}
		usleep(10000);
	}
}

size_t LocalSocket::Recv(void *data, size_t len)
{
	if (!len) {
		return 0;
	}

	return CheckIOResult<LocalSocketRecvError>(os_call_ssize(recv, (int)_sock, data, len, 0));
}


size_t LocalSocket::SendTo(const void *data, size_t len, const struct sockaddr_un &sa)
{
	if (!len) return 0;

	return CheckIOResult<LocalSocketSendError>(os_call_ssize(sendto,
		(int)_sock, data, len, 0, (const struct sockaddr *)&sa, (socklen_t)sizeof(sa)));
}

size_t LocalSocket::RecvFrom(void *data, size_t len, struct sockaddr_un &sa)
{
	if (!len) return 0;

	socklen_t sal = sizeof(sa);
	return CheckIOResult<LocalSocketRecvError>(os_call_ssize(recvfrom,
		(int)_sock, data, len, 0, (struct sockaddr *)&sa, &sal));
}

struct CMsgWrap
{
	struct msghdr msg;
	struct cmsghdr *c;
	struct iovec iov;
	int data = 0xdeadbeef;

	CMsgWrap(size_t len, int f1ll)
	{
		memset(&msg, 0, sizeof(msg));
		memset(&iov, 0, sizeof(iov));
		iov.iov_base = &data;
		iov.iov_len = sizeof(data);
		msg.msg_iov = &iov;
		msg.msg_iovlen = 1;

		const size_t cmspc = CMSG_SPACE(len);
		c = (struct cmsghdr *)malloc(cmspc);
		if (c) {
			memset(c, f1ll, cmspc);
			c->cmsg_len = CMSG_LEN(len);
			c->cmsg_level = SOL_SOCKET;
			c->cmsg_type = SCM_RIGHTS;
			msg.msg_control = c;
			msg.msg_controllen = cmspc;
		}
	}

	~CMsgWrap()
	{
		free(c);
	}
};

void LocalSocket::SendFD(int fd)
{
	CMsgWrap cmw(sizeof(int), 0x0b);
 
	memcpy(CMSG_DATA(cmw.c), &fd, sizeof(int));
 
	for (int attempt = 0;; ++attempt) {
		ssize_t r = os_call_ssize(sendmsg, (int)_sock, (const struct msghdr *)&cmw.msg, 0);
		if (r != -1) {
			break;
		}
		if (errno != ENOBUFS || attempt >= 100) {
			throw LocalSocketSendError();
		}
		usleep(10000);
	}
}

int LocalSocket::RecvFD()
{
	CMsgWrap cmw(sizeof(int), 0x0d);

	if (os_call_ssize(recvmsg, (int)_sock, &cmw.msg, 0) == -1)
		throw LocalSocketRecvError();

	int out;
	memcpy(&out, CMSG_DATA(cmw.c), sizeof(int));
	return out;
}

////////////////
static int CreateUnixSocket(LocalSocket::Kind sock_kind)
{
	int sock = socket(PF_UNIX, (sock_kind == LocalSocket::DATAGRAM) ? SOCK_DGRAM : SOCK_STREAM, 0);
	if (sock != -1) {
		int bufsz = 8192;
		setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &bufsz, sizeof(bufsz));
		bufsz = 8192;
		setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &bufsz, sizeof(bufsz));
	}
	return sock;
}

LocalSocketClient::LocalSocketClient(Kind sock_kind, const std::string &path_server, const std::string &path_client)
{
	_sock = CreateUnixSocket(sock_kind);
	if (!_sock.Valid())
		throw LocalSocketSocketError();

	struct sockaddr_un sa = {};
	sa.sun_family = AF_UNIX;
	strncpy(sa.sun_path, path_client.c_str(), sizeof(sa.sun_path) - 1);
	unlink(sa.sun_path);
	if (bind(_sock, (struct sockaddr *)&sa, sizeof(sa)) < 0)
		throw LocalSocketBindError();

	memset(&sa, 0, sizeof(sa));
	sa.sun_family = AF_UNIX;
	strncpy(sa.sun_path, path_server.c_str(), sizeof(sa.sun_path) - 1);

	if (connect(_sock, (struct sockaddr *)&sa, sizeof(sa)) == -1)
		throw LocalSocketConnectError();
}

////////////////

LocalSocketServer::LocalSocketServer(Kind sock_kind, const std::string &server, int backlog)
{
	FDScope &sock = (sock_kind == DATAGRAM) ? _sock : _accept_sock;

	sock = CreateUnixSocket(sock_kind);
	if (!sock.Valid())
		throw LocalSocketSocketError();

	struct sockaddr_un sa = {};
	sa.sun_family = AF_UNIX;
	strncpy(sa.sun_path, server.c_str(), sizeof(sa.sun_path) - 1);

	unlink(sa.sun_path);

	if (bind(sock, (struct sockaddr *)&sa, sizeof(sa)) < 0)
		throw LocalSocketBindError();

	if (sock_kind == STREAM) {
		listen(sock, backlog);
	}
}

void LocalSocketServer::WaitForClient(int fd_cancel)
{
	FDScope &sock = _accept_sock.Valid() ? _accept_sock : _sock;

	fd_set fdr, fde;
	int maxfd = (fd_cancel > (int)sock) ? fd_cancel : (int)sock;

	for (;;) {
		FD_ZERO(&fdr);
		FD_ZERO(&fde);

		if (fd_cancel != -1) {
			FD_SET(fd_cancel, &fdr);
			FD_SET(fd_cancel, &fde);
		}

		FD_SET(sock, &fdr);
		FD_SET(sock, &fde);

		if (select(maxfd + 1, &fdr, (fd_set*)nullptr, &fde, (timeval*)nullptr) == -1) {
			if (errno == EAGAIN || errno == EINTR)
				continue;

			throw LocalSocketSelectError();
		}

		if (fd_cancel != -1) {
			if (FD_ISSET(fd_cancel, &fde) || FD_ISSET(fd_cancel, &fdr)) {
				throw LocalSocketCancelled();
			}
		}

		if (FD_ISSET(sock, &fde) || FD_ISSET(sock, &fdr)) {
			if (!_accept_sock.Valid()) {
				break;
			}

			struct sockaddr_un clnt_sa {};
			socklen_t clnt_sa_len = sizeof(clnt_sa);
			_sock = accept(sock, (struct sockaddr *)&clnt_sa, &clnt_sa_len);
			if (_sock.Valid()) {
				break;
			}
		}
	}
}

