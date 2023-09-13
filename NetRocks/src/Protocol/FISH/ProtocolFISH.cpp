#include <sys/stat.h>
#include <fcntl.h>
#include <vector>
#include <string.h>
#include <errno.h>
#include "ProtocolFISH.h"
#include "FISHParse.h"
#include <StringConfig.h>
#include <utils.h>

#include <fstream>
#include <cstdlib> // для getenv

#define FISH_HAVE_HEAD         1
#define FISH_HAVE_SED          2
#define FISH_HAVE_AWK          4
#define FISH_HAVE_PERL         8
#define FISH_HAVE_LSQ         16
#define FISH_HAVE_DATE_MDYT   32
#define FISH_HAVE_TAIL        64

std::shared_ptr<IProtocol> CreateProtocol(const std::string &protocol, const std::string &host, unsigned int port,
	const std::string &username, const std::string &password, const std::string &options)
{
	fprintf(stderr, "*1\n");
	return std::make_shared<ProtocolFISH>(host, port, username, password, options);
}

static uint64_t GetIntegerBeforeStatusReplyLine(const std::string &data, size_t pos)
{
	size_t prev_line_start = pos;
	if (prev_line_start) do {
		--prev_line_start;
	} while (prev_line_start && data[prev_line_start] != '\n');
	if (data[prev_line_start] == '\n') {
		++prev_line_start;
	}
	return (uint64_t)strtoull(data.c_str() + prev_line_start, nullptr, 10);
}

////////////////////////////

ProtocolFISH::ProtocolFISH(const std::string &host, unsigned int port,
	const std::string &username, const std::string &password, const std::string &options)
	:
	_fish(std::make_shared<FISHClient>())
//	_dir_enum_cache(10)
{
	StringConfig protocol_options(options);
	// ---------------------------------------------

	if (g_netrocks_verbosity > 0) {
		fprintf(stderr, "*** host from config: %s\n", host.c_str());
		fprintf(stderr, "*** port from config: %i\n", port);
		fprintf(stderr, "*** username from config: %s\n", username.c_str());
		if (g_netrocks_verbosity > 1) {
			fprintf(stderr, "*** password from config: %s\n", password.c_str());
		}
	}

	std::string ssh_arg = host;
	if (!username.empty()) {
		ssh_arg.insert(0, 1, '@');
		ssh_arg.insert(0, username);
	}

    fprintf(stderr, "*** FISH CONNECT: %s\n", ssh_arg.c_str());

	if (!_fish->OpenApp("ssh", ssh_arg.c_str())) {
	    printf("err 1\n");
    }

    // везде ли "$ " признак успешного залогина? проверить на dd wrt

    // Мы таки залогинены, спрашивают пароль, ключ незнакомый?
    std::vector<const char *> login_replies {
		"$ ",
		"# ",
		"password: ",
		"This key is not known by any other names",
		"Are you sure",
		"Permission denied"
	};

    auto wr = _fish->SendAndWaitReply("", login_replies);
    while (wr.index != 0 && wr.index != 1) {
	    if (wr.index == 2) {
		    wr = _fish->SendAndWaitReply(password + "\n", login_replies);
	    }
	    if (wr.index == 3 || wr.index == 4) {
		    wr = _fish->SendAndWaitReply("yes\n", login_replies);
	    }
	    if (wr.index == 5) {
		    throw ProtocolAuthFailedError();
	    }
	}
    fprintf(stderr, "*** FISH CONNECTED\n");

	wr = _fish->SendAndWaitReply(
		"export PS1=;export PS2=;export PS3=;export PS4=;export PROMPT_COMMAND=;echo '###':$0:FISH:'###'\n",
		{":FISH:###\n"}
	);
	size_t p = wr.stdout_data.rfind("###:", wr.pos);
	if (p != std::string::npos) {
		_shell = wr.stdout_data.substr(p + 4, wr.pos - p - 4);
	}
	fprintf(stderr, "*** FISH SHELL: '%s'\n", _shell.c_str());

	if (_shell.find("bash") != std::string::npos) {
		// prevent bash from flooding with bracketed paste ESC-sequences
		_fish->SendAndWaitReply(
			"bind 'set enable-bracketed-paste off'; echo '###':FISH:'###'\n",
			{"###:FISH:###\n"}
		);
	}

	wr = _fish->SendHelperAndWaitReply("FISH/info", {"\n### 200", "\n### "});
	if (wr.index == 0 && wr.pos > 2) {
		_info = (unsigned int)GetIntegerBeforeStatusReplyLine(wr.stdout_data, wr.pos);
	}
	fprintf(stderr, "*** FISH INFO: %u\n", _info);
	SetDefaultSubstitutions();

	// если мы сюда добрались, значит, уже залогинены
	// fixme: таймайт? ***
}

ProtocolFISH::~ProtocolFISH()
{
	fprintf(stderr, "*** ProtocolFISH::%s\n", __FUNCTION__);
}

