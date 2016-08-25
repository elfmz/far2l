/*
 * Pseudo-tty backend for pterm.
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <assert.h>
#include <fcntl.h>
#include <termios.h>
#include <grp.h>
#include <utmp.h>
#include <pwd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <errno.h>

#include "putty.h"
#include "tree234.h"

#ifndef OMIT_UTMP
#include <utmpx.h>
#endif

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

/* updwtmpx() needs the name of the wtmp file.  Try to find it. */
#ifndef WTMPX_FILE
#ifdef _PATH_WTMPX
#define WTMPX_FILE _PATH_WTMPX
#else
#define WTMPX_FILE "/var/log/wtmpx"
#endif
#endif

#ifndef LASTLOG_FILE
#ifdef _PATH_LASTLOG
#define LASTLOG_FILE _PATH_LASTLOG
#else
#define LASTLOG_FILE "/var/log/lastlog"
#endif
#endif

/*
 * Set up a default for vaguely sane systems. The idea is that if
 * OMIT_UTMP is not defined, then at least one of the symbols which
 * enable particular forms of utmp processing should be, if only so
 * that a link error can warn you that you should have defined
 * OMIT_UTMP if you didn't want any. Currently HAVE_PUTUTLINE is
 * the only such symbol.
 */
#ifndef OMIT_UTMP
#if !defined HAVE_PUTUTLINE
#define HAVE_PUTUTLINE
#endif
#endif

typedef struct pty_tag *Pty;

/*
 * The pty_signal_pipe, along with the SIGCHLD handler, must be
 * process-global rather than session-specific.
 */
static int pty_signal_pipe[2] = { -1, -1 };   /* obviously bogus initial val */

struct pty_tag {
    Conf *conf;
    int master_fd, slave_fd;
    void *frontend;
    char name[FILENAME_MAX];
    pid_t child_pid;
    int term_width, term_height;
    int child_dead, finished;
    int exit_code;
    bufchain output_data;
};

/*
 * We store our pty backends in a tree sorted by master fd, so that
 * when we get an uxsel notification we know which backend instance
 * is the owner of the pty that caused it.
 */
static int pty_compare_by_fd(void *av, void *bv)
{
    Pty a = (Pty)av;
    Pty b = (Pty)bv;

    if (a->master_fd < b->master_fd)
	return -1;
    else if (a->master_fd > b->master_fd)
	return +1;
    return 0;
}

static int pty_find_by_fd(void *av, void *bv)
{
    int a = *(int *)av;
    Pty b = (Pty)bv;

    if (a < b->master_fd)
	return -1;
    else if (a > b->master_fd)
	return +1;
    return 0;
}

static tree234 *ptys_by_fd = NULL;

/*
 * We also have a tree sorted by child pid, so that when we wait()
 * in response to the signal we know which backend instance is the
 * owner of the process that caused the signal.
 */
static int pty_compare_by_pid(void *av, void *bv)
{
    Pty a = (Pty)av;
    Pty b = (Pty)bv;

    if (a->child_pid < b->child_pid)
	return -1;
    else if (a->child_pid > b->child_pid)
	return +1;
    return 0;
}

static int pty_find_by_pid(void *av, void *bv)
{
    pid_t a = *(pid_t *)av;
    Pty b = (Pty)bv;

    if (a < b->child_pid)
	return -1;
    else if (a > b->child_pid)
	return +1;
    return 0;
}

static tree234 *ptys_by_pid = NULL;

/*
 * If we are using pty_pre_init(), it will need to have already
 * allocated a pty structure, which we must then return from
 * pty_init() rather than allocating a new one. Here we store that
 * structure between allocation and use.
 * 
 * Note that although most of this module is entirely capable of
 * handling multiple ptys in a single process, pty_pre_init() is
 * fundamentally _dependent_ on there being at most one pty per
 * process, so the normal static-data constraints don't apply.
 * 
 * Likewise, since utmp is only used via pty_pre_init, it too must
 * be single-instance, so we can declare utmp-related variables
 * here.
 */
static Pty single_pty = NULL;

#ifndef OMIT_UTMP
static pid_t pty_utmp_helper_pid = -1;
static int pty_utmp_helper_pipe = -1;
static int pty_stamped_utmp;
static struct utmpx utmp_entry;
#endif

/*
 * pty_argv is a grievous hack to allow a proper argv to be passed
 * through from the Unix command line. Again, it doesn't really
 * make sense outside a one-pty-per-process setup.
 */
