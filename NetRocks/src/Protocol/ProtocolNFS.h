#pragma once
#include <memory>
#include <nfsc/libnfs.h>
#include "Protocol.h"

struct NFSConnection
{
	struct nfs_context *ctx = nullptr;

	NFSConnection() = default;

	~NFSConnection()
	{
		if (ctx != nullptr) {
			nfs_destroy_context(ctx);
		}
	}

private:
	NFSConnection(const NFSConnection &) = delete;
};

class ProtocolNFS : public IProtocol
{
	std::shared_ptr<NFSConnection> _nfs;
	std::string _host, _mount;

	std::string RootedPath(const std::string &path);
public:

	ProtocolNFS(const std::string &host, unsigned int port,
		const std::string &username, const std::string &password, const std::string &protocol_options) throw (std::runtime_error);
	virtual ~ProtocolNFS();

	virtual bool IsBroken();

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
	virtual std::shared_ptr<IFileWriter> FilePut(const std::string &path, mode_t mode, unsigned long long resume_pos = 0) throw (std::runtime_error);
};
