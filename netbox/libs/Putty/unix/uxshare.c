/*
 * Unix implementation of SSH connection-sharing IPC setup.
 */

#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <limits.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/file.h>

#define DEFINE_PLUG_METHOD_MACROS
#include "tree234.h"
#include "putty.h"
#include "network.h"
#include "proxy.h"
#include "ssh.h"

#define CONNSHARE_SOCKETDIR_PREFIX "/tmp/putty-connshare"
#define SALT_FILENAME "salt"
#define SALT_SIZE 64

/*
 * Functions provided by uxnet.c to help connection sharing.
 */
SockAddr unix_sock_addr(const char *path);
Socket new_unix_listener(SockAddr listenaddr, Plug plug);

static char *make_parentdir_name(void)
{
    char *username, *parent;

    username = get_username();
    parent = dupprintf("%s.%s", CONNSHARE_SOCKETDIR_PREFIX, username);
    sfree(username);
    assert(*parent == '/');

    return parent;
}

static char *make_dir_and_check_ours(const char *dirname)
{
    struct stat st;

    /*
     * Create the directory. We might have created it before, so
     * EEXIST is an OK error; but anything else is doom.
     */
    if (mkdir(dirname, 0700) < 0 && errno != EEXIST)
        return dupprintf("%s: mkdir: %s", dirname, strerror(errno));

    /*
     * Now check that that directory is _owned by us_ and not writable
     * by anybody else. This protects us against somebody else
     * previously having created the directory in a way that's
     * writable to us, and thus manipulating us into creating the
     * actual socket in a directory they can see so that they can
     * connect to it and use our authenticated SSH sessions.
     */
    if (stat(dirname, &st) < 0)
        return dupprintf("%s: stat: %s", dirname, strerror(errno));
    if (st.st_uid != getuid())
        return dupprintf("%s: directory owned by uid %d, not by us",
                         dirname, st.st_uid);
    if ((st.st_mode & 077) != 0)
        return dupprintf("%s: directory has overgenerous permissions %03o"
                         " (expected 700)", dirname, st.st_mode & 0777);

    return NULL;
}

