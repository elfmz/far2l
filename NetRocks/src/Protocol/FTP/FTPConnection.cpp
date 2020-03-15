#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <utils.h>
#include <ScopeHelpers.h>
#include <os_call.hpp>

#include "FTPConnection.h"

BaseTransport::~BaseTransport()
{
}

void BaseTransport::Send(const void *data, size_t len)
{
	for (size_t transferred = 0; transferred < len;) {
		ssize_t r = SendImpl(data, len);
		if (r <= 0) {
			throw std::runtime_error(StrPrintf("send errno=%d", errno));
			//return transferred ? transferred : r;
		}

		transferred+= (size_t)r;
	}
}

ssize_t BaseTransport::Recv(void *data, size_t len)
{
	return RecvImpl(data, len);
}

int BaseTransport::DetachSocket()
{
	return _sock.Detach();
}

void BaseTransport::GetPeerAddress(struct sockaddr_in &sin)
{
	socklen_t l = sizeof(sin);
	if (getpeername(_sock, (struct sockaddr *)&sin, &l) == -1) {
		throw std::runtime_error(StrPrintf("getpeername errno=%d", errno));
	}
}

void BaseTransport::GetLocalAddress(struct sockaddr_in &sin)
{
	socklen_t l = sizeof(sin);
	if (getsockname(_sock, (struct sockaddr *)&sin, &l) == -1) {
		throw std::runtime_error(StrPrintf("getsockname errno=%d", errno));
	}
}

////////////

SocketTransport::SocketTransport(int sock, const StringConfig &protocol_options)
{
	_sock = sock;
	ApplySocketOptions(protocol_options);
}

SocketTransport::SocketTransport(struct sockaddr_in &sin, const StringConfig &protocol_options)
{
	Connect(sin, protocol_options);
}

SocketTransport::SocketTransport(const std::string &host, unsigned int port, const StringConfig &protocol_options)
{
	struct sockaddr_in sin = {0};
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);

	if (inet_aton(host.c_str(), &sin.sin_addr) == 0) {
		struct hostent *he = gethostbyname(host.c_str());
		if (!he) {
			throw std::runtime_error(StrPrintf("gethostbyname('%s') errno=%d", host.c_str(), errno));
		}
		memcpy(&sin.sin_addr, he->h_addr, std::min(sizeof(sin.sin_addr), (size_t)he->h_length));
	}

	Connect(sin, protocol_options);
}

void SocketTransport::ApplySocketOptions(const StringConfig &protocol_options)
{
	if (protocol_options.GetInt("TcpNoDelay") ) {
		int nodelay = 1;
		if (setsockopt(_sock, IPPROTO_TCP, TCP_NODELAY, (void *)&nodelay, sizeof(nodelay)) == -1) {
			perror("SocketTransport - TCP_NODELAY");
		}
	}

	if (protocol_options.GetInt("TcpQuickAck") ) {
#ifdef TCP_QUICKACK
		int quickack = 1;
		if (setsockopt(_sock, IPPROTO_TCP, TCP_QUICKACK , (void *)&quickack, sizeof(quickack)) == -1) {
			perror("SocketTransport - TCP_QUICKACK ");
		}
#else
		fprintf(stderr, "SocketTransport: TCP_QUICKACK requested but not supported\n");
#endif
	}
}

void SocketTransport::Connect(struct sockaddr_in &sin, const StringConfig &protocol_options)
{
	_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (_sock == -1) {
		throw std::runtime_error(StrPrintf("socket() errno=%d", errno));
	}

	ApplySocketOptions(protocol_options);

	if (connect(_sock, (struct sockaddr *)&sin, sizeof(sin)) == -1) {
		throw std::runtime_error(StrPrintf("connect() errno=%d", errno));
	}
}

ssize_t SocketTransport::SendImpl(const void *data, size_t len)
{
	return os_call_ssize(send, (int)_sock, data, len, 0);
}

ssize_t SocketTransport::RecvImpl(void *data, size_t len)
{
	return os_call_ssize(recv, (int)_sock, data, len, 0);
}

#ifdef HAVE_OPENSSL
void TLSTransport::Shutdown()
{
	if (_ssl != nullptr)
	{
		SSL_shutdown(_ssl);
		SSL_free(_ssl);
		_ssl = nullptr;
	}

	if (_ctx != nullptr)
	{
		SSL_CTX_free(_ctx);
		_ctx = nullptr;
	}
}

