#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <map>
#include <set>
#include <vector>
#include <string>

#include "ProtocolFTP.h"


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
	_conn->RecvResponce(str, 200, 299);

	str = "USER ";
	str+= username;
	str+= "\r\n";

	unsigned int reply_code = _conn->SendRecvResponce(str);
	if (reply_code >= 300 && reply_code <= 399) {
		str = "PASS ";
		str+= password;
		str+= "\r\n";
		reply_code = _conn->SendRecvResponce(str);
	}

	FTPThrowIfBadResponce<ProtocolAuthFailedError>(str, reply_code, 200, 299);

	str = "FEAT\r\n";
	reply_code = _conn->SendRecvResponce(str);
	if (reply_code >= 200 && reply_code < 299) {
		if (str.find("MLST") != std::string::npos) _feat_mlst = true;
		if (str.find("MLSD") != std::string::npos) _feat_mlsd = true;
		if (str.find("REST") != std::string::npos) _feat_rest = true;
		if (str.find("SIZE") != std::string::npos) _feat_size = true;		
	}
}

ProtocolFTP::~ProtocolFTP()
{
}

std::string ProtocolFTP::Navigate(const std::string &path_name)
{
	std::vector<std::string> parts;
	StrExplode(parts, path_name, "/");

	for (auto it = parts.begin(); it != parts.end();) {
		if (it->empty()) {
			it = parts.erase(it);
		} else {
			++it;
		}
	}
	if (parts.empty()) {
		fprintf(stderr, "ProtocolFTP::Navigate('%s') - empty path_name\n", path_name.c_str());
		return std::string();
	}

	std::string name_part = parts.back();
	parts.pop_back();

	if (parts != _cwd) {
		std::string str = "CWD\r\n";
		unsigned int reply_code = _conn->SendRecvResponce(str);
		if (reply_code < 200 || reply_code > 299) {
			for (;!_cwd.empty(); _cwd.pop_back()) {
				str = "CDUP\r\n";
				reply_code = _conn->SendRecvResponce(str);
				if (reply_code < 200 || reply_code > 299) {
					str = "CWD ..\r\n";
					reply_code = _conn->SendRecvResponce(str);
					FTPThrowIfBadResponce(str, reply_code, 200, 299);
				}
			}

		} else {
			_cwd.clear();
		}

		for (const auto &part : parts) {
			str = "CWD ";
			str+= part;
			str+= "\r\n";
			_conn->SendRecvResponce(str, 200, 299);
			_cwd.emplace_back(part);
		}
	}

	return name_part;
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

static bool ParseMLsxLine(const char *line, const char *end, FileInformation &file_info, uid_t *uid = nullptr, gid_t *gid = nullptr, std::string *name = nullptr)
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
				file_info.mode|= strtol(eq + 1, nullptr, (*(eq + 1) == '0') ? 8 : 10);
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
					fprintf(stderr, "ParseMLsxLine: unknown type='%s'\n",
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
		if (fact != end && *fact == ' ') {
			++fact;
		}
		if (fact != end) {
			name->assign(fact, end - fact);
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
//	fprintf(stderr, "type:%s\n", ((file_info.mode & S_IFMT) == S_IFDIR) ? "DIR" : "FILE");


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

void ProtocolFTP::MLst(const std::string &path, FileInformation &file_info, uid_t *uid, gid_t *gid)
{
	std::string str = "MLST ";
	str+= path;
	str+= "\r\n";
	unsigned int reply_code = _conn->SendRecvResponce(str);
	if (reply_code >= 500 && reply_code <= 504) {
		throw ProtocolError("MLst not supported", reply_code);
	}

	std::vector<std::string> lines;
	StrExplode(lines, str, "\n");
	if (reply_code != 250) {
		throw ProtocolError("Not found", reply_code);
	}

	const std::string &line = lines[ (lines.size() > 1) ? 1 : 0 ];
	if (!ParseMLsxLine(line.c_str(), line.c_str() + line.size(), file_info, uid, gid)) {
		throw ProtocolError("MLst responce uninformative");
	}
}

mode_t ProtocolFTP::GetMode(const std::string &path, bool follow_symlink) throw (std::runtime_error)
{
	const std::string &name_part = Navigate(path);
	FileInformation file_info;
	if (_feat_mlst) {
		MLst(name_part, file_info);
	}
	return file_info.mode;
}

unsigned long long ProtocolFTP::GetSize(const std::string &path, bool follow_symlink) throw (std::runtime_error)
{
	const std::string &name_part = Navigate(path);
	FileInformation file_info;
	if (_feat_mlst) {
		MLst(name_part, file_info);
	}
	return file_info.size;
}

void ProtocolFTP::GetInformation(FileInformation &file_info, const std::string &path, bool follow_symlink) throw (std::runtime_error)
{
	const std::string &name_part = Navigate(path);
	if (_feat_mlst) {
		MLst(name_part, file_info);
	}
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

class FTPDataCommand
{
protected:
	std::shared_ptr<FTPConnection> _conn;
	std::shared_ptr<BaseTransport> _data_transport;

	void EnsureFinalized()
	{
		_data_transport.reset();
		if (_conn) {
			std::shared_ptr<FTPConnection> conn = _conn;
			_conn.reset();
			std::string str;
			conn->RecvResponce(str);
		}
	}

public:
	FTPDataCommand(std::shared_ptr<FTPConnection> &conn, std::shared_ptr<BaseTransport> &data_transport)
		: _conn(conn),
		_data_transport(data_transport)
	{
	}

	virtual ~FTPDataCommand()
	{
		try {
			EnsureFinalized();
		} catch (std::exception &e) {
			fprintf(stderr, "~FTPDataCommand: %s\n", e.what());
		}
	}
};

class FTPDirectoryEnumerMLSD : protected FTPDataCommand, public IDirectoryEnumer
{
	std::string _read_buffer;

public:
	using FTPDataCommand::FTPDataCommand;

	virtual bool Enum(std::string &name, std::string &owner, std::string &group, FileInformation &file_info) throw (std::runtime_error)
	{
		for (;;) {
			for (;;) {
				size_t p = _read_buffer.find('\n');
				if (p == std::string::npos) {
					break;
				}
				if (p > 0 && _read_buffer[p - 1] == '\r') {
					--p;
				}

				uid_t uid = 0;
				gid_t gid = 0;

				bool found = (ParseMLsxLine(_read_buffer.c_str(), _read_buffer.c_str() + p,
					file_info, &uid, &gid, &name) && FILENAME_ENUMERABLE(name.c_str()));

//			fprintf(stderr, "MLSD line: '%s' - '%s' %d\n",_read_buffer.substr(0, p).c_str(), name.c_str(), found);

				do {
					++p;
				} while ( p < _read_buffer.size() && (_read_buffer[p] == '\r' || _read_buffer[p] == '\n'));

				_read_buffer.erase(0, p);

				if (found) {
					owner = StrPrintf("uid:%lu", (unsigned long)uid);
					group = StrPrintf("gid:%lu", (unsigned long)gid);
					return true;
				}
			}

			if (_read_buffer.size() > 0x100000) {
				throw ProtocolError("Too long line in MLSD response");
			}
			char buf[0x1000];
			ssize_t r = _data_transport->Recv(buf, sizeof(buf));
			if (r <= 0) {
				EnsureFinalized();
				return false;
			}
			_read_buffer.append(buf, r);
		}
	}
};


std::shared_ptr<IDirectoryEnumer> ProtocolFTP::DirectoryEnum(const std::string &path) throw (std::runtime_error)
{
	const std::string &name_part = Navigate(path);
	if (_feat_mlsd) {
		std::string cmd = "MLSD ";
		cmd+= name_part;
		cmd+= "\r\n";
		std::shared_ptr<BaseTransport> data_transport = _conn->DataCommand(cmd);
		return std::shared_ptr<IDirectoryEnumer>(new FTPDirectoryEnumerMLSD(_conn, data_transport));
	}

	throw ProtocolError("TODO");
//	return std::shared_ptr<IDirectoryEnumer>(new FTPDirectoryEnumer(shared_from_this(), path));
}


class FTPFileIO : protected FTPDataCommand, public IFileReader, public IFileWriter
{
public:
	using FTPDataCommand::FTPDataCommand;

	virtual size_t Read(void *buf, size_t len) throw (std::runtime_error)
	{
		return _data_transport->Recv(buf, len);
	}

	virtual void Write(const void *buf, size_t len) throw (std::runtime_error)
	{
		return _data_transport->Send(buf, len);
	}

	virtual void WriteComplete() throw (std::runtime_error)
	{
		EnsureFinalized();
	}
};

std::shared_ptr<IFileReader> ProtocolFTP::FileGet(const std::string &path, unsigned long long resume_pos) throw (std::runtime_error)
{
	const std::string &name_part = Navigate(path);
	std::string cmd = "RETR ";
	cmd+= name_part;
	cmd+= "\r\n";
	std::shared_ptr<BaseTransport> data_transport = _conn->DataCommand(cmd, resume_pos);
	return std::make_shared<FTPFileIO>(_conn, data_transport);
}

std::shared_ptr<IFileWriter> ProtocolFTP::FilePut(const std::string &path, mode_t mode, unsigned long long size_hint, unsigned long long resume_pos) throw (std::runtime_error)
{
	const std::string &name_part = Navigate(path);
	std::string cmd = "STOR ";
	cmd+= name_part;
	cmd+= "\r\n";
	std::shared_ptr<BaseTransport> data_transport = _conn->DataCommand(cmd, resume_pos);
	return std::make_shared<FTPFileIO>(_conn, data_transport);
}