void ProtocolFISH::SetDefaultSubstitutions()
{
	_fish->SetSubstitution("${FISH_HAVE_HEAD}", (_info & 1) ? "1" : "");
	_fish->SetSubstitution("${FISH_HAVE_SED}", (_info & 2) ? "1" : "");
	_fish->SetSubstitution("${FISH_HAVE_AWK}", (_info & 4) ? "1" : "");
	_fish->SetSubstitution("${FISH_HAVE_PERL}", (_info & 8) ? "1" : "");
	_fish->SetSubstitution("${FISH_HAVE_LSQ}", (_info & 16) ? "1" : "");
	_fish->SetSubstitution("${FISH_HAVE_DATE_MDYT}", (_info & 32) ? "1" : "");
	_fish->SetSubstitution("${FISH_HAVE_TAIL}", (_info & 64) ? "1" : "");

	_fish->SetSubstitution("${FISH_LS_ARGS}", "H"); // follow_symlink by default
}

mode_t ProtocolFISH::GetMode(const std::string &path, bool follow_symlink)
{
	fprintf(stderr, "*** ProtocolFISH::%s\n", __FUNCTION__);
	FileInformation file_info;
	GetInformation(file_info, path, follow_symlink);
	return file_info.mode;
}

unsigned long long ProtocolFISH::GetSize(const std::string &path, bool follow_symlink)
{
	fprintf(stderr, "*** ProtocolFISH::%s\n", __FUNCTION__);
	FileInformation file_info;
	GetInformation(file_info, path, follow_symlink);
	return file_info.size;
}

void ProtocolFISH::GetInformation(FileInformation &file_info, const std::string &path, bool follow_symlink)
{
	fprintf(stderr, "*** ProtocolFISH::%s\n", __FUNCTION__);
	try {
		_fish->SetSubstitution("${FISH_HAVE_PERL}", ""); // perl always follows symlink and doesnt know about 'd'
		_fish->SetSubstitution("${FISH_LS_ARGS}", follow_symlink ? "Hd" : "d");
		auto enumer = DirectoryEnum(path);
		std::string name, owner, group;
		enumer->Enum(name, owner, group, file_info);

	} catch (...) {
		SetDefaultSubstitutions();
		throw;
	}
	SetDefaultSubstitutions();
}

void ProtocolFISH::FileDelete(const std::string &path)
{
	fprintf(stderr, "*** ProtocolFISH::%s\n", __FUNCTION__);
	//int rc = fish_unlink(_fish->ctx, MountedRootedPath(path).c_str());
	//if (rc != 0)
	//	throw ProtocolError("Delete file error", rc);
}

void ProtocolFISH::DirectoryDelete(const std::string &path)
{
	fprintf(stderr, "*** ProtocolFISH::%s\n", __FUNCTION__);
	//int rc = fish_rmdir(_fish->ctx, MountedRootedPath(path).c_str());
	//if (rc != 0)
	//	throw ProtocolError("Delete directory error", rc);
}

void ProtocolFISH::DirectoryCreate(const std::string &path, mode_t mode)
{
	fprintf(stderr, "*** ProtocolFISH::%s\n", __FUNCTION__);
	//int rc = fish_mkdir(_fish->ctx, MountedRootedPath(path).c_str());//, mode);
	//if (rc != 0)
	//	throw ProtocolError("Create directory error", rc);
}

void ProtocolFISH::Rename(const std::string &path_old, const std::string &path_new)
{
	fprintf(stderr, "*** ProtocolFISH::%s\n", __FUNCTION__);
	//int rc = fish_rename(_fish->ctx, MountedRootedPath(path_old).c_str(), MountedRootedPath(path_new).c_str());
	//if (rc != 0)
	//	throw ProtocolError("Rename error", rc);
}


void ProtocolFISH::SetTimes(const std::string &path, const timespec &access_time, const timespec &modification_time)
{
	fprintf(stderr, "*** ProtocolFISH::%s\n", __FUNCTION__);
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
	fprintf(stderr, "*** ProtocolFISH::%s\n", __FUNCTION__);
	/*
	int rc = fish_chmod(_fish->ctx, MountedRootedPath(path).c_str(), mode);
	if (rc != 0)
		throw ProtocolError("Set mode error", rc);
	*/
}

void ProtocolFISH::SymlinkCreate(const std::string &link_path, const std::string &link_target)
{
	fprintf(stderr, "*** ProtocolFISH::%s\n", __FUNCTION__);
	/*
	int rc = fish_symlink(_fish->ctx, MountedRootedPath(link_target).c_str(), MountedRootedPath(link_path).c_str());
	if (rc != 0)
		throw ProtocolError("Symlink create error", rc);
	*/
}

