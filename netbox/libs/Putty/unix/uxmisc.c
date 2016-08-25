/*
 * PuTTY miscellaneous Unix stuff
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <pwd.h>

#include "putty.h"

unsigned long getticks(void)
{
    /*
     * We want to use milliseconds rather than the microseconds or
     * nanoseconds given by the underlying clock functions, because we
     * need a decent number of them to fit into a 32-bit word so it
     * can be used for keepalives.
     */
#if defined HAVE_CLOCK_GETTIME && defined HAVE_DECL_CLOCK_MONOTONIC
    {
        /* Use CLOCK_MONOTONIC if available, so as to be unconfused if
         * the system clock changes. */
        struct timespec ts;
        if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0)
            return ts.tv_sec * TICKSPERSEC +
                ts.tv_nsec / (1000000000 / TICKSPERSEC);
    }
#endif
    {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return tv.tv_sec * TICKSPERSEC + tv.tv_usec / (1000000 / TICKSPERSEC);
    }
}

Filename *filename_from_str(const char *str)
{
    Filename *ret = snew(Filename);
    ret->path = dupstr(str);
    return ret;
}

Filename *filename_copy(const Filename *fn)
{
    return filename_from_str(fn->path);
}

const char *filename_to_str(const Filename *fn)
{
    return fn->path;
}

int filename_equal(const Filename *f1, const Filename *f2)
{
    return !strcmp(f1->path, f2->path);
}

int filename_is_null(const Filename *fn)
{
    return !fn->path[0];
}

void filename_free(Filename *fn)
{
    sfree(fn->path);
    sfree(fn);
}

int filename_serialise(const Filename *f, void *vdata)
{
    char *data = (char *)vdata;
    int len = strlen(f->path) + 1;     /* include trailing NUL */
    if (data) {
        strcpy(data, f->path);
    }
    return len;
}
Filename *filename_deserialise(void *vdata, int maxsize, int *used)
{
    char *data = (char *)vdata;
    char *end;
    end = memchr(data, '\0', maxsize);
    if (!end)
        return NULL;
    end++;
    *used = end - data;
    return filename_from_str(data);
}

char filename_char_sanitise(char c)
{
    if (c == '/')
        return '.';
    return c;
}

#ifdef DEBUG
static FILE *debug_fp = NULL;

void dputs(char *buf)
{
    if (!debug_fp) {
	debug_fp = fopen("debug.log", "w");
    }

    if (write(1, buf, strlen(buf)) < 0) {} /* 'error check' to placate gcc */

    fputs(buf, debug_fp);
    fflush(debug_fp);
}
#endif

char *get_username(void)
{
    struct passwd *p;
    uid_t uid = getuid();
    char *user, *ret = NULL;

    /*
     * First, find who we think we are using getlogin. If this
     * agrees with our uid, we'll go along with it. This should
     * allow sharing of uids between several login names whilst
     * coping correctly with people who have su'ed.
     */
    user = getlogin();
    setpwent();
    if (user)
	p = getpwnam(user);
    else
	p = NULL;
    if (p && p->pw_uid == uid) {
	/*
	 * The result of getlogin() really does correspond to
	 * our uid. Fine.
	 */
	ret = user;
    } else {
	/*
	 * If that didn't work, for whatever reason, we'll do
	 * the simpler version: look up our uid in the password
	 * file and map it straight to a name.
	 */
	p = getpwuid(uid);
	if (!p)
	    return NULL;
	ret = p->pw_name;
    }
    endpwent();

    return dupstr(ret);
}

/*
 * Display the fingerprints of the PGP Master Keys to the user.
 * (This is here rather than in uxcons because it's appropriate even for
 * Unix GUI apps.)
 */
void pgp_fingerprints(void)
{
    fputs("These are the fingerprints of the PuTTY PGP Master Keys. They can\n"
	  "be used to establish a trust path from this executable to another\n"
	  "one. See the manual for more information.\n"
	  "(Note: these fingerprints have nothing to do with SSH!)\n"
	  "\n"
	  "PuTTY Master Key as of 2015 (RSA, 4096-bit):\n"
	  "  " PGP_MASTER_KEY_FP "\n\n"
	  "Original PuTTY Master Key (RSA, 1024-bit):\n"
	  "  " PGP_RSA_MASTER_KEY_FP "\n"
	  "Original PuTTY Master Key (DSA, 1024-bit):\n"
	  "  " PGP_DSA_MASTER_KEY_FP "\n", stdout);
}

