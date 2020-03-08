#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <map>
#include <set>
#include <vector>
#include <string>

#include <sys/types.h>

#ifdef HAVE_OPENSSL
# include <openssl/ssl.h>
#endif

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <utils.h>
#include <ScopeHelpers.h>
#include <os_call.hpp>


#include "ProtocolFTP.h"



struct BaseTransport : public std::enable_shared_from_this<BaseTransport>
{
	virtual ~BaseTransport() {}

	void Send(const void *data, size_t len)
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

	ssize_t Recv(void *data, size_t len)
	{
		return RecvImpl(data, len);
	}

	virtual int DetachSocket()
	{
		return _sock.Detach();
	}

	void GetPeerAddress(struct sockaddr_in &sin)
	{
		socklen_t l = sizeof(sin);
		if (getpeername(_sock, (struct sockaddr *)&sin, &l) == -1) {
			throw std::runtime_error(StrPrintf("getpeername errno=%d", errno));
		}
	}

	void GetLocalAddress(struct sockaddr_in &sin)
	{
		socklen_t l = sizeof(sin);
		if (getsockname(_sock, (struct sockaddr *)&sin, &l) == -1) {
			throw std::runtime_error(StrPrintf("getsockname errno=%d", errno));
		}
	}

protected:
	FDScope _sock;

	virtual ssize_t SendImpl(const void *data, size_t len) = 0;
	virtual ssize_t RecvImpl(void *data, size_t len) = 0;
};

struct SocketTransport : public BaseTransport
{
	SocketTransport(int sock, const StringConfig &protocol_options)
	{
		_sock = sock;
		ApplySocketOptions(protocol_options);
	}

	SocketTransport(struct sockaddr_in &sin, const StringConfig &protocol_options)
	{
		Connect(sin, protocol_options);
	}

	SocketTransport(const std::string &host, unsigned int port, const StringConfig &protocol_options)
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

protected:
	void ApplySocketOptions(const StringConfig &protocol_options)
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

	void Connect(struct sockaddr_in &sin, const StringConfig &protocol_options)
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

	virtual ssize_t SendImpl(const void *data, size_t len)
	{
		return os_call_ssize(send, (int)_sock, data, len, 0);
	}

	virtual ssize_t RecvImpl(void *data, size_t len)
	{
		return os_call_ssize(recv, (int)_sock, data, len, 0);
	}
};

#ifdef HAVE_OPENSSL
struct TLSTransport : public BaseTransport
{
	SSL_CTX *_ctx = nullptr;
	SSL *_ssl = nullptr;

	void Shutdown()
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

public:
	TLSTransport(int sock, StringConfig &protocol_options)
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

	virtual ~TLSTransport()
	{
		Shutdown();
	}

	virtual ssize_t SendImpl(const void *data, size_t len)
	{
		if (!_ssl) {
			return -1;
		}

		return SSL_write(_ssl, data, len);
	}

	virtual ssize_t RecvImpl(void *data, size_t len)
	{
		if (!_ssl) {
			return -1;
		}

		return SSL_read(_ssl, data, len);
	}

	virtual int DetachSocket()
	{
		Shutdown();
		return BaseTransport::DetachSocket();
	}
};
#endif

struct FTPConnection : public std::enable_shared_from_this<FTPConnection>
{
	std::shared_ptr<BaseTransport> _transport;
	StringConfig _protocol_options;
	bool _encryption;

public:
	FTPConnection(bool implicit_encryption, const std::string &host, unsigned int port, const std::string &options)
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

	virtual ~FTPConnection()
	{
	}

