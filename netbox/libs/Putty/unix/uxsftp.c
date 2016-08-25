/*
 * uxsftp.c: the Unix-specific parts of PSFTP and PSCP.
 */

#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <utime.h>
#include <errno.h>
#include <assert.h>
#include <glob.h>
#ifndef HAVE_NO_SYS_SELECT_H
#include <sys/select.h>
#endif

#include "putty.h"
#include "ssh.h"
#include "psftp.h"
#include "int64.h"

/*
 * In PSFTP our selects are synchronous, so these functions are
 * empty stubs.
 */
int uxsel_input_add(int fd, int rwx) { return 0; }
void uxsel_input_remove(int id) { }

char *x_get_default(const char *key)
{
    return NULL;		       /* this is a stub */
}

void platform_get_x11_auth(struct X11Display *display, Conf *conf)
{
    /* Do nothing, therefore no auth. */
}
const int platform_uses_x11_unix_by_default = TRUE;

/*
 * Default settings that are specific to PSFTP.
 */
char *platform_default_s(const char *name)
{
    return NULL;
}

int platform_default_i(const char *name, int def)
{
    return def;
}

FontSpec *platform_default_fontspec(const char *name)
{
    return fontspec_new("");
}

Filename *platform_default_filename(const char *name)
{
    if (!strcmp(name, "LogFileName"))
	return filename_from_str("putty.log");
    else
	return filename_from_str("");
}

char *get_ttymode(void *frontend, const char *mode) { return NULL; }

int get_userpass_input(prompts_t *p, unsigned char *in, int inlen)
{
    int ret;
    ret = cmdline_get_passwd_input(p, in, inlen);
    if (ret == -1)
	ret = console_get_userpass_input(p, in, inlen);
    return ret;
}

/*
 * Set local current directory. Returns NULL on success, or else an
 * error message which must be freed after printing.
 */
char *psftp_lcd(char *dir)
{
    if (chdir(dir) < 0)
	return dupprintf("%s: chdir: %s", dir, strerror(errno));
    else
	return NULL;
}

/*
 * Get local current directory. Returns a string which must be
 * freed.
 */
char *psftp_getcwd(void)
{
    char *buffer, *ret;
    int size = 256;

    buffer = snewn(size, char);
    while (1) {
	ret = getcwd(buffer, size);
	if (ret != NULL)
	    return ret;
	if (errno != ERANGE) {
	    sfree(buffer);
	    return dupprintf("[cwd unavailable: %s]", strerror(errno));
	}
	/*
	 * Otherwise, ERANGE was returned, meaning the buffer
	 * wasn't big enough.
	 */
	size = size * 3 / 2;
	buffer = sresize(buffer, size, char);
    }
}

struct RFile {
    int fd;
};

RFile *open_existing_file(char *name, uint64 *size,
			  unsigned long *mtime, unsigned long *atime,
                          long *perms)
{
    int fd;
    RFile *ret;

    fd = open(name, O_RDONLY);
    if (fd < 0)
	return NULL;

    ret = snew(RFile);
    ret->fd = fd;

    if (size || mtime || atime || perms) {
	struct stat statbuf;
	if (fstat(fd, &statbuf) < 0) {
	    fprintf(stderr, "%s: stat: %s\n", name, strerror(errno));
	    memset(&statbuf, 0, sizeof(statbuf));
	}

	if (size)
	    *size = uint64_make((statbuf.st_size >> 16) >> 16,
				statbuf.st_size);
	 	
	if (mtime)
	    *mtime = statbuf.st_mtime;

	if (atime)
	    *atime = statbuf.st_atime;

	if (perms)
	    *perms = statbuf.st_mode;
    }

    return ret;
}

int read_from_file(RFile *f, void *buffer, int length)
{
    return read(f->fd, buffer, length);
}

void close_rfile(RFile *f)
{
    close(f->fd);
    sfree(f);
}

struct WFile {
    int fd;
    char *name;
};

WFile *open_new_file(char *name, long perms)
{
    int fd;
    WFile *ret;

    fd = open(name, O_CREAT | O_TRUNC | O_WRONLY,
              (mode_t)(perms ? perms : 0666));
    if (fd < 0)
	return NULL;

    ret = snew(WFile);
    ret->fd = fd;
    ret->name = dupstr(name);

    return ret;
}


WFile *open_existing_wfile(char *name, uint64 *size)
{
    int fd;
    WFile *ret;

    fd = open(name, O_APPEND | O_WRONLY);
    if (fd < 0)
	return NULL;

    ret = snew(WFile);
    ret->fd = fd;
    ret->name = dupstr(name);

    if (size) {
	struct stat statbuf;
	if (fstat(fd, &statbuf) < 0) {
	    fprintf(stderr, "%s: stat: %s\n", name, strerror(errno));
	    memset(&statbuf, 0, sizeof(statbuf));
	}

	*size = uint64_make((statbuf.st_size >> 16) >> 16,
			    statbuf.st_size);
    }

    return ret;
}

