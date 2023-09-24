#pragma once
#include <unistd.h>
#include <memory>
#include <string>
#include <map>
#include <set>
#include <ScopeHelpers.h>
#include <Threaded.h>
#include "../Protocol.h"

#include "ClientApp.h"

struct RemoteFeats
{
	uint32_t require_write_block; // typically few Kbs
	uint32_t limit_max_blocks;
	bool using_stat : 1;
	bool using_find : 1;
	bool using_ls : 1;
	bool support_read : 1;
	bool support_read_resume : 1;
	bool support_write : 1;
	bool support_write_resume : 1;
	bool require_write_base64 : 1;
};

class ProtocolSHELL : public IProtocol
{
	class ExecCmd : protected Threaded
	{
		std::string _working_dir;
		std::string _command_line;
		std::string _fifo;
		FDScope _fdinout, _fderr;
		int _kickass[2] {-1, -1};
		struct MarkerTrack
		{
			std::string marker;
			bool done{false};
			int status{-1};
			void Inspect(const char *append, ssize_t &len);

		private:
			std::string _tail;
			bool _matched{false};
		} _marker_track;

		void Abort();
		void OnReadFDCtl(int fd);
		void IOLoop();
		virtual void *ThreadProc();

	public:
		ExecCmd(std::shared_ptr<ClientApp> &app,
			const std::string &working_dir,
			const std::string &command_line,
			const std::string &fifo);
		~ExecCmd();

		bool KeepAlive();
	};

	std::shared_ptr<ClientApp> _app;
	pid_t _pid{0};
	RemoteFeats _feats{};
	std::unique_ptr<ExecCmd> _exec_cmd;

public:

	ProtocolSHELL(const std::string &host, unsigned int port,
		const std::string &username, const std::string &password, const std::string &protocol_options);
	virtual ~ProtocolSHELL();

	virtual void KeepAlive(const std::string &path_to_check);

	virtual void GetModes(bool follow_symlink, size_t count, const std::string *pathes, mode_t *modes) noexcept;

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
};
