#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <utils.h>
#include <map>
#include <set>
#include <vector>
#include <string>

#include <libsmbclient.h>

#include "ProtocolSMB.h"
#include "NMBEnum.h"

std::shared_ptr<IProtocol> CreateProtocol(const std::string &protocol, const std::string &host, unsigned int port,
	const std::string &username, const std::string &password, const std::string &options, int fd_ipc_recv)
{
	return std::make_shared<ProtocolSMB>(host, port, username, password, options);
}
/*
struct SMBConnection
{
	SMBCCTX	*ctx = nullptr;

	~SMBConnection()
	{
		if (ctx != nullptr) {
			smbc_getFunctionPurgeCachedServers(ctx)(ctx);
			smbc_free_context(ctx, 1);
		}
	}
private:
	SMBConnection(const SMBConnection &) = delete;
};
*/

////////////////////////////
static std::string smb_username, smb_password, smb_workgroup;

static void ProtocolSMB_AuthFn(const char *server, const char *share, char *wrkgrp,
	int wrkgrplen, char *user, int userlen, char *passwd, int passwdlen)
{
	(void) server;
	(void) share;

	strncpy(wrkgrp, smb_workgroup.c_str(), wrkgrplen - 1);
	wrkgrp[wrkgrplen - 1] = 0;

	strncpy(user, smb_username.c_str(), userlen - 1);
	user[userlen - 1] = 0;

	strncpy(passwd, smb_password.c_str(), passwdlen - 1);
	passwd[passwdlen - 1] = 0;
}


ProtocolSMB::ProtocolSMB(const std::string &host, unsigned int port,
	const std::string &username, const std::string &password, const std::string &options)
	: _host(host), _protocol_options(options)
{
//	_conn->ctx = create_smbctx();
//	if (!_conn->ctx)
//		throw ProtocolError("SMB context create failed");

	smb_workgroup = _protocol_options.GetString("Workgroup");
	smb_username = username;
	smb_password = password;

//	smbc_setFunctionAuthData(_conn->ctx, &ProtocolSMB_AuthFn);
//	smbc_setOptionUseCCache(_conn->ctx, false);
//	smbc_setOptionNoAutoAnonymousLogin(_conn->ctx, !password.empty());

	if (smbc_init(&ProtocolSMB_AuthFn, 0) < 0){
//		smbc_free_context(_conn->ctx, 1);
//		_conn->ctx = nullptr;
		throw ProtocolError("SMB context init failed", errno);
	}

	//TODO: smb_workgroup = ;
}

ProtocolSMB::~ProtocolSMB()
{
}

std::string ProtocolSMB::RootedPath(const std::string &path)
{
	std::string out = "smb://";
	for (size_t i = 0; i < _host.size(); ++i) {
		if (_host[i] != '/') {
			out+= _host.substr(i);
			break;
		}
	}
	while (out.size() > 6 && out[out.size() - 1] == '/') {
		out.resize(out.size() - 1);
	}
	if (out.size() > 6 && !path.empty() && path[0] != '/') {
		out+= '/';
	}
	out+= path;

	while (out.size() > 6 && out[out.size() - 1] == '.' && out[out.size() - 2] == '/') {
		out.resize((out.size() > 7) ? out.size() - 2 : out.size() - 1);
	}

	return out;
}

static bool IsRootedPathServerOnly(const std::string &path)
{
	size_t slashes_count = 0;
	for (size_t i = 0; i < path.size(); ++i) {
		if (path[i] == '/')
			slashes_count++;
	}
	return (slashes_count <= 2);
}

mode_t ProtocolSMB::GetMode(const std::string &path, bool follow_symlink)
{
	const std::string &rooted_path = RootedPath(path);
	if (IsRootedPathServerOnly(rooted_path)) {
		return S_IFDIR | DEFAULT_ACCESS_MODE_DIRECTORY;
	}
	struct stat s = {};
	int rc = smbc_stat(rooted_path.c_str(), &s);
	if (rc != 0)
		throw ProtocolError("Get mode error", errno);

	return s.st_mode;
}

unsigned long long ProtocolSMB::GetSize(const std::string &path, bool follow_symlink)
{
	const std::string &rooted_path = RootedPath(path);
	if (IsRootedPathServerOnly(rooted_path)) {
		return 0;
	}
	struct stat s = {};
	int rc = smbc_stat(rooted_path.c_str(), &s);
	if (rc != 0)
		throw ProtocolError("Get size error", errno);

	return s.st_size;
}

static int ProtocolSMB_GetInformationInternal(FileInformation &file_info, const std::string &path)
{
	struct stat s = {};
	int rc = smbc_stat(path.c_str(), &s);
	if (rc < 0)
		return rc;

	file_info.access_time = s.st_atim;
	file_info.modification_time = s.st_mtim;
	file_info.status_change_time = s.st_ctim;
	file_info.mode = s.st_mode;
	file_info.size = s.st_size;
	return 0;
}

