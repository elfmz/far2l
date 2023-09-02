#pragma once
#include <memory>
#include <string>
#include <mutex>
#include <list>
#include <atomic>

#include "Host.h"
#include "IPC.h"
#include "FileInformation.h"
#include "InitDeinitCmd.h"

#include "../SitesConfig.h"

class HostRemote : protected IPCEndpoint, public std::enable_shared_from_this<HostRemote>, public IHost
{
	friend class HostRemoteDirectoryEnumer;
	friend class HostRemoteFileIO;

	std::mutex _mutex; // to protect internal fields
	std::atomic<bool> _aborted{false};
	std::unique_ptr<InitDeinitCmd> _init_deinit_cmd;
	SiteSpecification _site_specification;

	Identity _identity;
	unsigned int _login_mode{0};
	std::string _password;
	std::string _options;
	unsigned int _codepage{0};
	int _timeadjust{0};
	std::string _codepage_str;
	std::wstring _codepage_wstr;
	timespec _ts_2_remote{};

	bool _busy = false;
	bool _cloning = false;
	std::atomic<pid_t> _peer{0};

	void RecvReply(IPCCommand cmd);

	bool OnServerIdentityChanged(const std::string &new_identity);

	const std::string &CodepageLocal2Remote(const std::string &str);
	void CodepageRemote2Local(std::string &str);

	const timespec &TimespecLocal2Remote(const timespec &ts);
	void TimespecRemote2Local(timespec &ts);

protected:
	void BusySet();
	void BusyReset();
	void AssertNotBusy();
	void CheckReady();
	void OnBroken();

public:
	inline HostRemote() {}

	HostRemote(const SiteSpecification &site_specification);
	HostRemote(const std::string &protocol, const std::string &host,
		unsigned int port, const std::string &username, const std::string &password);
	virtual ~HostRemote();

	virtual std::shared_ptr<IHost> Clone();

	virtual std::string SiteName();
	virtual void GetIdentity(Identity &identity);

	virtual void ReInitialize();
	virtual void Abort();

	virtual void GetModes(bool follow_symlink, size_t count, const std::string *pathes, mode_t *modes) noexcept;
	virtual mode_t GetMode(const std::string &path, bool follow_symlink = true);

	virtual unsigned long long GetSize(const std::string &path, bool follow_symlink = true);
	virtual void GetInformation(FileInformation &file_info, const std::string &path, bool follow_symlink = true);

	virtual void FileDelete(const std::string &path);
	virtual void DirectoryDelete(const std::string &path);

	virtual void DirectoryCreate(const std::string &path, mode_t mode);
	virtual void Rename(const std::string &path_old, const std::string &path_new);

	virtual void SetTimes(const std::string &path, const timespec &access_timem, const timespec &modification_time);
	virtual void SetMode(const std::string &path, mode_t mode);

	virtual void SymlinkCreate(const std::string &link_path, const std::string &link_target);
	virtual void SymlinkQuery(const std::string &link_path, std::string &link_target);


	virtual std::shared_ptr<IDirectoryEnumer> DirectoryEnum(const std::string &path);
	virtual std::shared_ptr<IFileReader> FileGet(const std::string &path, unsigned long long resume_pos = 0);
	virtual std::shared_ptr<IFileWriter> FilePut(const std::string &path, mode_t mode, unsigned long long size_hint, unsigned long long resume_pos = 0);

	virtual void ExecuteCommand(const std::string &working_dir, const std::string &command_line, const std::string &fifo);

	virtual bool Alive();
};
