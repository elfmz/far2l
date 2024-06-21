#include <mutex>
#include <condition_variable>

#include <sys/stat.h>
#include <fcntl.h>
#include <vector>
#include <string.h>
#include <errno.h>
#include <StringConfig.h>
#include <utils.h>
#include <Threaded.h>
#include <os_call.hpp>

#include "ProtocolWebDAV.h"
#include <neon/ne_auth.h>
#include <neon/ne_basic.h>
#include <neon/ne_props.h>
#include <neon/ne_socket.h>
#include <neon/ne_ssl.h>
#include <neon/ne_uri.h>


//static const ne_propname PROP_DISPLAYNAME = { "DAV:", "displayname" };

static const ne_propname PROP_ISCOLLECTION = { "DAV:", "iscollection" };
static const ne_propname PROP_COLLECTION = { "DAV:", "collection" };
static const ne_propname PROP_RESOURCETYPE = { "DAV:", "resourcetype" };
static const ne_propname PROP_EXECUTABLE = { "http://apache.org/dav/props/", "executable"};

static const ne_propname PROP_GETCONTENTLENGTH = { "DAV:", "getcontentlength" };
static const ne_propname PROP_CREATIONDATE = { "DAV:", "creationdate" };
static const ne_propname PROP_GETLASTMODIFIED = { "DAV:", "getlastmodified" };


static const char *s_months = "janfebmaraprmayjunjulaugsepoctnovdec";

static std::string RefinePath(std::string path, bool ending_slash = false)
{
	while (!path.empty() && path[path.size() - 1] == '/') {
		path.resize(path.size() - 1);
	}

	if (path.empty() || path[0] != '/') {
		path.insert(0, "/");
	}

	for (;;) {
		size_t i = path.rfind("/./");
		if (i == std::string::npos)
			break;

		path.erase(i, 2);
	}

	if (path.size() >= 2 && path[path.size() - 1] == '.' && path[path.size() - 2] == '/') {
		path.resize(path.size() - 2);
		if (path.empty()) {
			path = '/';
		}
	}

	if (ending_slash && (path.empty() || path[path.size() - 1] != '/')) {
		path+= '/';
	}

	char *xpath = ne_path_escape(path.c_str());
	path = xpath;
	free(xpath);

	return path;
}


/////////////////////

static void ParseWebDavDateTime(timespec &result, const std::string &str)
{
	// parse something like str = "Sat, 01 Jun 2019 10:40:09 GMT"
	result = timespec();

	std::vector<std::string> parts;
	StrExplode(parts, str, " ,");
	if (parts.size() < 5) {
		return;
	}

	std::vector<std::string> parts_of_time;
	StrExplode(parts_of_time, parts[parts.size() - 2], ":.-");

	struct tm tm_parts = {};
	if (parts_of_time.size() >= 3) tm_parts.tm_sec = atoi(parts_of_time[2].c_str());
	if (parts_of_time.size() >= 2) tm_parts.tm_min = atoi(parts_of_time[1].c_str());
	if (parts_of_time.size() >= 1) tm_parts.tm_hour = atoi(parts_of_time[0].c_str());
	tm_parts.tm_year = atoi(parts[parts.size() - 3].c_str());
	const char *mon = strcasestr(s_months, parts[parts.size() - 4].c_str());
	tm_parts.tm_mon = mon ? (mon - s_months) / 3 : 0;
	tm_parts.tm_mday = atoi(parts[parts.size() - 5].c_str());

/*
	fprintf(stderr, "ParseWebDavDateTime('%s') -> %u/%u/%u %u:%u.%u\n", str.c_str(),
		tm_parts.tm_year, tm_parts.tm_mon, tm_parts.tm_mday, tm_parts.tm_hour, tm_parts.tm_min, tm_parts.tm_sec);
	for (const auto &part : parts) {
		fprintf(stderr, "'%s' ", part.c_str());
	}
	fprintf(stderr, "\n");
*/

	result.tv_sec = mktime(&tm_parts);
}


