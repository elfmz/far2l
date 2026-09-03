#include "S3Repository.h"
#include "S3Auth.h"
#include "AWSFileReader.h"
#include "AWSFileWriter.h"
#include <StringConfig.h>
#include <neon/ne_request.h>
#include <neon/ne_socket.h>
#include <neon/ne_auth.h>
#include <neon/ne_ssl.h>
#include <neon/ne_uri.h>
#include <neon/ne_dates.h>
#include <map>
#include <memory>
#include <algorithm>
#include <cstring>
#include <cstdio>
#include <pwd.h>
#include <unistd.h>

static std::string TrimWS(const std::string &s)
{
	const char *ws = " \t\r\n";
	size_t start = s.find_first_not_of(ws);
	if (start == std::string::npos) return "";
	size_t end = s.find_last_not_of(ws);
	return s.substr(start, end - start + 1);
}

static std::string ParseSectionName(const std::string &line)
{
	size_t end = line.find(']');
	std::string hdr = TrimWS(end != std::string::npos ? line.substr(1, end - 1) : line.substr(1));
	if (hdr.size() >= 8 && hdr.substr(0, 8) == "profile ")
		hdr = TrimWS(hdr.substr(8));
	return hdr;
}

static std::map<std::string, std::string>
ParseAWSIniSection(const std::string &path, const std::string &section)
{
	std::map<std::string, std::string> result;
	FILE *f = fopen(path.c_str(), "r");
	if (!f) return result;

	char buf[512];
	bool in_section = false;
	while (fgets(buf, sizeof(buf), f)) {
		std::string s = TrimWS(buf);
		if (s.empty() || s[0] == '#' || s[0] == ';') continue;

		if (s[0] == '[') {
			in_section = (ParseSectionName(s) == section);
			continue;
		}

		if (!in_section) continue;
		auto eq = s.find('=');
		if (eq == std::string::npos) continue;
		result[TrimWS(s.substr(0, eq))] = TrimWS(s.substr(eq + 1));
	}
	fclose(f);
	return result;
}

static std::string AWSCredentialsDir()
{
	const char *home = getenv("HOME"); // NOSONAR(cpp:S1045)
	if (!home || !*home) {
		struct passwd *pw = getpwuid(getuid());
		home = pw ? pw->pw_dir : nullptr;
	}
	return home ? std::string(home) + "/.aws/" : "";
}

static void EnsureInitNEON()
{
	static int s_init = ne_sock_init();
	(void)s_init;
}

static std::string XmlExtract(const std::string &xml, const std::string &tag)
{
	std::string open  = "<" + tag + ">";
	std::string close = "</" + tag + ">";
	auto start = xml.find(open);
	if (start == std::string::npos) return "";
	start += open.size();
	auto end = xml.find(close, start);
	if (end == std::string::npos) return "";
	return xml.substr(start, end - start);
}

static std::vector<std::string> XmlExtractAll(const std::string &xml, const std::string &tag)
{
	std::vector<std::string> result;
	std::string open  = "<" + tag + ">";
	std::string close = "</" + tag + ">";
	size_t pos = 0;
	for (;;) {
		auto start = xml.find(open, pos);
		if (start == std::string::npos) break;
		start += open.size();
		auto end = xml.find(close, start);
		if (end == std::string::npos) break;
		result.push_back(xml.substr(start, end - start));
		pos = end + close.size();
	}
	return result;
}

struct ResponseCapture {
	std::string body;
	std::string header_name;
	std::string header_value;

	static int sBodyReader(void *userdata, const char *buf, size_t len) // NOSONAR(cpp:S5008)
	{
		auto *self = reinterpret_cast<ResponseCapture *>(userdata);
		self->body.append(buf, len);
		return 0;
	}
};

static void CheckS3Error(const std::string &body, const std::string &context)
{
	std::string code = XmlExtract(body, "Code");
	std::string msg  = XmlExtract(body, "Message");

	if (code == "AccessDenied" || code == "InvalidAccessKeyId" ||
	    code == "SignatureDoesNotMatch" || code == "InvalidSignature") {
		throw ProtocolAuthFailedError(code + ": " + msg);
	}

	if (!code.empty()) {
		std::string full = context.empty() ? code : context + ": " + code;
		if (!msg.empty()) full += " - " + msg;
		throw ProtocolError(full);
	}
}

static std::string BuildPath(const std::string &bucket, const std::string &key)
{
	if (bucket.empty()) return "/";
	if (key.empty()) return "/" + bucket;
	return "/" + bucket + "/" + key;
}

