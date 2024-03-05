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
	const std::string &username, const std::string &password, const std::string &options, int fd_ipc_recv)
{
	return std::make_shared<ProtocolFTP>(protocol, host, port, username, password, options);
}


ProtocolFTP::ProtocolFTP(const std::string &protocol, const std::string &host, unsigned int port,
	const std::string &username, const std::string &password, const std::string &options)
	:
	_conn(std::make_shared<FTPConnection>( (strcasecmp(protocol.c_str(), "ftps") == 0), host, port, options)),
	_dir_enum_cache(10)
{
	_conn->EnsureDataConnectionProtection();

	_str.assign("USER ").append(username);

	unsigned int reply_code = _conn->SendRecvResponse(_str);
	if (reply_code >= 300 && reply_code <= 399) {
		_str.assign("PASS ").append(password);
		reply_code = _conn->SendRecvResponse(_str);
	}

	FTPThrowIfBadResponse<ProtocolAuthFailedError>(_str, reply_code, 200, 299);

	_cmd.list_ = _conn->ProtocolOptions().GetString("ListCommand", "");
	if (_conn->ProtocolOptions().GetInt("MLSDMLST", 1) != 0) {
		_str.assign("FEAT");
		reply_code = _conn->SendRecvResponse(_str);
		if (reply_code >= 200 && reply_code < 299) {
			if (_str.find("MLST") != std::string::npos) _cmd.mlst = "MLST";
			if (_str.find("MLSD") != std::string::npos) _cmd.mlsd = "MLSD";
		}
	}

	_conn->SendRequest("TYPE I");
	_conn->SendRequest("PWD");
	_conn->RecvResponse(_str);

	if (!RecvPwdResponse()) {
		_conn->SendRequest("XPWD");
		if (RecvPwdResponse()) {
			_cmd.pwd = "XPWD";
			_cmd.cwd = "XCWD";
			_cmd.mkd = "XMKD";
			_cmd.rmd = "XRMD";
		} else {
			fprintf(stderr, "ProtocolFTP: RecvPwdResponse failed - ITS VERY BAD\n");
		}
	}

	fprintf(stderr, "ProtocolFTP: %s %s %s\n",
		_cmd.pwd, _cmd.mlst ? _cmd.mlst : "", _cmd.mlsd ? _cmd.mlsd : "");

	_cwd.home = _cwd.path;
	if (_cwd.home[_cwd.home.size() - 1] != '/') {
		_cwd.home+= '/';
	}
}

ProtocolFTP::~ProtocolFTP()
{
}

static void PathExplode(std::vector<std::string> &parts, const std::string &path)
{
	parts.clear();
	StrExplode(parts, path, "/");

	for (auto it = parts.begin(); it != parts.end();) {
		if (it->empty() || *it == ".") {
			it = parts.erase(it);
		} else {
			++it;
		}
	}
}

bool ProtocolFTP::RecvPwdResponse()
{
	_str.clear();
	unsigned int reply = _conn->RecvResponse(_str);
	if (reply != 257) {
		fprintf(stderr, "ProtocolFTP::RecvPwdResponse: reply=%u '%s'\n", reply, _str.c_str());
		return false;
	}

	size_t b = _str.find('\"');
	size_t e = _str.rfind('\"');
	if (b == std::string::npos || b == e) {
		fprintf(stderr, "ProtocolFTP::RecvPwdResponse: bad quotes '%s'\n", _str.c_str());
		return false;
	}

	if (g_netrocks_verbosity > 1) {
		fprintf(stderr, "ProtocolFTP::RecvPwdResponse: '%s'\n", _str.c_str());
	}

	_str.resize(e);
	_str.erase(0, b + 1);

	if ('/' != *_str.c_str()) {
		_str.insert(0, "/");
	}

	_cwd.path.swap(_str);
	PathExplode(_cwd.parts, _cwd.path);

	return true;
}

