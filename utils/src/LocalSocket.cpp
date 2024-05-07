#if defined(__HAIKU__)
#include <cstring>
#include <posix/sys/select.h>
#endif

#if defined(__DragonFly__)
#include <sys/select.h>
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "os_call.hpp"
#include "LocalSocket.h"

// Note on ENOBUFS
// Under FreeBSD 11.4 TTY revival failed occasionally due to send randomly failed with ENOBUFS error.
// This happened due to peer didn't retrieve data in time and BSD failed to send with such error.
// Workaround is trivial: retry send with some reasonable timeout in case failing with ENOBUFS.
// Although never saw such problem under Linux, but lets this behaviour be universal, just in case.

#define ENOBUFS_TIMEOUT_SECONDS 2

template <class IO_ERROR, class FN>
	static size_t SocketIO(FN fn)
{
	for (int attempt = 0;; ++attempt) {
		ssize_t r = fn();
		if (r > 0) {
			return (size_t)r;
		}
		if (r == 0) {
			throw LocalSocketDisconnected();
		}
		if (errno != EAGAIN && errno != EINTR && (errno != ENOBUFS || attempt >= ENOBUFS_TIMEOUT_SECONDS * 100)) {
			throw IO_ERROR();
		}
		usleep(10000); // 1/100 of second
	}
}

size_t LocalSocket::Send(const void *data, size_t len)
{
	return len
		? SocketIO<LocalSocketSendError>( [&]{ return send(_sock, data, len, 0); } )
		: 0;
}

size_t LocalSocket::Recv(void *data, size_t len)
{
	return len
		? SocketIO<LocalSocketRecvError>( [&]{ return recv(_sock, data, len, 0); } )
		: 0;
}

size_t LocalSocket::SendTo(const void *data, size_t len, const struct sockaddr_un &sa)
{
	return len
		? SocketIO<LocalSocketSendError>( [&]{ return sendto(_sock, data, len, 0, (const sockaddr *)&sa, sizeof(sa)); } )
		: 0;
}

size_t LocalSocket::RecvFrom(void *data, size_t len, struct sockaddr_un &sa)
{
	return len
		? SocketIO<LocalSocketRecvError>( [&]{
				socklen_t sal = sizeof(sa);
				return recvfrom(_sock, data, len, 0, (sockaddr *)&sa, &sal);
			} )
		: 0;
}

class ScmRightsMsg
{
	struct msghdr _msg;
	struct cmsghdr *_c;
	struct iovec _iov;
	char _dummy_data = '*';

public:
	ScmRightsMsg(size_t payload_len)
	{
		ZeroFill(_msg);
		ZeroFill(_iov);
		_iov.iov_base = &_dummy_data;
		_iov.iov_len = sizeof(_dummy_data);
		_msg.msg_iov = &_iov;
		_msg.msg_iovlen = 1;

		const size_t cmsg_spc = CMSG_SPACE(payload_len);
		_c = (struct cmsghdr *)calloc(1, cmsg_spc); // use calloc to ensure proper alignment
		if (!_c) {
			throw std::bad_alloc();
		}
		_c->cmsg_len = CMSG_LEN(payload_len);
		_c->cmsg_level = SOL_SOCKET;
		_c->cmsg_type = SCM_RIGHTS;
		_msg.msg_control = _c;
		_msg.msg_controllen = cmsg_spc;
	}

	~ScmRightsMsg()
	{
		free(_c);
	}

	inline void *Payload() { return CMSG_DATA(_c); }
	inline struct msghdr *Msg() { return &_msg; }
};

void LocalSocket::SendFD(int fd)
{
	ScmRightsMsg srm(sizeof(int));
	memcpy(srm.Payload(), &fd, sizeof(int));
	SocketIO<LocalSocketSendError>([&] { return sendmsg(_sock, srm.Msg(), 0); });
}

int LocalSocket::RecvFD()
{
	ScmRightsMsg srm(sizeof(int));
	SocketIO<LocalSocketRecvError>([&] { return recvmsg(_sock, srm.Msg(), 0); });
	int out;
	memcpy(&out, srm.Payload(), sizeof(int));
	return out;
}

////////////////
static int CreateUnixSocket(LocalSocket::Kind sock_kind)
{
	return socket(PF_UNIX, (sock_kind == LocalSocket::DATAGRAM) ? SOCK_DGRAM : SOCK_STREAM, 0);
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

	ZeroFill(sa);
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