static char *make_dirname(const char *pi_name, char **logtext)
{
    char *name, *parentdirname, *dirname, *err;

    /*
     * First, create the top-level directory for all shared PuTTY
     * connections owned by this user.
     */
    parentdirname = make_parentdir_name();
    if ((err = make_dir_and_check_ours(parentdirname)) != NULL) {
        *logtext = err;
        sfree(parentdirname);
        return NULL;
    }

    /*
     * Transform the platform-independent version of the connection
     * identifier into the name we'll actually use for the directory
     * containing the Unix socket.
     *
     * We do this by hashing the identifier with some user-specific
     * secret information, to avoid the privacy leak of having
     * "user@host" strings show up in 'netstat -x'. (Irritatingly, the
     * full pathname of a Unix-domain socket _does_ show up in the
     * 'netstat -x' output, at least on Linux, even if that socket is
     * in a directory not readable to the user running netstat. You'd
     * think putting things inside an 0700 directory would hide their
     * names from other users, but no.)
     *
     * The secret information we use to salt the hash lives in a file
     * inside the top-level directory we just created, so we must
     * first create that file (with some fresh random data in it) if
     * it's not already been done by a previous PuTTY.
     */
    {
        unsigned char saltbuf[SALT_SIZE];
        char *saltname;
        int saltfd, i, ret;

        saltname = dupprintf("%s/%s", parentdirname, SALT_FILENAME);
        saltfd = open(saltname, O_RDONLY);
        if (saltfd < 0) {
            char *tmpname;
            int pid;

            if (errno != ENOENT) {
                *logtext = dupprintf("%s: open: %s", saltname,
                                     strerror(errno));
                sfree(saltname);
                sfree(parentdirname);
                return NULL;
            }

            /*
             * The salt file doesn't already exist, so try to create
             * it. Another process may be attempting the same thing
             * simultaneously, so we must do this carefully: we write
             * a salt file under a different name, then hard-link it
             * into place, which guarantees that we won't change the
             * contents of an existing salt file.
             */
            pid = getpid();
            for (i = 0;; i++) {
                tmpname = dupprintf("%s/%s.tmp.%d.%d",
                                    parentdirname, SALT_FILENAME, pid, i);
                saltfd = open(tmpname, O_WRONLY | O_EXCL | O_CREAT, 0400);
                if (saltfd >= 0)
                    break;
                if (errno != EEXIST) {
                    *logtext = dupprintf("%s: open: %s", tmpname,
                                         strerror(errno));
                    sfree(tmpname);
                    sfree(saltname);
                    sfree(parentdirname);
                    return NULL;
                }
                sfree(tmpname);        /* go round and try again with i+1 */
            }
            /*
             * Invent some random data.
             */
            for (i = 0; i < SALT_SIZE; i++) {
                saltbuf[i] = random_byte();
            }
            ret = write(saltfd, saltbuf, SALT_SIZE);
            /* POSIX atomicity guarantee: because we wrote less than
             * PIPE_BUF bytes, the write either completed in full or
             * failed. */
            assert(SALT_SIZE < PIPE_BUF);
            assert(ret < 0 || ret == SALT_SIZE);
            if (ret < 0) {
                close(saltfd);
                *logtext = dupprintf("%s: write: %s", tmpname,
                                     strerror(errno));
                sfree(tmpname);
                sfree(saltname);
                sfree(parentdirname);
                return NULL;
            }
            if (close(saltfd) < 0) {
                *logtext = dupprintf("%s: close: %s", tmpname,
                                     strerror(errno));
                sfree(tmpname);
                sfree(saltname);
                sfree(parentdirname);
                return NULL;
            }

            /*
             * Now attempt to hard-link our temp file into place. We
             * tolerate EEXIST as an outcome, because that just means
             * another PuTTY got their attempt in before we did (and
             * we only care that there is a valid salt file we can
             * agree on, no matter who created it).
             */
            if (link(tmpname, saltname) < 0 && errno != EEXIST) {
                *logtext = dupprintf("%s: link: %s", saltname,
                                     strerror(errno));
                sfree(tmpname);
                sfree(saltname);
                sfree(parentdirname);
                return NULL;
            }

            /*
             * Whether that succeeded or not, get rid of our temp file.
             */
            if (unlink(tmpname) < 0) {
                *logtext = dupprintf("%s: unlink: %s", tmpname,
                                     strerror(errno));
                sfree(tmpname);
                sfree(saltname);
                sfree(parentdirname);
                return NULL;
            }

            /*
             * And now we've arranged for there to be a salt file, so
             * we can try to open it for reading again and this time
             * expect it to work.
             */
            sfree(tmpname);

            saltfd = open(saltname, O_RDONLY);
            if (saltfd < 0) {
                *logtext = dupprintf("%s: open: %s", saltname,
                                     strerror(errno));
                sfree(saltname);
                sfree(parentdirname);
                return NULL;
            }
        }

        for (i = 0; i < SALT_SIZE; i++) {
            ret = read(saltfd, saltbuf, SALT_SIZE);
            if (ret <= 0) {
                close(saltfd);
                *logtext = dupprintf("%s: read: %s", saltname,
                                     ret == 0 ? "unexpected EOF" :
                                     strerror(errno));
                sfree(saltname);
                sfree(parentdirname);
                return NULL;
            }
            assert(0 < ret && ret <= SALT_SIZE - i);
            i += ret;
        }

        close(saltfd);
        sfree(saltname);

        /*
         * Now we've got our salt, hash it with the connection
         * identifier to produce our actual socket name.
         */
        {
            SHA256_State sha;
            unsigned len;
            unsigned char lenbuf[4];
            unsigned char digest[32];
            char retbuf[65];

            SHA256_Init(&sha);
            PUT_32BIT(lenbuf, SALT_SIZE);
            SHA256_Bytes(&sha, lenbuf, 4);
            SHA256_Bytes(&sha, saltbuf, SALT_SIZE);
            len = strlen(pi_name);
            PUT_32BIT(lenbuf, len);
            SHA256_Bytes(&sha, lenbuf, 4);
            SHA256_Bytes(&sha, pi_name, len);
            SHA256_Final(&sha, digest);

            /*
             * And make it printable.
             */
            for (i = 0; i < 32; i++) {
                sprintf(retbuf + 2*i, "%02x", digest[i]);
                /* the last of those will also write the trailing NUL */
            }

            name = dupstr(retbuf);
        }

        smemclr(saltbuf, sizeof(saltbuf));
    }

    dirname = dupprintf("%s/%s", parentdirname, name);
    sfree(parentdirname);
    sfree(name);

    return dirname;
}