int write_to_file(WFile *f, void *buffer, int length)
{
    char *p = (char *)buffer;
    int so_far = 0;

    /* Keep trying until we've really written as much as we can. */
    while (length > 0) {
	int ret = write(f->fd, p, length);

	if (ret < 0)
	    return ret;

	if (ret == 0)
	    break;

	p += ret;
	length -= ret;
	so_far += ret;
    }

    return so_far;
}

void set_file_times(WFile *f, unsigned long mtime, unsigned long atime)
{
    struct utimbuf ut;

    ut.actime = atime;
    ut.modtime = mtime;

    utime(f->name, &ut);
}

/* Closes and frees the WFile */
void close_wfile(WFile *f)
{
    close(f->fd);
    sfree(f->name);
    sfree(f);
}

/* Seek offset bytes through file, from whence, where whence is
   FROM_START, FROM_CURRENT, or FROM_END */
int seek_file(WFile *f, uint64 offset, int whence)
{
    off_t fileofft;
    int lseek_whence;
    
    fileofft = (((off_t) offset.hi << 16) << 16) + offset.lo;

    switch (whence) {
    case FROM_START:
	lseek_whence = SEEK_SET;
	break;
    case FROM_CURRENT:
	lseek_whence = SEEK_CUR;
	break;
    case FROM_END:
	lseek_whence = SEEK_END;
	break;
    default:
	return -1;
    }

    return lseek(f->fd, fileofft, lseek_whence) >= 0 ? 0 : -1;
}

uint64 get_file_posn(WFile *f)
{
    off_t fileofft;
    uint64 ret;

    fileofft = lseek(f->fd, (off_t) 0, SEEK_CUR);

    ret = uint64_make((fileofft >> 16) >> 16, fileofft);

    return ret;
}

int file_type(char *name)
{
    struct stat statbuf;

    if (stat(name, &statbuf) < 0) {
	if (errno != ENOENT)
	    fprintf(stderr, "%s: stat: %s\n", name, strerror(errno));
	return FILE_TYPE_NONEXISTENT;
    }

    if (S_ISREG(statbuf.st_mode))
	return FILE_TYPE_FILE;

    if (S_ISDIR(statbuf.st_mode))
	return FILE_TYPE_DIRECTORY;

    return FILE_TYPE_WEIRD;
}

struct DirHandle {
    DIR *dir;
};

DirHandle *open_directory(char *name)
{
    DIR *dir;
    DirHandle *ret;

    dir = opendir(name);
    if (!dir)
	return NULL;

    ret = snew(DirHandle);
    ret->dir = dir;
    return ret;
}

char *read_filename(DirHandle *dir)
{
    struct dirent *de;

    do {
	de = readdir(dir->dir);
	if (de == NULL)
	    return NULL;
    } while ((de->d_name[0] == '.' &&
	      (de->d_name[1] == '\0' ||
	       (de->d_name[1] == '.' && de->d_name[2] == '\0'))));

    return dupstr(de->d_name);
}

void close_directory(DirHandle *dir)
{
    closedir(dir->dir);
    sfree(dir);
}

int test_wildcard(char *name, int cmdline)
{
    struct stat statbuf;

    if (stat(name, &statbuf) == 0) {
	return WCTYPE_FILENAME;
    } else if (cmdline) {
	/*
	 * On Unix, we never need to parse wildcards coming from
	 * the command line, because the shell will have expanded
	 * them into a filename list already.
	 */
	return WCTYPE_NONEXISTENT;
    } else {
	glob_t globbed;
	int ret = WCTYPE_NONEXISTENT;

	if (glob(name, GLOB_ERR, NULL, &globbed) == 0) {
	    if (globbed.gl_pathc > 0)
		ret = WCTYPE_WILDCARD;
	    globfree(&globbed);
	}

	return ret;
    }
}

/*
 * Actually return matching file names for a local wildcard.
 */
struct WildcardMatcher {
    glob_t globbed;
    int i;
};
WildcardMatcher *begin_wildcard_matching(char *name) {
    WildcardMatcher *ret = snew(WildcardMatcher);

    if (glob(name, 0, NULL, &ret->globbed) < 0) {
	sfree(ret);
	return NULL;
    }

    ret->i = 0;

    return ret;
}
char *wildcard_get_filename(WildcardMatcher *dir) {
    if (dir->i < dir->globbed.gl_pathc) {
	return dupstr(dir->globbed.gl_pathv[dir->i++]);
    } else
	return NULL;
}
void finish_wildcard_matching(WildcardMatcher *dir) {
    globfree(&dir->globbed);
    sfree(dir);
}

int vet_filename(char *name)
{
    if (strchr(name, '/'))
	return FALSE;

    if (name[0] == '.' && (!name[1] || (name[1] == '.' && !name[2])))
	return FALSE;

    return TRUE;
}

int create_directory(char *name)
{
    return mkdir(name, 0777) == 0;
}