char **pty_argv;

static void pty_close(Pty pty);
static void pty_try_write(Pty pty);

#ifndef OMIT_UTMP
static void setup_utmp(char *ttyname, char *location)
{
#ifdef HAVE_LASTLOG
    struct lastlog lastlog_entry;
    FILE *lastlog;
#endif
    struct passwd *pw;
    struct timeval tv;

    pw = getpwuid(getuid());
    memset(&utmp_entry, 0, sizeof(utmp_entry));
    utmp_entry.ut_type = USER_PROCESS;
    utmp_entry.ut_pid = getpid();
    strncpy(utmp_entry.ut_line, ttyname+5, lenof(utmp_entry.ut_line));
    strncpy(utmp_entry.ut_id, ttyname+8, lenof(utmp_entry.ut_id));
    strncpy(utmp_entry.ut_user, pw->pw_name, lenof(utmp_entry.ut_user));
    strncpy(utmp_entry.ut_host, location, lenof(utmp_entry.ut_host));
    /*
     * Apparently there are some architectures where (struct
     * utmpx).ut_tv is not essentially struct timeval (e.g. Linux
     * amd64). Hence the temporary.
     */
    gettimeofday(&tv, NULL);
    utmp_entry.ut_tv.tv_sec = tv.tv_sec;
    utmp_entry.ut_tv.tv_usec = tv.tv_usec;

    setutxent();
    pututxline(&utmp_entry);
    endutxent();

    updwtmpx(WTMPX_FILE, &utmp_entry);

#ifdef HAVE_LASTLOG
    memset(&lastlog_entry, 0, sizeof(lastlog_entry));
    strncpy(lastlog_entry.ll_line, ttyname+5, lenof(lastlog_entry.ll_line));
    strncpy(lastlog_entry.ll_host, location, lenof(lastlog_entry.ll_host));
    time(&lastlog_entry.ll_time);
    if ((lastlog = fopen(LASTLOG_FILE, "r+")) != NULL) {
	fseek(lastlog, sizeof(lastlog_entry) * getuid(), SEEK_SET);
	fwrite(&lastlog_entry, 1, sizeof(lastlog_entry), lastlog);
	fclose(lastlog);
    }
#endif

    pty_stamped_utmp = 1;

}

static void cleanup_utmp(void)
{
    struct timeval tv;

    if (!pty_stamped_utmp)
	return;

    utmp_entry.ut_type = DEAD_PROCESS;
    memset(utmp_entry.ut_user, 0, lenof(utmp_entry.ut_user));
    gettimeofday(&tv, NULL);
    utmp_entry.ut_tv.tv_sec = tv.tv_sec;
    utmp_entry.ut_tv.tv_usec = tv.tv_usec;

    updwtmpx(WTMPX_FILE, &utmp_entry);

    memset(utmp_entry.ut_line, 0, lenof(utmp_entry.ut_line));
    utmp_entry.ut_tv.tv_sec = 0;
    utmp_entry.ut_tv.tv_usec = 0;

    setutxent();
    pututxline(&utmp_entry);
    endutxent();

    pty_stamped_utmp = 0;	       /* ensure we never double-cleanup */
}
#endif

static void sigchld_handler(int signum)
{
    if (write(pty_signal_pipe[1], "x", 1) <= 0)
	/* not much we can do about it */;
}

#ifndef OMIT_UTMP
static void fatal_sig_handler(int signum)
{
    putty_signal(signum, SIG_DFL);
    cleanup_utmp();
    raise(signum);
}
#endif

static int pty_open_slave(Pty pty)
{
    if (pty->slave_fd < 0) {
	pty->slave_fd = open(pty->name, O_RDWR);
        cloexec(pty->slave_fd);
    }

    return pty->slave_fd;
}

