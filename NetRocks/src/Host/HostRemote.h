#pragma once
#include <memory>
#include <string>
#include <mutex>
#include <list>

#include "Host.h"
#include "IPC.h"
#include "FileInformation.h"

class HostRemote : protected IPCRecver, protected IPCSender, public std::enable_shared_from_this<HostRemote>, public IHost
{
	friend class HostRemoteDirectoryEnumer;
	friend class HostRemoteFileIO;

	std::mutex _mutex; // to protect internal fields

	std::string _site;

	Identity _identity;
	unsigned int _login_mode;
	std::string _password;
	std::string _options;

	bool _busy = false;
	bool _cloning = false;
	pid_t _peer = 0;

	void RecvReply(IPCCommand cmd);

	bool OnServerIdentityChanged(const std::string &new_identity);

protected:
	void BusySet();
	void BusyReset();
	void AssertNotBusy();
	void CheckReady();
	void OnBroken();

public:
	inline HostRemote() {}

	HostRemote(const std::string &site);
	HostRemote(const std::string &protocol, const std::string &host,
		unsigned int port, const std::string &username, const std::string &password);
	virtual ~HostRemote();

	virtual std::shared_ptr<IHost> Clone();

	virtual std::string SiteName();
	virtual void GetIdentity(Identity &identity);

	virtual void ReInitialize() throw (std::runtime_error);
	virtual void Abort();

	virtual mode_t GetMode(const std::string &path, bool follow_symlink = true) throw (std::runtime_error);
	virtual unsigned long long GetSize(const std::string &path, bool follow_symlink = true) throw (std::runtime_error);
	virtual void GetInformation(FileInformation &file_info, const std::string &path, bool follow_symlink = true) throw (std::runtime_error);

	virtual void FileDelete(const std::string &path) throw (std::runtime_error);
	virtual void DirectoryDelete(const std::string &path) throw (std::runtime_error);

	virtual void DirectoryCreate(const std::string &path, mode_t mode) throw (std::runtime_error);
	virtual void Rename(const std::string &path_old, const std::string &path_new) throw (std::runtime_error);

	virtual void SetTimes(const std::string &path, const timespec &access_timem, const timespec &modification_time) throw (std::runtime_error);
	virtual void SetMode(const std::string &path, mode_t mode) throw (std::runtime_error);

	virtual void SymlinkCreate(const std::string &link_path, const std::string &link_target) throw (std::runtime_error);
	virtual void SymlinkQuery(const std::string &link_path, std::string &link_target) throw (std::runtime_error);


	virtual std::shared_ptr<IDirectoryEnumer> DirectoryEnum(const std::string &path) throw (std::runtime_error);
	virtual std::shared_ptr<IFileReader> FileGet(const std::string &path, unsigned long long resume_pos = 0) throw (std::runtime_error);
	virtual std::shared_ptr<IFileWriter> FilePut(const std::string &path, mode_t mode, unsigned long long size_hint, unsigned long long resume_pos = 0) throw (std::runtime_error);

	virtual void ExecuteCommand(const std::string &working_dir, const std::string &command_line, const std::string &fifo) throw (std::runtime_error);

	virtual bool Alive();
};
