#pragma once
#include <memory>
#include "../Protocol.h"

struct SFTPConnection;

class ProtocolSFTP : public IProtocol
{
	std::shared_ptr<SFTPConnection> _conn;

public:
	ProtocolSFTP(const std::string &host, unsigned int port, const std::string &username,
		const std::string &password, const std::string &options);
	virtual ~ProtocolSFTP();

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

	virtual void ExecuteCommand(const std::string &working_dir, const std::string &command_line, const std::string &fifo);

	virtual void KeepAlive(const std::string &path_to_check);
};