static void pty_open_master(Pty pty)
{
#ifdef BSD_PTYS
    const char chars1[] = "pqrstuvwxyz";
    const char chars2[] = "0123456789abcdef";
    const char *p1, *p2;
    char master_name[20];
    struct group *gp;

    for (p1 = chars1; *p1; p1++)
	for (p2 = chars2; *p2; p2++) {
	    sprintf(master_name, "/dev/pty%c%c", *p1, *p2);
	    pty->master_fd = open(master_name, O_RDWR);
	    if (pty->master_fd >= 0) {
		if (geteuid() == 0 ||
		    access(master_name, R_OK | W_OK) == 0) {
		    /*
		     * We must also check at this point that we are
		     * able to open the slave side of the pty. We
		     * wouldn't want to allocate the wrong master,
		     * get all the way down to forking, and _then_
		     * find we're unable to open the slave.
		     */
		    strcpy(pty->name, master_name);
		    pty->name[5] = 't'; /* /dev/ptyXX -> /dev/ttyXX */

                    cloexec(pty->master_fd);

		    if (pty_open_slave(pty) >= 0 &&
			access(pty->name, R_OK | W_OK) == 0)
			goto got_one;
		    if (pty->slave_fd > 0)
			close(pty->slave_fd);
		    pty->slave_fd = -1;
		}
		close(pty->master_fd);
	    }
	}

    /* If we get here, we couldn't get a tty at all. */
    fprintf(stderr, "pterm: unable to open a pseudo-terminal device\n");
    exit(1);

    got_one:

    /* We need to chown/chmod the /dev/ttyXX device. */
    gp = getgrnam("tty");
    chown(pty->name, getuid(), gp ? gp->gr_gid : -1);
    chmod(pty->name, 0600);
#else

    const int flags = O_RDWR
#ifdef O_NOCTTY
        | O_NOCTTY
#endif
        ;

#ifdef HAVE_POSIX_OPENPT
    pty->master_fd = posix_openpt(flags);

    if (pty->master_fd < 0) {
	perror("posix_openpt");
	exit(1);
    }
#else
    pty->master_fd = open("/dev/ptmx", flags);

    if (pty->master_fd < 0) {
	perror("/dev/ptmx: open");
	exit(1);
    }
#endif

    if (grantpt(pty->master_fd) < 0) {
	perror("grantpt");
	exit(1);
    }
    
    if (unlockpt(pty->master_fd) < 0) {
	perror("unlockpt");
	exit(1);
    }

    cloexec(pty->master_fd);

    pty->name[FILENAME_MAX-1] = '\0';
    strncpy(pty->name, ptsname(pty->master_fd), FILENAME_MAX-1);
#endif

    nonblock(pty->master_fd);

    if (!ptys_by_fd)
	ptys_by_fd = newtree234(pty_compare_by_fd);
    add234(ptys_by_fd, pty);
}

/*
 * Pre-initialisation. This is here to get around the fact that GTK
 * doesn't like being run in setuid/setgid programs (probably
 * sensibly). So before we initialise GTK - and therefore before we
 * even process the command line - we check to see if we're running
 * set[ug]id. If so, we open our pty master _now_, chown it as
 * necessary, and drop privileges. We can always close it again
 * later. If we're potentially going to be doing utmp as well, we
 * also fork off a utmp helper process and communicate with it by
 * means of a pipe; the utmp helper will keep privileges in order
 * to clean up utmp when we exit (i.e. when its end of our pipe
 * closes).
 */
