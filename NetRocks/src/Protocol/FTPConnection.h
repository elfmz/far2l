#pragma once
#include <memory>
#include <sys/socket.h>

#ifdef HAVE_OPENSSL
# include <openssl/ssl.h>
#endif

#include <ScopeHelpers.h>
#include <StringConfig.h>


struct BaseTransport : public std::enable_shared_from_this<BaseTransport>
{
	virtual ~BaseTransport();

	void Send(const void *data, size_t len);
	ssize_t Recv(void *data, size_t len);
	virtual int DetachSocket();
	void GetPeerAddress(struct sockaddr_in &sin);
	void GetLocalAddress(struct sockaddr_in &sin);

protected:
	FDScope _sock;

	virtual ssize_t SendImpl(const void *data, size_t len) = 0;
	virtual ssize_t RecvImpl(void *data, size_t len) = 0;
};

struct SocketTransport : public BaseTransport
{
	SocketTransport(int sock, const StringConfig &protocol_options);
	SocketTransport(struct sockaddr_in &sin, const StringConfig &protocol_options);
	SocketTransport(const std::string &host, unsigned int port, const StringConfig &protocol_options);

protected:
	void Connect(struct sockaddr_in &sin, const StringConfig &protocol_options);
	void ApplySocketOptions(const StringConfig &protocol_options);

	virtual ssize_t SendImpl(const void *data, size_t len);
	virtual ssize_t RecvImpl(void *data, size_t len);
};

#ifdef HAVE_OPENSSL
struct TLSTransport : public BaseTransport
{
	SSL_CTX *_ctx = nullptr;
	SSL *_ssl = nullptr;

	void Shutdown();

public:
	TLSTransport(int sock, StringConfig &protocol_options);
	virtual ~TLSTransport();
	virtual ssize_t SendImpl(const void *data, size_t len);
	virtual ssize_t RecvImpl(void *data, size_t len);
	virtual int DetachSocket();
};
#endif

struct FTPConnection : public std::enable_shared_from_this<FTPConnection>
{
	std::shared_ptr<BaseTransport> _transport;
	StringConfig _protocol_options;
	bool _encryption;

	void DataCommand_PASV(std::shared_ptr<BaseTransport> &data_transport, const std::string &cmd, unsigned long long rest = 0);
	void DataCommand_PORT(std::shared_ptr<BaseTransport> &data_transport, const std::string &cmd, unsigned long long rest = 0);

public:
	FTPConnection(bool implicit_encryption, const std::string &host, unsigned int port, const std::string &options);
	virtual ~FTPConnection();
	unsigned int RecvResponce(std::string &str);
	unsigned int SendRecvResponce(std::string &str);
	void SendRestIfNeeded(unsigned long long rest);
	std::shared_ptr<BaseTransport> DataCommand(const std::string &cmd, unsigned long long rest = 0);
};
