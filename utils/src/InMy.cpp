#include "utils.h"
#include <WinCompat.h>
#include <WinPort.h>
#include <sys/stat.h>
#include <assert.h>
#include <mutex>
#include <fcntl.h>
#include "ConvertUTF.h"
#include <errno.h>


std::string InMyConfig(const char *subpath, bool create_path)
{
	std::string path;
#ifdef _WIN32
	path = "D:\\far2l";
#else	
	const char *home = getenv("HOME");
	if (home) {
		path = home;
		path+= "/.config";
	} else {
		char tmp[128];
		sprintf(tmp, "/tmp/far2l_%llx_cfg", (unsigned long long)geteuid());
		path = tmp;
	}
	if (create_path)
		mkdir(path.c_str(), 0700);
	path+= "/far2l";
#endif
	if (create_path)
		mkdir(path.c_str(), 0700);
	if (subpath) {
		if (*subpath != GOOD_SLASH) 
			path+= GOOD_SLASH;
		for (const char *p = subpath; *p; ++p) {
			if (*p == GOOD_SLASH && create_path)
				mkdir(path.c_str(), 0700);
			path+= *p;
		}
	}
	
	return path;
	
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

	const char *temp = getenv("TEMP");
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
