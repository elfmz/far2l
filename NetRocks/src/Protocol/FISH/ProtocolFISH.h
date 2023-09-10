#pragma once
#include <memory>
#include <string>
#include <map>
#include <set>
#include "../Protocol.h"

#include "FISHClient.h"

struct FISHConnection
{
	std::map<std::string, std::set<std::string> > srv2exports;
	//struct fish_context *ctx = nullptr;
	std::string mounted_path;

    FISHClient* sc;

	FISHConnection() = default;

	~FISHConnection()
	{
		//if (ctx != nullptr) {
			//fish_destroy_context(ctx);
		//}
	}

private:
	FISHConnection(const FISHConnection &) = delete;
};

class ProtocolFISH : public IProtocol
{
	std::shared_ptr<FISHConnection> _fish;

public:

	ProtocolFISH(const std::string &host, unsigned int port,
		const std::string &username, const std::string &password, const std::string &protocol_options);
	virtual ~ProtocolFISH();

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
