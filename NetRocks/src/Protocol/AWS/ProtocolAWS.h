#pragma once
#include <memory>
#include <string>
#include "../Protocol.h"
#include "S3Repository.h"

class ProtocolAWS : public IProtocol
{
	std::shared_ptr<S3Repository> _repository;

	static std::string RootedPath(const std::string &path);

public:
	ProtocolAWS(const std::string &host, unsigned int port, const std::string &username,
	            const std::string &password, const std::string &protocol_options);
	~ProtocolAWS() override = default;

	mode_t GetMode(const std::string &path, bool follow_symlink = true) override;
	unsigned long long GetSize(const std::string &path, bool follow_symlink = true) override;
	void GetInformation(FileInformation &file_info, const std::string &path, bool follow_symlink = true) override;

	void FileDelete(const std::string &path) override;
	void DirectoryDelete(const std::string &path) override;

	void DirectoryCreate(const std::string &path, mode_t mode) override;
	void Rename(const std::string &path_old, const std::string &path_new) override;

	void SetTimes(const std::string &path, const timespec &access_time, const timespec &modification_time) override;
	void SetMode(const std::string &path, mode_t mode) override;

	void SymlinkCreate(const std::string &link_path, const std::string &link_target) override;
	void SymlinkQuery(const std::string &link_path, std::string &link_target) override;

	std::shared_ptr<IDirectoryEnumer> DirectoryEnum(const std::string &path) override;
	std::shared_ptr<IFileReader> FileGet(const std::string &path, unsigned long long resume_pos = 0) override;
	std::shared_ptr<IFileWriter> FilePut(const std::string &path, mode_t mode, unsigned long long size_hint, unsigned long long resume_pos = 0) override;
};
