#pragma once
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#ifdef __APPLE__
	#include <sys/mount.h>
#elif !defined(__FreeBSD__) && !defined(__DragonFly__) && !defined(__HAIKU__)
	#include <sys/statfs.h>
#endif
#include <dirent.h>

#ifdef __cplusplus
extern "C" {
#endif

	typedef enum SudoClientMode_ {
		SCM_DISABLE,
		SCM_CONFIRM_MODIFY,
		SCM_CONFIRM_NONE
	} SudoClientMode;

	void sudo_client_configure(SudoClientMode mode, int password_expiration,
		const char *sudo_app, const char *askpass_app,
		const char *sudo_title, const char *sudo_prompt, const char *sudo_confirm);

	int sudo_main_askpass();
	int sudo_main_dispatcher(int argc, char *argv[]);

	int sudo_client_execute(const char *cmd, bool modify, bool no_wait);
	__attribute__ ((visibility("default"))) int sudo_client_is_required_for(const char *pathname, bool modify);
	__attribute__ ((visibility("default"))) void sudo_client_drop();

	__attribute__ ((visibility("default"))) void sudo_client_region_enter();
	__attribute__ ((visibility("default"))) void sudo_client_region_leave();
	__attribute__ ((visibility("default"))) void sudo_silent_query_region_enter();
	__attribute__ ((visibility("default"))) void sudo_silent_query_region_leave();

	__attribute__ ((visibility("default"))) int sdc_close(int fd);
	__attribute__ ((visibility("default"))) int sdc_open(const char* pathname, int flags, ...);
	__attribute__ ((visibility("default"))) off_t sdc_lseek(int fd, off_t offset, int whence);
	__attribute__ ((visibility("default"))) ssize_t sdc_write(int fd, const void *buf, size_t count);
	__attribute__ ((visibility("default"))) ssize_t sdc_read(int fd, void *buf, size_t count);
	__attribute__ ((visibility("default"))) ssize_t sdc_pwrite(int fd, const void *buf, size_t count, off_t offset);
	__attribute__ ((visibility("default"))) ssize_t sdc_pread(int fd, void *buf, size_t count, off_t offset);
	__attribute__ ((visibility("default"))) int sdc_statfs(const char *path, struct statfs *buf);
	__attribute__ ((visibility("default"))) int sdc_statvfs(const char *path, struct statvfs *buf);
	__attribute__ ((visibility("default"))) int sdc_stat(const char *path, struct stat *buf);
	__attribute__ ((visibility("default"))) int sdc_lstat(const char *path, struct stat *buf);
	__attribute__ ((visibility("default"))) int sdc_fstat(int fd, struct stat *buf);
	__attribute__ ((visibility("default"))) int sdc_ftruncate(int fd, off_t length);
	__attribute__ ((visibility("default"))) int sdc_fchmod(int fd, mode_t mode);
	__attribute__ ((visibility("default"))) int sdc_closedir(DIR *dir);
	__attribute__ ((visibility("default"))) DIR *sdc_opendir(const char *name);
	__attribute__ ((visibility("default"))) struct dirent *sdc_readdir(DIR *dir);
	__attribute__ ((visibility("default"))) int sdc_mkdir(const char *path, mode_t mode);
	__attribute__ ((visibility("default"))) int sdc_chdir(const char *path);
	__attribute__ ((visibility("default"))) int sdc_rmdir(const char *path);
	__attribute__ ((visibility("default"))) int sdc_remove(const char *path);
	__attribute__ ((visibility("default"))) int sdc_unlink(const char *path);
	__attribute__ ((visibility("default"))) int sdc_chmod(const char *pathname, mode_t mode);
	__attribute__ ((visibility("default"))) int sdc_chown(const char *pathname, uid_t owner, gid_t group);
	__attribute__ ((visibility("default"))) int sdc_utimens(const char *filename, const struct timespec times[2]);
	__attribute__ ((visibility("default"))) int sdc_futimens(int fd, const struct timespec times[2]);
	__attribute__ ((visibility("default"))) int sdc_rename(const char *path1, const char *path2);
	__attribute__ ((visibility("default"))) int sdc_symlink(const char *path1, const char *path2);
	__attribute__ ((visibility("default"))) int sdc_link(const char *path1, const char *path2);
	__attribute__ ((visibility("default"))) char *sdc_realpath(const char *path, char *resolved_path);
	__attribute__ ((visibility("default"))) ssize_t sdc_readlink(const char *pathname, char *buf, size_t bufsiz);
	__attribute__ ((visibility("default"))) char *sdc_getcwd(char *buf, size_t size);
	__attribute__ ((visibility("default"))) ssize_t sdc_flistxattr(int fd, char *namebuf, size_t size);
	__attribute__ ((visibility("default"))) ssize_t sdc_fgetxattr(int fd, const char *name,void *value, size_t size);
	__attribute__ ((visibility("default"))) int sdc_fsetxattr(int fd, const char *name, const void *value, size_t size, int flags);
	__attribute__ ((visibility("default"))) int sdc_fs_flags_get(const char *path, unsigned long *flags);
	__attribute__ ((visibility("default"))) int sdc_fs_flags_set(const char *path, unsigned long flags);
	__attribute__ ((visibility("default"))) int sdc_mkfifo(const char *path, mode_t mode);
	__attribute__ ((visibility("default"))) int sdc_mknod(const char *path, mode_t mode, dev_t dev);

#ifdef __cplusplus
}

struct SudoClientRegion
{
	inline SudoClientRegion()
	{
		sudo_client_region_enter();
	}

	inline ~SudoClientRegion()
	{
		sudo_client_region_leave();
	}
};

class SudoSilentQueryRegion
{
	bool _entered;

public:
	inline SudoSilentQueryRegion(bool enter = true) : _entered(enter)
	{
		if (enter)
			sudo_silent_query_region_enter();
	}

	inline void Enter()
	{
		if (!_entered) {
			_entered = true;
			sudo_silent_query_region_enter();
		}
	}

	inline ~SudoSilentQueryRegion()
	{
		if (_entered)
			sudo_silent_query_region_leave();
	}
};
#endif