std::string ProtocolFTP::SplitPathAndNavigate(const std::string &path_name, bool allow_empty_name_part)
{
	if ('/' != *path_name.c_str()) {
		std::string full_path_name = _cwd.home;
		full_path_name+= path_name;
		return SplitPathAndNavigate(full_path_name, allow_empty_name_part);
	}

	std::string name_part;

	std::vector<std::string> parts;
	PathExplode(parts, path_name);
	if (!parts.empty()) {
		name_part = parts.back();
		parts.pop_back();

	} else if (!allow_empty_name_part) {
		throw ProtocolError( StrPrintf(
			"ProtocolFTP::SplitPathAndNavigate('%s') - empty path_name\n", path_name.c_str()));
//		fprintf(stderr, "ProtocolFTP::SplitPathAndNavigate('%s') - empty path_name\n", path_name.c_str());
//		return std::string();
	}

	if (g_netrocks_verbosity > 0) {
		fprintf(stderr, "ProtocolFTP::SplitPathAndNavigate('%s') prev='%s'\n", path_name.c_str(), _cwd.path.c_str());
	}


	if (parts != _cwd.parts) {
		// first try most compatible way - sequence of CDUP followed by sequence of CWD
		size_t i;
		for (i = 0; (i != parts.size() && i != _cwd.parts.size() && parts[i] == _cwd.parts[i]); ++i) {
			;
		}

		if (g_netrocks_verbosity > 1) {
			fprintf(stderr, "ProtocolFTP::SplitPathAndNavigate: i=%lu _cwd.parts.size()=%lu parts.size()=%lu\n",
				(unsigned long)i, (unsigned long)_cwd.parts.size(), (unsigned long)parts.size());
		}

		for (size_t j = i; j != _cwd.parts.size(); ++j) {
			_conn->SendRequest(_cmd.cdup);
		}
		for (size_t j = i; j != parts.size(); ++j) {
			_str.assign(_cmd.cwd).append(" ").append(parts[j]);
			_conn->SendRequest(_str);
		}
		_conn->SendRequest(_cmd.pwd);

		bool ok = true;

		 // fetch all responses except PWD
		for (size_t j = 0, jj = (_cwd.parts.size() - i) + (parts.size() - i); j != jj; ++j) {
			try {
				_conn->RecvResponse(_str, 200, 299);
				if (g_netrocks_verbosity > 1) {
					fprintf(stderr, "ProtocolFTP::SplitPathAndNavigate: reply='%s'\n", _str.c_str());
				}
			} catch (std::exception &e) {
				fprintf(stderr, "ProtocolFTP::SplitPathAndNavigate('%s') - %s, part %lu of %lu\n",
					path_name.c_str(), e.what(), (unsigned long)j, (unsigned long)jj);
				ok = false;
			}
		}

		std::string fail_str;
		if (!ok) {
			// alternative way: CWD-to-home followed by CWD-where-we-want-to-be
			_conn->RecvResponse(_str); // skip PWD response for now, will send another one

			ok = true;
			// try other way: reset to home and apply full path
			_conn->SendRequest(_cmd.cwd);
			_str.assign(_cmd.cwd).append(" ");
			for (const auto &part : parts) {
				_str.append(part).append(1, '/');
			}
			_conn->SendRequest(_str);
			_conn->SendRequest(_cmd.pwd);
			try {
				_conn->RecvResponse(_str, 200, 299);
				if (g_netrocks_verbosity > 1) {
					fprintf(stderr, "ProtocolFTP::SplitPathAndNavigate: homey CWD reply='%s'\n", _str.c_str());
				}

			} catch (std::exception &e) {
				ok = false;
				fail_str = e.what();
				fprintf(stderr, "SplitPathAndNavigate('%s') homey CWD - %s\n", path_name.c_str(), e.what());
			}
			try {
				_conn->RecvResponse(_str, 200, 299);
				if (g_netrocks_verbosity > 1) {
					fprintf(stderr, "ProtocolFTP::SplitPathAndNavigate: full CWD reply='%s'\n", _str.c_str());
				}

			} catch (std::exception &e) {
				ok = false;
				if (fail_str.empty()) {
					fail_str = e.what();
				}
				fprintf(stderr, "ProtocolFTP::SplitPathAndNavigate('%s') full CWD - %s\n", path_name.c_str(), e.what());
			}
		}

		if (!RecvPwdResponse()) {
			_cwd.parts = parts;
			_cwd.path.clear();
			for (const auto &part : parts) {
				_cwd.path.append(1, '/').append(part);
			}
		}

		if (!ok) {
			// nothing worked, bail out with error
			throw ProtocolError(fail_str);
		}
	}

	return name_part;
}

std::string ProtocolFTP::PathAsRelative(const std::string &path)
{
	std::vector<std::string> parts;
	PathExplode(parts, path);

	size_t i;
	for (i = 0; (i < parts.size() && i < _cwd.parts.size() && parts[i] == _cwd.parts[i]); ++i) {
		;
	}

	std::string out;

	for (size_t j = i; j < _cwd.parts.size(); ++j) {
		if (out.empty()) {
			out+= "..";
		} else {
			out+= "/..";
		}
	}

	for (size_t j = i; j < parts.size(); ++j) {
		if (!out.empty()) {
			out+= '/';
		}
		out+= parts[j];
	}

	return out;
}