struct PropsList : std::map<std::string, std::string>
{
	mode_t GetMode()
	{
		mode_t mode;

		auto it = find(PROP_RESOURCETYPE.name);
		if (it != end() && it->second.find("<DAV:collection") != std::string::npos) {
			mode = S_IFDIR | DEFAULT_ACCESS_MODE_DIRECTORY;
		} else {
			it = find(PROP_ISCOLLECTION.name);
			if (it != end() && it->second == "1") {
				mode = S_IFDIR | DEFAULT_ACCESS_MODE_DIRECTORY;
			} else {
				mode = S_IFREG | DEFAULT_ACCESS_MODE_FILE;
			}
		}

		it = find(PROP_EXECUTABLE.name);
		if (it != end() && it->second == "T") {
			mode|= (S_IXUSR | S_IXGRP | S_IXOTH);
		}

		return mode;
	}

	unsigned long long GetSize()
	{
		auto it = find(PROP_RESOURCETYPE.name);
		if (it != end() && it->second.find("<DAV:collection") != std::string::npos) {
			return 0;
		}
		it = find(PROP_ISCOLLECTION.name);
		if (it != end() && it->second == "1") {
			return 0;
		}
		it = find(PROP_GETCONTENTLENGTH.name);
		if (it == end()) {
			return 0;
		}

		return (unsigned long long)atoll(it->second.c_str());
	}

	void GetFileInfo(FileInformation &file_info)
	{
		file_info.mode = GetMode();
		file_info.size = GetSize();

		auto it_cr = find(PROP_CREATIONDATE.name);
		if (it_cr != end()) {
			ParseWebDavDateTime(file_info.status_change_time, it_cr->second);
		}

		auto it_glm = find(PROP_GETLASTMODIFIED.name);
		if (it_glm != end()) {
			ParseWebDavDateTime(file_info.modification_time, it_glm->second);
			if (it_cr == end()) {
				file_info.status_change_time = file_info.modification_time;
			}

		} else if (it_cr != end() ) {
			file_info.modification_time = file_info.status_change_time;
		}

		file_info.access_time = file_info.modification_time;
	}
};

struct WebDavProps : std::map<std::string, PropsList >
{
	WebDavProps(ne_session *sess, const std::string &path, bool children, const ne_propname *pn = nullptr, ...)
	{

		std::vector<ne_propname> pn_v;
		va_list args;
		va_start(args, pn);
		while (pn) {
			pn_v.emplace_back(*pn);
			pn = va_arg(args, const ne_propname *);
		}
		va_end(args);

		int rc;
		if (pn_v.empty()) {
			rc = ne_simple_propfind(sess, path.c_str(), children ? NE_DEPTH_ONE : NE_DEPTH_ZERO, nullptr, sOnResults, this);
		} else {
			pn_v.emplace_back(ne_propname{nullptr, nullptr});
			rc = ne_simple_propfind(sess, path.c_str(), children ? NE_DEPTH_ONE : NE_DEPTH_ZERO, &pn_v[0], sOnResults, this);
		}

		if (children && !empty()) {
			auto shortest_it = begin();
			for (auto it = shortest_it; it != end(); ++it) {
				if (shortest_it->first.size() > it->first.size()) {
					shortest_it = it;
				}
			}
			erase(shortest_it);
		}

		if (rc != NE_OK) {
			fprintf(stderr, "WebDavProps('%s'): %d\n", path.c_str(), rc);
			if (rc == NE_AUTH) {
				throw ProtocolAuthFailedError(ne_get_error(sess));
			}
			throw ProtocolError("Query props", ne_get_error(sess), rc);
		}
	}

private:
	std::string _cur_uri_path;

	static void sOnResults(void *userdata, const ne_uri *uri, const ne_prop_result_set *results)
	{
		char *xpath = ne_path_unescape((uri && uri->path) ? uri->path : "");
		((WebDavProps *)userdata)->_cur_uri_path = xpath;
		free(xpath);
		ne_propset_iterate(results, &sOnProp, userdata);
	}

	static int sOnProp(void *userdata, const ne_propname *pname, const char *value, const ne_status *status)
	{
//		fprintf(stderr, "sOnProp: '%s' = '%s'\n", pname->name, value);
		if (value) {
			((WebDavProps *)userdata)->operator[](((WebDavProps *)userdata)->_cur_uri_path).emplace(pname->name, value);
		}
		return 0;
	}
};