	unsigned int RecvResponce(std::string &str)
	{
		char match[4];
		str.clear();
		for (size_t cnt = 0, ofs = 0;;) {
			char buf[0x1000];
			if (str.size() > 0x100000) {
				throw std::runtime_error("too long responce");
			}
			ssize_t r = _transport->Recv(buf, sizeof(buf));
			if (r <= 0) {
				throw std::runtime_error(StrPrintf("read responce error %d", errno));
			}
			str.append(buf, r);
			size_t p;
			while ( (p = str.find('\n', ofs)) != std::string::npos) {
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
	}

	unsigned int SendRecvResponce(std::string &str)
	{
		_transport->Send(str.c_str(), str.size());
		return RecvResponce(str);
	}

	void SendRestIfNeeded(unsigned long long rest)
	{
		if (rest != 0) {
			std::string str = StrPrintf("REST %lld\r\n", rest);
			unsigned int reply_code = SendRecvResponce(str);
			if (reply_code < 300 || reply_code >= 400) {
				throw std::runtime_error(StrPrintf(
					"REST %lld rejected: %u '%s'",
					rest, reply_code, str.c_str()));
			}
		}
	}
			

	std::shared_ptr<BaseTransport> DataCommand(const std::string &cmd, unsigned long long rest = 0)
	{
		std::shared_ptr<BaseTransport> data_transport;
		if (_protocol_options.GetInt("Passive") == 1) {
			std::string str = "PASV\r\n";
			unsigned int reply_code = SendRecvResponce(str);
			if (reply_code < 200 || reply_code >= 300) {
				throw std::runtime_error(StrPrintf("PASV rejected: %u '%s'", reply_code, str.c_str()));
			}
			size_t p = str.find('(');
			if (p == std::string::npos) {
				throw std::runtime_error(StrPrintf("PASV bad reply: %u '%s'", reply_code, str.c_str()));
			}

			unsigned int v[6];
			sscanf(str.c_str() + p + 1,"%u,%u,%u,%u,%u,%u", &v[2], &v[3], &v[4], &v[5], &v[0], &v[1]);

			SendRestIfNeeded(rest);

			_transport->Send(cmd.c_str(), cmd.size());

			sockaddr_in sin = {0};
			//_transport->GetPeerAddress(sin);
			sin.sin_family = AF_INET;
			sin.sin_port = (uint16_t)((v[5] << 8) + v[4]);
			sin.sin_addr.s_addr = (uint32_t)((v[0] << 24) + (v[1] << 16) + (v[2] << 8) + (v[3]));

			data_transport = std::make_shared<SocketTransport>(sin, _protocol_options);

			reply_code = RecvResponce(str);
			if (reply_code < 100 || reply_code >= 200) {
				throw std::runtime_error(StrPrintf("'%s' rejected: %u '%s'", cmd.c_str(), reply_code, str.c_str()));
			}

		} else {
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

			std::string str = StrPrintf("PORT %u,%u,%u,%u,%u,%u\r\n", (unsigned int)sa.sa_data[2],
				(unsigned int)sa.sa_data[3], (unsigned int)sa.sa_data[4], (unsigned int)sa.sa_data[5],
				(unsigned int)sa.sa_data[0], (unsigned int)sa.sa_data[1]);
			unsigned int reply_code = SendRecvResponce(str);
			if (reply_code < 200 || reply_code >= 300) {
				throw std::runtime_error(StrPrintf("PORT rejected: %u '%s'", reply_code, str.c_str()));
			}

			SendRestIfNeeded(rest);

			str = cmd;
			reply_code = SendRecvResponce(str);
			if (reply_code < 100 || reply_code >= 200) {
				throw std::runtime_error(StrPrintf("'%s' rejected: %u '%s'", cmd.c_str(), reply_code, str.c_str()));
			}

			for (;;) {
				l = sizeof(sin);
				int sc = accept(srv_sock, (struct sockaddr *)&sin, &l);
				if (sc != -1) {
					data_transport = std::make_shared<SocketTransport>(sc, _protocol_options);
					break;
				}
			}

		}

#ifdef HAVE_OPENSSL
		if (_protocol_options.GetInt("Encryption") != 0) {
			data_transport = std::make_shared<TLSTransport>(data_transport->DetachSocket(), _protocol_options);
		}
#endif

		return data_transport;
	}
};

std::shared_ptr<IProtocol> CreateProtocol(const std::string &protocol, const std::string &host, unsigned int port,
	const std::string &username, const std::string &password, const std::string &options) throw (std::runtime_error)
{
	return std::make_shared<ProtocolFTP>(protocol, host, port, username, password, options);
}


ProtocolFTP::ProtocolFTP(const std::string &protocol, const std::string &host, unsigned int port,
	const std::string &username, const std::string &password, const std::string &options) throw (std::runtime_error)
	:
	_conn(std::make_shared<FTPConnection>( (strcasecmp(protocol.c_str(), "ftps") == 0), host, port, options))
{
	std::string str;
	unsigned int reply_code = _conn->RecvResponce(str);
	if (reply_code < 200 || reply_code >= 300) {
		throw ProtocolError(str);
	}

	str = "USER ";
	str+= username;
	str+= "\r\n";

	reply_code = _conn->SendRecvResponce(str);
	if (reply_code >= 300 && reply_code < 400) {
		str = "PASS ";
		str+= password;
		str+= "\r\n";
		reply_code = _conn->SendRecvResponce(str);
	}

	if (reply_code < 200 || reply_code >= 300) {
		throw ProtocolAuthFailedError(str);
	}
}

ProtocolFTP::~ProtocolFTP()
{
}

static const char match__unix_mode[] = "UNIX.mode";
static const char match__unix_uid[] = "UNIX.uid";
static const char match__unix_gid[] = "UNIX.gid";

static const char match__type[] = "type";
static const char match__perm[] = "perm";
static const char match__size[] = "size";
static const char match__sizd[] = "sizd";

static const char match__file[] = "file";
static const char match__dir[] = "dir";
static const char match__cdir[] = "cdir";
static const char match__pdir[] = "pdir";

#define MATCH_SUBSTR(str, len, match) (sizeof(match) == (len) + 1 && CaseIgnoreEngStrMatch(str, match, sizeof(match) - 1))

static bool ParseXLSTLine(const char *line, const char *end, FileInformation &file_info, uid_t *uid = nullptr, gid_t *gid = nullptr, std::string *name = nullptr)
{
	file_info = FileInformation();

	bool has_mode = false, has_type = false, has_any_known = false;
	const char *perm = nullptr, *perm_end = nullptr;

	for (; line != end && *line == ' '; ++line) {;}

	const char *fact, *eq;
	for (fact = eq = line; line != end; ++line) if (*line == '=') {
		eq = line;

	} else if (*line ==';') {
		if (eq > fact) {
			const size_t name_len = eq - fact;
			const size_t value_len = line - (eq + 1);

			if (MATCH_SUBSTR(fact, name_len, match__unix_mode)) {
				file_info.mode = strtol(eq + 1, nullptr, (*(eq + 1) == '0') ? 8 : 10);
				has_any_known = has_mode = true;

			} else if (MATCH_SUBSTR(fact, name_len, match__unix_uid)) {
				if (uid) *uid = strtol(eq + 1, nullptr, 10);
				has_any_known = true;

			} else if (MATCH_SUBSTR(fact, name_len, match__unix_gid)) {
				if (gid) *gid = strtol(eq + 1, nullptr, 10);
				has_any_known = true;

			} else if (MATCH_SUBSTR(fact, name_len, match__type)) {
				if (MATCH_SUBSTR(eq + 1, value_len, match__file)) {
					file_info.mode|= S_IFREG;
					has_type = true;
					has_any_known = true;

				} else if (MATCH_SUBSTR(eq + 1, value_len, match__dir)
				  || MATCH_SUBSTR(eq + 1, value_len, match__cdir)
				  || MATCH_SUBSTR(eq + 1, value_len, match__pdir)) {
					file_info.mode|= S_IFDIR;
					has_type = true;
					has_any_known = true;

				} else if (g_netrocks_verbosity > 0) {
					fprintf(stderr, "ParseMNLSTLine: unknown type='%s'\n",
						std::string(eq + 1, value_len).c_str());
				}

			} else if (MATCH_SUBSTR(fact, name_len, match__perm)) {
				perm = eq + 1;
				perm_end = line;
				has_any_known = true;

			} else if (MATCH_SUBSTR(fact, name_len, match__size)
			  || MATCH_SUBSTR(fact, name_len, match__sizd)) {
				file_info.size = strtol(eq + 1, nullptr, 10);
				has_any_known = true;
			}
		}

		fact = line + 1;
	}

	if (name) {
		if (line != end && *line == ' ') {
			++line;
		}
		if (line != end) {
			name->assign(line, end - line);
		} else {
			name->clear();
		}
	}

	if (!has_type)  {
		if (perm) {
			if (CaseIgnoreEngStrChr('c', perm, perm_end - perm) != nullptr
			  || CaseIgnoreEngStrChr('l', perm, perm_end - perm) != nullptr) {
				file_info.mode|= S_IFDIR;

			} else {
				file_info.mode|= S_IFREG;
			}

		} else {
			file_info.mode|= S_IFREG; // last resort
		}
	}

	if (!has_mode)  {
		if (perm) {
			if (CaseIgnoreEngStrChr('r', perm, perm_end - perm) != nullptr
			  || CaseIgnoreEngStrChr('l', perm, perm_end - perm) != nullptr
			  || CaseIgnoreEngStrChr('e', perm, perm_end - perm) != nullptr) {
				file_info.mode|= 0444;
				if ((file_info.mode & S_IFMT) == S_IFDIR) {
					file_info.mode|= 0111;
				}
			}
			if (CaseIgnoreEngStrChr('w', perm, perm_end - perm) != nullptr
			  || CaseIgnoreEngStrChr('a', perm, perm_end - perm) != nullptr
			  || CaseIgnoreEngStrChr('c', perm, perm_end - perm) != nullptr) {
				file_info.mode|= 0220;
			}
			if (CaseIgnoreEngStrChr('x', perm, perm_end - perm) != nullptr
			  || CaseIgnoreEngStrChr('e', perm, perm_end - perm) != nullptr) {
				file_info.mode|= 0111;
			}

		} else if ((file_info.mode & S_IFMT) == S_IFDIR) {
			file_info.mode|= DEFAULT_ACCESS_MODE_DIRECTORY; // last resort

		} else {
			file_info.mode|= DEFAULT_ACCESS_MODE_FILE; // last resort
		}
	}

	return has_any_known;
}

ProtocolFTP::CommandAvailability ProtocolFTP::MLst(const std::string &path, FileInformation &file_info, uid_t *uid, gid_t *gid)
{
	if (_ca_mlst == CA_UNAVAILABLE) {
		return _ca_mlst;
	}

	std::string str = "MLst ";
	str+= path;
	str+= "\r\n";
	unsigned int reply_code = _conn->SendRecvResponce(str);
	if (reply_code >= 500 && reply_code <= 504) {
		if (_ca_mlst == CA_UNKNOWN) {
			_ca_mlst = CA_UNAVAILABLE;
		}
		return CA_UNAVAILABLE;
	}

	std::vector<std::string> lines;
	StrExplode(lines, str, "\n");
	if (reply_code != 250) {
		throw ProtocolError("Not found", reply_code);
	}

	const std::string &line = lines[ (lines.size() > 1) ? 1 : 0 ];
	if (!ParseXLSTLine(line.c_str(), line.c_str() + line.size(), file_info, uid, gid)) {
		return CA_UNAVAILABLE;
	}

	return _ca_mlst;
}

mode_t ProtocolFTP::GetMode(const std::string &path, bool follow_symlink) throw (std::runtime_error)
{
	FileInformation file_info;
	MLst(path, file_info);
	return file_info.mode;
}

unsigned long long ProtocolFTP::GetSize(const std::string &path, bool follow_symlink) throw (std::runtime_error)
{
	FileInformation file_info;
	MLst(path, file_info);
	return file_info.size;
}

void ProtocolFTP::GetInformation(FileInformation &file_info, const std::string &path, bool follow_symlink) throw (std::runtime_error)
{
	MLst(path, file_info);
}

void ProtocolFTP::FileDelete(const std::string &path) throw (std::runtime_error)
{
}

void ProtocolFTP::DirectoryDelete(const std::string &path) throw (std::runtime_error)
{
}

void ProtocolFTP::DirectoryCreate(const std::string &path, mode_t mode) throw (std::runtime_error)
{
}

void ProtocolFTP::Rename(const std::string &path_old, const std::string &path_new) throw (std::runtime_error)
{
}


void ProtocolFTP::SetTimes(const std::string &path, const timespec &access_time, const timespec &modification_time) throw (std::runtime_error)
{
}

void ProtocolFTP::SetMode(const std::string &path, mode_t mode) throw (std::runtime_error)
{
}

void ProtocolFTP::SymlinkCreate(const std::string &link_path, const std::string &link_target) throw (std::runtime_error)
{
	throw ProtocolUnsupportedError("Symlink creation unsupported");
}

void ProtocolFTP::SymlinkQuery(const std::string &link_path, std::string &link_target) throw (std::runtime_error)
{
	throw ProtocolUnsupportedError("Symlink querying unsupported");
}

class FTPDirectoryEnumer : public IDirectoryEnumer
{
	std::shared_ptr<ProtocolFTP> _protocol;
	std::string _rooted_path;

public:
	FTPDirectoryEnumer(std::shared_ptr<ProtocolFTP> protocol, const std::string &rooted_path)
		: _protocol(protocol),
		_rooted_path(rooted_path)
	{
		fprintf(stderr, "FTPDirectoryEnumer: '%s'\n", _rooted_path.c_str());
	}

	virtual ~FTPDirectoryEnumer()
	{
	}

	virtual bool Enum(std::string &name, std::string &owner, std::string &group, FileInformation &file_info) throw (std::runtime_error)
	{
		return false;
	}
};


std::shared_ptr<IDirectoryEnumer> ProtocolFTP::DirectoryEnum(const std::string &path) throw (std::runtime_error)
{
	return std::shared_ptr<IDirectoryEnumer>(new FTPDirectoryEnumer(shared_from_this(), path));
}


std::shared_ptr<IFileReader> ProtocolFTP::FileGet(const std::string &path, unsigned long long resume_pos) throw (std::runtime_error)
{
	return std::shared_ptr<IFileReader>();
//	return std::make_shared<SMBFileIO>(shared_from_this(), path, O_RDONLY, 0, resume_pos);
}

std::shared_ptr<IFileWriter> ProtocolFTP::FilePut(const std::string &path, mode_t mode, unsigned long long size_hint, unsigned long long resume_pos) throw (std::runtime_error)
{
	return std::shared_ptr<IFileWriter>();
//	return std::make_shared<SMBFileIO>(shared_from_this(), path, O_WRONLY | O_CREAT | (resume_pos ? 0 : O_TRUNC), mode, resume_pos);
}