std::string S3Repository::SimpleRequest(
	const std::string &method,
	const std::string &bucket,
	const std::string &key,
	const std::map<std::string, std::string> &query_params,
	const std::string &request_body,
	const std::map<std::string, std::string> &extra_headers)
{
	std::string dummy;
	return SimpleRequestHeader(method, bucket, key, query_params, request_body, dummy, extra_headers);
}

std::string S3Repository::SimpleRequestHeader(
	const std::string &method,
	const std::string &bucket,
	const std::string &key,
	const std::map<std::string, std::string> &query_params,
	const std::string &request_body,
	const std::string &capture_header,
	const std::map<std::string, std::string> &extra_headers)
{
	std::shared_ptr<S3Session> req_session;
	std::string host;
	std::string uri_path;
	std::string effective_region;

	if (_use_path_style || bucket.empty()) {
		req_session = _session;
		host = _endpoint;
		uri_path = BuildPath(bucket, key);
		effective_region = _creds.region.empty() ? "us-east-1" : _creds.region;
	} else {
		effective_region = GetEffectiveRegion(bucket);
		std::string neon_vhost;
		host = GetVirtualEndpoint(bucket, effective_region, &neon_vhost);
		req_session = GetOrCreateBucketSession(neon_vhost);
		uri_path = BuildPath("", key);
	}

	std::unique_ptr<char, decltype(&free)> escaped(ne_path_escape(uri_path.c_str()), free);
	std::string neon_path = escaped ? escaped.get() : uri_path;

	std::string query_str = BuildQueryString(query_params);
	if (!query_str.empty()) neon_path += "?" + query_str;

	std::string payload_hash = S3SHA256Hex(request_body);
	auto auth_headers = S3SignRequest(method, host, uri_path, query_params,
	                                  payload_hash, effective_region,
	                                  _creds.access_key, _creds.secret_key);

	ResponseCapture cap;
	cap.header_name = capture_header;

	ne_request *req = ne_request_create(req_session->sess, method.c_str(), neon_path.c_str());
	if (!req) throw ProtocolError("Failed to create neon request");

	for (auto &h : auth_headers) {
		ne_add_request_header(req, h.first.c_str(), h.second.c_str());
	}
	for (auto &h : extra_headers) {
		ne_add_request_header(req, h.first.c_str(), h.second.c_str());
	}
	if (!_useragent.empty()) {
		ne_add_request_header(req, "User-Agent", _useragent.c_str());
	}

	std::string body_copy = request_body;
	if (!request_body.empty()) {
		ne_set_request_body_buffer(req, body_copy.c_str(), body_copy.size());
		char cl[32];
		snprintf(cl, sizeof(cl), "%zu", body_copy.size());
		ne_add_request_header(req, "Content-Length", cl);
	} else if (method == "PUT" || method == "POST") {
		ne_add_request_header(req, "Content-Length", "0");
	}

	ne_add_response_body_reader(req, ne_accept_always, ResponseCapture::sBodyReader, &cap);

	int rc = ne_request_dispatch(req);

	if (!capture_header.empty()) {
		const char *hv = ne_get_response_header(req, capture_header.c_str());
		if (hv) cap.header_value = hv;
	}

	ne_request_destroy(req);

	if (rc != NE_OK) {
		const char *err = ne_get_error(req_session->sess);
		throw ProtocolError(err ? err : "S3 request failed");
	}

	CheckS3Error(cap.body, method + " /" + bucket + "/" + key);

	return cap.header_name.empty() ? cap.body : cap.header_value;
}

