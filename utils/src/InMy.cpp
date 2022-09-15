#include "utils.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <pwd.h>
#include <assert.h>
#include <fcntl.h>
#include <errno.h>

enum PrivacyLevel
{
	// if dir exists and accessible - don't care who owns it, if not exists - create accessible for all (0777)
	PL_ALL = 0,

	// if dir exists and accessible - don't care who owns it, if not exists - create accessible to current user only
	PL_ANY,

	// fail if dir owned by nor current user nor root
	PL_PRIVATE
};

static bool EnsureDir(const char *dir, PrivacyLevel pl)
{
	struct stat s{};
	for (int i = 0; stat(dir, &s) == -1; ++i) {
		if (i > 10) {
			fprintf(stderr, "%s('%s', %u) dir-pear; stat errno=%u\n", __FUNCTION__, dir, pl, errno);
			return false;
		}
		if (mkdir(dir, (pl == PL_ALL) ? 0777 : 0700) == 0) {
			return true;
		}
		if (errno != EEXIST) {
			return false;
		}
		usleep(i * 1000);
	}

	if ((s.st_mode & S_IFMT) != S_IFDIR) {
		fprintf(stderr, "%s('%s', %u) its not a dir; mode=0%o\n", __FUNCTION__, dir, pl, s.st_mode);
		return false;
	}

	const auto uid = geteuid();
	if (pl >= PL_PRIVATE && uid != s.st_uid && s.st_uid != 0) {
		fprintf(stderr, "%s('%s', %u) uid=%u but st_uid=%u\n", __FUNCTION__, dir, pl, uid, s.st_uid);
		return false;
	}

	if ( ((s.st_mode & S_IWOTH) != 0)
	  || ((s.st_mode & S_IWUSR) != 0 && s.st_uid == uid)
	  || ((s.st_mode & S_IWGRP) != 0 && s.st_gid == getegid())) {
		return true;
	}

	// may be mode bits lie us? check it with stick
	std::string check_path = dir;
	check_path+= "/.stick-check.far2l";
	int fd = open(check_path.c_str(), O_CREAT | O_RDWR | O_CLOEXEC, 0600);
	if (fd != -1) {
		close(fd);
		unlink(check_path.c_str());
		fprintf(stderr, "%s('%s', %u) file allowed; mode=0%o\n", __FUNCTION__, dir, pl, s.st_mode);
		return true;
	}

	return false;
}

static std::string GetTempSubdirUncached(const char *what)
{
	std::string out;
	std::string env_cache = what;
	env_cache+= "_tmp";
	const char *env = getenv(env_cache.c_str());
	if (env && *env) {
		out = env;
//		fprintf(stderr, "%s: '%s' cached as '%s'\n", __FUNCTION__, env_cache.c_str(), out.c_str());
		return out;
	}

	env = getenv("TMPDIR");
	if (env && *env == '/' && EnsureDir(env, PL_ALL)) {
		out = env;
		if (out.back() != '/') {
			out+= '/';
		}

	} else if (EnsureDir("/tmp", PL_ALL)) {
		out = "/tmp/";

	} else if (EnsureDir("/var/tmp", PL_ALL)) {
		out = "/var/tmp/";

	} else {
		perror("Can't find temp!");
		out = "/";
	}

	out+= what;
	out+= '_';

	const size_t base_len = out.size();
	const unsigned long long uid = geteuid();

	for (unsigned int i = 0; i < 0x10000; ++i) {
		char subdir[128];
		if (i == 0x1000) {
			srand(time(NULL) ^ (getpid() << 8));
		}
		sprintf(subdir, "%llx_%x", uid, (i < 0x1000) ? i : i | ((rand() & 0xffff) << 16) );

		out.replace(base_len, out.size() - base_len, subdir);

		if (EnsureDir(out.c_str(), PL_PRIVATE)) {
			break;
		}
	}

	setenv(env_cache.c_str(), out.c_str(), 1);
//	fprintf(stderr, "%s: '%s' inited as '%s'\n", __FUNCTION__, env_cache.c_str(), out.c_str());
	return out;
}