void pty_pre_init(void)
{
    Pty pty;

#ifndef OMIT_UTMP
    pid_t pid;
    int pipefd[2];
#endif

    pty = single_pty = snew(struct pty_tag);
    pty->conf = NULL;
    bufchain_init(&pty->output_data);

    /* set the child signal handler straight away; it needs to be set
     * before we ever fork. */
    putty_signal(SIGCHLD, sigchld_handler);
    pty->master_fd = pty->slave_fd = -1;
#ifndef OMIT_UTMP
    pty_stamped_utmp = FALSE;
#endif

    if (geteuid() != getuid() || getegid() != getgid()) {
	pty_open_master(pty);

#ifndef OMIT_UTMP
        /*
         * Fork off the utmp helper.
         */
        if (pipe(pipefd) < 0) {
            perror("pterm: pipe");
            exit(1);
        }
        cloexec(pipefd[0]);
        cloexec(pipefd[1]);
        pid = fork();
        if (pid < 0) {
            perror("pterm: fork");
            exit(1);
        } else if (pid == 0) {
            char display[128], buffer[128];
            int dlen, ret;

            close(pipefd[1]);
            /*
             * Now sit here until we receive a display name from the
             * other end of the pipe, and then stamp utmp. Unstamp utmp
             * again, and exit, when the pipe closes.
             */

            dlen = 0;
            while (1) {
	    
                ret = read(pipefd[0], buffer, lenof(buffer));
                if (ret <= 0) {
                    cleanup_utmp();
                    _exit(0);
                } else if (!pty_stamped_utmp) {
                    if (dlen < lenof(display))
                        memcpy(display+dlen, buffer,
                               min(ret, lenof(display)-dlen));
                    if (buffer[ret-1] == '\0') {
                        /*
                         * Now we have a display name. NUL-terminate
                         * it, and stamp utmp.
                         */
                        display[lenof(display)-1] = '\0';
                        /*
                         * Trap as many fatal signals as we can in the
                         * hope of having the best possible chance to
                         * clean up utmp before termination. We are
                         * unfortunately unprotected against SIGKILL,
                         * but that's life.
                         */
                        putty_signal(SIGHUP, fatal_sig_handler);
                        putty_signal(SIGINT, fatal_sig_handler);
                        putty_signal(SIGQUIT, fatal_sig_handler);
                        putty_signal(SIGILL, fatal_sig_handler);
                        putty_signal(SIGABRT, fatal_sig_handler);
                        putty_signal(SIGFPE, fatal_sig_handler);
                        putty_signal(SIGPIPE, fatal_sig_handler);
                        putty_signal(SIGALRM, fatal_sig_handler);
                        putty_signal(SIGTERM, fatal_sig_handler);
                        putty_signal(SIGSEGV, fatal_sig_handler);
                        putty_signal(SIGUSR1, fatal_sig_handler);
                        putty_signal(SIGUSR2, fatal_sig_handler);
#ifdef SIGBUS
                        putty_signal(SIGBUS, fatal_sig_handler);
#endif
#ifdef SIGPOLL
                        putty_signal(SIGPOLL, fatal_sig_handler);
#endif
#ifdef SIGPROF
                        putty_signal(SIGPROF, fatal_sig_handler);
#endif
#ifdef SIGSYS
                        putty_signal(SIGSYS, fatal_sig_handler);
#endif
#ifdef SIGTRAP
                        putty_signal(SIGTRAP, fatal_sig_handler);
#endif
#ifdef SIGVTALRM
                        putty_signal(SIGVTALRM, fatal_sig_handler);
#endif
#ifdef SIGXCPU
                        putty_signal(SIGXCPU, fatal_sig_handler);
#endif
#ifdef SIGXFSZ
                        putty_signal(SIGXFSZ, fatal_sig_handler);
#endif
#ifdef SIGIO
                        putty_signal(SIGIO, fatal_sig_handler);
#endif
                        setup_utmp(pty->name, display);
                    }
                }
            }
        } else {
            close(pipefd[0]);
            pty_utmp_helper_pid = pid;
            pty_utmp_helper_pipe = pipefd[1];
        }
#endif
    }

    /* Drop privs. */
    {
#ifndef HAVE_NO_SETRESUID
	int gid = getgid(), uid = getuid();
	int setresgid(gid_t, gid_t, gid_t);
	int setresuid(uid_t, uid_t, uid_t);
	if (setresgid(gid, gid, gid) < 0) {
            perror("setresgid");
            exit(1);
        }
	if (setresuid(uid, uid, uid) < 0) {
            perror("setresuid");
            exit(1);
        }
#else
	if (setgid(getgid()) < 0) {
            perror("setgid");
            exit(1);
        }
	if (setuid(getuid()) < 0) {
            perror("setuid");
            exit(1);
        }
#endif
    }
}