S3Repository::S3Repository(const std::string &host, unsigned int port,
                            const std::string &access_key, const std::string &secret,
                            const std::string &protocol_options)
{
	EnsureInitNEON();

	StringConfig options(protocol_options);
	_useragent = options.GetString("UserAgent");
	_creds.region = options.GetString("Region");
	_use_path_style = options.GetInt("UsePathStyle", 0) != 0;
	_verify_ssl = options.GetInt("VerifySSL", 1) != 0;
	if (_use_path_style && _creds.region.empty()) _creds.region = "us-east-1";

	std::string trimmed_host = Trim(host);
	if (trimmed_host.empty()) {
		throw ProtocolError("S3 endpoint not specified");
	}
	_endpoint = trimmed_host;

	bool scheme_explicit = false;
	if (_endpoint.size() > 8 && _endpoint.substr(0, 8) == "https://") {
		_scheme = "https";
		_endpoint = _endpoint.substr(8);
		scheme_explicit = true;
	// Plain HTTP is intentionally supported for S3-compatible endpoints
	// (MinIO, localhost, on-prem gateways); HTTPS is the default otherwise.
	} else if (_endpoint.size() > 7 && _endpoint.substr(0, 7) == "http://") { // NOSONAR(cpp:S5332)
		_scheme = "http";
		_endpoint = _endpoint.substr(7);
		scheme_explicit = true;
	}
	while (!_endpoint.empty() && _endpoint.back() == '/') _endpoint.pop_back();

	if (!access_key.empty() && !secret.empty()) {
		_creds.access_key = access_key;
		_creds.secret_key = secret;
	} else {
		const char *env_key = getenv("AWS_ACCESS_KEY_ID"); // NOSONAR(cpp:S1045)
		const char *env_sec = getenv("AWS_SECRET_ACCESS_KEY"); // NOSONAR(cpp:S1045)
		if (env_key && *env_key && env_sec && *env_sec) {
			_creds.access_key = env_key;
			_creds.secret_key = env_sec;
		} else {
			const char *profile_env = getenv("AWS_PROFILE"); // NOSONAR(cpp:S1045)
			std::string profile = (profile_env && *profile_env) ? profile_env : "default";
			std::string aws_dir = AWSCredentialsDir();
			if (!aws_dir.empty()) {
				auto vals = ParseAWSIniSection(aws_dir + "credentials", profile);
				auto it_k = vals.find("aws_access_key_id");
				auto it_s = vals.find("aws_secret_access_key");
				if (it_k != vals.end() && it_s != vals.end()) {
					_creds.access_key = it_k->second;
					_creds.secret_key = it_s->second;
				}
			}
			if (_creds.access_key.empty() || _creds.secret_key.empty()) {
				throw ProtocolError("AWS credentials not provided and not found in environment or ~/.aws/credentials");
			}
		}

		if (_creds.region.empty()) {
			const char *env_region = getenv("AWS_DEFAULT_REGION"); // NOSONAR(cpp:S1045)
			if (!env_region || !*env_region) env_region = getenv("AWS_REGION"); // NOSONAR(cpp:S1045)
			if (env_region && *env_region) _creds.region = env_region;
		}

		if (_creds.region.empty()) {
			const char *profile_env = getenv("AWS_PROFILE"); // NOSONAR(cpp:S1045)
			std::string profile = (profile_env && *profile_env) ? profile_env : "default";
			std::string aws_dir = AWSCredentialsDir();
			if (!aws_dir.empty()) {
				auto vals = ParseAWSIniSection(aws_dir + "config", profile);
				auto it = vals.find("region");
				if (it != vals.end()) _creds.region = it->second;
			}
		}
	}

	if (!scheme_explicit)
		_scheme = (port == 80) ? "http" : "https";
	_port = (port > 0) ? port : (_scheme == "http" ? 80 : 443);

	_neon_host = _endpoint;
	{
		auto colon = _neon_host.rfind(':');
		if (colon != std::string::npos) {
			std::string port_str = _neon_host.substr(colon + 1);
			bool all_digits = !port_str.empty() &&
				std::all_of(port_str.begin(), port_str.end(), ::isdigit);
			if (all_digits) {
				_port = (unsigned int)std::stoul(port_str);
				_neon_host = _neon_host.substr(0, colon);
			}
		}
	}

	if ((_scheme == "https" && _port != 443) || (_scheme == "http" && _port != 80)) {
		_endpoint = _neon_host + ":" + std::to_string(_port);
	} else {
		_endpoint = _neon_host;
	}

	if (options.GetInt("UseProxy", 0) != 0) {
		_proxy_host = options.GetString("ProxyHost");
		_proxy_port = (unsigned int)options.GetInt("ProxyPort", 3128);
	}

	_session = CreateConfiguredSession(_neon_host);
	if (!_session->sess) {
		throw ProtocolError("Failed to create neon session");
	}
}

std::shared_ptr<S3Session> S3Repository::CreateConfiguredSession(const std::string &host)
{
	auto s = std::make_shared<S3Session>();
	s->sess = ne_session_create(_scheme.c_str(), host.c_str(), _port);
	if (s->sess) {
		if (_verify_ssl) {
			ne_ssl_trust_default_ca(s->sess);
		} else {
			ne_ssl_set_verify(s->sess, [](void *, int, const ne_ssl_certificate *) { return 0; }, nullptr); // NOSONAR(cpp:S5008)
		}
		if (!_proxy_host.empty())
			ne_session_proxy(s->sess, _proxy_host.c_str(), _proxy_port);
	}
	return s;
}