void ProtocolFTP::MLst(const std::string &path, FileInformation &file_info, uid_t *uid, gid_t *gid, std::string *lnkto)
{
	_str.assign(_cmd.mlst).append(" ").append(path);

	unsigned int reply_code = _conn->SendRecvResponse(_str);
	if (reply_code >= 500 && reply_code <= 504) {
		throw ProtocolError(_str);
	}

	std::vector<std::string> lines;
	StrExplode(lines, _str, "\n");
	if (reply_code != 250) {
		throw ProtocolError(_str);
	}

	const std::string &line = lines[ (lines.size() > 1) ? 1 : 0 ];
	if (!ParseMLsxLine(line.c_str(), line.c_str() + line.size(), file_info, uid, gid, nullptr, lnkto)) {
		throw ProtocolError("MLst response uninformative");
	}
}

void ProtocolFTP::GetInformation(FileInformation &file_info, const std::string &path, bool follow_symlink)
{
	const std::string &name_part = SplitPathAndNavigate(path, true);
	if (name_part.empty()) {
		file_info = FileInformation();
		file_info.mode = S_IFDIR | DEFAULT_ACCESS_MODE_DIRECTORY;
		return;
	}

	if (_cmd.mlst != nullptr) {
		MLst(name_part, file_info);

	} else {
		std::shared_ptr<IDirectoryEnumer> enumer = NavigatedDirectoryEnum();
		std::string enum_name, enum_owner, enum_group;
		do {
			if (!enumer->Enum(enum_name, enum_owner, enum_group, file_info)) {
				throw ProtocolError("File not found");
			}
		} while (name_part != enum_name);
	}

	if (follow_symlink && S_ISLNK(file_info.mode)) {
		_str.assign(_cmd.size).append(" ").append(name_part);
		unsigned int reply_code = _conn->SendRecvResponse(_str);
		file_info.mode&= ~S_IFMT;
		file_info.mode|= S_IFDIR;
		if (reply_code == 213) {
			size_t p = _str.find(' ');
			if (p != std::string::npos) {
				while (p < _str.size() && _str[p] == ' ') {
					++p;
				}
				file_info.size = atol(_str.c_str() + p);
				file_info.mode&= ~S_IFMT;
				file_info.mode|= S_IFREG;
			}
		}
	}
}


mode_t ProtocolFTP::GetMode(const std::string &path, bool follow_symlink)
{
	FileInformation file_info;
	GetInformation(file_info, path, follow_symlink);
	return file_info.mode;
}

unsigned long long ProtocolFTP::GetSize(const std::string &path, bool follow_symlink)
{
	FileInformation file_info;
	GetInformation(file_info, path, follow_symlink);
	return file_info.size;
}


void ProtocolFTP::SimpleDispositionCommand(const char *cmd, const std::string &path)
{
	const std::string &name_part = SplitPathAndNavigate(path);
	_str.assign(cmd).append(" ").append(name_part);

	unsigned int reply_code = _conn->SendRecvResponse(_str);

	if (reply_code < 200 || reply_code > 299) {
		throw ProtocolError(_str);
	}

	if (_dir_enum_cache.HasValidEntries()) {
		_dir_enum_cache.Remove(_cwd.path);
	}
}

void ProtocolFTP::FileDelete(const std::string &path)
{
	SimpleDispositionCommand(_cmd.dele, path);
}

void ProtocolFTP::DirectoryDelete(const std::string &path)
{
	SimpleDispositionCommand(_cmd.rmd, path);
}

void ProtocolFTP::DirectoryCreate(const std::string &path, mode_t mode)
{
	SimpleDispositionCommand(_cmd.mkd, path);
	SetMode(path, mode);
}

void ProtocolFTP::Rename(const std::string &path_old, const std::string &path_new)
{
	const std::string &name_old = SplitPathAndNavigate(path_old);
	const std::string &path_new_relative = PathAsRelative(path_new);

	_str.assign(_cmd.rnfr).append(" ").append(name_old);
	unsigned int reply_code = _conn->SendRecvResponse(_str);
	if (reply_code != 350) {
		FTPThrowIfBadResponse(_str, reply_code, 200, 299);
	}

	_str.assign(_cmd.rnto).append(" ").append(path_new_relative);

	reply_code = _conn->SendRecvResponse(_str);
	FTPThrowIfBadResponse(_str, reply_code, 200, 299);

	if (_dir_enum_cache.HasValidEntries()) {
		_dir_enum_cache.Remove(_cwd.path);
	}
	SplitPathAndNavigate(path_new);
	if (_dir_enum_cache.HasValidEntries()) {
		_dir_enum_cache.Remove(_cwd.path);
	}
}


