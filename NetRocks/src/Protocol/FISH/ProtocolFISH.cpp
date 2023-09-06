#include <sys/stat.h>
#include <fcntl.h>
#include <vector>
#include <string.h>
#include <errno.h>
#include "ProtocolFISH.h"
#include <StringConfig.h>
#include <utils.h>

#include <fstream>
#include <cstdlib> // для getenv

std::shared_ptr<IProtocol> CreateProtocol(const std::string &protocol, const std::string &host, unsigned int port,
	const std::string &username, const std::string &password, const std::string &options)
{
	fprintf(stderr, "*1\n");
	return std::make_shared<ProtocolFISH>(host, port, username, password, options);
}

////////////////////////////

ProtocolFISH::ProtocolFISH(const std::string &host, unsigned int port,
	const std::string &username, const std::string &password, const std::string &options)
	:
	_fish(std::make_shared<FISHConnection>()),
	_host(host)
{
	fprintf(stderr, "*2\n");

	StringConfig protocol_options(options);
	// ---------------------------------------------

    fprintf(stderr, "*** CONNECTING\n");

    _fish->sc = new FISHClient();

	fprintf(stderr, "*** host from config: %s\n", host.c_str());
	fprintf(stderr, "*** port from config: %i\n", port);
	fprintf(stderr, "*** username from config: %s\n", username.c_str());
	fprintf(stderr, "*** password from config: %s\n", password.c_str());

	fprintf(stderr, "*** login string from config: %s\n", (username + "@" + host).c_str());

	if (!_fish->sc->OpenApp("ssh", (username + "@" + host).c_str())) {
	    printf("err 1\n");
    }

    // везде ли "$ " признак успешного залогина? проверить на dd wrt

    // Мы таки залогинены, спрашивают пароль, ключ незнакомый?
    std::vector<std::string> results = {"$ ", "password: ", "This key is not known by any other names"};

    /*
    // это ещё от sftp враппер код
    // todo: сделать обработку сообщения о неправильном пароле (просто выходить из проги пока)
    if (sc->WaitFor("Connected to", {
		"assword:",
    	"Permission denied, please try again."
    */

    _fish->wr = _fish->sc->WaitFor({}, results);
    while (_fish->wr.index) {
	    if (_fish->wr.index == 1) {
		    _fish->wr = _fish->sc->WaitFor({(password + "\n").c_str()}, results);
	    }
	    if (_fish->wr.index == 2) {
		    _fish->wr = _fish->sc->WaitFor({"yes\n"}, results);
	    }
		
	}

    fprintf(stderr, "*** CONNECTED\n");

	// если мы сюда добрались, значит, уже залогинены
	// fixme: таймайт? ***

}

ProtocolFISH::~ProtocolFISH()
{
	fprintf(stderr, "*3\n");
}

static int RootedPathLevel(const std::string &rooted_path)
{
	fprintf(stderr, "*4\n");
	// 0 on "fish://"
	// 1 on "fish://server" or "fish://server/"
	// 2 on "fish://server/export" or "fish://server/export/"
	//etc
	if (rooted_path.size() < 6)
		return -1;
	if (rooted_path.size() == 6)
		return 0;

	int out = 1;
	for (size_t i = 6; i < rooted_path.size(); ++i) {
		if (rooted_path[i] == '/' && i + 1 < rooted_path.size() && rooted_path[i + 1] != '/') {
			++out;
		}
	}

	return out;
}

std::string ProtocolFISH::RootedPath(const std::string &path)
{
	fprintf(stderr, "*5\n");

	std::string out = "fish://";
	out+= _host;
	if (!_host.empty() && !_mount.empty()) {
		out+= '/';
		out+= _mount;
	}
	if (!path.empty() && path != "." && path[0] != '/' && (!_host.empty() || !_mount.empty())) {
		out+= '/';
	}
	if (path != "." && path != "/.") {
		out+= path;
//		out+= "?auto-traverse-mounts=1";
	}

	for (size_t i = 7; i < out.size();) {
		if (out[i] == '.' && out[i - 1] == '/' && (i + 1 == out.size() || out[i + 1] == '/')) {
			out.erase(i - 1, 2);
			--i;
		} else {
			++i;
		}
	}

	return out;
}