std::shared_ptr<S3Session> S3Repository::GetOrCreateBucketSession(const std::string &neon_vhost)
{
	auto it = _bucket_sessions.find(neon_vhost);
	if (it != _bucket_sessions.end()) return it->second;
	auto s = CreateConfiguredSession(neon_vhost);
	_bucket_sessions[neon_vhost] = s;
	return s;
}

std::string S3Repository::ResolveRegion(const std::string &bucket)
{
	std::string path = "/" + bucket;
	std::unique_ptr<char, decltype(&free)> escaped(ne_path_escape(path.c_str()), free);
	std::string neon_path = escaped ? escaped.get() : path;

	std::string payload_hash = S3SHA256Hex("");
	auto auth_headers = S3SignRequest("HEAD", _endpoint, path, {},
	                                  payload_hash, "us-east-1",
	                                  _creds.access_key, _creds.secret_key);

	ne_request *req = ne_request_create(_session->sess, "HEAD", neon_path.c_str());
	if (!req) return "us-east-1";

	for (auto &h : auth_headers)
		ne_add_request_header(req, h.first.c_str(), h.second.c_str());

	ne_request_dispatch(req);

	const char *hv = ne_get_response_header(req, "x-amz-bucket-region");
	std::string region = (hv && *hv) ? hv : "us-east-1";
	ne_request_destroy(req);
	return region;
}

std::string S3Repository::GetEffectiveRegion(const std::string &bucket)
{
	if (!_creds.region.empty()) return _creds.region;

	auto it = _bucket_region_cache.find(bucket);
	if (it != _bucket_region_cache.end()) return it->second;

	std::string r = ResolveRegion(bucket);
	_bucket_region_cache[bucket] = r;
	return r;
}

std::string S3Repository::GetVirtualEndpoint(const std::string &bucket, const std::string &region,
                                              std::string *neon_vhost_out)
{
	std::string vhost;
	const std::string aws_suffix = "amazonaws.com";
	if (_neon_host.size() >= aws_suffix.size() &&
	    _neon_host.compare(_neon_host.size() - aws_suffix.size(), aws_suffix.size(), aws_suffix) == 0) {
		vhost = bucket + ".s3." + region + ".amazonaws.com";
	} else {
		vhost = bucket + "." + _neon_host;
	}

	if (neon_vhost_out) *neon_vhost_out = vhost;

	if ((_scheme == "https" && _port != 443) || (_scheme == "http" && _port != 80))
		return vhost + ":" + std::to_string(_port);
	return vhost;
}

std::string S3Repository::Trim(const std::string &str)
{
	const std::string unwanted = "/\\ ";
	auto result = str;
	size_t start = result.find_first_not_of(unwanted);
	if (start == std::string::npos) return "";
	result.erase(0, start);
	size_t end = result.find_last_not_of(unwanted);
	result.erase(end + 1);
	return result;
}

std::string S3Repository::ExtractFileName(const std::string &path)
{
	size_t pos = path.find_last_of('/');
	return (pos == std::string::npos) ? path : path.substr(pos + 1);
}

std::vector<AWSFile> S3Repository::ListBuckets()
{
	std::string body = SimpleRequest("GET", "", "", {}, "");

	std::vector<AWSFile> ls;
	for (const auto &item : XmlExtractAll(body, "Bucket")) {
		std::string name = XmlExtract(item, "Name");
		std::string date = XmlExtract(item, "CreationDate");
		if (!name.empty()) {
			ls.emplace_back(name, false, date, 0);
		}
	}
	return ls;
}