#define PROPS_MODE &PROP_ISCOLLECTION, &PROP_COLLECTION, &PROP_RESOURCETYPE, &PROP_EXECUTABLE,
#define PROPS_SIZE &PROP_GETCONTENTLENGTH,
#define PROPS_TIMES &PROP_CREATIONDATE, &PROP_GETLASTMODIFIED,
#define PROPS_ENUM PROPS_MODE PROPS_SIZE PROPS_TIMES //&PROP_DISPLAYNAME,

////////////////////////////////////////////////////////

std::shared_ptr<IProtocol> CreateProtocol(const std::string &protocol, const std::string &host, unsigned int port,
		const std::string &username, const std::string &password, const std::string &options, int fd_ipc_recv)
{
	if (protocol == "davs") {
		return std::make_shared<ProtocolWebDAV>("https", host, port, username, password, options);
	}

	return std::make_shared<ProtocolWebDAV>("http", host, port, username, password, options);
}

////////////////////////////

static void EnsureInitNEON()
{
	static int neon_init_status = ne_sock_init();
	if (neon_init_status != 0) {
		neon_init_status = ne_sock_init();
		fprintf(stderr, "EnsureInitNEON: %d\n", neon_init_status);
	}
}

int ProtocolWebDAV::sAuthCreds(void *userdata, const char *realm, int attempt, char *username, char *password)
{
	strncpy(username, ((ProtocolWebDAV *)userdata)->_username.c_str(), NE_ABUFSIZ - 1);
	strncpy(password, ((ProtocolWebDAV *)userdata)->_password.c_str(), NE_ABUFSIZ - 1);
	return attempt;
}

int ProtocolWebDAV::sProxyAuthCreds(void *userdata, const char *realm, int attempt, char *username, char *password)
{
	strncpy(username, ((ProtocolWebDAV *)userdata)->_proxy_username.c_str(), NE_ABUFSIZ - 1);
	strncpy(password, ((ProtocolWebDAV *)userdata)->_proxy_password.c_str(), NE_ABUFSIZ - 1);
	return attempt;
}

int ProtocolWebDAV::sVerifySsl(void *userdata, int failures, const ne_ssl_certificate *cert)
{
	char digest[NE_SSL_DIGESTLEN + 1] = {};
	ne_ssl_cert_digest(cert, digest);

	((ProtocolWebDAV *)userdata)->_current_server_identity.clear();
	for (const char *c = &digest[0]; *c; ++c) {
		if (*c != ':') {
			((ProtocolWebDAV *)userdata)->_current_server_identity+= *c;
		}
	}
	if (((ProtocolWebDAV *)userdata)->_current_server_identity != ((ProtocolWebDAV *)userdata)->_known_server_identity) {
		return -1;
	}
	return 0;
}

ProtocolWebDAV::ProtocolWebDAV(const char *scheme, const std::string &host, unsigned int port,
	const std::string &username, const std::string &password, const std::string &options)
	:
	_conn(std::make_shared<DavConnection>()),
	_username(username), _password(password)
{
	EnsureInitNEON();

	StringConfig protocol_options(options);
	_useragent = protocol_options.GetString("UserAgent");
	_known_server_identity = protocol_options.GetString("ServerIdentity");

	_conn->sess = ne_session_create(scheme, host.c_str(), port);
	if (_conn->sess == nullptr) {
		throw ProtocolError("Create session error", errno);
	}

	if (!_useragent.empty()) {
		ne_hook_create_request(_conn->sess, &sCreateRequestHook, this);
	}

	ne_ssl_set_verify(_conn->sess, sVerifySsl, this);

	if (!username.empty() || !password.empty()) {
		ne_set_server_auth(_conn->sess, sAuthCreds, this);
	}

	if (protocol_options.GetInt("UseProxy", 0) != 0) {
		ne_session_proxy(_conn->sess,
			protocol_options.GetString("ProxyHost").c_str(),
			(unsigned int)protocol_options.GetInt("ProxyPort"));

		if (protocol_options.GetInt("AuthProxy", 0) != 0) {
			_proxy_username = protocol_options.GetString("ProxyUsername");
			_proxy_password = protocol_options.GetString("ProxyPassword");
			ne_set_proxy_auth(_conn->sess, sProxyAuthCreds, this);
		}
	}

	try { // probe auth
		WebDavProps(_conn->sess, "/", false, &PROP_RESOURCETYPE, nullptr);

	} catch (ProtocolAuthFailedError &) {
		throw;

	} catch (std::exception &ex) {
		if (_current_server_identity != _known_server_identity) {
			throw ServerIdentityMismatchError(_current_server_identity);
		}

		fprintf(stderr, "Ignoring non-auth error on auth probe: %s\n", ex.what());
	}
}