void ProtocolFISH::RootedPathToMounted(std::string &path)
{
	fprintf(stderr, "*6\n");

	size_t p = path.find('/', 6);
	if (p == std::string::npos)
		throw ProtocolError("Cannot mount path without export", path.c_str());

	if (_fish->mounted_path.empty() || path.size() < _fish->mounted_path.size()
	 || memcmp(path.c_str(), _fish->mounted_path.c_str(), _fish->mounted_path.size()) != 0
	 || (path.size() > _fish->mounted_path.size() && path[_fish->mounted_path.size()] != '/')) {
		std::string server = path.substr(6, p - 6);
		std::string exported = path.substr(p);
		//int rc = 0; // ***
		//int rc = fish_mount(_fish->ctx, server.c_str(), exported.c_str());
		//if (rc != 0)
		//	throw ProtocolError("Mount error", rc);

		_fish->mounted_path = path;
	}

	path.erase(0, _fish->mounted_path.size());
	if (path.empty())
		path = "/";
}

std::string ProtocolFISH::MountedRootedPath(const std::string &path)
{
	fprintf(stderr, "*7\n");

	std::string out = RootedPath(path);
	if (RootedPathLevel(out) <= 1) {
		throw ProtocolError("Path is not mountable");
	}
	RootedPathToMounted(out);
	return out;
}

mode_t ProtocolFISH::GetMode(const std::string &path, bool follow_symlink)
{
	fprintf(stderr, "*8\n");

	std::string xpath = RootedPath(path);
	if (RootedPathLevel(xpath) <= 1) {
		return S_IFDIR | 0755;
	}
	fprintf(stderr, "ProtocolFISH::GetMode rooted_path='%s'\n", xpath.c_str());
	RootedPathToMounted(xpath);

#ifdef LIBFISH_FEATURE_READAHEAD
	/*
	struct fish_stat_64 s = {};
	int rc = follow_symlink
		? fish_stat64(_fish->ctx, xpath.c_str(), &s)
		: fish_lstat64(_fish->ctx, xpath.c_str(), &s);
	auto out = s.fish_mode;
	*/
	int rc = 0; // ***
#else
	struct stat s = {};
	//int rc = fish_stat(_fish->ctx, xpath.c_str(), &s);
	int rc = 0; // ***
	auto out = s.st_mode;
#endif
	fprintf(stderr, "GetMode result %d\n", rc);
	if (rc != 0)
		throw ProtocolError("Get mode error", rc);
	return out;
}

unsigned long long ProtocolFISH::GetSize(const std::string &path, bool follow_symlink)
{
	fprintf(stderr, "*9\n");

	std::string xpath = RootedPath(path);
	if (RootedPathLevel(xpath) <= 1) {
		return 0;
	}
	fprintf(stderr, "ProtocolFISH::GetSize rooted_path='%s'\n", xpath.c_str());
	RootedPathToMounted(xpath);

#ifdef LIBFISH_FEATURE_READAHEAD
	/*
	struct fish_stat_64 s = {};
	int rc = follow_symlink
		? fish_stat64(_fish->ctx, xpath.c_str(), &s)
		: fish_lstat64(_fish->ctx, xpath.c_str(), &s);
	auto out = s.fish_size;
	*/
	int rc = 0; // ***
#else
	struct stat s = {};
	//int rc = fish_stat(_fish->ctx, xpath.c_str(), &s);
	int rc = 0; // ***
	auto out = s.st_size;
#endif
	if (rc != 0)
		throw ProtocolError("Get size error", rc);

	return out;
}

void ProtocolFISH::GetInformation(FileInformation &file_info, const std::string &path, bool follow_symlink)
{
	fprintf(stderr, "*10\n");

	std::string xpath = RootedPath(path);
	if (RootedPathLevel(xpath) <= 1) {
		file_info = FileInformation();
		file_info.mode = S_IFDIR | 0755;
		return;
	}
	fprintf(stderr, "ProtocolFISH::GetInformation rooted_path='%s'\n", xpath.c_str());
	RootedPathToMounted(xpath);

#ifdef LIBFISH_FEATURE_READAHEAD
	/*
	struct fish_stat_64 s = {};
	int rc = follow_symlink
		? fish_stat64(_fish->ctx, xpath.c_str(), &s)
		: fish_lstat64(_fish->ctx, xpath.c_str(), &s);
	if (rc != 0)
		throw ProtocolError("Get info error", rc);

	file_info.access_time.tv_sec = s.fish_atime;
	file_info.access_time.tv_nsec = s.fish_atime_nsec;
	file_info.modification_time.tv_sec = s.fish_mtime;
	file_info.modification_time.tv_nsec = s.fish_mtime_nsec;
	file_info.status_change_time.tv_sec = s.fish_ctime;
	file_info.status_change_time.tv_nsec = s.fish_ctime_nsec;
	file_info.mode = s.fish_mode;
	file_info.size = s.fish_size;
	*/
	int rc = 0; // ***
#else
	struct stat s = {};
	//int rc = fish_stat(_fish->ctx, xpath.c_str(), &s);
	//if (rc != 0)
	//	throw ProtocolError("Get info error", rc);
	//int rc = 0; // ***

	file_info.access_time = s.st_atim;
	file_info.modification_time = s.st_mtim;
	file_info.status_change_time = s.st_ctim;
	file_info.size = s.st_size;
	file_info.mode = s.st_mode;
#endif
}