std::vector<AWSFile> S3Repository::ListFolder(const std::string &path)
{
	Path localPath(path);
	std::string continuation;
	size_t prefix_len = 0;

	std::string prefix;
	if (localPath.hasKey()) {
		prefix = localPath.keyWithSlash();
		prefix_len = prefix.size();
	}

	std::vector<AWSFile> ls;

	do {
		std::map<std::string, std::string> qp = {{"delimiter", "/"}, {"list-type", "2"}};
		if (!prefix.empty()) qp["prefix"] = prefix;
		if (!continuation.empty()) qp["continuation-token"] = continuation;

		std::string body = SimpleRequest("GET", localPath.bucket(), "", qp, "");

		for (const auto &item : XmlExtractAll(body, "Contents")) {
			std::string full_key = XmlExtract(item, "Key");
			if (full_key.size() <= prefix_len) continue;
			std::string rel_key = full_key.substr(prefix_len);
			if (rel_key == "/") continue;
			if (rel_key.back() == '/') {
				rel_key.pop_back();
				if (rel_key.empty() || rel_key.find('/') != std::string::npos) continue;
				ls.emplace_back(rel_key, false, "", 0);
				continue;
			}
			if (rel_key.find('/') != std::string::npos) continue;
			std::string date = XmlExtract(item, "LastModified");
			long long   size = std::stoll(XmlExtract(item, "Size").empty() ? "0" : XmlExtract(item, "Size"));
			ls.emplace_back(rel_key, true, date, size);
		}

		for (const auto &item : XmlExtractAll(body, "CommonPrefixes")) {
			std::string p = XmlExtract(item, "Prefix");
			if (p.size() <= prefix_len) continue;
			p = p.substr(prefix_len);
			if (!p.empty() && p.back() == '/') p.pop_back();
			if (!p.empty()) ls.emplace_back(p, false, "", 0);
		}

		continuation = XmlExtract(body, "NextContinuationToken");
	} while (!continuation.empty());

	return ls;
}

bool S3Repository::IsFolder(const Path &p)
{
	std::map<std::string, std::string> qp = {
		{"list-type", "2"}, {"max-keys", "1"}, {"prefix", p.keyWithSlash()}
	};
	try {
		std::string body = SimpleRequest("GET", p.bucket(), "", qp, "");
		auto contents = XmlExtractAll(body, "Contents");
		auto prefixes = XmlExtractAll(body, "CommonPrefixes");
		return !contents.empty() || !prefixes.empty();
	} catch (...) {
		return false;
	}
}

AWSFile S3Repository::GetFileInfo(const std::string &path)
{
	Path localPath(path);

	if (!localPath.hasKey()) {
		return AWSFile(ExtractFileName(path), false);
	}

	try {
		std::shared_ptr<S3Session> req_session;
		std::string host;
		std::string uri_path;
		std::string effective_region;

		if (_use_path_style) {
			req_session = _session;
			host = _endpoint;
			uri_path = "/" + localPath.bucket() + "/" + localPath.key();
			effective_region = _creds.region.empty() ? "us-east-1" : _creds.region;
		} else {
			effective_region = GetEffectiveRegion(localPath.bucket());
			std::string neon_vhost;
			host = GetVirtualEndpoint(localPath.bucket(), effective_region, &neon_vhost);
			req_session = GetOrCreateBucketSession(neon_vhost);
			uri_path = "/" + localPath.key();
		}

		std::string payload_hash = S3SHA256Hex("");
		auto auth_headers = S3SignRequest("HEAD", host, uri_path, {},
		                                  payload_hash, effective_region,
		                                  _creds.access_key, _creds.secret_key);

		std::unique_ptr<char, decltype(&free)> escaped(ne_path_escape(uri_path.c_str()), free);
		std::string neon_path = escaped ? escaped.get() : uri_path;

		ne_request *req = ne_request_create(req_session->sess, "HEAD", neon_path.c_str());
		if (!req) throw ProtocolError("HEAD request failed");

		for (auto &h : auth_headers) {
			ne_add_request_header(req, h.first.c_str(), h.second.c_str());
		}
		if (!_useragent.empty()) {
			ne_add_request_header(req, "User-Agent", _useragent.c_str());
		}

		int rc = ne_request_dispatch(req);
		const char *cl_hdr  = ne_get_response_header(req, "Content-Length");
		const char *lm_hdr  = ne_get_response_header(req, "Last-Modified");

		const ne_status *status = ne_get_status(req);
		int http_status = status ? status->code : 0;
		std::string cl_val  = cl_hdr  ? cl_hdr  : "0";
		std::string lm_val  = lm_hdr  ? lm_hdr  : "";
		ne_request_destroy(req);

		if (rc == NE_OK && http_status == 200) {
			long long size = cl_val.empty() ? 0 : std::stoll(cl_val);
			AWSFile f(ExtractFileName(path), true);
			f.size = size;
			time_t t = lm_val.empty() ? 0 : ne_httpdate_parse(lm_val.c_str());
			f.modified.tv_sec  = (t != (time_t)-1) ? t : 0;
			f.modified.tv_nsec = 0;
			return f;
		}
		// Non-200 status (including 404): fall through to IsFolder check
	} catch (ProtocolAuthFailedError &) {
		throw;
	} catch (...) { /* HEAD failed; fall through to IsFolder check */ }

	if (IsFolder(localPath)) {
		return AWSFile(ExtractFileName(path), false);
	}

	throw ProtocolError("No such file or directory: " + path);
}

