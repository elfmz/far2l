#pragma once
#include <memory>
#include <string>
#include <map>
#include <set>
#include <neon/ne_session.h>
#include <neon/ne_request.h>
#include "../Protocol.h"

struct DavConnection
{
	ne_session *sess = nullptr;

	DavConnection() = default;

	~DavConnection()
	{
		if (sess != nullptr) {
			ne_session_destroy(sess);
		}
	}

private:
	DavConnection(const DavConnection &) = delete;
};

class ProtocolWebDAV : public IProtocol
{
	std::shared_ptr<DavConnection> _conn;
	std::string _useragent;
	std::string _username, _password;
	std::string _proxy_username, _proxy_password;
	std::string _known_server_identity, _current_server_identity;

	static int sAuthCreds(void *userdata, const char *realm, int attempt, char *username, char *password);
	static int sProxyAuthCreds(void *userdata, const char *realm, int attempt, char *username, char *password);
	static int sVerifySsl(void *userdata, int failures, const ne_ssl_certificate *cert);
	static void sCreateRequestHook(ne_request *req, void *userdata, const char *method, const char *requri);
public:

	ProtocolWebDAV(const char *scheme, const std::string &host, unsigned int port,
		const std::string &username, const std::string &password, const std::string &protocol_options);
	virtual ~ProtocolWebDAV();

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
};
