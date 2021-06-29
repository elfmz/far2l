#include "utils.h"
#include <WinCompat.h>
#include <WinPort.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

#include <assert.h>
#include <mutex>
#include <fcntl.h>
#include "ConvertUTF.h"
#include <errno.h>


std::string GetMyHome()
{
	std::string out;

	struct passwd *pw = getpwuid(getuid());
	if (pw && pw->pw_dir && *pw->pw_dir) {
		out = pw->pw_dir;
	} else {
		const char *env = getenv("HOME");
		if (env && *env) {
			out = env;
		} else {
			out = "/tmp";
		}
	}
	return out;
}

static std::string InProfileSubdir(const char *what, const char *subpath, bool create_path)
{
	std::string path, settings;
	const char *env_settings = getenv("FARSETTINGS");
	if (env_settings) {
		// save env_settings cuz getenv() result can be invalided by another getenv()
		settings = env_settings;
	}

	if (!settings.empty() && settings[0] == '/') {
		path = settings;
		path+= '/';
		path+= what;

	} else {
		path = GetMyHome();
		path+= '/';
		path+= what;
		path+= "/far2l";
		if (!settings.empty()) {
			path+= "/custom/";
			path+= settings;
		}
	}

	if (subpath) {
		if (*subpath != GOOD_SLASH) {
			path+= GOOD_SLASH;
		}
		path+= subpath;
	}

	if (create_path) {
		size_t p = path.rfind(GOOD_SLASH);
		struct stat s;
		if (stat(path.substr(0, p).c_str(), &s) == -1) {
			for (size_t i = 1; i <= p; ++i) if (path[i] == GOOD_SLASH) {
				mkdir(path.substr(0, i).c_str(), 0700);
			}
		}
	}

	return path;
}

std::string InMyConfig(const char *subpath, bool create_path)
{
	return InProfileSubdir(".config", subpath, create_path);
}

std::string InMyCache(const char *subpath, bool create_path)
{
	return InProfileSubdir(".cache", subpath, create_path);
}


static std::string s_ready_temp;
static std::mutex s_ready_temp_mutex;

static void GetTempRoot(std::string &path)
{
	struct stat s;
	std::lock_guard<std::mutex> lock(s_ready_temp_mutex);
	if (!s_ready_temp.empty()) {
		path = s_ready_temp;
		return;
	}

	const char *temp = getenv("TMPDIR ");
	if (!temp || stat(temp, &s) == -1 || (s.st_mode & S_IFMT) != S_IFDIR) {
		temp = "/tmp";
		if (stat(temp, &s) == -1 || (s.st_mode & S_IFMT) != S_IFDIR) {
			temp = "/var/tmp";
			if (stat(temp, &s) == -1 || (s.st_mode & S_IFMT) != S_IFDIR) {
				static std::string s_in_prof_temp = InMyConfig("tmp");
				temp = s_in_prof_temp.c_str();
				if (stat(temp, &s) == -1 || (s.st_mode & S_IFMT) != S_IFDIR) {
					perror("Can't get temp!");
					return;
				}
			}
		}
	}

	for (unsigned int i = 0; ; ++i )  {
		path = temp;
		path+= GOOD_SLASH;
		char buf[128];
		sprintf(buf, "far2l_%llx_%u", (unsigned long long)geteuid(), i);
		path+= buf;
		mkdir(path.c_str(), 0700);
		if (stat(path.c_str(), &s) == 0 && 
			(s.st_mode & S_IFMT) == S_IFDIR && 
			s.st_uid == geteuid()) {
			break;
		}

		if (i == (unsigned int)-1) {
			perror("Can't init temp!");
			return;
		}
	}
	s_ready_temp = path;
	
}

std::string InMyTemp(const char *subpath)
{
	std::string path;
	GetTempRoot(path);
	path+= GOOD_SLASH;
	if (subpath) {
		for (;*subpath; ++subpath) {
			if (*subpath == GOOD_SLASH) {
				mkdir(path.c_str(), 0700);
			}
			path+= *subpath;
		}
	}

	return path;
}
