#pragma once
#include <sys/stat.h>
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
 int sudo_main_dispatcher();

 int sudo_client_execute(const char *cmd, bool modify, bool no_wait);
 __attribute__ ((visibility("default"))) int sudo_client_is_required_for(const char *pathname, bool modify);

 __attribute__ ((visibility("default"))) void sudo_client_region_enter();
 __attribute__ ((visibility("default"))) void sudo_client_region_leave();

 __attribute__ ((visibility("default"))) int sdc_close(int fd);
 __attribute__ ((visibility("default"))) int sdc_open(const char* pathname, int flags, ...);
 __attribute__ ((visibility("default"))) off_t sdc_lseek(int fd, off_t offset, int whence);
 __attribute__ ((visibility("default"))) ssize_t sdc_write(int fd, const void *buf, size_t count);
 __attribute__ ((visibility("default"))) ssize_t sdc_read(int fd, void *buf, size_t count);
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
 __attribute__ ((visibility("default"))) int sdc_utimes(const char *filename, const struct timeval times[2]);
 __attribute__ ((visibility("default"))) int sdc_rename(const char *path1, const char *path2);
 __attribute__ ((visibility("default"))) int sdc_symlink(const char *path1, const char *path2);
 __attribute__ ((visibility("default"))) int sdc_link(const char *path1, const char *path2);
 __attribute__ ((visibility("default"))) char *sdc_realpath(const char *path, char *resolved_path);
 __attribute__ ((visibility("default"))) char *sdc_getcwd(char *buf, size_t size);
 
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
#endif