void ProtocolSMB::GetInformation(FileInformation &file_info, const std::string &path, bool follow_symlink)
{
	const std::string &rooted_path = RootedPath(path);
	if (IsRootedPathServerOnly(rooted_path)) {
		file_info = FileInformation();
		file_info.mode = S_IFDIR | DEFAULT_ACCESS_MODE_DIRECTORY;
		return;
	}
	int rc = ProtocolSMB_GetInformationInternal(file_info, rooted_path);
	if (rc != 0)
		throw ProtocolError("Get info error", errno);
}

void ProtocolSMB::FileDelete(const std::string &path)
{
	int rc = smbc_unlink(RootedPath(path).c_str());
	if (rc != 0)
		throw ProtocolError("Delete file error", errno);
}

void ProtocolSMB::DirectoryDelete(const std::string &path)
{
	int rc = smbc_rmdir(RootedPath(path).c_str());
	if (rc != 0)
		throw ProtocolError("Delete directory error", errno);
}

void ProtocolSMB::DirectoryCreate(const std::string &path, mode_t mode)
{
	int rc = smbc_mkdir(RootedPath(path).c_str(), mode);
	if (rc != 0)
		throw ProtocolError("Create directory error", errno);
}

void ProtocolSMB::Rename(const std::string &path_old, const std::string &path_new)
{
	int rc = smbc_rename(RootedPath(path_old).c_str(), RootedPath(path_new).c_str());
	if (rc != 0)
		throw ProtocolError("Rename error", errno);
}


void ProtocolSMB::SetTimes(const std::string &path, const timespec &access_time, const timespec &modification_time)
{
	struct timeval times[2] = {};
	times[0].tv_sec = access_time.tv_sec;
	times[0].tv_usec = suseconds_t(access_time.tv_nsec / 1000);
	times[1].tv_sec = modification_time.tv_sec;
	times[1].tv_usec = suseconds_t(modification_time.tv_nsec / 1000);

	int rc = smbc_utimes(RootedPath(path).c_str(), times);
	if (rc != 0)
		throw ProtocolError("Set times error", errno);
}

void ProtocolSMB::SetMode(const std::string &path, mode_t mode)
{
	int rc = smbc_chmod(RootedPath(path).c_str(), mode);
	if (rc != 0)
		throw ProtocolError("Set mode error", errno);
}

void ProtocolSMB::SymlinkCreate(const std::string &link_path, const std::string &link_target)
{
	throw ProtocolUnsupportedError("Symlink creation unsupported");
}

void ProtocolSMB::SymlinkQuery(const std::string &link_path, std::string &link_target)
{
	throw ProtocolUnsupportedError("Symlink querying unsupported");
}

class SMBDirectoryEnumer : public IDirectoryEnumer
{
	std::shared_ptr<ProtocolSMB> _protocol;
	std::string _rooted_path;
	int _dir = -1;
	char _buf[0x4000]{}, *_entry = nullptr;
	int _remain = 0;

public:
	SMBDirectoryEnumer(std::shared_ptr<ProtocolSMB> protocol, const std::string &rooted_path)
		: _protocol(protocol),
		_rooted_path(rooted_path)
	{
		fprintf(stderr, "SMBDirectoryEnumer: '%s'\n", _rooted_path.c_str());

		_dir = smbc_opendir(_rooted_path.c_str());
		if (_dir < 0) {
			throw ProtocolError("Directory open error", _rooted_path.c_str(), errno);
		}

		if (_rooted_path.empty() || _rooted_path[_rooted_path.size() - 1] != '/') {
			_rooted_path+= '/';
		}
	}

	virtual ~SMBDirectoryEnumer()
	{
		if (_dir != -1) {
			smbc_closedir(_dir);
		}
	}

	virtual bool Enum(std::string &name, std::string &owner, std::string &group, FileInformation &file_info)
	{
		std::string subpath;
		owner.clear();
		group.clear();
		file_info = FileInformation();
		for (;;) {
			if (_remain > 0 && _entry != nullptr) {
				struct smbc_dirent *de = (struct smbc_dirent *)_entry;
				_remain-= de->dirlen;
				if (_remain > 0) {
					_entry+= de->dirlen;
				}
				if (FILENAME_ENUMERABLE(de->name)) {
					name = de->name;
					subpath = _rooted_path;
					subpath+= name;

					file_info.size = 0;

					switch (de->smbc_type) {
						case SMBC_WORKGROUP: case SMBC_SERVER:
						case SMBC_FILE_SHARE: case SMBC_PRINTER_SHARE:
						case SMBC_DIR: case SMBC_LINK:
							ProtocolSMB_GetInformationInternal(file_info, subpath);
							file_info.mode = S_IFDIR | DEFAULT_ACCESS_MODE_DIRECTORY;
							return true;

						case SMBC_FILE:
							ProtocolSMB_GetInformationInternal(file_info, subpath);
							file_info.mode = S_IFREG | DEFAULT_ACCESS_MODE_FILE;
							return true;
					}
				}
			}


			if (_remain <= 0) {
				_entry = _buf;
				_remain = (_dir == -1) ? 0 : smbc_getdents(_dir, (struct smbc_dirent *)_buf, sizeof(_buf));
				if (_remain == 0)
					return false;

				if (_remain < 0)
					throw ProtocolError("Directory enum error", errno);
			}
		}
	}
};


