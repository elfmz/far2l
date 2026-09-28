#pragma once
#include <memory>
#include <deque>
#include <sys/socket.h>

#ifdef HAVE_OPENSSL
# include <openssl/ssl.h>
#endif

#include <ScopeHelpers.h>
#include <StringConfig.h>

#include "../../Erroring.h"

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
struct OpenSSLContext
{
	OpenSSLContext(const StringConfig &protocol_options);
	virtual ~OpenSSLContext();

	SSL *NewSSL(int sock);

private:
	SSL_CTX *_ctx;
	SSL_SESSION *_session = nullptr;

	static int sNewClientSessionCB(SSL *ssl, SSL_SESSION *session);
};

struct TLSTransport : public BaseTransport
{
	std::shared_ptr<OpenSSLContext> _ctx;
	SSL *_ssl = nullptr;

	void Shutdown();

public:
	TLSTransport(std::shared_ptr<OpenSSLContext> &ctx, int sock);
	virtual ~TLSTransport();
	virtual ssize_t SendImpl(const void *data, size_t len);
	virtual ssize_t RecvImpl(void *data, size_t len);
	virtual int DetachSocket();

	std::string GetPeerFingerprint();
};
#endif

template <class E = ProtocolError>
	static void FTPThrowIfBadResponse(const std::string &str, unsigned int reply_code, unsigned int reply_ok_min, unsigned int reply_ok_max)
{
	if (reply_code < reply_ok_min || reply_code > reply_ok_max) {
		size_t n = str.size();
		while (n > 0 && (str[n - 1] == '\r' || str[n - 1] == '\n')) {
			--n;
		}
		throw E(str.substr(0, n));
	}
}


struct FTPConnection : public std::enable_shared_from_this<FTPConnection>
{
	std::shared_ptr<BaseTransport> _transport;
	StringConfig _protocol_options;
	std::string _str;

	struct {
		struct Response
		{
			unsigned int code;
			std::string str;
		};

		std::deque<std::string> requests;
		std::deque<Response> responses;
		bool pipelining = false;
	} _batch;

#ifdef HAVE_OPENSSL
	std::shared_ptr<OpenSSLContext> _openssl_ctx;
	bool _data_encryption_enabled = false;
#endif

	void BatchPerform();

	void DataCommand_PASV(std::shared_ptr<BaseTransport> &data_transport, const std::string &cmd, unsigned long long rest = 0);
	void DataCommand_PORT(std::shared_ptr<BaseTransport> &data_transport, const std::string &cmd, unsigned long long rest = 0);

	bool EnableDataConnectionProtection();


public:
	FTPConnection(bool implicit_encryption, const std::string &host, unsigned int port, const std::string &options);
	virtual ~FTPConnection();
	void EnsureDataConnectionProtection();

	const StringConfig &ProtocolOptions() const { return _protocol_options; }

	void SendRequest(const std::string &str);
	unsigned int RecvResponse(std::string &str);
	unsigned int SendRecvResponse(std::string &str);

	unsigned int RecvResponseFromTransport(std::string &str);

	void RecvResponse(std::string &str, unsigned int reply_ok_min, unsigned int reply_ok_max);
	void SendRecvResponse(std::string &str, unsigned int reply_ok_min, unsigned int reply_ok_max);
	void SendRestIfNeeded(unsigned long long rest);

	std::shared_ptr<BaseTransport> DataCommand(const std::string &cmd, unsigned long long rest = 0);
};