ProtocolWebDAV::~ProtocolWebDAV()
{
}

void ProtocolWebDAV::sCreateRequestHook(ne_request *req, void *userdata, const char *method, const char *requri)
{
	ProtocolWebDAV *it = (ProtocolWebDAV *)userdata;
	if (!it->_useragent.empty()) {
		ne_add_request_header(req, "User-Agent", it->_useragent.c_str());
//		fprintf(stderr, "USERAGENT '%s' for '%s' '%s'\n", it->_useragent.c_str(), method, requri);
	}
}


mode_t ProtocolWebDAV::GetMode(const std::string &path, bool follow_symlink)
{
	WebDavProps wdp(_conn->sess, RefinePath(path), false, PROPS_MODE nullptr);
	if (wdp.empty())
		return S_IFREG | DEFAULT_ACCESS_MODE_FILE;

	return wdp.begin()->second.GetMode();
}


unsigned long long ProtocolWebDAV::GetSize(const std::string &path, bool follow_symlink)
{
	WebDavProps wdp(_conn->sess, RefinePath(path), false, PROPS_SIZE nullptr);
	if (wdp.empty())
		return 0;

	return wdp.begin()->second.GetSize();
}


void ProtocolWebDAV::GetInformation(FileInformation &file_info, const std::string &path, bool follow_symlink)
{
	WebDavProps wdp(_conn->sess, RefinePath(path), false, PROPS_MODE PROPS_SIZE PROPS_TIMES nullptr);
	file_info = FileInformation();
	if (!wdp.empty())
		wdp.begin()->second.GetFileInfo(file_info);
}

void ProtocolWebDAV::FileDelete(const std::string &path)
{
	int rc = ne_delete(_conn->sess, RefinePath(path).c_str());
	if (rc != NE_OK) {
		throw ProtocolError("Delete file", ne_get_error(_conn->sess), rc);
	}
}

void ProtocolWebDAV::DirectoryDelete(const std::string &path)
{
	int rc = ne_delete(_conn->sess, RefinePath(path).c_str());
	if (rc != NE_OK) {
		throw ProtocolError("Delete directory", ne_get_error(_conn->sess), rc);
	}
}

void ProtocolWebDAV::DirectoryCreate(const std::string &path, mode_t mode)
{
	if (path.empty() || path == "/") {
		throw ProtocolError("Cannot create root directory");
	}

	int rc = ne_mkcol(_conn->sess, RefinePath(path, true).c_str());
	if (rc != NE_OK) {
		throw ProtocolError("Create directory", ne_get_error(_conn->sess), rc);
	}
}

void ProtocolWebDAV::Rename(const std::string &path_old, const std::string &path_new)
{
	int rc = ne_move(_conn->sess, 1, RefinePath(path_old).c_str(), RefinePath(path_new).c_str());
	if (rc != NE_OK) {
		throw ProtocolError("Rename", ne_get_error(_conn->sess), rc);
	}
}


void ProtocolWebDAV::SetTimes(const std::string &path, const timespec &access_time, const timespec &modification_time)
{
}

static void ProtocolWebDAV_ChangeExecutable(ne_session *sess, const std::string &path, bool executable)
{
	ne_proppatch_operation ops[] = { { &PROP_EXECUTABLE, ne_propset, executable ? "T" : "F"}, {} };
	int rc = ne_proppatch(sess, path.c_str(), ops);
	if (rc != NE_OK) {
		fprintf(stderr, "ProtocolWebDAV_ChangeExecutable('%s', %d) error %d - '%s'\n",
			path.c_str(), executable, rc, ne_get_error(sess));
	}
}

void ProtocolWebDAV::SetMode(const std::string &path, mode_t mode)
{
	ProtocolWebDAV_ChangeExecutable(_conn->sess, RefinePath(path), (mode & (S_IXUSR | S_IXGRP | S_IXOTH)) != 0);
}

