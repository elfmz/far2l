#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>

#include <utils.h>
#include <ScopeHelpers.h>
#include <os_call.hpp>

#include "FTPConnection.h"

#define FTP_ENDLINE "\r\n"


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
	if (protocol_options.GetInt("TcpNoDelay", 1) ) {
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

static int SSLStartup()
{
	int out = OpenSSL_add_ssl_algorithms();
	SSL_load_error_strings();
	return out;
}

int OpenSSLContext::sNewClientSessionCB(SSL *ssl, SSL_SESSION *session)
{
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
	OpenSSLContext *ctx = (OpenSSLContext *)SSL_get_app_data(ssl);
	if (ctx != NULL && ctx->_session == NULL && session != NULL) {
		SSL_SESSION_up_ref(session);
		ctx->_session = session;
	}
#else
	ABORT();
#endif
	return 0;
}

OpenSSLContext::OpenSSLContext(const StringConfig &protocol_options)
{
	static int s_ssl_startup = SSLStartup();
	if (!s_ssl_startup) {
		s_ssl_startup = SSLStartup();
		if (!s_ssl_startup) {
			fprintf(stderr, "TLSTransport: SSLStartup failed\n");
		}
	}

	_ctx = SSL_CTX_new(SSLv23_client_method());

	if (!_ctx) {
		throw std::runtime_error(StrPrintf("SSL_CTX_new() errno=%d", errno));
	}

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

#if OPENSSL_VERSION_NUMBER >= 0x10100000L
	SSL_CTX_sess_set_new_cb(_ctx, sNewClientSessionCB);
	SSL_CTX_set_session_cache_mode(_ctx,
		SSL_SESS_CACHE_CLIENT | SSL_SESS_CACHE_NO_INTERNAL_STORE | SSL_SESS_CACHE_NO_AUTO_CLEAR);
#else
	SSL_CTX_set_session_cache_mode(_ctx, SSL_SESS_CACHE_CLIENT | SSL_SESS_CACHE_NO_AUTO_CLEAR);
#endif
}

OpenSSLContext::~OpenSSLContext()
{
	if (_session) {
		SSL_SESSION_free(_session);
	}
	SSL_CTX_free(_ctx);
}

SSL *OpenSSLContext::NewSSL(int sock)
{
	SSL *ssl = SSL_new(_ctx);
	if (!ssl) {
		throw std::runtime_error(StrPrintf("SSL_new() errno=%d", errno));
	}

#if OPENSSL_VERSION_NUMBER >= 0x10100000L
	SSL_set_app_data(ssl, this);

	if (_session) {
		SSL_set_session(ssl, _session);
		_session = NULL; // "TLS 1.3 does strongly encourage single-use of resumption ticket"
	}

#else
	if (_session) {
		SSL_set_session(ssl, _session);
	}
#endif

	if (SSL_set_fd(ssl, sock) != 1) {
		SSL_free(ssl);
		throw std::runtime_error(StrPrintf("SSL_set_fd() errno=%d", errno));
	}

	if (SSL_connect(ssl) != 1) {
		SSL_shutdown(ssl);
		SSL_free(ssl);
		throw std::runtime_error(StrPrintf("SSL_connect() errno=%d", errno));
	}

#if OPENSSL_VERSION_NUMBER < 0x10100000L
//  Doesn't work since TLS1.3 that requires storing session from sNewClientSessionCB
//  for correct TLS session reuse functionality, however old OpenSSL doesn't have
//  SSL_SESSION_up_ref that makes it incompatible with TLS1.3 servers that require
//  session reuse.
	if (!_session) {
		_session = SSL_get1_session(ssl);
	}
#endif

	return ssl;
}

///

TLSTransport::TLSTransport(std::shared_ptr<OpenSSLContext> &ctx, int sock)
	: _ctx(ctx)
{
	_sock = sock;

	_ssl = _ctx->NewSSL(sock);
}

TLSTransport::~TLSTransport()
{
	Shutdown();
}

void TLSTransport::Shutdown()
{
	if (_ssl != nullptr)
	{
		SSL_shutdown(_ssl);
		SSL_free(_ssl);
		_ssl = nullptr;
	}
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

std::string TLSTransport::GetPeerFingerprint()
{
	std::string out;
	X509 *cert = SSL_get_peer_certificate(_ssl);
	if (cert) {
		unsigned char digest[EVP_MAX_MD_SIZE];
		unsigned int digest_size = sizeof(digest);

#ifndef OPENSSL_NO_SHA256
		if (X509_digest(cert, EVP_sha256(), digest, &digest_size))
#else
		if (X509_digest(cert, EVP_md5(), digest, &digest_size))
#endif
		{
			for (unsigned int i = 0; i != digest_size; ++i) {
				out+= StrPrintf("%02x", (unsigned int)digest[i]);
			}
		}
		X509_free(cert);
	}

	return out;
}

#endif

FTPConnection::FTPConnection(bool implicit_encryption, const std::string &host, unsigned int port, const std::string &options)
	: _protocol_options(options)
{
	// sleep(30);
	_batch.pipelining = _protocol_options.GetInt("CommandsPipelining", 0) != 0;
	if (g_netrocks_verbosity > 1) {
		fprintf(stderr, "FTPConnection(%d, %s, %u) _batch.pipelining=%u\n",
			implicit_encryption, host.c_str(), port, _batch.pipelining);
	}

	if (!port) {
		port = implicit_encryption ? 990 : 21;
	}

	_transport = std::make_shared<SocketTransport>(host, port, _protocol_options);

	std::string str;

#ifdef HAVE_OPENSSL
	if (implicit_encryption) {
		_openssl_ctx = std::make_shared<OpenSSLContext>(_protocol_options);

	} else if (_protocol_options.GetInt("ExplicitEncryption", 0)) {
		unsigned int banner_code = RecvResponseFromTransport(str);
		FTPThrowIfBadResponse(str, banner_code, 200, 299);

		str = "AUTH TLS";
		unsigned int reply_code = SendRecvResponse(str);
		if (reply_code != 234) {
			if (_protocol_options.GetInt("EncryptionProtocol", 3) < 3) {
				str = "AUTH SSL";
				reply_code = SendRecvResponse(str);
				if (reply_code == 234) {
					fprintf(stderr, "FTPConnection: AUTH SSL\n");
				}
			}

			if (reply_code != 234) {
				throw ProtocolError(str);
			}

		} else {
			fprintf(stderr, "FTPConnection: AUTH TLS\n");
		}

		_openssl_ctx = std::make_shared<OpenSSLContext>(_protocol_options);
	}

	if (_openssl_ctx) {
		auto tls_transport = std::make_shared<TLSTransport>(_openssl_ctx, _transport->DetachSocket());
		const std::string &fingerprint = tls_transport->GetPeerFingerprint();
		if (fingerprint != _protocol_options.GetString("ServerIdentity"))
			throw ServerIdentityMismatchError(fingerprint);

		_transport = tls_transport;

	}

	if (!_openssl_ctx || implicit_encryption) {
		unsigned int banner_code = RecvResponseFromTransport(str);
		FTPThrowIfBadResponse(str, banner_code, 200, 299);
	}

#else
	if (implicit_encryption || _protocol_options.GetInt("ExplicitEncryption", 0)) {
		throw ProtocolError("Encrypted FTP requires OpenSSL support");
	}

	unsigned int banner_code = RecvResponseFromTransport(str);
	FTPThrowIfBadResponse(str, banner_code, 200, 299);
#endif

}

FTPConnection::~FTPConnection()
{
}

bool FTPConnection::EnableDataConnectionProtection()
{
	SendRequest("PBSZ 0");
	SendRequest("PROT P");

	std::string str_pbsz, str_prot;

	unsigned int code_pbsz = RecvResponse(str_pbsz);
	unsigned int code_prot = RecvResponse(str_prot);

	if (code_pbsz != 200) {
		fprintf(stderr, "FTPConnection::EnableDataConnectionProtection: <PBSZ 0> - '%s'\n", str_pbsz.c_str());
		return false;
	}

	if (code_prot != 200) {
		fprintf(stderr, "FTPConnection::EnableDataConnectionProtection: <PROT P> - '%s'\n", str_prot.c_str());
		return false;
	}

	fprintf(stderr, "FTPConnection::EnableDataConnectionProtection: OK\n");
	return true;
}

unsigned int FTPConnection::RecvResponseFromTransport(std::string &str)
{
	char match[4];
	str.clear();
	for (size_t cnt = 0, ofs = 0;;) {
		char ch;
		do {
			if (str.size() > 0x100000) {
				throw std::runtime_error("too long response");
			}
			ssize_t r = _transport->Recv(&ch, sizeof(ch));
			if (r <= 0) {
				throw std::runtime_error(StrPrintf("read response error %d", errno));
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


void FTPConnection::BatchPerform()
{
	if (_batch.requests.empty()) {
		return;
	}

	if (_batch.pipelining && _batch.requests.size() > 1) {
		//_batch.responses.reserve(_batch.responses.size() + _batch.requests.size());
		_str.clear();
		for (const auto &r : _batch.requests) {
			_str+= r;
		}
		_transport->Send(_str.c_str(), _str.size());
		for (size_t i = _batch.requests.size(); i != 0; --i) {
			_batch.responses.emplace_back();
			_batch.responses.back().code = RecvResponseFromTransport(_batch.responses.back().str);
		}

	} else {
		for (const auto &r : _batch.requests) {
			_transport->Send(r.c_str(), r.size());
			_batch.responses.emplace_back();
			_batch.responses.back().code = RecvResponseFromTransport(_batch.responses.back().str);
		}
	}

	_batch.requests.clear();
}

unsigned int FTPConnection::SendRecvResponse(std::string &str)
{
	if (g_netrocks_verbosity > 1) {
		fprintf(stderr, "FTPConnection::SendRecvResponse('%s')\n", str.c_str());
		for (const auto &r : _batch.requests) {
			fprintf(stderr, "FTPConnection::SendRecvResponse: pending request '%s'\n", r.c_str());
		}
		for (const auto &r : _batch.responses) {
			fprintf(stderr, "FTPConnection::SendRecvResponse: pending response '%s'\n", r.str.c_str());
		}
	}

	if (!_batch.requests.empty() || !_batch.responses.empty()) {
		throw std::runtime_error("FTPConnection::SendRecvResponse while batch not empty");
	}

	str+= FTP_ENDLINE;
	_transport->Send(str.c_str(), str.size());

	unsigned int reply_code = RecvResponseFromTransport(str);

	if (g_netrocks_verbosity > 1) {
		fprintf(stderr, "FTPConnection::SendRecvResponse: response '%s'\n", str.c_str());
	}

	return reply_code;
}

void FTPConnection::SendRequest(const std::string &str)
{
	_batch.requests.emplace_back(str);
	_batch.requests.back().append(FTP_ENDLINE);

	if (g_netrocks_verbosity > 1) {
		fprintf(stderr, "FTPConnection::SendRequest('%s')\n", str.c_str());
	}
}

unsigned int FTPConnection::RecvResponse(std::string &str)
{
	BatchPerform();
	if (_batch.responses.empty()) {
		throw std::runtime_error("FTPConnection::RecvResponse: batch is empty");
	}

	str.swap(_batch.responses.front().str);
	unsigned int reply_code = _batch.responses.front().code;
	_batch.responses.pop_front();

	if (g_netrocks_verbosity > 1) {
		fprintf(stderr, "FTPConnection::RecvResponse: %s\n", str.c_str());
	}

	return reply_code;
}

void FTPConnection::RecvResponse(std::string &str, unsigned int reply_ok_min, unsigned int reply_ok_max)
{
	unsigned int reply_code = RecvResponse(str);
	FTPThrowIfBadResponse(str, reply_code, reply_ok_min, reply_ok_max);
}

void FTPConnection::SendRecvResponse(std::string &str, unsigned int reply_ok_min, unsigned int reply_ok_max)
{
	unsigned int reply_code = SendRecvResponse(str);
	FTPThrowIfBadResponse(str, reply_code, reply_ok_min, reply_ok_max);
}

void FTPConnection::SendRestIfNeeded(unsigned long long rest)
{
	if (rest != 0) {
		std::string str = StrPrintf("REST %lld", rest);
		unsigned int reply_code = SendRecvResponse(str);
		if (reply_code < 300 || reply_code >= 400) {
			throw ProtocolError(StrPrintf(
				"REST %lld rejected: %u '%s'",
				rest, reply_code, str.c_str()));
		}
	}
}

void FTPConnection::DataCommand_PASV(std::shared_ptr<BaseTransport> &data_transport, const std::string &cmd, unsigned long long rest)
{
	std::string str = "PASV";
	SendRecvResponse(str, 200, 299);

	size_t p = str.find('(');
	if (p == std::string::npos) {
		throw ProtocolError(str);
	}

	unsigned int v[6];
	sscanf(str.c_str() + p + 1,"%u,%u,%u,%u,%u,%u", &v[0], &v[1], &v[2], &v[3], &v[4], &v[5]);
	// doesn't make sense if (_protocol_options.GetInt("RestrictDataPeer", 1) != 0)

	SendRestIfNeeded(rest);

//	str = cmd;
//	str+= FTP_ENDLINE;
//	 _transport->Send(str.c_str(), str.size());

	sockaddr_in sin = {0};
	sin.sin_family = AF_INET;
	sin.sin_port = (uint16_t)((v[5] << 8) | v[4]);
	//sin.sin_addr.s_addr = (uint32_t)((v[3] << 24) | (v[2] << 16) | (v[1] << 8) | (v[0]));
	struct sockaddr_in peer_sin{};
	_transport->GetPeerAddress(peer_sin);
	sin.sin_addr = peer_sin.sin_addr;
	data_transport = std::make_shared<SocketTransport>(sin, _protocol_options);

	str = cmd;
	SendRecvResponse(str, 100, 199);
//	unsigned int reply_code = RecvResponseFromTransport(str);
//	FTPThrowIfBadResponse(str, reply_code, 100, 199);
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

	std::string str = StrPrintf("PORT %u,%u,%u,%u,%u,%u",
		(unsigned int)(unsigned char)sa.sa_data[2], (unsigned int)(unsigned char)sa.sa_data[3],
		(unsigned int)(unsigned char)sa.sa_data[4], (unsigned int)(unsigned char)sa.sa_data[5],
		(unsigned int)(unsigned char)sa.sa_data[0], (unsigned int)(unsigned char)sa.sa_data[1]);

	SendRecvResponse(str, 200, 299);

	SendRestIfNeeded(rest);

	str = cmd;
	SendRecvResponse(str, 100, 199);

	for (;;) {
		l = sizeof(sin);
		int sc = accept(srv_sock, (struct sockaddr *)&sin, &l);
		if (sc != -1) {
			data_transport = std::make_shared<SocketTransport>(sc, _protocol_options);
			if (_protocol_options.GetInt("RestrictDataPeer", 1) != 0) {
				struct sockaddr_in cmd_sin {}, data_sin {};
				_transport->GetPeerAddress(cmd_sin);
				data_transport->GetPeerAddress(data_sin);
				if (cmd_sin.sin_addr.s_addr != data_sin.sin_addr.s_addr) {
					fprintf(stderr, "Address mismatch: 0x%x != 0x%x\n",
						cmd_sin.sin_addr.s_addr, data_sin.sin_addr.s_addr);
					throw ProtocolError("Unmatched data and command connection address");
				}
			}
			break;
		}
	}
}


void FTPConnection::EnsureDataConnectionProtection()
{
#ifdef HAVE_OPENSSL
	if (_openssl_ctx && !_data_encryption_enabled) {
		_data_encryption_enabled = EnableDataConnectionProtection();
	}
#endif
}

std::shared_ptr<BaseTransport> FTPConnection::DataCommand(const std::string &cmd, unsigned long long rest)
{
	EnsureDataConnectionProtection();
	std::shared_ptr<BaseTransport> data_transport;
	if (_protocol_options.GetInt("Passive", 1) == 1) {
		DataCommand_PASV(data_transport, cmd, rest);

	} else {
		DataCommand_PORT(data_transport, cmd, rest);
	}

#ifdef HAVE_OPENSSL
	if (_data_encryption_enabled) {
		auto tls_transport = std::make_shared<TLSTransport>(_openssl_ctx, data_transport->DetachSocket());
		if (_protocol_options.GetInt("RestrictDataPeer", 1) != 0) {
			const std::string &fingerprint = tls_transport->GetPeerFingerprint();
			if (fingerprint != _protocol_options.GetString("ServerIdentity")) {
				throw ProtocolError("Unmatched data and command connection certificates");
			}
		}
		data_transport = tls_transport;

	} else if (_openssl_ctx && _protocol_options.GetInt("RestrictDataPeer", 1) != 0) {
		throw ProtocolError("Unencrypted data connection");
	}
#endif

	return data_transport;
}