int pty_real_select_result(Pty pty, int event, int status)
{
    char buf[4096];
    int ret;
    int finished = FALSE;

    if (event < 0) {
	/*
	 * We've been called because our child process did
	 * something. `status' tells us what.
	 */
	if ((WIFEXITED(status) || WIFSIGNALED(status))) {
	    /*
	     * The primary child process died. We could keep
	     * the terminal open for remaining subprocesses to
	     * output to, but conventional wisdom seems to feel
	     * that that's the Wrong Thing for an xterm-alike,
	     * so we bail out now (though we don't necessarily
	     * _close_ the window, depending on the state of
	     * Close On Exit). This would be easy enough to
	     * change or make configurable if necessary.
	     */
	    pty->exit_code = status;
	    pty->child_dead = TRUE;
	    del234(ptys_by_pid, pty);
	    finished = TRUE;
	}
    } else {
	if (event == 1) {

	    ret = read(pty->master_fd, buf, sizeof(buf));

	    /*
	     * Clean termination condition is that either ret == 0, or ret
	     * < 0 and errno == EIO. Not sure why the latter, but it seems
	     * to happen. Boo.
	     */
	    if (ret == 0 || (ret < 0 && errno == EIO)) {
		/*
		 * We assume a clean exit if the pty has closed but the
		 * actual child process hasn't. The only way I can
		 * imagine this happening is if it detaches itself from
		 * the pty and goes daemonic - in which case the
		 * expected usage model would precisely _not_ be for
		 * the pterm window to hang around!
		 */
		finished = TRUE;
		if (!pty->child_dead)
		    pty->exit_code = 0;
	    } else if (ret < 0) {
		perror("read pty master");
		exit(1);
	    } else if (ret > 0) {
		from_backend(pty->frontend, 0, buf, ret);
	    }
	} else if (event == 2) {
            /*
             * Attempt to send data down the pty.
             */
            pty_try_write(pty);
        }
    }

    if (finished && !pty->finished) {
	int close_on_exit;

	uxsel_del(pty->master_fd);
	pty_close(pty);
	pty->master_fd = -1;

	pty->finished = TRUE;

	/*
	 * This is a slight layering-violation sort of hack: only
	 * if we're not closing on exit (COE is set to Never, or to
	 * Only On Clean and it wasn't a clean exit) do we output a
	 * `terminated' message.
	 */
	close_on_exit = conf_get_int(pty->conf, CONF_close_on_exit);
	if (close_on_exit == FORCE_OFF ||
	    (close_on_exit == AUTO && pty->exit_code != 0)) {
	    char message[512];
            message[0] = '\0';
	    if (WIFEXITED(pty->exit_code))
		sprintf(message, "\r\n[pterm: process terminated with exit"
			" code %d]\r\n", WEXITSTATUS(pty->exit_code));
	    else if (WIFSIGNALED(pty->exit_code))
#ifdef HAVE_NO_STRSIGNAL
		sprintf(message, "\r\n[pterm: process terminated on signal"
			" %d]\r\n", WTERMSIG(pty->exit_code));
#else
		sprintf(message, "\r\n[pterm: process terminated on signal"
			" %d (%.400s)]\r\n", WTERMSIG(pty->exit_code),
			strsignal(WTERMSIG(pty->exit_code)));
#endif
	    from_backend(pty->frontend, 0, message, strlen(message));
	}

	notify_remote_exit(pty->frontend);
    }

    return !finished;
}

int pty_select_result(int fd, int event)
{
    int ret = TRUE;
    Pty pty;

    if (fd == pty_signal_pipe[0]) {
	pid_t pid;
	int status;
	char c[1];

	if (read(pty_signal_pipe[0], c, 1) <= 0)
	    /* ignore error */;
	/* ignore its value; it'll be `x' */

	do {
	    pid = waitpid(-1, &status, WNOHANG);

	    pty = find234(ptys_by_pid, &pid, pty_find_by_pid);

	    if (pty)
		ret = ret && pty_real_select_result(pty, -1, status);
	} while (pid > 0);
    } else {
	pty = find234(ptys_by_fd, &fd, pty_find_by_fd);

	if (pty)
	    ret = ret && pty_real_select_result(pty, event, 0);
    }

    return ret;
}

static void pty_uxsel_setup(Pty pty)
{
    int rwx;

    rwx = 1;                           /* always want to read from pty */
    if (bufchain_size(&pty->output_data))
        rwx |= 2;                      /* might also want to write to it */
    uxsel_set(pty->master_fd, rwx, pty_select_result);

    /*
     * In principle this only needs calling once for all pty
     * backend instances, but it's simplest just to call it every
     * time; uxsel won't mind.
     */
    uxsel_set(pty_signal_pipe[0], 1, pty_select_result);
}

/*
 * Called to set up the pty.
 * 
 * Returns an error message, or NULL on success.
 *
 * Also places the canonical host name into `realhost'. It must be
 * freed by the caller.
 */