/*
 * Set and clear fcntl options on a file descriptor. We don't
 * realistically expect any of these operations to fail (the most
 * plausible error condition is EBADF, but we always believe ourselves
 * to be passing a valid fd so even that's an assertion-fail sort of
 * response), so we don't make any effort to return sensible error
 * codes to the caller - we just log to standard error and die
 * unceremoniously. However, nonblock and no_nonblock do return the
 * previous state of O_NONBLOCK.
 */
void cloexec(int fd) {
    int fdflags;

    fdflags = fcntl(fd, F_GETFD);
    if (fdflags < 0) {
        fprintf(stderr, "%d: fcntl(F_GETFD): %s\n", fd, strerror(errno));
        exit(1);
    }
    if (fcntl(fd, F_SETFD, fdflags | FD_CLOEXEC) < 0) {
        fprintf(stderr, "%d: fcntl(F_SETFD): %s\n", fd, strerror(errno));
        exit(1);
    }
}
void noncloexec(int fd) {
    int fdflags;

    fdflags = fcntl(fd, F_GETFD);
    if (fdflags < 0) {
        fprintf(stderr, "%d: fcntl(F_GETFD): %s\n", fd, strerror(errno));
        exit(1);
    }
    if (fcntl(fd, F_SETFD, fdflags & ~FD_CLOEXEC) < 0) {
        fprintf(stderr, "%d: fcntl(F_SETFD): %s\n", fd, strerror(errno));
        exit(1);
    }
}
int nonblock(int fd) {
    int fdflags;

    fdflags = fcntl(fd, F_GETFL);
    if (fdflags < 0) {
        fprintf(stderr, "%d: fcntl(F_GETFL): %s\n", fd, strerror(errno));
        exit(1);
    }
    if (fcntl(fd, F_SETFL, fdflags | O_NONBLOCK) < 0) {
        fprintf(stderr, "%d: fcntl(F_SETFL): %s\n", fd, strerror(errno));
        exit(1);
    }

    return fdflags & O_NONBLOCK;
}
int no_nonblock(int fd) {
    int fdflags;

    fdflags = fcntl(fd, F_GETFL);
    if (fdflags < 0) {
        fprintf(stderr, "%d: fcntl(F_GETFL): %s\n", fd, strerror(errno));
        exit(1);
    }
    if (fcntl(fd, F_SETFL, fdflags & ~O_NONBLOCK) < 0) {
        fprintf(stderr, "%d: fcntl(F_SETFL): %s\n", fd, strerror(errno));
        exit(1);
    }

    return fdflags & O_NONBLOCK;
}

FILE *f_open(const Filename *filename, char const *mode, int is_private)
{
    if (!is_private) {
	return fopen(filename->path, mode);
    } else {
	int fd;
	assert(mode[0] == 'w');	       /* is_private is meaningless for read,
					  and tricky for append */
	fd = open(filename->path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
	if (fd < 0)
	    return NULL;
	return fdopen(fd, mode);
    }
}

FontSpec *fontspec_new(const char *name)
{
    FontSpec *f = snew(FontSpec);
    f->name = dupstr(name);
    return f;
}
FontSpec *fontspec_copy(const FontSpec *f)
{
    return fontspec_new(f->name);
}
void fontspec_free(FontSpec *f)
{
    sfree(f->name);
    sfree(f);
}
int fontspec_serialise(FontSpec *f, void *data)
{
    int len = strlen(f->name);
    if (data)
        strcpy(data, f->name);
    return len + 1;                    /* include trailing NUL */
}
FontSpec *fontspec_deserialise(void *vdata, int maxsize, int *used)
{
    char *data = (char *)vdata;
    char *end = memchr(data, '\0', maxsize);
    if (!end)
        return NULL;
    *used = end - data + 1;
    return fontspec_new(data);
}