int platform_ssh_share(const char *pi_name, Conf *conf,
                       Plug downplug, Plug upplug, Socket *sock,
                       char **logtext, char **ds_err, char **us_err,
                       int can_upstream, int can_downstream)
{
    char *dirname, *lockname, *sockname, *err;
    int lockfd;
    Socket retsock;

    /*
     * Sort out what we're going to call the directory in which we
     * keep the socket. This has the side effect of potentially
     * creating its top-level containing dir and/or the salt file
     * within that, if they don't already exist.
     */
    dirname = make_dirname(pi_name, logtext);
    if (!dirname) {
        return SHARE_NONE;
    }

    /*
     * Now make sure the subdirectory exists.
     */
    if ((err = make_dir_and_check_ours(dirname)) != NULL) {
        *logtext = err;
        sfree(dirname);
        return SHARE_NONE;
    }

    /*
     * Acquire a lock on a file in that directory.
     */
    lockname = dupcat(dirname, "/lock", (char *)NULL);
    lockfd = open(lockname, O_CREAT | O_RDWR | O_TRUNC, 0600);
    if (lockfd < 0) {
        *logtext = dupprintf("%s: open: %s", lockname, strerror(errno));
        sfree(dirname);
        sfree(lockname);
        return SHARE_NONE;
    }
    if (flock(lockfd, LOCK_EX) < 0) {
        *logtext = dupprintf("%s: flock(LOCK_EX): %s",
                             lockname, strerror(errno));
        sfree(dirname);
        sfree(lockname);
        close(lockfd);
        return SHARE_NONE;
    }

    sockname = dupprintf("%s/socket", dirname);

    *logtext = NULL;

    if (can_downstream) {
        retsock = new_connection(unix_sock_addr(sockname),
                                 "", 0, 0, 1, 0, 0, downplug, conf);
        if (sk_socket_error(retsock) == NULL) {
            sfree(*logtext);
            *logtext = sockname;
            *sock = retsock;
            sfree(dirname);
            sfree(lockname);
            close(lockfd);
            return SHARE_DOWNSTREAM;
        }
        sfree(*ds_err);
        *ds_err = dupprintf("%s: %s", sockname, sk_socket_error(retsock));
        sk_close(retsock);
    }

    if (can_upstream) {
        retsock = new_unix_listener(unix_sock_addr(sockname), upplug);
        if (sk_socket_error(retsock) == NULL) {
            sfree(*logtext);
            *logtext = sockname;
            *sock = retsock;
            sfree(dirname);
            sfree(lockname);
            close(lockfd);
            return SHARE_UPSTREAM;
        }
        sfree(*us_err);
        *us_err = dupprintf("%s: %s", sockname, sk_socket_error(retsock));
        sk_close(retsock);
    }

    /* One of the above clauses ought to have happened. */
    assert(*logtext || *ds_err || *us_err);

    sfree(dirname);
    sfree(lockname);
    sfree(sockname);
    close(lockfd);
    return SHARE_NONE;
}

void platform_ssh_share_cleanup(const char *name)
{
    char *dirname, *filename, *logtext;

    dirname = make_dirname(name, &logtext);
    if (!dirname) {
        sfree(logtext);                /* we can't do much with this */
        return;
    }

    filename = dupcat(dirname, "/socket", (char *)NULL);
    remove(filename);
    sfree(filename);

    filename = dupcat(dirname, "/lock", (char *)NULL);
    remove(filename);
    sfree(filename);

    rmdir(dirname);

    /*
     * We deliberately _don't_ clean up the parent directory
     * /tmp/putty-connshare.<username>, because if we leave it around
     * then it reduces the ability for other users to be a nuisance by
     * putting their own directory in the way of it. Also, the salt
     * file in it can be reused.
     */

    sfree(dirname);
}