static const char *pty_init(void *frontend, void **backend_handle, Conf *conf,
			    char *host, int port, char **realhost, int nodelay,
			    int keepalive)
{
    int slavefd;
    pid_t pid, pgrp;
#ifndef NOT_X_WINDOWS		       /* for Mac OS X native compilation */
    long windowid;
#endif
    Pty pty;

    if (single_pty) {
	pty = single_pty;
        assert(pty->conf == NULL);
    } else {
	pty = snew(struct pty_tag);
	pty->master_fd = pty->slave_fd = -1;
#ifndef OMIT_UTMP
	pty_stamped_utmp = FALSE;
#endif
    }

    pty->frontend = frontend;
    *backend_handle = NULL;	       /* we can't sensibly use this, sadly */

    pty->conf = conf_copy(conf);
    pty->term_width = conf_get_int(conf, CONF_width);
    pty->term_height = conf_get_int(conf, CONF_height);

    if (pty->master_fd < 0)
	pty_open_master(pty);

    /*
     * Set up configuration-dependent termios settings on the new pty.
     */
    {
	struct termios attrs;
	tcgetattr(pty->master_fd, &attrs);

        /*
         * Set the backspace character to be whichever of ^H and ^? is
         * specified by bksp_is_delete.
         */
	attrs.c_cc[VERASE] = conf_get_int(conf, CONF_bksp_is_delete)
	    ? '\177' : '\010';

        /*
         * Set the IUTF8 bit iff the character set is UTF-8.
         */
#ifdef IUTF8
        if (frontend_is_utf8(frontend))
            attrs.c_iflag |= IUTF8;
        else
            attrs.c_iflag &= ~IUTF8;
#endif

	tcsetattr(pty->master_fd, TCSANOW, &attrs);
    }

#ifndef OMIT_UTMP
    /*
     * Stamp utmp (that is, tell the utmp helper process to do so),
     * or not.
     */
    if (pty_utmp_helper_pipe >= 0) {   /* if it's < 0, we can't anyway */
        if (!conf_get_int(conf, CONF_stamp_utmp)) {
            close(pty_utmp_helper_pipe);   /* just let the child process die */
            pty_utmp_helper_pipe = -1;
        } else {
            char *location = get_x_display(pty->frontend);
            int len = strlen(location)+1, pos = 0;   /* +1 to include NUL */
            while (pos < len) {
                int ret = write(pty_utmp_helper_pipe, location+pos, len - pos);
                if (ret < 0) {
                    perror("pterm: writing to utmp helper process");
                    close(pty_utmp_helper_pipe);   /* arrgh, just give up */
                    pty_utmp_helper_pipe = -1;
                    break;
                }
                pos += ret;
            }
	}
    }
#endif

#ifndef NOT_X_WINDOWS		       /* for Mac OS X native compilation */
    windowid = get_windowid(pty->frontend);
#endif

    /*
     * Fork and execute the command.
     */
    pid = fork();
    if (pid < 0) {
	perror("fork");
	exit(1);
    }

    if (pid == 0) {
	/*
	 * We are the child.
	 */

	slavefd = pty_open_slave(pty);
	if (slavefd < 0) {
	    perror("slave pty: open");
	    _exit(1);
	}

	close(pty->master_fd);
	noncloexec(slavefd);
	dup2(slavefd, 0);
	dup2(slavefd, 1);
	dup2(slavefd, 2);
	close(slavefd);
	setsid();
#ifdef TIOCSCTTY
	ioctl(0, TIOCSCTTY, 1);
#endif
	pgrp = getpid();
	tcsetpgrp(0, pgrp);
	setpgid(pgrp, pgrp);
        {
            int ptyfd = open(pty->name, O_WRONLY, 0);
            if (ptyfd >= 0)
                close(ptyfd);
        }
	setpgid(pgrp, pgrp);
	{
	    char *term_env_var = dupprintf("TERM=%s",
					   conf_get_str(conf, CONF_termtype));
	    putenv(term_env_var);
	    /* We mustn't free term_env_var, as putenv links it into the
	     * environment in place.
	     */
	}
#ifndef NOT_X_WINDOWS		       /* for Mac OS X native compilation */
	{
	    char *windowid_env_var = dupprintf("WINDOWID=%ld", windowid);
	    putenv(windowid_env_var);
	    /* We mustn't free windowid_env_var, as putenv links it into the
	     * environment in place.
	     */
	}
        {
            /*
             * In case we were invoked with a --display argument that
             * doesn't match DISPLAY in our actual environment, we
             * should set DISPLAY for processes running inside the
             * terminal to match the display the terminal itself is
             * on.
             */
            const char *x_display = get_x_display(pty->frontend);
            char *x_display_env_var = dupprintf("DISPLAY=%s", x_display);
            putenv(x_display_env_var);
            /* As above, we don't free this. */
        }
#endif
	{
	    char *key, *val;

	    for (val = conf_get_str_strs(conf, CONF_environmt, NULL, &key);
		 val != NULL;
		 val = conf_get_str_strs(conf, CONF_environmt, key, &key)) {
		char *varval = dupcat(key, "=", val, NULL);
		putenv(varval);
		/*
		 * We must not free varval, since putenv links it
		 * into the environment _in place_. Weird, but
		 * there we go. Memory usage will be rationalised
		 * as soon as we exec anyway.
		 */
	    }
	}

	/*
	 * SIGINT, SIGQUIT and SIGPIPE may have been set to ignored by
	 * our parent, particularly by things like sh -c 'pterm &' and
	 * some window or session managers. SIGCHLD, meanwhile, was
	 * blocked during pt_main() startup. Reverse all this for our
	 * child process.
	 */
	putty_signal(SIGINT, SIG_DFL);
	putty_signal(SIGQUIT, SIG_DFL);
	putty_signal(SIGPIPE, SIG_DFL);
	block_signal(SIGCHLD, 0);
	if (pty_argv) {
            /*
             * Exec the exact argument list we were given.
             */
	    execvp(pty_argv[0], pty_argv);
            /*
             * If that fails, and if we had exactly one argument, pass
             * that argument to $SHELL -c.
             *
             * This arranges that we can _either_ follow 'pterm -e'
             * with a list of argv elements to be fed directly to
             * exec, _or_ with a single argument containing a command
             * to be parsed by a shell (but, in cases of doubt, the
             * former is more reliable).
             *
             * A quick survey of other terminal emulators' -e options
             * (as of Debian squeeze) suggests that:
             *
             *  - xterm supports both modes, more or less like this
             *  - gnome-terminal will only accept a one-string shell command
             *  - Eterm, kterm and rxvt will only accept a list of
             *    argv elements (as did older versions of pterm).
             *
             * It therefore seems important to support both usage
             * modes in order to be a drop-in replacement for either
             * xterm or gnome-terminal, and hence for anyone's
             * plausible uses of the Debian-style alias
             * 'x-terminal-emulator'...
             */
            if (pty_argv[1] == NULL) {
                char *shell = getenv("SHELL");
                if (shell)
                    execl(shell, shell, "-c", pty_argv[0], (void *)NULL);
            }
        } else {
	    char *shell = getenv("SHELL");
	    char *shellname;
	    if (conf_get_int(conf, CONF_login_shell)) {
		char *p = strrchr(shell, '/');
		shellname = snewn(2+strlen(shell), char);
		p = p ? p+1 : shell;
		sprintf(shellname, "-%s", p);
	    } else
		shellname = shell;
	    execl(getenv("SHELL"), shellname, (void *)NULL);
	}

	/*
	 * If we're here, exec has gone badly foom.
	 */
	perror("exec");
	_exit(127);
    } else {
	pty->child_pid = pid;
	pty->child_dead = FALSE;
	pty->finished = FALSE;
	if (pty->slave_fd > 0)
	    close(pty->slave_fd);
	if (!ptys_by_pid)
	    ptys_by_pid = newtree234(pty_compare_by_pid);
	add234(ptys_by_pid, pty);
    }

    if (pty_signal_pipe[0] < 0) {
	if (pipe(pty_signal_pipe) < 0) {
	    perror("pipe");
	    exit(1);
	}
	cloexec(pty_signal_pipe[0]);
	cloexec(pty_signal_pipe[1]);
    }
    pty_uxsel_setup(pty);

    *backend_handle = pty;

    *realhost = dupstr("");

    return NULL;
}

