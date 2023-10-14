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

	struct { // command codes or nullptr if assumed that command is not supported
		const char *pwd = "PWD";
		const char *cwd = "CWD";
		const char *cdup = "CDUP";
		const char *mkd = "MKD";
		const char *rmd = "RMD";
		const char *size = "SIZE";
		const char *mfmt = "MFMT";
		const char *chmod = "CHMOD";
		const char *dele = "DELE";
		const char *rnfr = "RNFR";
		const char *rnto = "RNTO";
		std::string list_;// = "LIST";
		const char *mlsd = nullptr; // set to non-NULL during connection init
		const char *mlst = nullptr; // set to non-NULL during connection init
		const char *retr = "RETR";
		const char *stor = "STOR";
	} _cmd;

	struct {
		std::vector<std::string> parts;
		std::string path;
		std::string home;
	} _cwd;

	std::string _str;

	bool RecvPwdResponse();
//	bool RecvPwdAndRememberAsCwd();
	std::string SplitPathAndNavigate(const std::string &path_name, bool allow_empty_name_part = false);
	std::string PathAsRelative(const std::string &path);

	void MLst(const std::string &path, FileInformation &file_info, uid_t *uid = nullptr, gid_t *gid = nullptr, std::string *lnkto = nullptr);

	std::shared_ptr<IDirectoryEnumer> NavigatedDirectoryEnum();

	void SimpleDispositionCommand(const char *cmd, const std::string &path);

public:
	ProtocolFTP(const std::string &protocol, const std::string &host, unsigned int port,
		const std::string &username, const std::string &password, const std::string &protocol_options);
	virtual ~ProtocolFTP();

	const std::vector<std::string> &EnumHosts();

	std::string RootedPath(const std::string &path);

	virtual mode_t GetMode(const std::string &path, bool follow_symlink = true);
	virtual unsigned long long GetSize(const std::string &path, bool follow_symlink = true);
	virtual void GetInformation(FileInformation &file_info, const std::string &path, bool follow_symlink = true);

	virtual void FileDelete(const std::string &path);
	virtual void DirectoryDelete(const std::string &path);

	virtual void DirectoryCreate(const std::string &path, mode_t mode);
	virtual void Rename(const std::string &path_old, const std::string &path_new);

	virtual void SetTimes(const std::string &path, const timespec &access_time, const timespec &modification_time);
	virtual void SetMode(const std::string &path, mode_t mode);

	virtual void SymlinkCreate(const std::string &link_path, const std::string &link_target);
	virtual void SymlinkQuery(const std::string &link_path, std::string &link_target);

	virtual std::shared_ptr<IDirectoryEnumer> DirectoryEnum(const std::string &path);
	virtual std::shared_ptr<IFileReader> FileGet(const std::string &path, unsigned long long resume_pos = 0);
	virtual std::shared_ptr<IFileWriter> FilePut(const std::string &path, mode_t mode, unsigned long long size_hint, unsigned long long resume_pos = 0);
};
