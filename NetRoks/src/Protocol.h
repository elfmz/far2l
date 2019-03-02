#pragma once
#include <memory>
#include "Erroring.h"


struct FileInformation
{
	timespec access_time;
	timespec modification_time;
	timespec status_change_time;
	unsigned long long size;
	mode_t mode;
};

struct IFileReader
{
	virtual ~IFileReader() {};

	virtual size_t Read(void *buf, size_t len) throw (ProtocolError) = 0;
};

struct IFileWriter
{
	virtual ~IFileWriter() {};

	virtual void Write(const void *buf, size_t len) throw (ProtocolError) = 0;
};

struct IDirectoryEnumer
{
	virtual ~IDirectoryEnumer() {};

	virtual bool Enum(std::string &name, std::string &owner, std::string &group, FileInformation &file_info) throw (ProtocolError) = 0;
};


struct IProtocol
{
	virtual ~IProtocol() {};

	virtual bool IsBroken() const = 0;

	virtual mode_t GetMode(const std::string &path, bool follow_symlink = true) throw (ProtocolError) = 0;
	virtual unsigned long long GetSize(const std::string &path, bool follow_symlink = true) throw (ProtocolError) = 0;

	virtual void FileDelete(const std::string &path) throw (ProtocolError) = 0;
	virtual void DirectoryDelete(const std::string &path) throw (ProtocolError) = 0;

	virtual void DirectoryCreate(const std::string &path, mode_t mode) throw (ProtocolError) = 0;
	virtual void Rename(const std::string &path_old, const std::string &path_new) throw (ProtocolError) = 0;

	virtual std::shared_ptr<IDirectoryEnumer> DirectoryEnum(const std::string &path) throw (ProtocolError) = 0;
	virtual std::shared_ptr<IFileReader> FileGet(const std::string &path, unsigned long long resume_pos = 0) throw (ProtocolError) = 0;
	virtual std::shared_ptr<IFileWriter> FilePut(const std::string &path, mode_t mode, unsigned long long resume_pos = 0) throw (ProtocolError) = 0;
};