static std::string GetMyHomeUncached()
{
	struct passwd *pw = getpwuid(geteuid());
	if (pw && pw->pw_dir && *pw->pw_dir && EnsureDir(pw->pw_dir, PL_ANY)) {
		return std::string(pw->pw_dir);
	}

	const char *env = getenv("HOME");
	if (env && EnsureDir(env, PL_ANY)) {
		return std::string(env);
	}

	// fallback to in-temp location
	return GetTempSubdirUncached("far2l_home");
}

const std::string &GetMyHome()
{
	static std::string s_out = GetMyHomeUncached();
	return s_out;
}

/*** ProfileDir is a helper for InMy... functions, it constructs
base settings/caches path from env with following order of precedence:
	if $FARSETTINGS is set to full path:
		return $FARSETTINGS/<usual_name>
	elsif $<xdg_env> is set and $FARSETTINGS is set:
		return $<xdg_env>/custom/$FARSETTINGS
	elsif $<xdg_env> is set:
		return $<xdg_env>
	elsif $FARSETTINGS is set:
		return $HOME/<usual_name>/custom/$FARSETTINGS
	else
		return $HOME/<usual_name>
*/
class ProfileDir
{
	const char *_usual_name;
	const char *_xdg_env;
	std::string _base_path;

	ProfileDir(const char *usual_name, const char *xdg_env)
		: _usual_name(usual_name), _xdg_env(xdg_env)
	{
		Update();
	}

public:
	void Update()
	{
		std::string settings;
		const char *settings_env = getenv("FARSETTINGS");
		if (settings_env && *settings_env) {
			settings = settings_env;
			// cosmetic sanity
			while (settings.size() > 1 && settings.back() == GOOD_SLASH) {
				settings.pop_back();
			}
		}

		if (!settings.empty() && settings[0] == GOOD_SLASH) {
			_base_path = settings;
			if (_base_path.back() != GOOD_SLASH) {
				_base_path+= GOOD_SLASH;
			}
			_base_path+= _usual_name;

		} else {
			const char *xdg_val = getenv(_xdg_env);
			if (xdg_val && *xdg_val == GOOD_SLASH) {
				_base_path = xdg_val;

			} else {
				if (UNLIKELY(xdg_val)) {
					fprintf(stderr, "ProfileDir: %s ignored cuz its not a full path: '%s'\n", _xdg_env, xdg_val);
				}
				_base_path = GetMyHome();
				_base_path+= GOOD_SLASH;
				_base_path+= _usual_name;
			}

			if (_base_path.empty() || _base_path.back() != GOOD_SLASH) {
				_base_path+= GOOD_SLASH;
			}

			_base_path+= "far2l";

			if (!settings.empty()) {
				_base_path+= "/custom/";
				_base_path+= settings;
			}
		}
	}

	std::string In(const char *sub_path, bool create_path) const
	{
		std::string path = _base_path;

		if (sub_path) {
			if (*sub_path != GOOD_SLASH) {
				path+= GOOD_SLASH;
			}
			path+= sub_path;
		}

		if (create_path) {
			const size_t p = path.rfind(GOOD_SLASH);
			struct stat s;
			if (stat(path.substr(0, p).c_str(), &s) == -1) {
				for (size_t i = 1; i <= p; ++i) if (path[i] == GOOD_SLASH) {
					EnsureDir(path.substr(0, i).c_str(), PL_PRIVATE);
				}
			}
		}

		return path;
	}

///
	static ProfileDir &Config()
	{
		static ProfileDir s_out(".config", "XDG_CONFIG_HOME");
		return s_out;
	}

	static ProfileDir &Cache()
	{
		static ProfileDir s_out(".cache", "XDG_CACHE_HOME");
		return s_out;
	}
};

void InMyPathChanged()
{
	ProfileDir::Config().Update();
	ProfileDir::Cache().Update();
}

std::string InMyConfig(const char *subpath, bool create_path)
{
	return ProfileDir::Config().In(subpath, create_path);
}

std::string InMyCache(const char *subpath, bool create_path)
{
	return ProfileDir::Cache().In(subpath, create_path);
}

std::string InMyTemp(const char *subpath)
{
	static std::string temp(GetTempSubdirUncached("far2l"));
	std::string path = temp;
	path+= GOOD_SLASH;
	if (subpath) {
		for (;*subpath; ++subpath) {
			if (*subpath == GOOD_SLASH) {
				EnsureDir(path.c_str(), PL_PRIVATE);
			}
			path+= *subpath;
		}
	}

	return path;
}