void ProtocolFTP::SetTimes(const std::string &path, const timespec &access_time, const timespec &modification_time)
{
	if (_cmd.mfmt == nullptr) {
		return;
	}

	const std::string &name_part = SplitPathAndNavigate(path);

	struct tm t {};
	if (gmtime_r(&modification_time.tv_sec, &t) == NULL) {
		return;
	}
	//MFMT YYYYMMDDHHMMSS path, where:
	//YYYY - the 4-digit year
	//MM - the 2-digit month
	//DD - the 2-digit day of the month
	//HH - the hour in 24-hour format
	//MM - the minute
	//SS - the seconds
	_str = StrPrintf("%s %04u%02u%02u%02u%02u%02u %s", _cmd.mfmt,
		(unsigned int)t.tm_year + 1900, (unsigned int)t.tm_mon,
		(unsigned int)t.tm_mday, (unsigned int)t.tm_hour,
		(unsigned int)t.tm_min, (unsigned int)t.tm_sec,
		name_part.c_str());

	unsigned int reply_code = _conn->SendRecvResponse(_str);

	if (reply_code < 200 || reply_code > 299) {
		if (reply_code >= 500 && reply_code <= 504) {
			fprintf(stderr, "ProtocolFTP::SetTimes('%s'): '%s' - assume unsupported\n",
				path.c_str(), _str.c_str());
			_cmd.mfmt = nullptr;

		} else {
			throw ProtocolError(_str);
		}

	} else if (_dir_enum_cache.HasValidEntries()) {
		_dir_enum_cache.Remove(_cwd.path);
	}
}

void ProtocolFTP::SetMode(const std::string &path, mode_t mode)
{
	if (_cmd.chmod == nullptr) {
		return;
	}

	const std::string &name_part = SplitPathAndNavigate(path);

	_str = StrPrintf("%s %03o %s", _cmd.chmod, (unsigned int)mode, name_part.c_str());
	unsigned int reply_code = _conn->SendRecvResponse(_str);

	if (reply_code < 200 || reply_code > 299) {
		if (reply_code >= 500 && reply_code <= 504) {
			fprintf(stderr, "ProtocolFTP::SetMode('%s', %03o): '%s' - assume unsupported\n",
				path.c_str(), mode, _str.c_str());
			_cmd.chmod = nullptr;

		} else {
			throw ProtocolError(_str);
		}

	} else if (_dir_enum_cache.HasValidEntries()) {
		_dir_enum_cache.Remove(_cwd.path);
	}
}

void ProtocolFTP::SymlinkCreate(const std::string &link_path, const std::string &link_target)
{
	throw ProtocolUnsupportedError("Symlink creation unsupported");
}

