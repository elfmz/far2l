#pragma once
#include <memory>
#include <string>
#include <map>
#include <list>
#include <StringConfig.h>
#include "../Protocol.h"
#include "../DirectoryEnumCache.h"
#include "FTPConnection.h"

class ProtocolFTP : public IProtocol, public std::enable_shared_from_this<ProtocolFTP>
{
	std::shared_ptr<FTPConnection> _conn;
	DirectoryEnumCache _dir_enum_cache;

	std::vector<std::string> _cwd;

	bool _feat_mlst = false, _feat_mlsd = false;
	bool _feat_chmod = true;

	std::string Navigate(const std::string &path_name);
	std::string PathAsRelative(const std::string &path);

	void MLst(const std::string &path, FileInformation &file_info, uid_t *uid = nullptr, gid_t *gid = nullptr);
	std::shared_ptr<IDirectoryEnumer> NavigatedDirectoryEnum();

	void SimpleCommand(const char *fmt, ...);

public:
	ProtocolFTP(const std::string &protocol, const std::string &host, unsigned int port,
		const std::string &username, const std::string &password, const std::string &protocol_options) throw (std::runtime_error);
	virtual ~ProtocolFTP();

	const std::vector<std::string> &EnumHosts();

	std::string RootedPath(const std::string &path);

	virtual mode_t GetMode(const std::string &path, bool follow_symlink = true) throw (std::runtime_error);
	virtual unsigned long long GetSize(const std::string &path, bool follow_symlink = true) throw (std::runtime_error);
	virtual void GetInformation(FileInformation &file_info, const std::string &path, bool follow_symlink = true) throw (std::runtime_error);

	virtual void FileDelete(const std::string &path) throw (std::runtime_error);
	virtual void DirectoryDelete(const std::string &path) throw (std::runtime_error);

	virtual void DirectoryCreate(const std::string &path, mode_t mode) throw (std::runtime_error);
	virtual void Rename(const std::string &path_old, const std::string &path_new) throw (std::runtime_error);

	virtual void SetTimes(const std::string &path, const timespec &access_time, const timespec &modification_time) throw (std::runtime_error);
	virtual void SetMode(const std::string &path, mode_t mode) throw (std::runtime_error);

	virtual void SymlinkCreate(const std::string &link_path, const std::string &link_target) throw (std::runtime_error);
	virtual void SymlinkQuery(const std::string &link_path, std::string &link_target) throw (std::runtime_error);

	virtual std::shared_ptr<IDirectoryEnumer> DirectoryEnum(const std::string &path) throw (std::runtime_error);
	virtual std::shared_ptr<IFileReader> FileGet(const std::string &path, unsigned long long resume_pos = 0) throw (std::runtime_error);
	virtual std::shared_ptr<IFileWriter> FilePut(const std::string &path, mode_t mode, unsigned long long size_hint, unsigned long long resume_pos = 0) throw (std::runtime_error);
};