void ProtocolFISH::FileDelete(const std::string &path)
{
	fprintf(stderr, "*11\n");
	//int rc = fish_unlink(_fish->ctx, MountedRootedPath(path).c_str());
	//if (rc != 0)
	//	throw ProtocolError("Delete file error", rc);
}

void ProtocolFISH::DirectoryDelete(const std::string &path)
{
	fprintf(stderr, "*12\n");
	//int rc = fish_rmdir(_fish->ctx, MountedRootedPath(path).c_str());
	//if (rc != 0)
	//	throw ProtocolError("Delete directory error", rc);
}

void ProtocolFISH::DirectoryCreate(const std::string &path, mode_t mode)
{
	fprintf(stderr, "*13\n");
	//int rc = fish_mkdir(_fish->ctx, MountedRootedPath(path).c_str());//, mode);
	//if (rc != 0)
	//	throw ProtocolError("Create directory error", rc);
}

void ProtocolFISH::Rename(const std::string &path_old, const std::string &path_new)
{
	fprintf(stderr, "*14\n");
	//int rc = fish_rename(_fish->ctx, MountedRootedPath(path_old).c_str(), MountedRootedPath(path_new).c_str());
	//if (rc != 0)
	//	throw ProtocolError("Rename error", rc);
}


void ProtocolFISH::SetTimes(const std::string &path, const timespec &access_time, const timespec &modification_time)
{
	fprintf(stderr, "*15\n");
	/*
	struct timeval times[2] = {};
	times[0].tv_sec = access_time.tv_sec;
	times[0].tv_usec = suseconds_t(access_time.tv_nsec / 1000);
	times[1].tv_sec = modification_time.tv_sec;
	times[1].tv_usec = suseconds_t(modification_time.tv_nsec / 1000);

	int rc = fish_utimes(_fish->ctx, MountedRootedPath(path).c_str(), times);
	if (rc != 0)
		throw ProtocolError("Set times error", rc);
	*/
}

void ProtocolFISH::SetMode(const std::string &path, mode_t mode)
{
	fprintf(stderr, "*16\n");
	/*
	int rc = fish_chmod(_fish->ctx, MountedRootedPath(path).c_str(), mode);
	if (rc != 0)
		throw ProtocolError("Set mode error", rc);
	*/
}

void ProtocolFISH::SymlinkCreate(const std::string &link_path, const std::string &link_target)
{
	fprintf(stderr, "*17\n");
	/*
	int rc = fish_symlink(_fish->ctx, MountedRootedPath(link_target).c_str(), MountedRootedPath(link_path).c_str());
	if (rc != 0)
		throw ProtocolError("Symlink create error", rc);
	*/
}

void ProtocolFISH::SymlinkQuery(const std::string &link_path, std::string &link_target)
{
	fprintf(stderr, "*18\n");
	/*
	char buf[0x1001] = {};
	int rc = fish_readlink(_fish->ctx, MountedRootedPath(link_path).c_str(), buf, sizeof(buf) - 1);
	if (rc != 0)
		throw ProtocolError("Symlink query error", rc);

	link_target = buf;
	*/
}

class FISHDirectoryEnumer : public IDirectoryEnumer {
private:
    std::vector<FileInfo> files;
    size_t currentIndex = 0;