static void pty_reconfig(void *handle, Conf *conf)
{
    Pty pty = (Pty)handle;
    /*
     * We don't have much need to reconfigure this backend, but
     * unfortunately we do need to pick up the setting of Close On
     * Exit so we know whether to give a `terminated' message.
     */
    conf_copy_into(pty->conf, conf);
}

/*
 * Stub routine (never called in pterm).
 */
static void pty_free(void *handle)
{
    Pty pty = (Pty)handle;

    /* Either of these may fail `not found'. That's fine with us. */
    del234(ptys_by_pid, pty);
    del234(ptys_by_fd, pty);

    conf_free(pty->conf);
    pty->conf = NULL;

    if (pty == single_pty) {
        /*
         * Leave this structure around in case we need to Restart
         * Session.
         */
    } else {
        sfree(pty);
    }
}

static void pty_try_write(Pty pty)
{
    void *data;
    int len, ret;

    assert(pty->master_fd >= 0);

    while (bufchain_size(&pty->output_data) > 0) {
        bufchain_prefix(&pty->output_data, &data, &len);
	ret = write(pty->master_fd, data, len);

        if (ret < 0 && (errno == EWOULDBLOCK)) {
            /*
             * We've sent all we can for the moment.
             */
            break;
        }
	if (ret < 0) {
	    perror("write pty master");
	    exit(1);
	}
	bufchain_consume(&pty->output_data, ret);
    }

    pty_uxsel_setup(pty);
}