void ProtocolWebDAV::SymlinkCreate(const std::string &link_path, const std::string &link_target)
{
	throw ProtocolUnsupportedError("Symlink creation unsupported");
}

void ProtocolWebDAV::SymlinkQuery(const std::string &link_path, std::string &link_target)
{
	throw ProtocolUnsupportedError("Symlink querying unsupported");
}

class DavDirectoryEnumer : public IDirectoryEnumer
{
	WebDavProps _wdp;
public:
	DavDirectoryEnumer(std::shared_ptr<DavConnection> &conn, std::string path)
		: _wdp(conn->sess, RefinePath(path, true), true, PROPS_ENUM nullptr)
//		: _wdp(conn->sess, RefinePath(path, true), true, nullptr)
	{
/*
		for (const auto &path_i : _wdp) {
			fprintf(stderr, "Path: %s\n", path_i.first.c_str());
			for (const auto &prop : path_i.second) {
				fprintf(stderr, "    '%s'='%s'\n", prop.first.c_str(), prop.second.c_str());
			}
		}
*/
	}

	virtual ~DavDirectoryEnumer()
	{
	}

	virtual bool Enum(std::string &name, std::string &owner, std::string &group, FileInformation &file_info)
	{
		if (_wdp.empty()) {
			return false;
		}

		owner.clear();
		group.clear();
		file_info = FileInformation();

		auto i = _wdp.begin();

		i->second.GetFileInfo(file_info);

/*		auto prop_i = i->second.find(PROP_DISPLAYNAME.name);
		if (prop_i != i->second.end() && !prop_i->second.empty()) {
			name = prop_i->second;
		} else */
			name = i->first;

		if (!name.empty() && name[name.size() - 1] == '/') {
			name.resize(name.size() - 1);
		}

		size_t p = name.rfind('/');
		if (p != std::string::npos) {
			name.erase(0, p + 1);
		}

		_wdp.erase(i);
		return true;
	}
};

std::shared_ptr<IDirectoryEnumer> ProtocolWebDAV::DirectoryEnum(const std::string &path)
{
	return std::make_shared<DavDirectoryEnumer>(_conn, path);
}

class DavFileIO : public IFileReader, public IFileWriter, protected Threaded
{
	std::shared_ptr<DavConnection> _conn;
	std::string _path;
	mode_t _mode;
	unsigned long long _resume_pos;
	bool _writing;
	ne_request *_req = nullptr;

	int _ne_status = NE_ERROR;
	std::string _ne_error;
	bool _done = false;

	std::vector<char> _buf;
	std::mutex _mtx;
	std::condition_variable _cond;

	enum {
		INTERMEDIATE_BUFFER = 0x100000
	};

	bool TryAddToBuffer(const void *data, size_t len)
	{
		try {
			const size_t prev_size = _buf.size();
			if (prev_size > INTERMEDIATE_BUFFER) {
				return false;
			}

			_buf.resize(prev_size + len);
			memcpy(&_buf[prev_size], data, len);

		} catch (std::exception &ex) {
			(void)ex;
			return false;
		}

		return true;
	}

	size_t TryFetchFromBuffer(void *data, size_t len)
	{
		len = std::min(len, _buf.size());
		if (len != 0) {
			memcpy(data, &_buf[0], len);
			_buf.erase(_buf.begin(), _buf.begin() + len);
		}

		return len;
	}

	static ssize_t sWriteCallback(void *userdata, char *buf, size_t buflen)
	{
		return ((DavFileIO *)userdata)->WriteCallback(buf, buflen);
	}

	static int sReadCallback(void *userdata, const char *buf, size_t len)
	{
		return ((DavFileIO *)userdata)->ReadCallback(buf, len);
	}

	ssize_t WriteCallback(char *buf, size_t buflen)
	{
		if (buflen == 0) {
			return 0;
		}

		std::unique_lock<std::mutex> lock(_mtx);
		for (;;) {
			const size_t fetched = TryFetchFromBuffer(buf, buflen);
			if (fetched) {
				_cond.notify_all();
				return fetched;
			}
			if (_done) {
				return 0;
			}
			_cond.wait(lock);
		}
	}

