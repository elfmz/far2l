#pragma once
#include <memory>
#include <string>
#include <atomic>
#include <list>

#include "Host.h"
#include "IPC.h"
#include "FileInformation.h"

class HostRemote : protected IPCRecver, protected IPCSender, public std::enable_shared_from_this<HostRemote>, public IHost
{
	friend class HostRemoteDirectoryEnumer;
	friend class HostRemoteFileIO;

	std::string _protocol;
	std::string _host;
	unsigned int _port;
	unsigned int _login_mode;
	std::string _username;
	std::string _password;
	std::string _options;

	std::string _site, _site_info;
	bool _broken = true;
	bool _busy = false;

	void RecvReply(IPCCommand cmd);

	bool OnServerIdentityChanged(const std::string &new_identity);

protected:
	void BusySet();
	void BusyReset();
	void AssertNotBusy();
	void OnBroken();
	void CheckReady();

public:
	HostRemote(const std::string &site, int OpMode) throw (std::runtime_error);
	virtual ~HostRemote();

	virtual std::string SiteName() const;
	virtual bool IsBroken();
	virtual void ReInitialize() throw (std::runtime_error);
	virtual void Abort();

	virtual mode_t GetMode(const std::string &path, bool follow_symlink = true) throw (std::runtime_error);
	virtual unsigned long long GetSize(const std::string &path, bool follow_symlink = true) throw (std::runtime_error);
	virtual void GetInformation(FileInformation &file_info, const std::string &path, bool follow_symlink = true) throw (std::runtime_error);

	virtual void FileDelete(const std::string &path) throw (std::runtime_error);
	virtual void DirectoryDelete(const std::string &path) throw (std::runtime_error);

	virtual void DirectoryCreate(const std::string &path, mode_t mode) throw (std::runtime_error);
	virtual void Rename(const std::string &path_old, const std::string &path_new) throw (std::runtime_error);

	virtual std::shared_ptr<IDirectoryEnumer> DirectoryEnum(const std::string &path) throw (std::runtime_error);
	virtual std::shared_ptr<IFileReader> FileGet(const std::string &path, unsigned long long resume_pos = 0) throw (std::runtime_error);
	virtual std::shared_ptr<IFileWriter> FilePut(const std::string &path, mode_t mode, unsigned long long resume_pos = 0) throw (std::runtime_error);
};
