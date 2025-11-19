#include "ProtocolAWS.h"
#include <fstream>
#include <vector>


std::shared_ptr<IProtocol> CreateProtocol(const std::string &protocol, const std::string &host, unsigned int port,
	const std::string &username, const std::string &password, const std::string &options, int fd_ipc_recv)
{
	return std::make_shared<ProtocolAWS>(host, port, username, password, options);
}

// Initialize the AWS SDK
ProtocolAWS::ProtocolAWS(const std::string &host, unsigned int port,
                         const std::string &username, const std::string &password,
                         const std::string &protocol_options)
{
    _repository = std::make_shared<S3Repository>(host, port, username, password, protocol_options);
}

ProtocolAWS::~ProtocolAWS()
{
}

std::string ProtocolAWS::RootedPath(const std::string &path1)
{
	auto path = path1;
	while (!path.empty() && (path[0] == '/')) {
		path.erase(0, 1);
	}
	while (!path.empty() && (path[path.length() - 1] == '/')) {
		path.erase(path.length() - 1, 1);
	}
	if (path.empty()) {
		return path;
	}
    if (path[0] == '.') {
		path.erase(0, 1);
	}
	while (!path.empty() && (path[0] == '/')) {
		path.erase(0, 1);
	}
	return path;
}

mode_t ProtocolAWS::GetMode(const std::string &path, bool follow_symlink)
{
	auto f = _repository->GetFileInfo(path);
	if (f.isFile) {
		return S_IFREG | DEFAULT_ACCESS_MODE_FILE;
	}
	return S_IFDIR | DEFAULT_ACCESS_MODE_DIRECTORY;
}

unsigned long long ProtocolAWS::GetSize(const std::string &path, bool follow_symlink)
{
	return _repository->GetFileInfo(path).size;
}

void ProtocolAWS::FileDelete(const std::string &path)
{
	_repository->DeleteFile(RootedPath(path));
}

void ProtocolAWS::DirectoryCreate(const std::string &path, mode_t mode)
{
	auto rooted_path = RootedPath(path);
	if (rooted_path.find('/') < 0) {
		_repository->CreateBucket(rooted_path);
	} else {
		_repository->CreateDirectory(rooted_path);
	}
}

class AWSDirectoryEnumer : public IDirectoryEnumer
{
	std::shared_ptr<S3Repository> _repository;
	std::vector<AWSFile> ls;

public:
	AWSDirectoryEnumer(std::shared_ptr<S3Repository> repository, const std::string &rooted_path)
		: _repository(repository)
	{
		if (rooted_path.empty()) {
			ls = _repository->ListBuckets();
		} else {
			ls = _repository->ListFolder(rooted_path);
		}
	}

	virtual ~AWSDirectoryEnumer()
	{
	}

	virtual bool Enum(std::string &name, std::string &owner, std::string &group, FileInformation &file_info)
	{
		if (ls.empty()) return false;

		file_info = FileInformation();
		auto f = ls.front();
		name = f.name;
		file_info.access_time = f.modified;
		file_info.modification_time = f.modified;
		file_info.status_change_time = f.modified;
		file_info.size = f.size;
		if (f.isFile) {
			file_info.mode = S_IFREG | DEFAULT_ACCESS_MODE_FILE;
		} else {
			file_info.mode = S_IFDIR | DEFAULT_ACCESS_MODE_DIRECTORY;
		}

		ls.erase(ls.begin());
		return true;
	}
};


std::shared_ptr<IDirectoryEnumer> ProtocolAWS::DirectoryEnum(const std::string &path) {
	const std::string &rooted_path = RootedPath(path);
	return std::shared_ptr<IDirectoryEnumer>(new AWSDirectoryEnumer(_repository, rooted_path));
}

std::shared_ptr<IFileReader> ProtocolAWS::FileGet(const std::string &path, unsigned long long resume_pos)
{
    return _repository->GetDownloader(
		RootedPath(path),
		resume_pos,
		_repository->GetFileInfo(RootedPath(path)).size
	);
}

std::shared_ptr<IFileWriter> ProtocolAWS::FilePut(const std::string &path, mode_t mode,
                                                  unsigned long long size_hint, unsigned long long resume_pos)
{
    return _repository->GetUploader(RootedPath(path));
}

void ProtocolAWS::GetInformation(FileInformation &file_info, const std::string &path, bool follow_symlink)
{
	auto rooted_path = RootedPath(path);
	auto f = _repository->GetFileInfo(rooted_path);
	file_info = FileInformation();
	file_info.access_time = f.modified;
	file_info.modification_time = file_info.access_time;
	file_info.status_change_time = file_info.access_time;
	file_info.size = f.size;
	if (f.isFile) {
		file_info.mode = S_IFREG | DEFAULT_ACCESS_MODE_FILE;
	} else {
		file_info.mode = S_IFDIR | DEFAULT_ACCESS_MODE_DIRECTORY;
	}
}

void ProtocolAWS::DirectoryDelete(const std::string &path)
{
	_repository->DeleteDirectory(RootedPath(path));
}

void ProtocolAWS::Rename(const std::string &path_old, const std::string &path_new)
{
	throw ProtocolUnsupportedError("Rename unsupported");
}

void ProtocolAWS::SetTimes(const std::string &path, const timespec &access_time, const timespec &modification_time)
{
}

void ProtocolAWS::SetMode(const std::string &path, mode_t mode)
{
}

void ProtocolAWS::SymlinkCreate(const std::string &link_path, const std::string &link_target)
{
	throw ProtocolUnsupportedError("Symlink creation unsupported");
}

void ProtocolAWS::SymlinkQuery(const std::string &link_path, std::string &link_target)
{
	throw ProtocolUnsupportedError("Symlink unsupported");
}