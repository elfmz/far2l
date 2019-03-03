#pragma once
#include <memory>
#include "Protocol.h"

struct SFTPConnection;

class ProtocolSFTP : public IProtocol
{
	std::shared_ptr<SFTPConnection> _conn;

public:
	ProtocolSFTP(const std::string &host, unsigned int port, unsigned int options, const std::string &username,
		const std::string &password, const std::string &directory) throw (ProtocolError);
	virtual ~ProtocolSFTP();

	virtual bool IsBroken() const;

	virtual mode_t GetMode(const std::string &path, bool follow_symlink = true) throw (ProtocolError);
	virtual unsigned long long GetSize(const std::string &path, bool follow_symlink = true) throw (ProtocolError);

	virtual void FileDelete(const std::string &path) throw (ProtocolError);
	virtual void DirectoryDelete(const std::string &path) throw (ProtocolError);

	virtual void DirectoryCreate(const std::string &path, mode_t mode) throw (ProtocolError);
	virtual void Rename(const std::string &path_old, const std::string &path_new) throw (ProtocolError);

	virtual std::shared_ptr<IDirectoryEnumer> DirectoryEnum(const std::string &path) throw (ProtocolError);
	virtual std::shared_ptr<IFileReader> FileGet(const std::string &path, unsigned long long resume_pos = 0) throw (ProtocolError);
	virtual std::shared_ptr<IFileWriter> FilePut(const std::string &path, mode_t mode, unsigned long long resume_pos = 0) throw (ProtocolError);
};
