#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "os_call.hpp"
#include "LocalSocket.h"

static size_t CheckIOResult(ssize_t r)
{
	if (r < 0) {
		throw LocalSocketIOError();
	}
	if (r == 0) {
		throw LocalSocketDisconnected();
	}

	return (size_t)r;
}

size_t LocalSocket::Send(const void *data, size_t len) throw(std::exception)
{
	if (!len) return 0;

	return CheckIOResult(os_call_ssize(send, (int)_sock, data, len, 0));
}

size_t LocalSocket::Recv(void *data, size_t len) throw(std::exception)
{
	if (!len) return 0;

	return CheckIOResult(os_call_ssize(recv, (int)_sock, data, len, 0));
}


size_t LocalSocket::SendTo(const void *data, size_t len, const struct sockaddr_un &sa) throw(std::exception)
{
	if (!len) return 0;

	return CheckIOResult(os_call_ssize(sendto,
		(int)_sock, data, len, 0, (const struct sockaddr *)&sa, (socklen_t)sizeof(sa)));
}

size_t LocalSocket::RecvFrom(void *data, size_t len, struct sockaddr_un &sa) throw(std::exception)
{
	if (!len) return 0;

	socklen_t sal = sizeof(sa);
	return CheckIOResult(os_call_ssize(recvfrom,
		(int)_sock, data, len, 0, (struct sockaddr *)&sa, &sal));
}

void LocalSocket::SendFD(int fd) throw(std::exception)
{
	std::vector<char> buf(CMSG_SPACE(sizeof(int)));
	std::fill(buf.begin(), buf.end(), 0x0b);

	struct cmsghdr *cmsghdr = (struct cmsghdr *)&buf[0];
	cmsghdr->cmsg_len = CMSG_LEN(sizeof(int));
	cmsghdr->cmsg_level = SOL_SOCKET;
	cmsghdr->cmsg_type = SCM_RIGHTS;
 
	struct iovec iov[1] {};
	char c = '*';
	iov[0].iov_base = &c;
	iov[0].iov_len = sizeof(c);

	struct msghdr msg {};
	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	msg.msg_iov = iov;
	msg.msg_iovlen = sizeof(iov) / sizeof(iov[0]);
	msg.msg_control = cmsghdr;
	msg.msg_controllen = CMSG_LEN(sizeof(int));
	msg.msg_flags = 0;

	memcpy(CMSG_DATA(cmsghdr), &fd, sizeof(int));
 
	if (os_call_ssize(sendmsg, (int)_sock, (const struct msghdr *)&msg, 0) == -1)
		throw LocalSocketIOError();
}

int LocalSocket::RecvFD() throw(std::exception)
{
	std::vector<char> buf(CMSG_SPACE(sizeof(int)));
	std::fill(buf.begin(), buf.end(), 0x0d);

	struct cmsghdr *cmsghdr = (struct cmsghdr *)&buf[0];
	cmsghdr->cmsg_len = CMSG_LEN(sizeof(int));
	cmsghdr->cmsg_level = SOL_SOCKET;
	cmsghdr->cmsg_type = SCM_RIGHTS;

	struct iovec iov[1] {};
	char c;
	iov[0].iov_base = &c;
	iov[0].iov_len = sizeof(c);

	struct msghdr msg {};
	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	msg.msg_iov = iov;
	msg.msg_iovlen = sizeof(iov) / sizeof(iov[0]);
	msg.msg_control = cmsghdr;
	msg.msg_controllen = CMSG_LEN(sizeof(int));
	msg.msg_flags = 0;

	if (os_call_ssize(recvmsg, (int)_sock, &msg, 0) == -1)
		throw LocalSocketIOError();

	int out;
	memcpy(&out, CMSG_DATA(cmsghdr), sizeof(int));
	return out;
}

////////////////

LocalSocketClient::LocalSocketClient(Kind sock_kind, const std::string &path_server, const std::string &path_client)
{
	_sock = socket(PF_UNIX, (sock_kind == DATAGRAM) ? SOCK_DGRAM : SOCK_STREAM, 0);
	if (!_sock.Valid())
		throw LocalSocketSocketError();

	struct sockaddr_un sa = {};
	sa.sun_family = AF_UNIX;
	strncpy(sa.sun_path, path_client.c_str(), sizeof(sa.sun_path));
	unlink(sa.sun_path);
	if (bind(_sock, (struct sockaddr *)&sa, sizeof(sa)) < 0)
		throw LocalSocketBindError();

	memset(&sa, 0, sizeof(sa));
	sa.sun_family = AF_UNIX;
	strncpy(sa.sun_path, path_server.c_str(), sizeof(sa.sun_path));

	if (connect(_sock, (struct sockaddr *)&sa, sizeof(sa)) == -1)
		throw LocalSocketConnectError();
}

////////////////

LocalSocketServer::LocalSocketServer(Kind sock_kind, const std::string &server, int backlog)
{
	FDScope &sock = (sock_kind == DATAGRAM) ? _sock : _accept_sock;

	sock = socket(PF_UNIX, (sock_kind == DATAGRAM) ? SOCK_DGRAM : SOCK_STREAM, 0);
	if (!sock.Valid())
		throw LocalSocketSocketError();

	struct sockaddr_un sa = {};
	sa.sun_family = AF_UNIX;
	strncpy(sa.sun_path, server.c_str(), sizeof(sa.sun_path));

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

			throw LocalSocketIOError();
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