TLSTransport::TLSTransport(int sock, StringConfig &protocol_options)
{
	_sock = sock;

	_ctx = SSL_CTX_new(SSLv23_client_method());

	switch (protocol_options.GetInt("EncryptionProtocol", 3)) {
		case 4:
			SSL_CTX_set_options(_ctx, SSL_OP_NO_TLSv1_1);

		case 3:
			SSL_CTX_set_options(_ctx, SSL_OP_NO_TLSv1);

		case 2:
			SSL_CTX_set_options(_ctx, SSL_OP_NO_SSLv3);

		case 1:
			SSL_CTX_set_options(_ctx, SSL_OP_NO_SSLv2);

		case 0:
		default:
			;
	}

	if (!_ctx) {
		throw std::runtime_error(StrPrintf("SSL_CTX_new() errno=%d", errno));
	}

	_ssl = SSL_new(_ctx);
	if (!_ssl) {
		Shutdown();
		throw std::runtime_error(StrPrintf("SSL_new() errno=%d", errno));
	}

	if (SSL_set_fd(_ssl, _sock) != 1) {
		Shutdown();
		throw std::runtime_error(StrPrintf("SSL_set_fd() errno=%d", errno));
	}

	if (SSL_connect(_ssl) != 1) {
		Shutdown();
		throw std::runtime_error(StrPrintf("SSL_connect() errno=%d", errno));
	}
}

TLSTransport::~TLSTransport()
{
	Shutdown();
}

ssize_t TLSTransport::SendImpl(const void *data, size_t len)
{
	if (!_ssl) {
		return -1;
	}

	return SSL_write(_ssl, data, len);
}

ssize_t TLSTransport::RecvImpl(void *data, size_t len)
{
	if (!_ssl) {
		return -1;
	}

	return SSL_read(_ssl, data, len);
}

int TLSTransport::DetachSocket()
{
	Shutdown();
	return BaseTransport::DetachSocket();
}
#endif

FTPConnection::FTPConnection(bool implicit_encryption, const std::string &host, unsigned int port, const std::string &options)
	: _protocol_options(options), _encryption(implicit_encryption)
{
	if (!port) {
		port = implicit_encryption ? 990 : 21;
	}

	_transport = std::make_shared<SocketTransport>(host, port, _protocol_options);

	if (!_encryption && _protocol_options.GetInt("ExplicitEncryption", 0)) {
		// TODO
		_encryption = true;
		}

	if (_encryption) {
#ifdef HAVE_OPENSSL
		_transport = std::make_shared<TLSTransport>(_transport->DetachSocket(), _protocol_options);
#else
		throw std::runtime_error("OpenSSL was not enabled in build");
#endif
	}
}

FTPConnection::~FTPConnection()
{
}

void FTPConnection::Send(const std::string &str)
{
	_transport->Send(str.c_str(), str.size());
}

unsigned int FTPConnection::RecvResponce(std::string &str)
{
	char match[4];
	str.clear();
	for (size_t cnt = 0, ofs = 0;;) {
		char ch;
		do {
			if (str.size() > 0x100000) {
				throw std::runtime_error("too long responce");
			}
			ssize_t r = _transport->Recv(&ch, sizeof(ch));
			if (r <= 0) {
				throw std::runtime_error(StrPrintf("read responce error %d", errno));
			}
			str+= ch;
		} while (ch != '\n');

		size_t p = str.size() - 1;
		++cnt;
		if (cnt == 1 && p - ofs > 3 && str[ofs + 3] == '-') {
			memcpy(match, str.c_str() + ofs, 3);
			match[3] = ' ';

		} else if (cnt == 1 || (p >= 4 && memcmp(str.c_str() + ofs, match, 4) == 0)) {
			unsigned int out = atoi(str.c_str() + ofs);
			if (out == 0) {
				out = atoi(str.c_str());
			}
			return out;
		}

		ofs = p + 1;
	}
}

unsigned int FTPConnection::SendRecvResponce(std::string &str)
{
	Send(str);
	return RecvResponce(str);
}

void FTPConnection::RecvResponce(std::string &str, unsigned int reply_ok_min, unsigned int reply_ok_max)
{
	unsigned int reply_code = RecvResponce(str);
	FTPThrowIfBadResponce(str, reply_code, reply_ok_min, reply_ok_max);
}