char *dir_file_cat(char *dir, char *file)
{
    return dupcat(dir, "/", file, NULL);
}

/*
 * Do a select() between all currently active network fds and
 * optionally stdin.
 */
static int ssh_sftp_do_select(int include_stdin, int no_fds_ok)
{
    fd_set rset, wset, xset;
    int i, fdcount, fdsize, *fdlist;
    int fd, fdstate, rwx, ret, maxfd;
    unsigned long now = GETTICKCOUNT();
    unsigned long next;

    fdlist = NULL;
    fdcount = fdsize = 0;

    do {

	/* Count the currently active fds. */
	i = 0;
	for (fd = first_fd(&fdstate, &rwx); fd >= 0;
	     fd = next_fd(&fdstate, &rwx)) i++;

	if (i < 1 && !no_fds_ok)
	    return -1;		       /* doom */

	/* Expand the fdlist buffer if necessary. */
	if (i > fdsize) {
	    fdsize = i + 16;
	    fdlist = sresize(fdlist, fdsize, int);
	}

	FD_ZERO(&rset);
	FD_ZERO(&wset);
	FD_ZERO(&xset);
	maxfd = 0;

	/*
	 * Add all currently open fds to the select sets, and store
	 * them in fdlist as well.
	 */
	fdcount = 0;
	for (fd = first_fd(&fdstate, &rwx); fd >= 0;
	     fd = next_fd(&fdstate, &rwx)) {
	    fdlist[fdcount++] = fd;
	    if (rwx & 1)
		FD_SET_MAX(fd, maxfd, rset);
	    if (rwx & 2)
		FD_SET_MAX(fd, maxfd, wset);
	    if (rwx & 4)
		FD_SET_MAX(fd, maxfd, xset);
	}

	if (include_stdin)
	    FD_SET_MAX(0, maxfd, rset);

        if (toplevel_callback_pending()) {
            struct timeval tv;
            tv.tv_sec = 0;
            tv.tv_usec = 0;
            ret = select(maxfd, &rset, &wset, &xset, &tv);
            if (ret == 0)
                run_toplevel_callbacks();
        } else if (run_timers(now, &next)) {
            do {
                unsigned long then;
                long ticks;
                struct timeval tv;

		then = now;
		now = GETTICKCOUNT();
		if (now - then > next - then)
		    ticks = 0;
		else
		    ticks = next - now;
		tv.tv_sec = ticks / 1000;
		tv.tv_usec = ticks % 1000 * 1000;
                ret = select(maxfd, &rset, &wset, &xset, &tv);
                if (ret == 0)
                    now = next;
                else
                    now = GETTICKCOUNT();
            } while (ret < 0 && errno == EINTR);
        } else {
            ret = select(maxfd, &rset, &wset, &xset, NULL);
        }
    } while (ret == 0);

    if (ret < 0) {
	perror("select");
	exit(1);
    }

    for (i = 0; i < fdcount; i++) {
	fd = fdlist[i];
	/*
	 * We must process exceptional notifications before
	 * ordinary readability ones, or we may go straight
	 * past the urgent marker.
	 */
	if (FD_ISSET(fd, &xset))
	    select_result(fd, 4);
	if (FD_ISSET(fd, &rset))
	    select_result(fd, 1);
	if (FD_ISSET(fd, &wset))
	    select_result(fd, 2);
    }

    sfree(fdlist);

    run_toplevel_callbacks();

    return FD_ISSET(0, &rset) ? 1 : 0;
}

/*
 * Wait for some network data and process it.
 */
int ssh_sftp_loop_iteration(void)
{
    return ssh_sftp_do_select(FALSE, FALSE);
}

/*
 * Read a PSFTP command line from stdin.
 */
char *ssh_sftp_get_cmdline(char *prompt, int no_fds_ok)
{
    char *buf;
    int buflen, bufsize, ret;

    fputs(prompt, stdout);
    fflush(stdout);

    buf = NULL;
    buflen = bufsize = 0;

    while (1) {
	ret = ssh_sftp_do_select(TRUE, no_fds_ok);
	if (ret < 0) {
	    printf("connection died\n");
            sfree(buf);
	    return NULL;	       /* woop woop */
	}
	if (ret > 0) {
	    if (buflen >= bufsize) {
		bufsize = buflen + 512;
		buf = sresize(buf, bufsize, char);
	    }
	    ret = read(0, buf+buflen, 1);
	    if (ret < 0) {
		perror("read");
                sfree(buf);
		return NULL;
	    }
	    if (ret == 0) {
		/* eof on stdin; no error, but no answer either */
                sfree(buf);
		return NULL;
	    }

	    if (buf[buflen++] == '\n') {
		/* we have a full line */
		return buf;
	    }
	}
    }
}

void frontend_net_error_pending(void) {}

/*
 * Main program: do platform-specific initialisation and then call
 * psftp_main().
 */
int main(int argc, char *argv[])
{
    uxsel_init();
    return psftp_main(argc, argv);
}
