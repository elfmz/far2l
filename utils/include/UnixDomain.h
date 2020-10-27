#pragma once
#include <exception>
#include <stdexcept>
#include <utils.h>
#include <ScopeHelpers.h>

template <int WHAT>
	struct UnixDomainError : std::runtime_error
{
	UnixDomainError()
		: std::runtime_error(StrPrintf("UnixDomain<%d>: error %u", WHAT, errno))
	{ }
};

struct UnixDomainCancelled : UnixDomainError<__LINE__> {};
struct UnixDomainSocketError : UnixDomainError<__LINE__> {};
struct UnixDomainBindError : UnixDomainError<__LINE__> {};
struct UnixDomainConnectError : UnixDomainError<__LINE__> {};
struct UnixDomainIOError : UnixDomainError<__LINE__> {};

class UnixDomain
{
protected:
	FDScope _sock;

public:
	size_t Send(const void *data, size_t len) throw(std::exception);
	size_t Recv(void *data, size_t len) throw(std::exception);

	size_t SendTo(const void *data, size_t len, const struct sockaddr_un &sa) throw(std::exception);
	size_t RecvFrom(void *data, size_t len, struct sockaddr_un &sa) throw(std::exception);

	void SendFD(int fd) throw(std::exception);
	int RecvFD() throw(std::exception);
};

class UnixDomainServer : public UnixDomain
{
	FDScope _accept_sock;

public:
	UnixDomainServer(unsigned int sock_type, const std::string &server, int backlog = 1);
	void WaitForClient(int fd_cancel = -1);
};

class UnixDomainClient : public UnixDomain
{
public:
	UnixDomainClient(unsigned int sock_type, const std::string &server, const std::string &client);
};
