#pragma once
#include <memory>
#include <string>
#include <StringConfig.h>
#include "../Protocol.h"
#include "../SHELL/WayToShell.h"
#include "FishPlusSession.h"

/*
	FISH+ backend for NetRocks.

	FISH+ is an evolution of the classic fish protocol: instead of simulating a
	file system with one shell command per question, it uploads a helper that
	stays resident and answers framed requests. The wire format, the helper and
	the whole server side come from f4 (https://github.com/unxed/f4), where the
	protocol was designed; see Helpers/UPSTREAM.md.

	What this class implements is the part of FISH+ that today's IProtocol can
	express. The protocol itself can do considerably more - remote grep, remote
	line indexing, delta based saving, background jobs, tree scans, duplicate
	search - and none of that has a place in IProtocol to live in. INTEGRATION.md
	lists what each of those would need on the far2l side.
*/

class ProtocolFISHPLUS : public IProtocol
{
	std::shared_ptr<WayToShell> _way;
	std::shared_ptr<FishPlus::Session> _sess;

	int _fd_ipc_recv{-1};
	StringConfig _protocol_options;
	std::string _host;
	unsigned int _port{0};
	std::string _username;
	std::string _password;
	std::string _way_name;
	std::string _home;		// absolute home dir, resolved once at connect

	void SubstituteCreds(std::string &str);
	void OpenWay();
	void PerformLogin();
	void Initialize();

	// Fills fi from an info/linfo answer about path.
	void QueryInformation(FileInformation &file_info, const std::string &path, bool follow_symlink);

public:
	ProtocolFISHPLUS(const std::string &host, unsigned int port, const std::string &username,
		const std::string &password, const std::string &protocol_options, int fd_ipc_recv);
	virtual ~ProtocolFISHPLUS();

	// Cheapest possible round trip - far cheaper than the default, which stats
	// a path just to find out the link is alive.
	virtual void KeepAlive(const std::string &path_to_check);

	virtual mode_t GetMode(const std::string &path, bool follow_symlink = true);
	virtual unsigned long long GetSize(const std::string &path, bool follow_symlink = true);
	virtual void GetInformation(FileInformation &file_info, const std::string &path, bool follow_symlink = true);

	virtual void FileDelete(const std::string &path);
	virtual void DirectoryDelete(const std::string &path);

	virtual void DirectoryCreate(const std::string &path, mode_t mode);
	virtual void Rename(const std::string &path_old, const std::string &path_new);
	virtual void FileCopy(const std::string &path_src, const std::string &path_dst);

	virtual void SetTimes(const std::string &path, const timespec &access_time, const timespec &modification_time);
	virtual void SetMode(const std::string &path, mode_t mode);

	virtual void SymlinkCreate(const std::string &link_path, const std::string &link_target);
	virtual void SymlinkQuery(const std::string &link_path, std::string &link_target);

	virtual std::string RealPath(const std::string &path);

	virtual std::shared_ptr<IDirectoryEnumer> DirectoryEnum(const std::string &path);
	virtual std::shared_ptr<IFileReader> FileGet(const std::string &path, unsigned long long resume_pos = 0);
	virtual std::shared_ptr<IFileWriter> FilePut(const std::string &path, mode_t mode, unsigned long long size_hint, unsigned long long resume_pos = 0);
};
