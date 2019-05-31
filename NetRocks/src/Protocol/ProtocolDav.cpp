#include <sys/stat.h>
#include <fcntl.h>
#include <vector>
#include <string.h>
#include <errno.h>
#include <StringConfig.h>
#include <utils.h>

#include "ProtocolDav.h"
#include <neon/ne_auth.h>
#include <neon/ne_basic.h>
#include <neon/ne_props.h>



std::shared_ptr<IProtocol> CreateProtocolDav(const std::string &host, unsigned int port,
	const std::string &username, const std::string &password, const std::string &options) throw (std::runtime_error)
{
	return std::make_shared<ProtocolDav>("http", host, port, username, password, options);
}

std::shared_ptr<IProtocol> CreateProtocolDavS(const std::string &host, unsigned int port,
	const std::string &username, const std::string &password, const std::string &options) throw (std::runtime_error)
{
	return std::make_shared<ProtocolDav>("https", host, port, username, password, options);
}

////////////////////////////

int ProtocolDav::sAuthCreds(void *userdata, const char *realm, int attempt, char *username, char *password)
{
	strncpy(username, ((ProtocolDav *)userdata)->_username.c_str(), NE_ABUFSIZ);
	strncpy(password, ((ProtocolDav *)userdata)->_password.c_str(), NE_ABUFSIZ);
	return attempt;
}

ProtocolDav::ProtocolDav(const char *scheme, const std::string &host, unsigned int port,
	const std::string &username, const std::string &password, const std::string &options) throw (std::runtime_error)
	:
	_conn(std::make_shared<DavConnection>()),
	_username(username), _password(password)
{
	_conn->sess = ne_session_create(scheme, host.c_str(), port);
	if (_conn->sess == nullptr) {
		throw ProtocolError("Create session error", errno);
	}
	if (!username.empty() || !password.empty()) {
		fprintf(stderr, "ProtocolDav: this=%p\n", this);
		ne_set_server_auth(_conn->sess, sAuthCreds, this);
	}
/*
	if (protocol_options.GetInt("UseProxy", 0) != 0) {
		ne_session_proxy(_conn->sess,
			protocol_options.GetString("ProxyHost").c_str(),
			(unsigned int)protocol_options.GetInt("ProxyPort"));
	}
*/
}

ProtocolDav::~ProtocolDav()
{
}


bool ProtocolDav::IsBroken()
{
	return false;//(!_conn || !_conn->ctx || !_conn->SMB || (ssh_get_status(_conn->ssh) & (SSH_CLOSED|SSH_CLOSED_ERROR)) != 0);
}

static std::string RefinePath(std::string path, bool ending_slash = false)
{
	if (path.empty() || path[0] != '/') {
		path.insert(0, "/");
	}
	while (path.size() > 1 && path[path.size() - 1] == '.' && path[path.size() - 2] == '/') {
		if (path.size() > 2) {
			path.resize(path.size() - 2);
		} else {
			path.resize(path.size() - 1);
		}
	}

	if (ending_slash && (path.empty() || path[path.size() - 1] != '/')) {
		path+= '/';
	}

	return path;
}


/////////////////////

static const ne_propname PROP_ISCOLLECTION = { "DAV:", "iscollection" };
static const ne_propname PROP_COLLECTION = { "DAV:", "collection" };
static const ne_propname PROP_RESOURCETYPE = { "DAV:", "resourcetype" };
static const ne_propname PROP_EXECUTABLE = { "http://apache.org/dav/props/", "executable"};

static const ne_propname PROP_GETCONTENTLENGTH = { "DAV:", "getcontentlength" };
static const ne_propname PROP_CREATIONDATE = { "DAV:", "creationdate" };
static const ne_propname PROP_GETLASTMODIFIED = { "DAV:", "getlastmodified" };


static const char *s_months = "janfebmaraprmayjunjulaugsepoctnovdec";

static void ParseWebDavDateTime(timespec &result, const std::string &str)
{
//#define RFC1123_FORMAT "%3s, %02d %3s %4d %02d:%02d:%02d GMT"
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
	const char *mon = strstr(s_months, parts[parts.size() - 4].c_str());
	tm_parts.tm_mon = mon ? (mon - s_months) / 3 : 0;
	tm_parts.tm_mday = atoi(parts[parts.size() - 5].c_str());

	result.tv_sec = mktime(&tm_parts);
}