	std::shared_ptr<FISHConnection> _fish;
	struct fishdir *_dir = nullptr;

public:
	FISHDirectoryEnumer(std::shared_ptr<FISHConnection> &fish, const std::string &path)
		: _fish(fish)
	{
		fprintf(stderr, "*19\n");

	    std::vector<std::string> lines;
		std::string homeDir = getenv("HOME") ? getenv("HOME") : "";
	    std::ifstream file(homeDir + "/.config/far2l/plugins/NetRocks/fish/helpers/ls");

	    if (file.is_open()) {
	        std::string line;
	        while (std::getline(file, line)) {
	            lines.push_back(line + "\n");
	        }
	        file.close();
	    } else {
	        std::cout << "Не удалось открыть файл" << std::endl;
			return; // fixme: error processing
	        //return 1;
	    }

	    lines.push_back("\n");

	    _fish->wr = _fish->sc->WaitFor(lines, {"\n### "}); // fish command end

		files = _fish->sc->ParseLs(_fish->wr.stdout_data); // path не передаётся тут никак в ls, fixme ***

        fprintf(stderr, "*** LIST READ\n");

        //files = fetchDirectoryContents(path);
    }

    virtual bool Enum(std::string &name, std::string &owner, std::string &group, FileInformation &file_info) override {

        fprintf(stderr, "*** ENUM; %li, %li\n", currentIndex, files.size());

        if (currentIndex && (currentIndex >= files.size())) {

	        fprintf(stderr, "*** ENUM MID 10; %li, %li\n", currentIndex, files.size());
        	currentIndex = 0;

            return false;
        }

        fprintf(stderr, "*** ENUM MID 20\n");

        const auto& fileInfo = files[currentIndex++];
        
        name = fileInfo.path;

        fprintf(stderr, "*** %s\n", name.c_str());

        owner = fileInfo.owner;
        group = fileInfo.group;

        file_info.access_time = {}; // Заполните это поле, если у вас есть информация
        // fixme: это не собирается
        //file_info.modification_time = std::chrono::time_point_cast<std::chrono::nanoseconds>(fileInfo.modified_time);
        file_info.status_change_time = {}; // Заполните это поле, если у вас есть информация
        file_info.size = fileInfo.size;
        file_info.mode = fileInfo.permissions;

        if (fileInfo.is_directory) {
	        fprintf(stderr, "*** ENUM MID 30\n");
            file_info.mode |= S_IFDIR;
        }

        fprintf(stderr, "*** ENUM END\n");

        return true;
    }
};

struct FISHContainersEnumer : IDirectoryEnumer
{
protected:
	std::set<std::string> _names;

	virtual bool Enum(std::string &name, std::string &owner, std::string &group, FileInformation &file_info)
	{
        fprintf(stderr, "*** TEST 1\n");

		if (_names.empty()) {
			return false;
		}

		name = *_names.begin();
		owner.clear();
		group.clear();
		file_info = FileInformation();
		file_info.mode|= S_IFDIR | DEFAULT_ACCESS_MODE_DIRECTORY;
		_names.erase(_names.begin());
		return true;
	}
};

struct FISHServersEnumer : FISHContainersEnumer
{
	FISHServersEnumer(std::shared_ptr<FISHConnection> &fish)
	{
        fprintf(stderr, "*** TEST 2\n");
		/*
		if (fish->srv2exports.empty()) {
			struct fish_server_list *servers = fish_find_local_servers();
			fprintf(stderr, "FISHServersEnumer: %p\n", servers);
			if (servers != nullptr) {
				std::set<std::string> empty;
				struct fish_server_list *server = servers;
				do {
					fish->srv2exports.emplace(server->addr, empty);
					server = server->next;
				} while (server != nullptr);
				free_fish_srvr_list(servers);
			}
		}

		for (const auto &i : fish->srv2exports) {
			_names.emplace(i.first);
		}
		*/
	}

};

struct FISHExportsEnumer : FISHContainersEnumer
{

	FISHExportsEnumer(std::shared_ptr<FISHConnection> &fish, const std::string &rooted_path)
	{
		fprintf(stderr, "*** TEST 3\n");

		if (rooted_path.size() <= 6) {
			fprintf(stderr, "FISHExportsEnumer('%s'): path too short\n", rooted_path.c_str());
			return;
		}

		std::string server = rooted_path.substr(6);

		while (!server.empty() && server[server.size() - 1] == '/') {
			server.resize(server.size() - 1);
		}
		if (server.empty()) {
			fprintf(stderr, "FISHExportsEnumer('%s'): no server in path\n", rooted_path.c_str());
			return;
		}

		auto &cached_exports = fish->srv2exports[server];
		if (cached_exports.empty()) {
			//struct exportnode *en_list = mount_getexports(server.c_str());
			struct exportnode *en_list; // ***
			fprintf(stderr, "FISHExportsEnumer('%s'): %p\n", server.c_str(), en_list);
			if (en_list != nullptr) {
				struct exportnode *en = en_list;
				do {
					//cached_exports.emplace((*en->ex_dir == '/') ? en->ex_dir + 1 : en->ex_dir);
					//en = en->ex_next;
				} while (en != nullptr);
				//mount_free_export_list(en_list);
			}
		}

		_names = cached_exports;
	}
};