class SMBNetworkEnumer : public IDirectoryEnumer
{
	std::map<std::string, FileInformation> _net;

public:
	SMBNetworkEnumer(std::shared_ptr<ProtocolSMB> protocol)
	{
		if (!protocol->_cached_net.empty()) {
			_net = protocol->_cached_net;
			return;
		}

		const auto enum_by = (unsigned int)protocol->_protocol_options.GetInt("EnumBy", 0xff);

		std::unique_ptr<NMBEnum> nmb_enum;
		if (enum_by & 2) {
			nmb_enum.reset(new NMBEnum(smb_workgroup));
		}

		if (enum_by & 1) {
			try {
				SMBDirectoryEnumer smb_enum(protocol, "smb://");
				std::string name, owner, group;
				FileInformation file_info;
				while(smb_enum.Enum(name, owner, group, file_info)) {
					_net.emplace(name, file_info);
				}
			} catch (std::exception &ex) {
				fprintf(stderr, "SMBNetworkEnumer: %s\n", ex.what());
				if (!nmb_enum)
					throw;
			}
		}

		if (nmb_enum) {
			FileInformation file_info{};
			file_info.mode = S_IFDIR | DEFAULT_ACCESS_MODE_DIRECTORY;
			for (const auto &i : nmb_enum->WaitResults()) {
				_net.emplace(i.first, file_info);
			}
		}

		protocol->_cached_net = _net;
	}


	virtual bool Enum(std::string &name, std::string &owner, std::string &group, FileInformation &file_info)
	{
		if (_net.empty())
			return false;

		auto i = _net.begin();
		owner.clear();
		group.clear();
		name = i->first;
		file_info = i->second;
		_net.erase(i);
		return true;
	}
};

std::shared_ptr<IDirectoryEnumer> ProtocolSMB::DirectoryEnum(const std::string &path)
{
	const std::string &rooted_path = RootedPath(path);

	if (rooted_path == "smb://") {
		return std::shared_ptr<IDirectoryEnumer>(new SMBNetworkEnumer(shared_from_this()));
	}

	return std::shared_ptr<IDirectoryEnumer>(new SMBDirectoryEnumer(shared_from_this(), rooted_path));
}


class SMBFileIO : public IFileReader, public IFileWriter
{
	std::shared_ptr<ProtocolSMB> _protocol;
	int _file = -1;

public:
	SMBFileIO(std::shared_ptr<ProtocolSMB> protocol, const std::string &path, int flags, mode_t mode, unsigned long long resume_pos)
		: _protocol(protocol)
	{
		_file = smbc_open(protocol->RootedPath(path).c_str(), flags, mode);
		if (_file == -1)
			throw ProtocolError("Failed to open file", errno);

		if (resume_pos) {
			off_t rc = smbc_lseek(_file, resume_pos, SEEK_SET);
			if (rc == (off_t)-1) {
				smbc_close(_file);
				_file = -1;
				throw ProtocolError("Failed to seek file", errno);
			}
		}
	}

	virtual ~SMBFileIO()
	{
		if (_file != -1) {
			smbc_close(_file);
		}
	}

	virtual size_t Read(void *buf, size_t len)
	{
		const ssize_t rc = smbc_read(_file, buf, len);
		if (rc < 0)
			throw ProtocolError("Read file error", errno);
		// uncomment to simulate connection stuck if ( (rand()%100) == 0) sleep(60);

		return (size_t)rc;
	}

	virtual void Write(const void *buf, size_t len)
	{
		if (len > 0) for (;;) {
			const ssize_t rc = smbc_write(_file, buf, len);
			if (rc <= 0)
				throw ProtocolError("Write file error", errno);
			if ((size_t)rc >= len)
				break;

			len-= (size_t)rc;
			buf = (const char *)buf + rc;
		}
	}

	virtual void WriteComplete()
	{
		// what?
	}
};


std::shared_ptr<IFileReader> ProtocolSMB::FileGet(const std::string &path, unsigned long long resume_pos)
{
	return std::make_shared<SMBFileIO>(shared_from_this(), path, O_RDONLY, 0, resume_pos);
}

std::shared_ptr<IFileWriter> ProtocolSMB::FilePut(const std::string &path, mode_t mode, unsigned long long size_hint, unsigned long long resume_pos)
{
	return std::make_shared<SMBFileIO>(shared_from_this(), path, O_WRONLY | O_CREAT | (resume_pos ? 0 : O_TRUNC), mode, resume_pos);
}
