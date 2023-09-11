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
	_fish(std::make_shared<FISHClient>())
//	_dir_enum_cache(10)
{
	fprintf(stderr, "*** ProtocolFISH::%s\n", __FUNCTION__);

	StringConfig protocol_options(options);
	// ---------------------------------------------

    fprintf(stderr, "*** CONNECTING\n");

	fprintf(stderr, "*** host from config: %s\n", host.c_str());
	fprintf(stderr, "*** port from config: %i\n", port);
	fprintf(stderr, "*** username from config: %s\n", username.c_str());
	fprintf(stderr, "*** password from config: %s\n", password.c_str());

	fprintf(stderr, "*** login string from config: %s\n", (username + "@" + host).c_str());

	if (!_fish->OpenApp("ssh", (username + "@" + host).c_str())) {
	    printf("err 1\n");
    }

    // везде ли "$ " признак успешного залогина? проверить на dd wrt

    // Мы таки залогинены, спрашивают пароль, ключ незнакомый?
    std::vector<const char *> results = {"$ ", "# ", "password: ", "This key is not known by any other names", "Are you sure"};

    /*
    // это ещё от sftp враппер код
    // todo: сделать обработку сообщения о неправильном пароле (просто выходить из проги пока)
    if (sc->WaitFor("Connected to", {
		"assword:",
    	"Permission denied, please try again."
    */

    auto wr = _fish->SendAndWaitReply("", results);
    while (wr.index != 0 && wr.index != 1) {
	    if (wr.index == 2) {
		    wr = _fish->SendAndWaitReply(password + "\r", results);
	    }
	    if (wr.index == 3 || wr.index == 4) {
		    wr = _fish->SendAndWaitReply("yes\r", results);
	    }
	}

    fprintf(stderr, "*** CONNECTED\n");
	_fish->SendAndWaitReply(
		"export PS1=;export PS2=;export PS3=;export PS4=;export PROMPT_COMMAND=;echo '###'FISH'###'\r",
		{"###FISH###\n"}
	);

	wr = _fish->SendHelperAndWaitReply("FISH/info", {"\n### 200", "\n### "});
	if (wr.index == 0) {
		size_t ofs;
		for (ofs = 0; ofs < wr.stdout_data.size() && !isdigit(wr.stdout_data[ofs]);) {
			++ofs;
		}
		_info = atoi(wr.stdout_data.c_str() + ofs);
		_info&= ~8; // disable PERL for now
	}
	fprintf(stderr, "*** info=%u\n", _info);
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
		fprintf(stderr, "*19 path='%s'\n", path.c_str());
		_unquote = (info & (2 | 8 | 16)) != 0;
		_fish->SetSubstitution("${FISH_FILENAME}", path);
	    const auto &wr = _fish->SendHelperAndWaitReply("FISH/ls", {"\n### "}); // fish command end

		FISHParseLS(_files, wr.stdout_data); // path не передаётся тут никак в ls, fixme ***

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
		size_t pos_of_str_remain = wr.pos;
		if (pos_of_str_remain) do {
			--pos_of_str_remain;
		} while (pos_of_str_remain && wr.stdout_data[pos_of_str_remain] != '\n');
		_remain = (uint64_t)strtoull(wr.stdout_data.c_str() + pos_of_str_remain + 1, nullptr, 10);
		_buffer = wr.stdout_data.substr(wr.pos + 9);
	}

	virtual ~FISHFileReader()
	{
		try {
			while (_remain && !_failed) {
				char tmp[0x1000];
				Read(tmp, sizeof(tmp));
			}
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
				abort();
				_failed = true;
				throw ProtocolError("get file error");
			}
			piece+= (size_t)r;
			_remain-= (uint64_t)r;
		}

		return piece;
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
	return std::make_shared<FISHFileReader>(_fish, path, resume_pos);
}

std::shared_ptr<IFileWriter> ProtocolFISH::FilePut(const std::string &path, mode_t mode, unsigned long long size_hint, unsigned long long resume_pos)
{
	fprintf(stderr, "*25\n");
	return std::shared_ptr<IFileWriter>();
}