	int ReadCallback(const char *buf, size_t len)
	{
		std::unique_lock<std::mutex> lock(_mtx);
		for (;;) {
			if (_done) {
				return -1;
			}
			if (len == 0) {
				return 0;
			}
			if (TryAddToBuffer(buf, len)) {
				_cond.notify_all();
				return 0;
			}
			_cond.wait(lock);
		}
	}

protected:
	void *ThreadProc()
	{
		_ne_status = ne_request_dispatch(_req);
		std::unique_lock<std::mutex> lock(_mtx);
		if (_ne_status != NE_OK) {
			const char *ner = ne_get_error(_conn->sess);
			_ne_error = ner ? ner : "";
		}
		_done = true;
		_cond.notify_all();
		return nullptr;
	}

	void EnsureAllDone()
	{
		{
			std::unique_lock<std::mutex> lock(_mtx);
			_done = true;
			_cond.notify_all();
		}
		WaitThread();
	}

public:
	DavFileIO(std::shared_ptr<DavConnection> &conn, const std::string &path, bool writing, mode_t mode, unsigned long long resume_pos)
		: _conn(conn), _path(RefinePath(path, false)), _mode(mode), _resume_pos(resume_pos), _writing(writing)
	{
		if (_writing && _resume_pos) {
			throw ProtocolUnsupportedError("WebDav doesn't support upload resume");
		}

		_req = ne_request_create(_conn->sess, _writing ? "PUT" : "GET", _path.c_str());
		if (_req == nullptr) {
			throw ProtocolError("Can't create request");
		}

		if (_resume_pos) {
			char brange[64] = {};
			snprintf(brange, sizeof(brange) - 1, "bytes=%llu-", _resume_pos);
			ne_add_request_header(_req, "Range", brange);
			ne_add_request_header(_req, "Accept-Ranges", "bytes");
		}

		if (_writing) {
			ne_set_request_body_provider(_req, -1, sWriteCallback, this);
		} else {
			ne_add_response_body_reader(_req, ne_accept_always, sReadCallback, this);
		}

		if (!StartThread()) {
			throw std::runtime_error("Can't start thread");
		}
	}

	virtual ~DavFileIO()
	{
		EnsureAllDone();
		if (_req != nullptr) {
			ne_request_destroy(_req);
		}
	}

	virtual size_t Read(void *buf, size_t buflen)
	{
		if (buflen == 0) {
			return 0;
		}

		std::unique_lock<std::mutex> lock(_mtx);
		for (;;) {
			const size_t fetched = TryFetchFromBuffer(buf, buflen);
			if (fetched) {
				_cond.notify_all();
				return fetched;
			}
			if (_done) {
				if (_ne_status != NE_OK) {
					throw ProtocolError("Read error", _ne_error.c_str(), _ne_status);
				}
				return 0;
			}
			_cond.wait(lock);
		}
	}

	virtual void Write(const void *buf, size_t len)
	{
		if (len == 0) {
			return;
		}

		std::unique_lock<std::mutex> lock(_mtx);
		for (;;) {
			if (_done) {
				throw ProtocolError("Upload error", _ne_error.c_str(), _ne_status);
			}
			if (TryAddToBuffer(buf, len)) {
				_cond.notify_all();
				break;
			}
			_cond.wait(lock);
		}
		// for the sake of smooth progress update: wait while buffer will be mostly uploaded
		while (_buf.size() > len / 8) {
			_cond.wait(lock);
		}

	}

	virtual void WriteComplete()
	{
		EnsureAllDone();
		if (_ne_status != NE_OK) {
			throw ProtocolError("Finalize error", _ne_error.c_str(), _ne_status);
		}

		if ((_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) != 0) {
			ProtocolWebDAV_ChangeExecutable(_conn->sess, _path, true);
		}
	}
};

std::shared_ptr<IFileReader> ProtocolWebDAV::FileGet(const std::string &path, unsigned long long resume_pos)
{
	return std::make_shared<DavFileIO>(_conn, path, false, 0, resume_pos);
}

std::shared_ptr<IFileWriter> ProtocolWebDAV::FilePut(const std::string &path, mode_t mode, unsigned long long size_hint, unsigned long long resume_pos)
{
	return std::make_shared<DavFileIO>(_conn, path, true, mode, resume_pos);
}
