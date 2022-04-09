#pragma once
#include <exception>
#include <stdexcept>
#include <utils.h>
#include <ScopeHelpers.h>

template <int WHAT>
	struct LocalSocketError : std::runtime_error
{
	LocalSocketError()
		: std::runtime_error(StrPrintf("LocalSocket<%d>: error %u", WHAT, errno))
	{ }
};

struct LocalSocketCancelled : LocalSocketError<__LINE__> {};
struct LocalSocketSocketError : LocalSocketError<__LINE__> {};
struct LocalSocketBindError : LocalSocketError<__LINE__> {};
struct LocalSocketConnectError : LocalSocketError<__LINE__> {};
struct LocalSocketSelectError : LocalSocketError<__LINE__> {};
struct LocalSocketRecvError : LocalSocketError<__LINE__> {};
struct LocalSocketSendError : LocalSocketError<__LINE__> {};
struct LocalSocketDisconnected : LocalSocketError<__LINE__> {};

class LocalSocket
{
protected:
	FDScope _sock;

public:
	size_t Send(const void *data, size_t len);
	size_t Recv(void *data, size_t len);

	size_t SendTo(const void *data, size_t len, const struct sockaddr_un &sa);
	size_t RecvFrom(void *data, size_t len, struct sockaddr_un &sa);

	void SendFD(int fd);
	int RecvFD();

	enum Kind
	{
		DATAGRAM,
		STREAM
	};
};

class LocalSocketServer : public LocalSocket
{
	FDScope _accept_sock;

public:
	LocalSocketServer(Kind sock_kind, const std::string &server, int backlog = 1);
	void WaitForClient(int fd_cancel = -1);
};

class LocalSocketClient : public LocalSocket
{
public:
	LocalSocketClient(Kind sock_kind, const std::string &server, const std::string &client);
};