struct PropsList : std::map<std::string, std::string>
{
	mode_t GetMode()
	{
		mode_t mode = 0;

		auto it = find(PROP_RESOURCETYPE.name);
		if (it != end() && it->second.find("<DAV:collection") != std::string::npos) {
			mode|= S_IFDIR;
		} else {
			it = find(PROP_ISCOLLECTION.name);
			if (it != end() && it->second == "1") {
				mode|= S_IFDIR;
			} else {
				mode|= S_IFREG;
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

		if (children) {
			erase(path);
		}

		if (rc != NE_OK) {
			throw ProtocolError("Query props", ne_get_error(sess), rc);
		}
	}

private:
	std::string _cur_uri_path;

	static void sOnResults(void *userdata, const ne_uri *uri, const ne_prop_result_set *results)
	{
		((WebDavProps *)userdata)->_cur_uri_path = (uri && uri->path) ? uri->path : "";
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

mode_t ProtocolDav::GetMode(const std::string &path, bool follow_symlink) throw (std::runtime_error)
{
	WebDavProps wdp(_conn->sess, path, false, PROPS_MODE nullptr);
	if (wdp.empty())
		return S_IFREG;

	return wdp.begin()->second.GetMode();
}


unsigned long long ProtocolDav::GetSize(const std::string &path, bool follow_symlink) throw (std::runtime_error)
{
	WebDavProps wdp(_conn->sess, path, false, PROPS_SIZE nullptr);
	if (wdp.empty())
		return 0;

	return wdp.begin()->second.GetSize();
}


void ProtocolDav::GetInformation(FileInformation &file_info, const std::string &path, bool follow_symlink) throw (std::runtime_error)
{
	WebDavProps wdp(_conn->sess, path, false, PROPS_MODE PROPS_SIZE PROPS_TIMES nullptr);
	file_info = FileInformation();
	if (!wdp.empty())
		wdp.begin()->second.GetFileInfo(file_info);
}

void ProtocolDav::FileDelete(const std::string &path) throw (std::runtime_error)
{
	int rc = ne_delete(_conn->sess, path.c_str());
	if (rc != NE_OK) {
		throw ProtocolError("Delete file error", rc);
	}
}

void ProtocolDav::DirectoryDelete(const std::string &path) throw (std::runtime_error)
{
	int rc = ne_delete(_conn->sess, path.c_str());
	if (rc != NE_OK) {
		throw ProtocolError("Delete directory error", rc);
	}
}

void ProtocolDav::DirectoryCreate(const std::string &path, mode_t mode) throw (std::runtime_error)
{
	if (path.empty() || path == "/") {
		throw ProtocolError("Cannot create root directory");
	}

	std::string xpath = path;
	if (xpath[xpath.size() - 1] != '/') {
		xpath+= '/';
	}
	int rc = ne_mkcol(_conn->sess, xpath.c_str());
	if (rc != NE_OK) {
		throw ProtocolError("Create directory error", rc);
	}
}

void ProtocolDav::Rename(const std::string &path_old, const std::string &path_new) throw (std::runtime_error)
{
	int rc = ne_move(_conn->sess, 1, path_old.c_str(), path_new.c_str());
	if (rc != NE_OK) {
		throw ProtocolError("Rename error", rc);
	}
}


void ProtocolDav::SetTimes(const std::string &path, const timespec &access_time, const timespec &modification_time) throw (std::runtime_error)
{
}

void ProtocolDav::SetMode(const std::string &path, mode_t mode) throw (std::runtime_error)
{
}

void ProtocolDav::SymlinkCreate(const std::string &link_path, const std::string &link_target) throw (std::runtime_error)
{
	throw ProtocolUnsupportedError("Symlink creation unsupported");
}

void ProtocolDav::SymlinkQuery(const std::string &link_path, std::string &link_target) throw (std::runtime_error)
{
	throw ProtocolUnsupportedError("Symlink querying unsupported");
}

class DavDirectoryEnumer : public IDirectoryEnumer
{
	WebDavProps _wdp;
public:
	DavDirectoryEnumer(std::shared_ptr<DavConnection> &conn, std::string path)
		: _wdp(conn->sess, RefinePath(path, true), true, PROPS_MODE PROPS_SIZE PROPS_TIMES nullptr)
	{
/*		for (const auto &path_i : _wdp) {
			fprintf(stderr, "Path: %s\n", path_i.first.c_str());
			for (const auto &prop : path_i.second) {
				fprintf(stderr, "    '%s'='%s'\n", prop.first.c_str(), prop.second.c_str());
			}
		}*/
	}

	virtual ~DavDirectoryEnumer()
	{
	}

	virtual bool Enum(std::string &name, std::string &owner, std::string &group, FileInformation &file_info) throw (std::runtime_error)
	{
		if (_wdp.empty()) {
			return false;
		}

		owner.clear();
		group.clear();
		file_info = FileInformation();

		auto i = _wdp.begin();

		i->second.GetFileInfo(file_info);

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

std::shared_ptr<IDirectoryEnumer> ProtocolDav::DirectoryEnum(const std::string &path) throw (std::runtime_error)
{
	return std::make_shared<DavDirectoryEnumer>(_conn, path);
}

#if 0
class DavFileIO : public IFileReader, public IFileWriter
{
	std::shared_ptr<DavConnection> _nfs;
	struct nfsfh *_file = nullptr;

public:
	DavFileIO(std::shared_ptr<DavConnection> &nfs, const std::string &path, int flags, mode_t mode, unsigned long long resume_pos)
		: _nfs(nfs)
	{
		fprintf(stderr, "TRying to open path: '%s'\n", path.c_str());
		int rc = (flags & O_CREAT)
			? nfs_creat(_nfs->ctx, path.c_str(), mode & (~O_CREAT), &_file)
			: nfs_open(_nfs->ctx, path.c_str(), flags, &_file);
		
		if (rc != 0 || _file == nullptr) {
			_file = nullptr;
			throw ProtocolError("Failed to open file",  rc);
		}
		if (resume_pos) {
			uint64_t current_offset = resume_pos;
			int rc = nfs_lseek(_nfs->ctx, _file, resume_pos, SEEK_SET, &current_offset);
			if (rc < 0) {
				nfs_close(_nfs->ctx, _file);
				_file = nullptr;
				throw ProtocolError("Failed to seek file",  rc);
			}
		}
	}

	virtual ~DavFileIO()
	{
		if (_file != nullptr) {
			nfs_close(_nfs->ctx, _file);
		}
	}

	virtual size_t Read(void *buf, size_t len) throw (std::runtime_error)
	{
		const auto rc = nfs_read(_nfs->ctx, _file, len, (char *)buf);
		if (rc < 0)
			throw ProtocolError("Read file error",  errno);
		// uncomment to simulate connection stuck if ( (rand()%100) == 0) sleep(60);

		return (size_t)rc;
	}

	virtual void Write(const void *buf, size_t len) throw (std::runtime_error)
	{
		if (len > 0) for (;;) {
			const auto rc = nfs_write(_nfs->ctx, _file, len, (char *)buf);
			if (rc <= 0)
				throw ProtocolError("Write file error",  errno);
			if ((size_t)rc >= len)
				break;

			len-= (size_t)rc;
			buf = (const char *)buf + rc;
		}
	}

	virtual void WriteComplete() throw (std::runtime_error)
	{
		// what?
	}
};
#endif

std::shared_ptr<IFileReader> ProtocolDav::FileGet(const std::string &path, unsigned long long resume_pos) throw (std::runtime_error)
{
	throw ProtocolError("FilePut not implemented");
//	return std::make_shared<DavFileIO>(_nfs, MountedRootedPath(path), O_RDONLY, 0, resume_pos);
}

std::shared_ptr<IFileWriter> ProtocolDav::FilePut(const std::string &path, mode_t mode, unsigned long long resume_pos) throw (std::runtime_error)
{
	throw ProtocolError("FilePut not implemented");
//	return std::make_shared<DavFileIO>(_nfs, MountedRootedPath(path), O_WRONLY | O_CREAT | (resume_pos ? 0 : O_TRUNC), mode, resume_pos);
}