std::shared_ptr<AWSFileReader> S3Repository::GetDownloader(const std::string &path,
                                                            unsigned long long position,
                                                            unsigned long long size)
{
	Path localPath(path);
	S3Credentials effective_creds = _creds;
	std::shared_ptr<S3Session> req_session;
	std::string effective_host;
	std::string path_prefix;

	if (_use_path_style) {
		req_session = _session;
		effective_host = _endpoint;
		path_prefix = "/" + localPath.bucket();
		if (effective_creds.region.empty()) effective_creds.region = "us-east-1";
	} else {
		effective_creds.region = GetEffectiveRegion(localPath.bucket());
		std::string neon_vhost;
		effective_host = GetVirtualEndpoint(localPath.bucket(), effective_creds.region, &neon_vhost);
		req_session = GetOrCreateBucketSession(neon_vhost);
		path_prefix = "";
	}

	return std::make_shared<AWSFileReader>(req_session, effective_creds, effective_host, _useragent,
	                                       path_prefix, localPath.key(), position, size);
}

std::shared_ptr<AWSFileWriter> S3Repository::GetUploader(const std::string &path)
{
	Path localPath(path);
	S3Credentials effective_creds = _creds;
	std::shared_ptr<S3Session> req_session;
	std::string effective_host;
	std::string path_prefix;

	if (_use_path_style) {
		req_session = _session;
		effective_host = _endpoint;
		path_prefix = "/" + localPath.bucket();
		if (effective_creds.region.empty()) effective_creds.region = "us-east-1";
	} else {
		effective_creds.region = GetEffectiveRegion(localPath.bucket());
		std::string neon_vhost;
		effective_host = GetVirtualEndpoint(localPath.bucket(), effective_creds.region, &neon_vhost);
		req_session = GetOrCreateBucketSession(neon_vhost);
		path_prefix = "";
	}

	return std::make_shared<AWSFileWriter>(req_session, effective_creds, effective_host, _useragent,
	                                       path_prefix, localPath.key());
}

void S3Repository::CreateBucket(const std::string &bucket)
{
	std::string body;
	std::map<std::string, std::string> extra;
	std::string region = _creds.region.empty() ? "us-east-1" : _creds.region;
	if (region != "us-east-1") {
		body = "<CreateBucketConfiguration xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">"
		       "<LocationConstraint>" + region + "</LocationConstraint>"
		       "</CreateBucketConfiguration>";
		extra["Content-Type"] = "application/xml";
	}
	SimpleRequest("PUT", bucket, "", {}, body, extra);
}

void S3Repository::CreateDirectory(const std::string &path)
{
	Path localPath(path);
	std::map<std::string, std::string> extra;
	extra["Content-Type"] = "application/x-directory";
	SimpleRequest("PUT", localPath.bucket(), localPath.keyWithSlash(), {}, "", extra);
}

void S3Repository::DeleteDirectory(const std::string &path)
{
	Path localPath(path);
	std::string prefix = localPath.hasKey() ? localPath.keyWithSlash() : "";
	std::string continuation;
	do {
		std::map<std::string, std::string> qp = {{"list-type", "2"}};
		if (!prefix.empty()) qp["prefix"] = prefix;
		if (!continuation.empty()) qp["continuation-token"] = continuation;

		std::string body = SimpleRequest("GET", localPath.bucket(), "", qp, "");

		for (const auto &item : XmlExtractAll(body, "Contents")) {
			std::string key = XmlExtract(item, "Key");
			if (!key.empty()) {
				SimpleRequest("DELETE", localPath.bucket(), key, {}, "");
			}
		}

		continuation = XmlExtract(body, "NextContinuationToken");
	} while (!continuation.empty());

	if (localPath.key().empty()) {
		SimpleRequest("DELETE", localPath.bucket(), "", {}, "");
	}
}

void S3Repository::DeleteFile(const std::string &path)
{
	Path localPath(path);
	SimpleRequest("DELETE", localPath.bucket(), localPath.key(), {}, "");
}