/*
 * Called to send data down the pty.
 */
static int pty_send(void *handle, char *buf, int len)
{
    Pty pty = (Pty)handle;

    if (pty->master_fd < 0)
	return 0;                      /* ignore all writes if fd closed */

    bufchain_add(&pty->output_data, buf, len);
    pty_try_write(pty);

    return bufchain_size(&pty->output_data);
}

static void pty_close(Pty pty)
{
    if (pty->master_fd >= 0) {
	close(pty->master_fd);
	pty->master_fd = -1;
    }
#ifndef OMIT_UTMP
    if (pty_utmp_helper_pipe >= 0) {
	close(pty_utmp_helper_pipe);   /* this causes utmp to be cleaned up */
	pty_utmp_helper_pipe = -1;
    }
#endif
}

/*
 * Called to query the current socket sendability status.
 */
static int pty_sendbuffer(void *handle)
{
    /* Pty pty = (Pty)handle; */
    return 0;
}

/*
 * Called to set the size of the window
 */
static void pty_size(void *handle, int width, int height)
{
    Pty pty = (Pty)handle;
    struct winsize size;

    pty->term_width = width;
    pty->term_height = height;

    size.ws_row = (unsigned short)pty->term_height;
    size.ws_col = (unsigned short)pty->term_width;
    size.ws_xpixel = (unsigned short) pty->term_width *
	font_dimension(pty->frontend, 0);
    size.ws_ypixel = (unsigned short) pty->term_height *
	font_dimension(pty->frontend, 1);
    ioctl(pty->master_fd, TIOCSWINSZ, (void *)&size);
    return;
}

/*
 * Send special codes.
 */
static void pty_special(void *handle, Telnet_Special code)
{
    /* Pty pty = (Pty)handle; */
    /* Do nothing! */
    return;
}

/*
 * Return a list of the special codes that make sense in this
 * protocol.
 */
static const struct telnet_special *pty_get_specials(void *handle)
{
    /* Pty pty = (Pty)handle; */
    /*
     * Hmm. When I get round to having this actually usable, it
     * might be quite nice to have the ability to deliver a few
     * well chosen signals to the child process - SIGINT, SIGTERM,
     * SIGKILL at least.
     */
    return NULL;
}

static int pty_connected(void *handle)
{
    /* Pty pty = (Pty)handle; */
    return TRUE;
}

static int pty_sendok(void *handle)
{
    /* Pty pty = (Pty)handle; */
    return 1;
}

static void pty_unthrottle(void *handle, int backlog)
{
    /* Pty pty = (Pty)handle; */
    /* do nothing */
}

static int pty_ldisc(void *handle, int option)
{
    /* Pty pty = (Pty)handle; */
    return 0;			       /* neither editing nor echoing */
}

static void pty_provide_ldisc(void *handle, void *ldisc)
{
    /* Pty pty = (Pty)handle; */
    /* This is a stub. */
}

static void pty_provide_logctx(void *handle, void *logctx)
{
    /* Pty pty = (Pty)handle; */
    /* This is a stub. */
}

static int pty_exitcode(void *handle)
{
    Pty pty = (Pty)handle;
    if (!pty->finished)
	return -1;		       /* not dead yet */
    else
	return pty->exit_code;
}

static int pty_cfg_info(void *handle)
{
    /* Pty pty = (Pty)handle; */
    return 0;
}

Backend pty_backend = {
    pty_init,
    pty_free,
    pty_reconfig,
    pty_send,
    pty_sendbuffer,
    pty_size,
    pty_special,
    pty_get_specials,
    pty_connected,
    pty_exitcode,
    pty_sendok,
    pty_ldisc,
    pty_provide_ldisc,
    pty_provide_logctx,
    pty_unthrottle,
    pty_cfg_info,
    "pty",
    -1,
    0
};
