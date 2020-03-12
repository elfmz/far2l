#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <map>
#include <set>
#include <vector>
#include <string>

#include "ProtocolFTP.h"
#include "FTPParseMLST.h"
#include "FTPParseLIST.h"


std::shared_ptr<IProtocol> CreateProtocol(const std::string &protocol, const std::string &host, unsigned int port,
	const std::string &username, const std::string &password, const std::string &options) throw (std::runtime_error)
{
	return std::make_shared<ProtocolFTP>(protocol, host, port, username, password, options);
}


ProtocolFTP::ProtocolFTP(const std::string &protocol, const std::string &host, unsigned int port,
	const std::string &username, const std::string &password, const std::string &options) throw (std::runtime_error)
	:
	_conn(std::make_shared<FTPConnection>( (strcasecmp(protocol.c_str(), "ftps") == 0), host, port, options)),
	_dir_enum_cache(10)
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

	str = "TYPE I\r\n";
	_conn->SendRecvResponce(str);
}

ProtocolFTP::~ProtocolFTP()
{
}

std::string ProtocolFTP::Navigate(const std::string &path_name)
{
	std::vector<std::string> parts;
	StrExplode(parts, path_name, "/");

	for (auto it = parts.begin(); it != parts.end();) {
		if (it->empty() || *it == ".") {
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


void ProtocolFTP::MLst(const std::string &path, FileInformation &file_info, uid_t *uid, gid_t *gid)
{
//	const std::string &name_part = Navigate(path + "/.");
	std::string str = "MLST ";
	str+= path;
	str+= "\r\n";
	unsigned int reply_code = _conn->SendRecvResponce(str);
	if (reply_code >= 500 && reply_code <= 504) {
		throw ProtocolError(str);
	}

	std::vector<std::string> lines;
	StrExplode(lines, str, "\n");
	if (reply_code != 250) {
		throw ProtocolError(str);
	}

	const std::string &line = lines[ (lines.size() > 1) ? 1 : 0 ];
	if (!ParseMLsxLine(line.c_str(), line.c_str() + line.size(), file_info, uid, gid)) {
		throw ProtocolError("MLst responce uninformative");
	}
}

void ProtocolFTP::GetInformation(FileInformation &file_info, const std::string &path, bool follow_symlink) throw (std::runtime_error)
{
	std::string name_part = Navigate(path);
#if 0
	if (_feat_mlst) {
		MLst(name_part, file_info);
	}
#endif

	if (name_part.empty()) {
		file_info = FileInformation();
		file_info.mode = S_IFDIR | DEFAULT_ACCESS_MODE_DIRECTORY;
		return;
	}

	std::shared_ptr<IDirectoryEnumer> enumer = NavigatedDirectoryEnum();
	std::string enum_name, enum_owner, enum_group;
	do {
		if (!enumer->Enum(enum_name, enum_owner, enum_group, file_info)) {
			throw ProtocolError("File not found");
		}
	} while (name_part != enum_name);
}


mode_t ProtocolFTP::GetMode(const std::string &path, bool follow_symlink) throw (std::runtime_error)
{
	FileInformation file_info;
	GetInformation(file_info, path, follow_symlink);
	return file_info.mode;
}

unsigned long long ProtocolFTP::GetSize(const std::string &path, bool follow_symlink) throw (std::runtime_error)
{
	FileInformation file_info;
	GetInformation(file_info, path, follow_symlink);
	return file_info.size;
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

class FTPBaseDirectoryEnumer : protected FTPDataCommand, public IDirectoryEnumer
{
	std::string _read_buffer;

protected:
	virtual bool OnParseLine(const char *buf, size_t len, std::string &name, std::string &owner, std::string &group, FileInformation &file_info) throw (std::runtime_error) = 0;

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

				bool line_parsed = OnParseLine(_read_buffer.c_str(), p, name, owner, group, file_info);

				do {
					++p;
				} while ( p < _read_buffer.size() && (_read_buffer[p] == '\r' || _read_buffer[p] == '\n'));

				_read_buffer.erase(0, p);

				if (line_parsed) {
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


class FTPDirectoryEnumerMLSD : public FTPBaseDirectoryEnumer
{

protected:

	virtual bool OnParseLine(const char *buf, size_t len, std::string &name,
		std::string &owner, std::string &group, FileInformation &file_info) throw (std::runtime_error)
	{
		uid_t uid = 0;
		gid_t gid = 0;

		if (!ParseMLsxLine(buf, buf + len, file_info, &uid, &gid, &name)) {
			return false;
		}

		if (!FILENAME_ENUMERABLE(name.c_str())) {
			return false;
		}

		owner = StrPrintf("uid:%lu", (unsigned long)uid);
		group = StrPrintf("gid:%lu", (unsigned long)gid);
		return true;
	}

public:
	using FTPBaseDirectoryEnumer::FTPBaseDirectoryEnumer;
};

class FTPDirectoryEnumerLIST : public FTPBaseDirectoryEnumer
{

protected:

	virtual bool OnParseLine(const char *buf, size_t len, std::string &name,
		std::string &owner, std::string &group, FileInformation &file_info) throw (std::runtime_error)
	{
		struct ftpparse fp{};

		if (ftpparse(&fp, buf, (int)len) != 1 || !fp.name || !fp.namelen) {
			return false;
		}

		name.assign(fp.name, fp.namelen);

		if (!FILENAME_ENUMERABLE(name.c_str())) {
			return false;
		}


		file_info.mode = fp.mode;
		file_info.size = fp.size;
		if (fp.flagtryretr) {
			file_info.mode|= S_IXUSR | S_IXGRP | S_IXOTH;
		}
		owner = fp.owner;
		group = fp.group;
		file_info.access_time.tv_sec
			= file_info.status_change_time.tv_sec
				= file_info.modification_time.tv_sec = fp.mtime;
		return true;
	}

public:
	using FTPBaseDirectoryEnumer::FTPBaseDirectoryEnumer;
};

std::shared_ptr<IDirectoryEnumer> ProtocolFTP::DirectoryEnum(const std::string &path) throw (std::runtime_error)
{
	Navigate(path + "/*");
	return NavigatedDirectoryEnum();
}

std::shared_ptr<IDirectoryEnumer> ProtocolFTP::NavigatedDirectoryEnum()
{
	std::string str = "PWD\r\n", pwd;
	unsigned int reply = _conn->SendRecvResponce(str);
	if (reply == 257) {
		size_t p = str.find('\"');
		if (p != std::string::npos) {
			str.erase(0, p + 1);
			p = str.find('\"');
			if (p != std::string::npos && p != 0) {
				str.resize(p);
				pwd.swap(str);
				auto cached_enumer = _dir_enum_cache.GetCachedDirectoryEnumer(pwd);
				if (cached_enumer) {
					fprintf(stderr, "Cached enum '%s'\n", pwd.c_str());
					return cached_enumer;
				}
			}
		}
	}

	std::shared_ptr<IDirectoryEnumer> enumer;
/*
	const std::string &name_part = Navigate(path);
	if (_feat_mlsd) {
		str = "MLSD\r\n";
		std::shared_ptr<BaseTransport> data_transport = _conn->DataCommand(str);
		enumer std::shared_ptr<IDirectoryEnumer>(new FTPDirectoryEnumerMLSD(_conn, data_transport));
	}
*/
	str = "LIST\r\n";
	std::shared_ptr<BaseTransport> data_transport = _conn->DataCommand(str);

	enumer = std::shared_ptr<IDirectoryEnumer>(new FTPDirectoryEnumerLIST(_conn, data_transport));

	if (!pwd.empty()) {
		fprintf(stderr, "Caching enum '%s'\n", pwd.c_str());
		enumer = _dir_enum_cache.GetCachingWrapperDirectoryEnumer(pwd, enumer);
	}

	return enumer;
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