void FTPConnection::SendRecvResponce(std::string &str, unsigned int reply_ok_min, unsigned int reply_ok_max)
{
	unsigned int reply_code = SendRecvResponce(str);
	FTPThrowIfBadResponce(str, reply_code, reply_ok_min, reply_ok_max);
}

void FTPConnection::SendRestIfNeeded(unsigned long long rest)
{
	if (rest != 0) {
		std::string str = StrPrintf("REST %lld\r\n", rest);
		unsigned int reply_code = SendRecvResponce(str);
		if (reply_code < 300 || reply_code >= 400) {
			throw ProtocolError(StrPrintf(
				"REST %lld rejected: %u '%s'",
				rest, reply_code, str.c_str()));
		}
	}
}	

void FTPConnection::DataCommand_PASV(std::shared_ptr<BaseTransport> &data_transport, const std::string &cmd, unsigned long long rest)
{
	std::string str = "PASV\r\n";
	SendRecvResponce(str, 200, 299);

	size_t p = str.find('(');
	if (p == std::string::npos) {
		throw ProtocolError(str);
	}

	unsigned int v[6];
	sscanf(str.c_str() + p + 1,"%u,%u,%u,%u,%u,%u", &v[0], &v[1], &v[2], &v[3], &v[4], &v[5]);

	SendRestIfNeeded(rest);

	_transport->Send(cmd.c_str(), cmd.size());

	sockaddr_in sin = {0};
	sin.sin_family = AF_INET;
	sin.sin_port = (uint16_t)((v[5] << 8) | v[4]);
	sin.sin_addr.s_addr = (uint32_t)((v[3] << 24) | (v[2] << 16) | (v[1] << 8) | (v[0]));
	data_transport = std::make_shared<SocketTransport>(sin, _protocol_options);

	RecvResponce(str, 100, 199);
}

void FTPConnection::DataCommand_PORT(std::shared_ptr<BaseTransport> &data_transport, const std::string &cmd, unsigned long long rest)
{
	sockaddr_in sin = {};
	_transport->GetLocalAddress(sin);
	sin.sin_port = 0;

	FDScope srv_sock(socket(PF_INET, SOCK_STREAM, IPPROTO_TCP));
	if (srv_sock == -1) {
		throw std::runtime_error(StrPrintf("socket() errno=%d", errno));
	}

	if (bind(srv_sock, (struct sockaddr *)&sin, sizeof(sin)) == -1) {
		throw std::runtime_error(StrPrintf("bind() errno=%d", errno));
	}

	if (listen(srv_sock, 1) == -1) {
		throw std::runtime_error(StrPrintf("bind() errno=%d", errno));
	}

	sockaddr sa = {};
	socklen_t l = sizeof(sa);
	if (getsockname(srv_sock, &sa, &l) == -1) {
		throw std::runtime_error(StrPrintf("getsockname() errno=%d", errno));
	}

	std::string str = StrPrintf("PORT %u,%u,%u,%u,%u,%u\r\n",
		(unsigned int)(unsigned char)sa.sa_data[2], (unsigned int)(unsigned char)sa.sa_data[3],
		(unsigned int)(unsigned char)sa.sa_data[4], (unsigned int)(unsigned char)sa.sa_data[5],
		(unsigned int)(unsigned char)sa.sa_data[0], (unsigned int)(unsigned char)sa.sa_data[1]);

	SendRecvResponce(str, 200, 299);

	SendRestIfNeeded(rest);

	str = cmd;
	SendRecvResponce(str, 100, 199);

	for (;;) {
		l = sizeof(sin);
		int sc = accept(srv_sock, (struct sockaddr *)&sin, &l);
		if (sc != -1) {
			data_transport = std::make_shared<SocketTransport>(sc, _protocol_options);
			break;
		}
	}
}


std::shared_ptr<BaseTransport> FTPConnection::DataCommand(const std::string &cmd, unsigned long long rest)
{
	std::shared_ptr<BaseTransport> data_transport;
	if (_protocol_options.GetInt("Passive", 1) == 1) {
		DataCommand_PASV(data_transport, cmd, rest);

	} else {
		DataCommand_PORT(data_transport, cmd, rest);
	}

#ifdef HAVE_OPENSSL
	if (_encryption) {
		data_transport = std::make_shared<TLSTransport>(data_transport->DetachSocket(), _protocol_options);
	}
#endif

	return data_transport;
}

