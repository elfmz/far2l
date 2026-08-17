#pragma once
#include <string>
#include <vector>
#include <memory>
#include <neon/ne_session.h>
#include "AWSFile.h"
#include "../Protocol.h"

class AWSFileReader;
class AWSFileWriter;

struct S3Credentials
{
	std::string access_key;
	std::string secret_key;
	std::string region;
};

struct S3Session
{
	ne_session *sess = nullptr;

	S3Session() = default;
	~S3Session() {
		if (sess) ne_session_destroy(sess);
	}
private:
	S3Session(const S3Session &) = delete;
	S3Session &operator=(const S3Session &) = delete;
};

class S3Repository
{
public:
	S3Repository(const std::string &host, unsigned int port,
	             const std::string &access_key, const std::string &secret,
	             const std::string &protocol_options);
	~S3Repository() = default;

	std::vector<AWSFile> ListBuckets();
	std::vector<AWSFile> ListFolder(const std::string &path);

	AWSFile GetFileInfo(const std::string &path);

	std::shared_ptr<AWSFileReader> GetDownloader(const std::string &path,
	                                              unsigned long long position,
	                                              unsigned long long size);
	std::shared_ptr<AWSFileWriter> GetUploader(const std::string &path);

	void CreateBucket(const std::string &bucket);
	void CreateDirectory(const std::string &path);
	void DeleteDirectory(const std::string &path);
	void DeleteFile(const std::string &path);

	class Path {
	public:
		explicit Path(const std::string &path)
			: _bucket(ParseBucket(path)), _key(ParseKey(path)) {}

		const std::string &bucket()   const { return _bucket; }
		const std::string &key()      const { return _key; }
		std::string keyWithSlash()    const { return _key + "/"; }
		bool hasKey()                 const { return !_key.empty(); }

	private:
		std::string _bucket;
		std::string _key;

		static std::string ParseBucket(const std::string &path) {
			size_t start = path.find_first_not_of('/');
			if (start == std::string::npos) return "";
			auto pos = path.find('/', start);
			return (pos == std::string::npos) ? path.substr(start)
			                                  : path.substr(start, pos - start);
		}

		static std::string ParseKey(const std::string &path) {
			std::string t = path;
			while (!t.empty() && t.front() == '/') t.erase(0, 1);
			while (!t.empty() && t.back()  == '/') t.pop_back();
			auto pos = t.find('/');
			return (pos == std::string::npos) ? "" : t.substr(pos + 1);
		}
	};

	const S3Credentials &Creds()   const { return _creds; }
	const std::string   &Endpoint() const { return _endpoint; }

private:
	std::shared_ptr<S3Session> _session;
	S3Credentials _creds;
	std::string _endpoint;   // host[:port] used for neon session and SigV4 host header
	std::string _neon_host;  // pure hostname without port
	std::string _scheme;     // "https" or "http"
	unsigned int _port;
	std::string _useragent;
	std::string _proxy_host;
	unsigned int _proxy_port = 0;
	bool _use_path_style = false;
	bool _verify_ssl = true;

	std::map<std::string, std::shared_ptr<S3Session>> _bucket_sessions;
	std::map<std::string, std::string> _bucket_region_cache;

	std::shared_ptr<S3Session> CreateConfiguredSession(const std::string &host);
	std::shared_ptr<S3Session> GetOrCreateBucketSession(const std::string &neon_vhost);
	std::string ResolveRegion(const std::string &bucket);
	std::string GetEffectiveRegion(const std::string &bucket);
	std::string GetVirtualEndpoint(const std::string &bucket, const std::string &region,
	                               std::string *neon_vhost_out = nullptr);

	std::string SimpleRequest(
		const std::string &method,
		const std::string &bucket,
		const std::string &key,
		const std::map<std::string, std::string> &query_params,
		const std::string &request_body,
		const std::map<std::string, std::string> &extra_headers = {});

	std::string SimpleRequestHeader(
		const std::string &method,
		const std::string &bucket,
		const std::string &key,
		const std::map<std::string, std::string> &query_params,
		const std::string &request_body,
		const std::string &capture_header,
		const std::map<std::string, std::string> &extra_headers = {});

	bool IsFolder(const Path &p);

	static std::string ExtractFileName(const std::string &path);
	static std::string Trim(const std::string &str);
};
