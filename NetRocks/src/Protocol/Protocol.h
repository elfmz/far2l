#pragma once
#include <memory>
#ifdef __FreeBSD__
# include <sys/types.h>
#endif
#include "Erroring.h"
#include "FileInformation.h"

struct ExecFIFO_CtlMsg
{
	enum Cmd
	{
		CMD_PTY_SIZE = 0,
		CMD_SIGNAL
	} cmd;

	union {
		struct {
			unsigned int cols;
			unsigned int rows;
		} pty_size;

		unsigned int signum;
	} u;
};

// all methods of this interfaces are NOT thread-safe unless explicitely marked as MT-safe

struct IFileReader
{
	virtual ~IFileReader() {};

	virtual size_t Read(void *buf, size_t len) throw (std::runtime_error) = 0;
};

struct IFileWriter
{
	virtual ~IFileWriter() {};

	virtual void Write(const void *buf, size_t len) throw (std::runtime_error) = 0;

	/// optional call used to ensure that all data written (flush, gracefully close connection etc) but may be not called
	virtual void WriteComplete() throw (std::runtime_error) = 0;
};

struct IDirectoryEnumer
{
	virtual ~IDirectoryEnumer() {};

	virtual bool Enum(std::string &name, std::string &owner, std::string &group, FileInformation &file_info) throw (std::runtime_error) = 0;
};

struct IProtocol
{
	virtual ~IProtocol() {};

	virtual mode_t GetMode(const std::string &path, bool follow_symlink = true) throw (std::runtime_error) = 0;
	virtual unsigned long long GetSize(const std::string &path, bool follow_symlink = true) throw (std::runtime_error) = 0;
	virtual void GetInformation(FileInformation &file_info, const std::string &path, bool follow_symlink = true) throw (std::runtime_error) = 0;

	virtual void FileDelete(const std::string &path) throw (std::runtime_error) = 0;
	virtual void DirectoryDelete(const std::string &path) throw (std::runtime_error) = 0;

	virtual void DirectoryCreate(const std::string &path, mode_t mode) throw (std::runtime_error) = 0;
	virtual void Rename(const std::string &path_old, const std::string &path_new) throw (std::runtime_error) = 0;

	virtual void SetTimes(const std::string &path, const timespec &access_timem, const timespec &modification_time) throw (std::runtime_error) = 0;
	virtual void SetMode(const std::string &path, mode_t mode) throw (std::runtime_error) = 0;

	virtual void SymlinkCreate(const std::string &link_path, const std::string &link_target) throw (std::runtime_error) = 0;
	virtual void SymlinkQuery(const std::string &link_path, std::string &link_target) throw (std::runtime_error) = 0;

	virtual std::shared_ptr<IDirectoryEnumer> DirectoryEnum(const std::string &path) throw (std::runtime_error) = 0;
	virtual std::shared_ptr<IFileReader> FileGet(const std::string &path, unsigned long long resume_pos = 0) throw (std::runtime_error) = 0;
	virtual std::shared_ptr<IFileWriter> FilePut(const std::string &path, mode_t mode, unsigned long long size_hint, unsigned long long resume_pos = 0) throw (std::runtime_error) = 0;

	virtual void ExecuteCommand(const std::string &working_dir, const std::string &command_line, const std::string &fifo) throw (std::runtime_error)
		{ throw ProtocolUnsupportedError(""); }
};


#define FILENAME_ENUMERABLE(PSZ) ((PSZ)[0] != 0 && ((PSZ)[0] != '.' || ((PSZ)[1] != 0 && ((PSZ)[1] != '.' || (PSZ)[2] != 0)) ))

struct ProtocolInfo
{
	const char *name;
	const char *broker;
	int default_port; // -1 if port cannot be represented/changed
	bool require_server; // false if protocol can be instantiated with empty server
	bool support_creds; // false if protocol doesnt support username:password authentification
	void (*Configure)(std::string &options);
};

const ProtocolInfo *ProtocolInfoHead();
const ProtocolInfo *ProtocolInfoLookup(const char *name);