void ProtocolFISH::SymlinkQuery(const std::string &link_path, std::string &link_target)
{
	fprintf(stderr, "*** ProtocolFISH::%s\n", __FUNCTION__);
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
    std::vector<FileInfo> _files;
    size_t _index = 0;
	bool _unquote{false};

	std::shared_ptr<FISHClient> _fish;

public:
	FISHDirectoryEnumer(std::shared_ptr<FISHClient> &fish, const std::string &path, unsigned int info)
		: _fish(fish)
	{
		_unquote = (info & (FISH_HAVE_SED | FISH_HAVE_PERL | FISH_HAVE_LSQ)) != 0;
		_fish->SetSubstitution("${FISH_FILENAME}", path);
	    const auto &wr = _fish->SendHelperAndWaitReply("FISH/ls", {"### 200\n", "### 500\n"}); // fish command end
		if (wr.index == 0) {
			FISHParseLS(_files, wr.stdout_data);
		} else {
			throw ProtocolError("dir query error");
		}

        fprintf(stderr, "*** LIST READ\n");

        //files = fetchDirectoryContents(path);
    }

    virtual bool Enum(std::string &name, std::string &owner, std::string &group, FileInformation &file_info) override
	{
		const FileInfo *file;
		do {
	        if (_index >= _files.size()) {
				return false;
	        }
			file = &_files[_index++];
			name = file->path;
			if (S_ISLNK(file->mode)) {
				size_t p = name.rfind(" -> ");
				if (p != std::string::npos) {
					name.resize(p);
				}
			}
			if (_unquote && name.size() > 2 && name.front() == '\"' && name.back() == '\"') {
				name.pop_back();
				name.erase(0, 1);
			}

		} while (!FILENAME_ENUMERABLE(name));

        owner = file->owner;
        group = file->group;

        file_info.access_time = {}; // Заполните это поле, если у вас есть информация
        // fixme: это не собирается
        //file_info.modification_time = std::chrono::time_point_cast<std::chrono::nanoseconds>(fileInfo.modified_time);
        file_info.status_change_time = {}; // Заполните это поле, если у вас есть информация
        file_info.size = file->size;
        file_info.mode = file->mode;

        return true;
    }
};

std::shared_ptr<IDirectoryEnumer> ProtocolFISH::DirectoryEnum(const std::string &path)
{
/*	std::shared_ptr<IDirectoryEnumer> enumer = _dir_enum_cache.GetCachedDirectoryEnumer(path);
	if (enumer) {
		if (g_netrocks_verbosity > 0) {
			fprintf(stderr, "Cached enum '%s'\n", path.c_str());
		}
	} else {
		fprintf(stderr, "Uncached enum '%s'\n", path.c_str());
		enumer = std::shared_ptr<IDirectoryEnumer>(new FISHDirectoryEnumer(_fish, path));
		enumer = _dir_enum_cache.GetCachingWrapperDirectoryEnumer(path, enumer);
	}

	return enumer;
*/
	fprintf(stderr, "Enum '%s'\n", path.c_str());
	return std::shared_ptr<IDirectoryEnumer>(new FISHDirectoryEnumer(_fish, path, _info));
}

class FISHFileReader : public IFileReader
{
	std::shared_ptr<FISHClient> _fish;
	std::string _buffer;
	uint64_t _remain{0};
	bool _failed{false};

public:
	FISHFileReader(std::shared_ptr<FISHClient> &fish, const std::string &path, unsigned long long resume_pos)
		: _fish(fish)
	{
		_fish->SetSubstitution("${FISH_START_OFFSET}", StrPrintf("%llu", resume_pos));
	    const auto &wr = _fish->SendHelperAndWaitReply("FISH/get", {"\n### 100\n", "\n### 500\n"});
		if (wr.index != 0) {
			throw ProtocolError("get file error");
		}
		_remain = GetIntegerBeforeStatusReplyLine(wr.stdout_data, wr.pos);
		_buffer = wr.stdout_data.substr(wr.pos + 9);
		fprintf(stderr, "FISHFileReader: will read %llu bytes\n", (unsigned long long)_remain);
	}

	virtual ~FISHFileReader()
	{
		try { // there is no cancellation for now, so have to fetch all unread yet data
			while (_remain && !_failed) {
				char tmp[0x1000];
				Read(tmp, sizeof(tmp));
			}
		} catch (...) {
		}

		if (_failed) try { // best effort...
			_fish->Resynchronize();
		} catch (...) {
		}
	}

	virtual size_t Read(void *buf, size_t len)
	{
		if ((uint64_t)len > _remain) {
			len = (size_t)_remain;
		}
		size_t piece = std::min(len, _buffer.size());
		if (piece) {
			memcpy(buf, _buffer.c_str(), piece);
			_buffer.erase(0, piece);
			_remain-= (uint64_t)piece;
		}

		while (piece < len) {
			ssize_t r = _fish->ReadStdout((unsigned char *)buf + piece, len - piece);
			if (r <= 0) {
				_failed = true;
				throw ProtocolError("get file error");
			}
			piece+= (size_t)r;
			_remain-= (uint64_t)r;
		}

		return piece;
	}
};


std::shared_ptr<IFileReader> ProtocolFISH::FileGet(const std::string &path, unsigned long long resume_pos)
{
	fprintf(stderr, "*24\n");
	return std::make_shared<FISHFileReader>(_fish, path, resume_pos);
}

std::shared_ptr<IFileWriter> ProtocolFISH::FilePut(const std::string &path, mode_t mode, unsigned long long size_hint, unsigned long long resume_pos)
{
	fprintf(stderr, "*25\n");
	return std::shared_ptr<IFileWriter>();
}