void ProtocolFTP::SymlinkQuery(const std::string &link_path, std::string &link_target)
{
	if (_cmd.mlst != nullptr) {
		const std::string &name_part = SplitPathAndNavigate(link_path, true);
		if (!name_part.empty()) {
			FileInformation file_info;
			MLst(name_part, file_info, nullptr, nullptr, &link_target);
			return;
		}
	}

	link_target.clear();
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
			conn->RecvResponseFromTransport(str);
			if (g_netrocks_verbosity > 1) {
				fprintf(stderr, "FTPDataCommand::EnsureFinalized: response '%s'\n", str.c_str());
			}
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
	virtual bool OnParseLine(const char *buf, size_t len, std::string &name, std::string &owner, std::string &group, FileInformation &file_info) = 0;

public:
	using FTPDataCommand::FTPDataCommand;

	virtual bool Enum(std::string &name, std::string &owner, std::string &group, FileInformation &file_info)
	{
		if (!_data_transport) {
			return false;
		}

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
		std::string &owner, std::string &group, FileInformation &file_info)
	{
		uid_t uid = 0;
		gid_t gid = 0;

		if (!ParseMLsxLine(buf, buf + len, file_info, &uid, &gid, &name, nullptr)) {
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
	time_t default_mtime = time(NULL);

protected:

	virtual bool OnParseLine(const char *buf, size_t len, std::string &name,
		std::string &owner, std::string &group, FileInformation &file_info)
	{
		struct ftpparse fp{};

		if (ftpparse(&fp, buf, (int)len) != 1 || !fp.name || !fp.namelen) {
			return false;
		}

		name.assign(fp.name, fp.namelen);

		if (!FILENAME_ENUMERABLE(name.c_str())) {
			return false;
		}

		file_info = FileInformation();

		file_info.mode = fp.mode;
		file_info.size = fp.size;
		if (fp.flagtryretr) {
			file_info.mode|= S_IXUSR | S_IXGRP | S_IXOTH;
		}
		owner = fp.owner;
		group = fp.group;
		file_info.access_time.tv_sec
			= file_info.status_change_time.tv_sec
				= file_info.modification_time.tv_sec
					= (fp.mtimetype == FTPPARSE_MTIME_UNKNOWN) ? default_mtime : fp.mtime;

		return true;
	}

public:
	using FTPBaseDirectoryEnumer::FTPBaseDirectoryEnumer;
};

std::shared_ptr<IDirectoryEnumer> ProtocolFTP::DirectoryEnum(const std::string &path)
{
	SplitPathAndNavigate(path + "/*");
	return NavigatedDirectoryEnum();
}

std::shared_ptr<IDirectoryEnumer> ProtocolFTP::NavigatedDirectoryEnum()
{
	std::shared_ptr<IDirectoryEnumer> enumer = _dir_enum_cache.GetCachedDirectoryEnumer(_cwd.path);
	if (enumer) {
		if (g_netrocks_verbosity > 0) {
			fprintf(stderr, "Cached enum '%s'\n", _cwd.path.c_str());
		}
		return enumer;
	}

//	const std::string &name_part = SplitPathAndNavigate(path);
	if (_cmd.mlsd != nullptr) {
		std::shared_ptr<BaseTransport> data_transport = _conn->DataCommand(std::string(_cmd.mlsd));
		enumer = std::shared_ptr<IDirectoryEnumer>(new FTPDirectoryEnumerMLSD(_conn, data_transport));

	} else {
		if (_cmd.list_.empty()) {
			try {
				std::shared_ptr<BaseTransport> tmp_data_transport = _conn->DataCommand("LIST -la");
				auto tmp_enumer = std::shared_ptr<IDirectoryEnumer>(new FTPDirectoryEnumerLIST(_conn, tmp_data_transport));
				std::string tmp_name, tmp_owner, tmp_group;
				FileInformation tmp_file_info;
				tmp_enumer->Enum(tmp_name, tmp_owner, tmp_group, tmp_file_info);
				_cmd.list_ = "LIST -la";
			} catch (std::exception &) {
				_cmd.list_ = "LIST";
			}
			fprintf(stderr, "NR/FTP - list cmd: '%s'\n", _cmd.list_.c_str());
		}

		std::shared_ptr<BaseTransport> data_transport = _conn->DataCommand(_cmd.list_);
		enumer = std::shared_ptr<IDirectoryEnumer>(new FTPDirectoryEnumerLIST(_conn, data_transport));
	}

	if (g_netrocks_verbosity > 0) {
		fprintf(stderr, "Caching enum '%s'\n", _cwd.path.c_str());
	}
	enumer = _dir_enum_cache.GetCachingWrapperDirectoryEnumer(_cwd.path, enumer);

	return enumer;
}


class FTPFileIO : protected FTPDataCommand, public IFileReader, public IFileWriter
{
public:
	using FTPDataCommand::FTPDataCommand;

	virtual size_t Read(void *buf, size_t len)
	{
		return _data_transport->Recv(buf, len);
	}

	virtual void Write(const void *buf, size_t len)
	{
		return _data_transport->Send(buf, len);
	}

	virtual void WriteComplete()
	{
		EnsureFinalized();
	}
};

std::shared_ptr<IFileReader> ProtocolFTP::FileGet(const std::string &path, unsigned long long resume_pos)
{
	const std::string &name_part = SplitPathAndNavigate(path);
	_str.assign(_cmd.retr).append(" ").append(name_part);
	std::shared_ptr<BaseTransport> data_transport = _conn->DataCommand(_str, resume_pos);
	return std::make_shared<FTPFileIO>(_conn, data_transport);
}

std::shared_ptr<IFileWriter> ProtocolFTP::FilePut(const std::string &path, mode_t mode, unsigned long long size_hint, unsigned long long resume_pos)
{
	const std::string &name_part = SplitPathAndNavigate(path);

	if (_dir_enum_cache.HasValidEntries()) {
		_dir_enum_cache.Remove(_cwd.path);
	}

	_str.assign(_cmd.stor).append(" ").append(name_part);
	std::shared_ptr<BaseTransport> data_transport = _conn->DataCommand(_str, resume_pos);
	return std::make_shared<FTPFileIO>(_conn, data_transport);
}