std::shared_ptr<IDirectoryEnumer> ProtocolFISH::DirectoryEnum(const std::string &path)
{
    fprintf(stderr, "*** TEST 4\n");
//	std::string xpath = RootedPath(path);
//	fprintf(stderr, "ProtocolFISH::DirectoryEnum: rooted_path='%s'\n", xpath.c_str());
//	switch (RootedPathLevel(xpath)) {
//		case 0: return std::shared_ptr<IDirectoryEnumer>(new FISHServersEnumer(_fish));
//		case 1: return std::shared_ptr<IDirectoryEnumer>(new FISHExportsEnumer(_fish, xpath));
//		default:
//			RootedPathToMounted(xpath);
//			return std::shared_ptr<IDirectoryEnumer>(new FISHDirectoryEnumer(_fish, xpath));
//	}
// ***
fprintf(stderr, "ProtocolFISH::DirectoryEnum: path='%s'\n", path.c_str());
return std::shared_ptr<IDirectoryEnumer>(new FISHDirectoryEnumer(_fish, path));
}


class FISHFileIO : public IFileReader, public IFileWriter
{
	std::shared_ptr<FISHConnection> _fish;
	struct fishfh *_file = nullptr;

public:
	FISHFileIO(std::shared_ptr<FISHConnection> &fish, const std::string &path, int flags, mode_t mode, unsigned long long resume_pos)
		: _fish(fish)
	{
		fprintf(stderr, "TRying to open path: '%s'\n", path.c_str());

		//int rc = (flags & O_CREAT)
			//? fish_creat(_fish->ctx, path.c_str(), mode & (~O_CREAT), &_file)
			//: fish_open(_fish->ctx, path.c_str(), flags, &_file);
		
		int rc = 0; // ***

		if (rc != 0 || _file == nullptr) {
			_file = nullptr;
			throw ProtocolError("Failed to open file", rc);
		}
		if (resume_pos) {
			//uint64_t current_offset = resume_pos;
			//int rc = fish_lseek(_fish->ctx, _file, resume_pos, SEEK_SET, &current_offset);
			int rc = 0; // ***
			if (rc < 0) {
				//fish_close(_fish->ctx, _file);
				_file = nullptr;
				throw ProtocolError("Failed to seek file", rc);
			}
		}
	}

	virtual ~FISHFileIO()
	{
		fprintf(stderr, "*20\n");
		if (_file != nullptr) {
			//fish_close(_fish->ctx, _file);
		}
	}

	virtual size_t Read(void *buf, size_t len)
	{
		fprintf(stderr, "*21\n");

		//const auto rc = fish_read(_fish->ctx, _file, len, (char *)buf);
		int rc = 0; // ***
		if (rc < 0)
			throw ProtocolError("Read file error", errno);
		// uncomment to simulate connection stuck if ( (rand()%100) == 0) sleep(60);

		return (size_t)rc;
	}

	virtual void Write(const void *buf, size_t len)
	{
		fprintf(stderr, "*22\n");

		if (len > 0) for (;;) {
			int rc = 0; // ***
			//const auto rc = fish_write(_fish->ctx, _file, len, (char *)buf);
			if (rc <= 0)
				throw ProtocolError("Write file error", errno);
			if ((size_t)rc >= len)
				break;

			len-= (size_t)rc;
			buf = (const char *)buf + rc;
		}
	}

	virtual void WriteComplete()
	{
		fprintf(stderr, "*23\n");
		// what?
	}
};


std::shared_ptr<IFileReader> ProtocolFISH::FileGet(const std::string &path, unsigned long long resume_pos)
{
	fprintf(stderr, "*24\n");
	return std::make_shared<FISHFileIO>(_fish, MountedRootedPath(path), O_RDONLY, 0, resume_pos);
}

std::shared_ptr<IFileWriter> ProtocolFISH::FilePut(const std::string &path, mode_t mode, unsigned long long size_hint, unsigned long long resume_pos)
{
	fprintf(stderr, "*25\n");
	return std::make_shared<FISHFileIO>(_fish, MountedRootedPath(path), O_WRONLY | O_CREAT | (resume_pos ? 0 : O_TRUNC), mode, resume_pos);
}
