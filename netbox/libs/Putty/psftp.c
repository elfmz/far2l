/*
 * psftp.c: (platform-independent) front end for PSFTP.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <limits.h>

#define PUTTY_DO_GLOBALS
#include "putty.h"
#include "psftp.h"
#include "storage.h"
#include "ssh.h"
#include "sftp.h"
#include "int64.h"

const char *const appname = "PSFTP";

/*
 * Since SFTP is a request-response oriented protocol, it requires
 * no buffer management: when we send data, we stop and wait for an
 * acknowledgement _anyway_, and so we can't possibly overfill our
 * send buffer.
 */

static int psftp_connect(char *userhost, char *user, int portnumber);
static int do_sftp_init(void);
void do_sftp_cleanup();

/* ----------------------------------------------------------------------
 * sftp client state.
 */

char *pwd, *homedir;
static Backend *back;
static void *backhandle;
static Conf *conf;
int sent_eof = FALSE;

/* ----------------------------------------------------------------------
 * Manage sending requests and waiting for replies.
 */
struct sftp_packet *sftp_wait_for_reply(struct sftp_request *req)
{
    struct sftp_packet *pktin;
    struct sftp_request *rreq;

    sftp_register(req);
    pktin = sftp_recv();
    if (pktin == NULL)
        connection_fatal(NULL, "did not receive SFTP response packet "
                         "from server");
    rreq = sftp_find_request(pktin);
    if (rreq != req)
        connection_fatal(NULL, "unable to understand SFTP response packet "
                         "from server: %s", fxp_error());
    return pktin;
}

/* ----------------------------------------------------------------------
 * Higher-level helper functions used in commands.
 */

/*
 * Attempt to canonify a pathname starting from the pwd. If
 * canonification fails, at least fall back to returning a _valid_
 * pathname (though it may be ugly, eg /home/simon/../foobar).
 */
char *canonify(char *name)
{
    char *fullname, *canonname;
    struct sftp_packet *pktin;
    struct sftp_request *req;

    if (name[0] == '/') {
	fullname = dupstr(name);
    } else {
	char *slash;
	if (pwd[strlen(pwd) - 1] == '/')
	    slash = "";
	else
	    slash = "/";
	fullname = dupcat(pwd, slash, name, NULL);
    }

    req = fxp_realpath_send(fullname);
    pktin = sftp_wait_for_reply(req);
    canonname = fxp_realpath_recv(pktin, req);

    if (canonname) {
	sfree(fullname);
	return canonname;
    } else {
	/*
	 * Attempt number 2. Some FXP_REALPATH implementations
	 * (glibc-based ones, in particular) require the _whole_
	 * path to point to something that exists, whereas others
	 * (BSD-based) only require all but the last component to
	 * exist. So if the first call failed, we should strip off
	 * everything from the last slash onwards and try again,
	 * then put the final component back on.
	 * 
	 * Special cases:
	 * 
	 *  - if the last component is "/." or "/..", then we don't
	 *    bother trying this because there's no way it can work.
	 * 
	 *  - if the thing actually ends with a "/", we remove it
	 *    before we start. Except if the string is "/" itself
	 *    (although I can't see why we'd have got here if so,
	 *    because surely "/" would have worked the first
	 *    time?), in which case we don't bother.
	 * 
	 *  - if there's no slash in the string at all, give up in
	 *    confusion (we expect at least one because of the way
	 *    we constructed the string).
	 */

	int i;
	char *returnname;

	i = strlen(fullname);
	if (i > 2 && fullname[i - 1] == '/')
	    fullname[--i] = '\0';      /* strip trailing / unless at pos 0 */
	while (i > 0 && fullname[--i] != '/');

	/*
	 * Give up on special cases.
	 */
	if (fullname[i] != '/' ||      /* no slash at all */
	    !strcmp(fullname + i, "/.") ||	/* ends in /. */
	    !strcmp(fullname + i, "/..") ||	/* ends in /.. */
	    !strcmp(fullname, "/")) {
	    return fullname;
	}

	/*
	 * Now i points at the slash. Deal with the final special
	 * case i==0 (ie the whole path was "/nonexistentfile").
	 */
	fullname[i] = '\0';	       /* separate the string */
	if (i == 0) {
	    req = fxp_realpath_send("/");
	} else {
	    req = fxp_realpath_send(fullname);
	}
	pktin = sftp_wait_for_reply(req);
	canonname = fxp_realpath_recv(pktin, req);

	if (!canonname) {
	    /* Even that failed. Restore our best guess at the
	     * constructed filename and give up */
	    fullname[i] = '/';	/* restore slash and last component */
	    return fullname;
	}

	/*
	 * We have a canonical name for all but the last path
	 * component. Concatenate the last component and return.
	 */
	returnname = dupcat(canonname,
			    canonname[strlen(canonname) - 1] ==
			    '/' ? "" : "/", fullname + i + 1, NULL);
	sfree(fullname);
	sfree(canonname);
	return returnname;
    }
}

/*
 * Return a pointer to the portion of str that comes after the last
 * slash (or backslash or colon, if `local' is TRUE).
 */
static char *stripslashes(char *str, int local)
{
    char *p;

    if (local) {
        p = strchr(str, ':');
        if (p) str = p+1;
    }

    p = strrchr(str, '/');
    if (p) str = p+1;

    if (local) {
	p = strrchr(str, '\\');
	if (p) str = p+1;
    }

    return str;
}

/*
 * qsort comparison routine for fxp_name structures. Sorts by real
 * file name.
 */
static int sftp_name_compare(const void *av, const void *bv)
{
    const struct fxp_name *const *a = (const struct fxp_name *const *) av;
    const struct fxp_name *const *b = (const struct fxp_name *const *) bv;
    return strcmp((*a)->filename, (*b)->filename);
}

/*
 * Likewise, but for a bare char *.
 */
static int bare_name_compare(const void *av, const void *bv)
{
    const char **a = (const char **) av;
    const char **b = (const char **) bv;
    return strcmp(*a, *b);
}

static void not_connected(void)
{
    printf("psftp: not connected to a host; use \"open host.name\"\n");
}

/* ----------------------------------------------------------------------
 * The meat of the `get' and `put' commands.
 */
int sftp_get_file(char *fname, char *outfname, int recurse, int restart)
{
    struct fxp_handle *fh;
    struct sftp_packet *pktin;
    struct sftp_request *req;
    struct fxp_xfer *xfer;
    uint64 offset;
    WFile *file;
    int ret, shown_err = FALSE;
    struct fxp_attrs attrs;

    /*
     * In recursive mode, see if we're dealing with a directory.
     * (If we're not in recursive mode, we need not even check: the
     * subsequent FXP_OPEN will return a usable error message.)
     */
    if (recurse) {
	int result;

        req = fxp_stat_send(fname);
        pktin = sftp_wait_for_reply(req);
	result = fxp_stat_recv(pktin, req, &attrs);

	if (result &&
	    (attrs.flags & SSH_FILEXFER_ATTR_PERMISSIONS) &&
	    (attrs.permissions & 0040000)) {

	    struct fxp_handle *dirhandle;
	    int nnames, namesize;
	    struct fxp_name **ournames;
	    struct fxp_names *names;
	    int i;

	    /*
	     * First, attempt to create the destination directory,
	     * unless it already exists.
	     */
	    if (file_type(outfname) != FILE_TYPE_DIRECTORY &&
		!create_directory(outfname)) {
		printf("%s: Cannot create directory\n", outfname);
		return 0;
	    }

	    /*
	     * Now get the list of filenames in the remote
	     * directory.
	     */
            req = fxp_opendir_send(fname);
            pktin = sftp_wait_for_reply(req);
	    dirhandle = fxp_opendir_recv(pktin, req);

	    if (!dirhandle) {
		printf("%s: unable to open directory: %s\n",
		       fname, fxp_error());
		return 0;
	    }
	    nnames = namesize = 0;
	    ournames = NULL;
	    while (1) {
		int i;

		req = fxp_readdir_send(dirhandle);
                pktin = sftp_wait_for_reply(req);
		names = fxp_readdir_recv(pktin, req);

		if (names == NULL) {
		    if (fxp_error_type() == SSH_FX_EOF)
			break;
		    printf("%s: reading directory: %s\n", fname, fxp_error());

                    req = fxp_close_send(dirhandle);
                    pktin = sftp_wait_for_reply(req);
                    fxp_close_recv(pktin, req);

		    sfree(ournames);
		    return 0;
		}
		if (names->nnames == 0) {
		    fxp_free_names(names);
		    break;
		}
		if (nnames + names->nnames >= namesize) {
		    namesize += names->nnames + 128;
		    ournames = sresize(ournames, namesize, struct fxp_name *);
		}
		for (i = 0; i < names->nnames; i++)
		    if (strcmp(names->names[i].filename, ".") &&
			strcmp(names->names[i].filename, "..")) {
			if (!vet_filename(names->names[i].filename)) {
			    printf("ignoring potentially dangerous server-"
				   "supplied filename '%s'\n",
				   names->names[i].filename);
			} else {
			    ournames[nnames++] =
				fxp_dup_name(&names->names[i]);
			}
		    }
		fxp_free_names(names);
	    }
	    req = fxp_close_send(dirhandle);
            pktin = sftp_wait_for_reply(req);
	    fxp_close_recv(pktin, req);

	    /*
	     * Sort the names into a clear order. This ought to
	     * make things more predictable when we're doing a
	     * reget of the same directory, just in case two
	     * readdirs on the same remote directory return a
	     * different order.
	     */
            if (nnames > 0)
                qsort(ournames, nnames, sizeof(*ournames), sftp_name_compare);

	    /*
	     * If we're in restart mode, find the last filename on
	     * this list that already exists. We may have to do a
	     * reget on _that_ file, but shouldn't have to do
	     * anything on the previous files.
	     * 
	     * If none of them exists, of course, we start at 0.
	     */
	    i = 0;
            if (restart) {
                while (i < nnames) {
                    char *nextoutfname;
                    int ret;
                    nextoutfname = dir_file_cat(outfname,
                                                ournames[i]->filename);
                    ret = (file_type(nextoutfname) == FILE_TYPE_NONEXISTENT);
                    sfree(nextoutfname);
                    if (ret)
                        break;
                    i++;
                }
                if (i > 0)
                    i--;
            }

	    /*
	     * Now we're ready to recurse. Starting at ournames[i]
	     * and continuing on to the end of the list, we
	     * construct a new source and target file name, and
	     * call sftp_get_file again.
	     */
	    for (; i < nnames; i++) {
		char *nextfname, *nextoutfname;
		int ret;
		
		nextfname = dupcat(fname, "/", ournames[i]->filename, NULL);
                nextoutfname = dir_file_cat(outfname, ournames[i]->filename);
		ret = sftp_get_file(nextfname, nextoutfname, recurse, restart);
		restart = FALSE;       /* after first partial file, do full */
		sfree(nextoutfname);
		sfree(nextfname);
		if (!ret) {
		    for (i = 0; i < nnames; i++) {
			fxp_free_name(ournames[i]);
		    }
		    sfree(ournames);
		    return 0;
		}
	    }

	    /*
	     * Done this recursion level. Free everything.
	     */
	    for (i = 0; i < nnames; i++) {
		fxp_free_name(ournames[i]);
	    }
	    sfree(ournames);

	    return 1;
	}
    }

    req = fxp_stat_send(fname);
    pktin = sftp_wait_for_reply(req);
    if (!fxp_stat_recv(pktin, req, &attrs))
        attrs.flags = 0;

    req = fxp_open_send(fname, SSH_FXF_READ, NULL);
    pktin = sftp_wait_for_reply(req);
    fh = fxp_open_recv(pktin, req);

    if (!fh) {
	printf("%s: open for read: %s\n", fname, fxp_error());
	return 0;
    }

    if (restart) {
	file = open_existing_wfile(outfname, NULL);
    } else {
	file = open_new_file(outfname, GET_PERMISSIONS(attrs));
    }

    if (!file) {
	printf("local: unable to open %s\n", outfname);

        req = fxp_close_send(fh);
        pktin = sftp_wait_for_reply(req);
	fxp_close_recv(pktin, req);

	return 0;
    }

    if (restart) {
	char decbuf[30];
	if (seek_file(file, uint64_make(0,0) , FROM_END) == -1) {
	    close_wfile(file);
	    printf("reget: cannot restart %s - file too large\n",
		   outfname);
	    req = fxp_close_send(fh);
            pktin = sftp_wait_for_reply(req);
	    fxp_close_recv(pktin, req);
		
	    return 0;
	}
	    
	offset = get_file_posn(file);
	uint64_decimal(offset, decbuf);
	printf("reget: restarting at file position %s\n", decbuf);
    } else {
	offset = uint64_make(0, 0);
    }

    printf("remote:%s => local:%s\n", fname, outfname);

    /*
     * FIXME: we can use FXP_FSTAT here to get the file size, and
     * thus put up a progress bar.
     */
    ret = 1;
    xfer = xfer_download_init(fh, offset);
    while (!xfer_done(xfer)) {
	void *vbuf;
	int ret, len;
	int wpos, wlen;

	xfer_download_queue(xfer);
	pktin = sftp_recv();
	ret = xfer_download_gotpkt(xfer, pktin);
	if (ret <= 0) {
	    if (!shown_err) {
		printf("error while reading: %s\n", fxp_error());
		shown_err = TRUE;
	    }
            if (ret == INT_MIN)        /* pktin not even freed */
                sfree(pktin);
            ret = 0;
	}

	while (xfer_download_data(xfer, &vbuf, &len)) {
	    unsigned char *buf = (unsigned char *)vbuf;

	    wpos = 0;
	    while (wpos < len) {
		wlen = write_to_file(file, buf + wpos, len - wpos);
		if (wlen <= 0) {
		    printf("error while writing local file\n");
		    ret = 0;
		    xfer_set_error(xfer);
		    break;
		}
		wpos += wlen;
	    }
	    if (wpos < len) {	       /* we had an error */
		ret = 0;
		xfer_set_error(xfer);
	    }

	    sfree(vbuf);
	}
    }

    xfer_cleanup(xfer);

    close_wfile(file);

    req = fxp_close_send(fh);
    pktin = sftp_wait_for_reply(req);
    fxp_close_recv(pktin, req);

    return ret;
}

int sftp_put_file(char *fname, char *outfname, int recurse, int restart)
{
    struct fxp_handle *fh;
    struct fxp_xfer *xfer;
    struct sftp_packet *pktin;
    struct sftp_request *req;
    uint64 offset;
    RFile *file;
    int ret, err, eof;
    struct fxp_attrs attrs;
    long permissions;

    /*
     * In recursive mode, see if we're dealing with a directory.
     * (If we're not in recursive mode, we need not even check: the
     * subsequent fopen will return an error message.)
     */
    if (recurse && file_type(fname) == FILE_TYPE_DIRECTORY) {
	int result;
	int nnames, namesize;
	char *name, **ournames;
	DirHandle *dh;
	int i;

	/*
	 * First, attempt to create the destination directory,
	 * unless it already exists.
	 */
	req = fxp_stat_send(outfname);
        pktin = sftp_wait_for_reply(req);
	result = fxp_stat_recv(pktin, req, &attrs);
	if (!result ||
	    !(attrs.flags & SSH_FILEXFER_ATTR_PERMISSIONS) ||
	    !(attrs.permissions & 0040000)) {
	    req = fxp_mkdir_send(outfname);
            pktin = sftp_wait_for_reply(req);
	    result = fxp_mkdir_recv(pktin, req);

	    if (!result) {
		printf("%s: create directory: %s\n",
		       outfname, fxp_error());
		return 0;
	    }
	}

	/*
	 * Now get the list of filenames in the local directory.
	 */
	nnames = namesize = 0;
	ournames = NULL;

	dh = open_directory(fname);
	if (!dh) {
	    printf("%s: unable to open directory\n", fname);
	    return 0;
	}
	while ((name = read_filename(dh)) != NULL) {
	    if (nnames >= namesize) {
		namesize += 128;
		ournames = sresize(ournames, namesize, char *);
	    }
	    ournames[nnames++] = name;
	}
	close_directory(dh);

	/*
	 * Sort the names into a clear order. This ought to make
	 * things more predictable when we're doing a reput of the
	 * same directory, just in case two readdirs on the same
	 * local directory return a different order.
	 */
        if (nnames > 0)
            qsort(ournames, nnames, sizeof(*ournames), bare_name_compare);

	/*
	 * If we're in restart mode, find the last filename on this
	 * list that already exists. We may have to do a reput on
	 * _that_ file, but shouldn't have to do anything on the
	 * previous files.
	 *
	 * If none of them exists, of course, we start at 0.
	 */
	i = 0;
        if (restart) {
            while (i < nnames) {
                char *nextoutfname;
                nextoutfname = dupcat(outfname, "/", ournames[i], NULL);
                req = fxp_stat_send(nextoutfname);
                pktin = sftp_wait_for_reply(req);
                result = fxp_stat_recv(pktin, req, &attrs);
                sfree(nextoutfname);
                if (!result)
                    break;
                i++;
            }
            if (i > 0)
                i--;
        }

        /*
         * Now we're ready to recurse. Starting at ournames[i]
	 * and continuing on to the end of the list, we
	 * construct a new source and target file name, and
	 * call sftp_put_file again.
	 */
	for (; i < nnames; i++) {
	    char *nextfname, *nextoutfname;
	    int ret;

            nextfname = dir_file_cat(fname, ournames[i]);
	    nextoutfname = dupcat(outfname, "/", ournames[i], NULL);
	    ret = sftp_put_file(nextfname, nextoutfname, recurse, restart);
	    restart = FALSE;	       /* after first partial file, do full */
	    sfree(nextoutfname);
	    sfree(nextfname);
	    if (!ret) {
		for (i = 0; i < nnames; i++) {
		    sfree(ournames[i]);
		}
		sfree(ournames);
		return 0;
	    }
	}

	/*
	 * Done this recursion level. Free everything.
	 */
	for (i = 0; i < nnames; i++) {
	    sfree(ournames[i]);
	}
	sfree(ournames);

	return 1;
    }

    file = open_existing_file(fname, NULL, NULL, NULL, &permissions);
    if (!file) {
	printf("local: unable to open %s\n", fname);
	return 0;
    }
    attrs.flags = 0;
    PUT_PERMISSIONS(attrs, permissions);
    if (restart) {
	req = fxp_open_send(outfname, SSH_FXF_WRITE, &attrs);
    } else {
	req = fxp_open_send(outfname,
                            SSH_FXF_WRITE | SSH_FXF_CREAT | SSH_FXF_TRUNC,
                            &attrs);
    }
    pktin = sftp_wait_for_reply(req);
    fh = fxp_open_recv(pktin, req);

    if (!fh) {
	close_rfile(file);
	printf("%s: open for write: %s\n", outfname, fxp_error());
	return 0;
    }

    if (restart) {
	char decbuf[30];
	struct fxp_attrs attrs;

	req = fxp_fstat_send(fh);
        pktin = sftp_wait_for_reply(req);
	ret = fxp_fstat_recv(pktin, req, &attrs);

	if (!ret) {
	    printf("read size of %s: %s\n", outfname, fxp_error());
            goto cleanup;
	}
	if (!(attrs.flags & SSH_FILEXFER_ATTR_SIZE)) {
	    printf("read size of %s: size was not given\n", outfname);
            ret = 0;
            goto cleanup;
	}
	offset = attrs.size;
	uint64_decimal(offset, decbuf);
	printf("reput: restarting at file position %s\n", decbuf);

	if (seek_file((WFile *)file, offset, FROM_START) != 0)
	    seek_file((WFile *)file, uint64_make(0,0), FROM_END);    /* *shrug* */
    } else {
	offset = uint64_make(0, 0);
    }

    printf("local:%s => remote:%s\n", fname, outfname);

    /*
     * FIXME: we can use FXP_FSTAT here to get the file size, and
     * thus put up a progress bar.
     */
    ret = 1;
    xfer = xfer_upload_init(fh, offset);
    err = eof = 0;
    while ((!err && !eof) || !xfer_done(xfer)) {
	char buffer[4096];
	int len, ret;

	while (xfer_upload_ready(xfer) && !err && !eof) {
	    len = read_from_file(file, buffer, sizeof(buffer));
	    if (len == -1) {
		printf("error while reading local file\n");
		err = 1;
	    } else if (len == 0) {
		eof = 1;
	    } else {
		xfer_upload_data(xfer, buffer, len);
	    }
	}

	if (!xfer_done(xfer)) {
	    pktin = sftp_recv();
	    ret = xfer_upload_gotpkt(xfer, pktin);
	    if (ret <= 0) {
                if (ret == INT_MIN)        /* pktin not even freed */
                    sfree(pktin);
                if (!err) {
                    printf("error while writing: %s\n", fxp_error());
                    err = 1;
                }
	    }
	}
    }

    xfer_cleanup(xfer);

  cleanup:
    req = fxp_close_send(fh);
    pktin = sftp_wait_for_reply(req);
    fxp_close_recv(pktin, req);

    close_rfile(file);

    return ret;
}

/* ----------------------------------------------------------------------
 * A remote wildcard matcher, providing a similar interface to the
 * local one in psftp.h.
 */

typedef struct SftpWildcardMatcher {
    struct fxp_handle *dirh;
    struct fxp_names *names;
    int namepos;
    char *wildcard, *prefix;
} SftpWildcardMatcher;

SftpWildcardMatcher *sftp_begin_wildcard_matching(char *name)
{
    struct sftp_packet *pktin;
    struct sftp_request *req;
    char *wildcard;
    char *unwcdir, *tmpdir, *cdir;
    int len, check;
    SftpWildcardMatcher *swcm;
    struct fxp_handle *dirh;

    /*
     * We don't handle multi-level wildcards; so we expect to find
     * a fully specified directory part, followed by a wildcard
     * after that.
     */
    wildcard = stripslashes(name, 0);

    unwcdir = dupstr(name);
    len = wildcard - name;
    unwcdir[len] = '\0';
    if (len > 0 && unwcdir[len-1] == '/')
	unwcdir[len-1] = '\0';
    tmpdir = snewn(1 + len, char);
    check = wc_unescape(tmpdir, unwcdir);
    sfree(tmpdir);

    if (!check) {
	printf("Multiple-level wildcards are not supported\n");
	sfree(unwcdir);
	return NULL;
    }

    cdir = canonify(unwcdir);

    req = fxp_opendir_send(cdir);
    pktin = sftp_wait_for_reply(req);
    dirh = fxp_opendir_recv(pktin, req);

    if (dirh) {
	swcm = snew(SftpWildcardMatcher);
	swcm->dirh = dirh;
	swcm->names = NULL;
	swcm->wildcard = dupstr(wildcard);
	swcm->prefix = unwcdir;
    } else {
	printf("Unable to open %s: %s\n", cdir, fxp_error());
	swcm = NULL;
	sfree(unwcdir);
    }

    sfree(cdir);

    return swcm;
}

char *sftp_wildcard_get_filename(SftpWildcardMatcher *swcm)
{
    struct fxp_name *name;
    struct sftp_packet *pktin;
    struct sftp_request *req;

    while (1) {
	if (swcm->names && swcm->namepos >= swcm->names->nnames) {
	    fxp_free_names(swcm->names);
	    swcm->names = NULL;
	}

	if (!swcm->names) {
	    req = fxp_readdir_send(swcm->dirh);
            pktin = sftp_wait_for_reply(req);
	    swcm->names = fxp_readdir_recv(pktin, req);

	    if (!swcm->names) {
		if (fxp_error_type() != SSH_FX_EOF)
		    printf("%s: reading directory: %s\n", swcm->prefix,
			   fxp_error());
		return NULL;
	    } else if (swcm->names->nnames == 0) {
                /*
                 * Another failure mode which we treat as EOF is if
                 * the server reports success from FXP_READDIR but
                 * returns no actual names. This is unusual, since
                 * from most servers you'd expect at least "." and
                 * "..", but there's nothing forbidding a server from
                 * omitting those if it wants to.
                 */
                return NULL;
            }

	    swcm->namepos = 0;
	}

	assert(swcm->names && swcm->namepos < swcm->names->nnames);

	name = &swcm->names->names[swcm->namepos++];

	if (!strcmp(name->filename, ".") || !strcmp(name->filename, ".."))
	    continue;		       /* expected bad filenames */

	if (!vet_filename(name->filename)) {
	    printf("ignoring potentially dangerous server-"
		   "supplied filename '%s'\n", name->filename);
	    continue;		       /* unexpected bad filename */
	}

	if (!wc_match(swcm->wildcard, name->filename))
	    continue;		       /* doesn't match the wildcard */

	/*
	 * We have a working filename. Return it.
	 */
	return dupprintf("%s%s%s", swcm->prefix,
			 (!swcm->prefix[0] ||
			  swcm->prefix[strlen(swcm->prefix)-1]=='/' ?
			  "" : "/"),
			 name->filename);
    }
}

void sftp_finish_wildcard_matching(SftpWildcardMatcher *swcm)
{
    struct sftp_packet *pktin;
    struct sftp_request *req;

    req = fxp_close_send(swcm->dirh);
    pktin = sftp_wait_for_reply(req);
    fxp_close_recv(pktin, req);

    if (swcm->names)
	fxp_free_names(swcm->names);

    sfree(swcm->prefix);
    sfree(swcm->wildcard);

    sfree(swcm);
}

/*
 * General function to match a potential wildcard in a filename
 * argument and iterate over every matching file. Used in several
 * PSFTP commands (rmdir, rm, chmod, mv).
 */
int wildcard_iterate(char *filename, int (*func)(void *, char *), void *ctx)
{
    char *unwcfname, *newname, *cname;
    int is_wc, ret;

    unwcfname = snewn(strlen(filename)+1, char);
    is_wc = !wc_unescape(unwcfname, filename);

    if (is_wc) {
	SftpWildcardMatcher *swcm = sftp_begin_wildcard_matching(filename);
	int matched = FALSE;
	sfree(unwcfname);

	if (!swcm)
	    return 0;

	ret = 1;

	while ( (newname = sftp_wildcard_get_filename(swcm)) != NULL ) {
	    cname = canonify(newname);
	    if (!cname) {
		printf("%s: canonify: %s\n", newname, fxp_error());
		ret = 0;
	    }
            sfree(newname);
	    matched = TRUE;
	    ret &= func(ctx, cname);
	    sfree(cname);
	}

	if (!matched) {
	    /* Politely warn the user that nothing matched. */
	    printf("%s: nothing matched\n", filename);
	}

	sftp_finish_wildcard_matching(swcm);
    } else {
	cname = canonify(unwcfname);
	if (!cname) {
	    printf("%s: canonify: %s\n", filename, fxp_error());
	    ret = 0;
	}
	ret = func(ctx, cname);
	sfree(cname);
	sfree(unwcfname);
    }

    return ret;
}

/*
 * Handy helper function.
 */
int is_wildcard(char *name)
{
    char *unwcfname = snewn(strlen(name)+1, char);
    int is_wc = !wc_unescape(unwcfname, name);
    sfree(unwcfname);
    return is_wc;
}

/* ----------------------------------------------------------------------
 * Actual sftp commands.
 */
struct sftp_command {
    char **words;
    int nwords, wordssize;
    int (*obey) (struct sftp_command *);	/* returns <0 to quit */
};

int sftp_cmd_null(struct sftp_command *cmd)
{
    return 1;			       /* success */
}

int sftp_cmd_unknown(struct sftp_command *cmd)
{
    printf("psftp: unknown command \"%s\"\n", cmd->words[0]);
    return 0;			       /* failure */
}

int sftp_cmd_quit(struct sftp_command *cmd)
{
    return -1;
}

int sftp_cmd_close(struct sftp_command *cmd)
{
    if (back == NULL) {
	not_connected();
	return 0;
    }

    if (back != NULL && back->connected(backhandle)) {
	char ch;
	back->special(backhandle, TS_EOF);
        sent_eof = TRUE;
	sftp_recvdata(&ch, 1);
    }
    do_sftp_cleanup();

    return 0;
}

/*
 * List a directory. If no arguments are given, list pwd; otherwise
 * list the directory given in words[1].
 */
int sftp_cmd_ls(struct sftp_command *cmd)
{
    struct fxp_handle *dirh;
    struct fxp_names *names;
    struct fxp_name **ournames;
    int nnames, namesize;
    char *dir, *cdir, *unwcdir, *wildcard;
    struct sftp_packet *pktin;
    struct sftp_request *req;
    int i;

    if (back == NULL) {
	not_connected();
	return 0;
    }

    if (cmd->nwords < 2)
	dir = ".";
    else
	dir = cmd->words[1];

    unwcdir = snewn(1 + strlen(dir), char);
    if (wc_unescape(unwcdir, dir)) {
	dir = unwcdir;
	wildcard = NULL;
    } else {
	char *tmpdir;
	int len, check;

        sfree(unwcdir);
	wildcard = stripslashes(dir, 0);
	unwcdir = dupstr(dir);
	len = wildcard - dir;
	unwcdir[len] = '\0';
	if (len > 0 && unwcdir[len-1] == '/')
	    unwcdir[len-1] = '\0';
	tmpdir = snewn(1 + len, char);
	check = wc_unescape(tmpdir, unwcdir);
	sfree(tmpdir);
	if (!check) {
	    printf("Multiple-level wildcards are not supported\n");
	    sfree(unwcdir);
	    return 0;
	}
	dir = unwcdir;
    }

    cdir = canonify(dir);
    if (!cdir) {
	printf("%s: canonify: %s\n", dir, fxp_error());
	sfree(unwcdir);
	return 0;
    }

    printf("Listing directory %s\n", cdir);

    req = fxp_opendir_send(cdir);
    pktin = sftp_wait_for_reply(req);
    dirh = fxp_opendir_recv(pktin, req);

    if (dirh == NULL) {
	printf("Unable to open %s: %s\n", dir, fxp_error());
    } else {
	nnames = namesize = 0;
	ournames = NULL;

	while (1) {

	    req = fxp_readdir_send(dirh);
            pktin = sftp_wait_for_reply(req);
	    names = fxp_readdir_recv(pktin, req);

	    if (names == NULL) {
		if (fxp_error_type() == SSH_FX_EOF)
		    break;
		printf("Reading directory %s: %s\n", dir, fxp_error());
		break;
	    }
	    if (names->nnames == 0) {
		fxp_free_names(names);
		break;
	    }

	    if (nnames + names->nnames >= namesize) {
		namesize += names->nnames + 128;
		ournames = sresize(ournames, namesize, struct fxp_name *);
	    }

	    for (i = 0; i < names->nnames; i++)
		if (!wildcard || wc_match(wildcard, names->names[i].filename))
		    ournames[nnames++] = fxp_dup_name(&names->names[i]);

	    fxp_free_names(names);
	}
	req = fxp_close_send(dirh);
        pktin = sftp_wait_for_reply(req);
	fxp_close_recv(pktin, req);

	/*
	 * Now we have our filenames. Sort them by actual file
	 * name, and then output the longname parts.
	 */
        if (nnames > 0)
            qsort(ournames, nnames, sizeof(*ournames), sftp_name_compare);

	/*
	 * And print them.
	 */
	for (i = 0; i < nnames; i++) {
	    printf("%s\n", ournames[i]->longname);
	    fxp_free_name(ournames[i]);
	}
	sfree(ournames);
    }

    sfree(cdir);
    sfree(unwcdir);

    return 1;
}

/*
 * Change directories. We do this by canonifying the new name, then
 * trying to OPENDIR it. Only if that succeeds do we set the new pwd.
 */
int sftp_cmd_cd(struct sftp_command *cmd)
{
    struct fxp_handle *dirh;
    struct sftp_packet *pktin;
    struct sftp_request *req;
    char *dir;

    if (back == NULL) {
	not_connected();
	return 0;
    }

    if (cmd->nwords < 2)
	dir = dupstr(homedir);
    else
	dir = canonify(cmd->words[1]);

    if (!dir) {
	printf("%s: canonify: %s\n", dir, fxp_error());
	return 0;
    }

    req = fxp_opendir_send(dir);
    pktin = sftp_wait_for_reply(req);
    dirh = fxp_opendir_recv(pktin, req);

    if (!dirh) {
	printf("Directory %s: %s\n", dir, fxp_error());
	sfree(dir);
	return 0;
    }

    req = fxp_close_send(dirh);
    pktin = sftp_wait_for_reply(req);
    fxp_close_recv(pktin, req);

    sfree(pwd);
    pwd = dir;
    printf("Remote directory is now %s\n", pwd);

    return 1;
}

/*
 * Print current directory. Easy as pie.
 */
int sftp_cmd_pwd(struct sftp_command *cmd)
{
    if (back == NULL) {
	not_connected();
	return 0;
    }

    printf("Remote directory is %s\n", pwd);
    return 1;
}

/*
 * Get a file and save it at the local end. We have three very
 * similar commands here. The basic one is `get'; `reget' differs
 * in that it checks for the existence of the destination file and
 * starts from where a previous aborted transfer left off; `mget'
 * differs in that it interprets all its arguments as files to
 * transfer (never as a different local name for a remote file) and
 * can handle wildcards.
 */
int sftp_general_get(struct sftp_command *cmd, int restart, int multiple)
{
    char *fname, *unwcfname, *origfname, *origwfname, *outfname;
    int i, ret;
    int recurse = FALSE;

    if (back == NULL) {
	not_connected();
	return 0;
    }

    i = 1;
    while (i < cmd->nwords && cmd->words[i][0] == '-') {
	if (!strcmp(cmd->words[i], "--")) {
	    /* finish processing options */
	    i++;
	    break;
	} else if (!strcmp(cmd->words[i], "-r")) {
	    recurse = TRUE;
	} else {
	    printf("%s: unrecognised option '%s'\n", cmd->words[0], cmd->words[i]);
	    return 0;
	}
	i++;
    }

    if (i >= cmd->nwords) {
	printf("%s: expects a filename\n", cmd->words[0]);
	return 0;
    }

    ret = 1;
    do {
	SftpWildcardMatcher *swcm;

	origfname = cmd->words[i++];
	unwcfname = snewn(strlen(origfname)+1, char);

	if (multiple && !wc_unescape(unwcfname, origfname)) {
	    swcm = sftp_begin_wildcard_matching(origfname);
	    if (!swcm) {
		sfree(unwcfname);
		continue;
	    }
	    origwfname = sftp_wildcard_get_filename(swcm);
	    if (!origwfname) {
		/* Politely warn the user that nothing matched. */
		printf("%s: nothing matched\n", origfname);
		sftp_finish_wildcard_matching(swcm);
		sfree(unwcfname);
		continue;
	    }
	} else {
	    origwfname = origfname;
	    swcm = NULL;
	}

	while (origwfname) {
	    fname = canonify(origwfname);

	    if (!fname) {
                sftp_finish_wildcard_matching(swcm);
		printf("%s: canonify: %s\n", origwfname, fxp_error());
		sfree(origwfname);
		sfree(unwcfname);
		return 0;
	    }

	    if (!multiple && i < cmd->nwords)
		outfname = cmd->words[i++];
	    else
		outfname = stripslashes(origwfname, 0);

	    ret = sftp_get_file(fname, outfname, recurse, restart);

	    sfree(fname);

	    if (swcm) {
		sfree(origwfname);
		origwfname = sftp_wildcard_get_filename(swcm);
	    } else {
		origwfname = NULL;
	    }
	}
	sfree(unwcfname);
	if (swcm)
	    sftp_finish_wildcard_matching(swcm);
	if (!ret)
	    return ret;

    } while (multiple && i < cmd->nwords);

    return ret;
}
int sftp_cmd_get(struct sftp_command *cmd)
{
    return sftp_general_get(cmd, 0, 0);
}
int sftp_cmd_mget(struct sftp_command *cmd)
{
    return sftp_general_get(cmd, 0, 1);
}
int sftp_cmd_reget(struct sftp_command *cmd)
{
    return sftp_general_get(cmd, 1, 0);
}

/*
 * Send a file and store it at the remote end. We have three very
 * similar commands here. The basic one is `put'; `reput' differs
 * in that it checks for the existence of the destination file and
 * starts from where a previous aborted transfer left off; `mput'
 * differs in that it interprets all its arguments as files to
 * transfer (never as a different remote name for a local file) and
 * can handle wildcards.
 */
int sftp_general_put(struct sftp_command *cmd, int restart, int multiple)
{
    char *fname, *wfname, *origoutfname, *outfname;
    int i, ret;
    int recurse = FALSE;

    if (back == NULL) {
	not_connected();
	return 0;
    }

    i = 1;
    while (i < cmd->nwords && cmd->words[i][0] == '-') {
	if (!strcmp(cmd->words[i], "--")) {
	    /* finish processing options */
	    i++;
	    break;
	} else if (!strcmp(cmd->words[i], "-r")) {
	    recurse = TRUE;
	} else {
	    printf("%s: unrecognised option '%s'\n", cmd->words[0], cmd->words[i]);
	    return 0;
	}
	i++;
    }

    if (i >= cmd->nwords) {
	printf("%s: expects a filename\n", cmd->words[0]);
	return 0;
    }

    ret = 1;
    do {
	WildcardMatcher *wcm;
	fname = cmd->words[i++];

	if (multiple && test_wildcard(fname, FALSE) == WCTYPE_WILDCARD) {
	    wcm = begin_wildcard_matching(fname);
	    wfname = wildcard_get_filename(wcm);
	    if (!wfname) {
		/* Politely warn the user that nothing matched. */
		printf("%s: nothing matched\n", fname);
		finish_wildcard_matching(wcm);
		continue;
	    }
	} else {
	    wfname = fname;
	    wcm = NULL;
	}

	while (wfname) {
	    if (!multiple && i < cmd->nwords)
		origoutfname = cmd->words[i++];
	    else
		origoutfname = stripslashes(wfname, 1);

	    outfname = canonify(origoutfname);
	    if (!outfname) {
		printf("%s: canonify: %s\n", origoutfname, fxp_error());
		if (wcm) {
		    sfree(wfname);
		    finish_wildcard_matching(wcm);
		}
		return 0;
	    }
	    ret = sftp_put_file(wfname, outfname, recurse, restart);
	    sfree(outfname);

	    if (wcm) {
		sfree(wfname);
		wfname = wildcard_get_filename(wcm);
	    } else {
		wfname = NULL;
	    }
	}

	if (wcm)
	    finish_wildcard_matching(wcm);

	if (!ret)
	    return ret;

    } while (multiple && i < cmd->nwords);

    return ret;
}
int sftp_cmd_put(struct sftp_command *cmd)
{
    return sftp_general_put(cmd, 0, 0);
}
int sftp_cmd_mput(struct sftp_command *cmd)
{
    return sftp_general_put(cmd, 0, 1);
}
int sftp_cmd_reput(struct sftp_command *cmd)
{
    return sftp_general_put(cmd, 1, 0);
}

int sftp_cmd_mkdir(struct sftp_command *cmd)
{
    char *dir;
    struct sftp_packet *pktin;
    struct sftp_request *req;
    int result;
    int i, ret;

    if (back == NULL) {
	not_connected();
	return 0;
    }

    if (cmd->nwords < 2) {
	printf("mkdir: expects a directory\n");
	return 0;
    }

    ret = 1;
    for (i = 1; i < cmd->nwords; i++) {
	dir = canonify(cmd->words[i]);
	if (!dir) {
	    printf("%s: canonify: %s\n", dir, fxp_error());
	    return 0;
	}

	req = fxp_mkdir_send(dir);
        pktin = sftp_wait_for_reply(req);
	result = fxp_mkdir_recv(pktin, req);

	if (!result) {
	    printf("mkdir %s: %s\n", dir, fxp_error());
	    ret = 0;
	} else
	    printf("mkdir %s: OK\n", dir);

	sfree(dir);
    }

    return ret;
}

static int sftp_action_rmdir(void *vctx, char *dir)
{
    struct sftp_packet *pktin;
    struct sftp_request *req;
    int result;

    req = fxp_rmdir_send(dir);
    pktin = sftp_wait_for_reply(req);
    result = fxp_rmdir_recv(pktin, req);

    if (!result) {
	printf("rmdir %s: %s\n", dir, fxp_error());
	return 0;
    }

    printf("rmdir %s: OK\n", dir);

    return 1;
}

int sftp_cmd_rmdir(struct sftp_command *cmd)
{
    int i, ret;

    if (back == NULL) {
	not_connected();
	return 0;
    }

    if (cmd->nwords < 2) {
	printf("rmdir: expects a directory\n");
	return 0;
    }

    ret = 1;
    for (i = 1; i < cmd->nwords; i++)
	ret &= wildcard_iterate(cmd->words[i], sftp_action_rmdir, NULL);

    return ret;
}

static int sftp_action_rm(void *vctx, char *fname)
{
    struct sftp_packet *pktin;
    struct sftp_request *req;
    int result;

    req = fxp_remove_send(fname);
    pktin = sftp_wait_for_reply(req);
    result = fxp_remove_recv(pktin, req);

    if (!result) {
	printf("rm %s: %s\n", fname, fxp_error());
	return 0;
    }

    printf("rm %s: OK\n", fname);

    return 1;
}

int sftp_cmd_rm(struct sftp_command *cmd)
{
    int i, ret;

    if (back == NULL) {
	not_connected();
	return 0;
    }

    if (cmd->nwords < 2) {
	printf("rm: expects a filename\n");
	return 0;
    }

    ret = 1;
    for (i = 1; i < cmd->nwords; i++)
	ret &= wildcard_iterate(cmd->words[i], sftp_action_rm, NULL);

    return ret;
}

static int check_is_dir(char *dstfname)
{
    struct sftp_packet *pktin;
    struct sftp_request *req;
    struct fxp_attrs attrs;
    int result;

    req = fxp_stat_send(dstfname);
    pktin = sftp_wait_for_reply(req);
    result = fxp_stat_recv(pktin, req, &attrs);

    if (result &&
	(attrs.flags & SSH_FILEXFER_ATTR_PERMISSIONS) &&
	(attrs.permissions & 0040000))
	return TRUE;
    else
	return FALSE;
}

struct sftp_context_mv {
    char *dstfname;
    int dest_is_dir;
};

static int sftp_action_mv(void *vctx, char *srcfname)
{
    struct sftp_context_mv *ctx = (struct sftp_context_mv *)vctx;
    struct sftp_packet *pktin;
    struct sftp_request *req;
    const char *error;
    char *finalfname, *newcanon = NULL;
    int ret, result;

    if (ctx->dest_is_dir) {
	char *p;
	char *newname;

	p = srcfname + strlen(srcfname);
	while (p > srcfname && p[-1] != '/') p--;
	newname = dupcat(ctx->dstfname, "/", p, NULL);
	newcanon = canonify(newname);
	if (!newcanon) {
	    printf("%s: canonify: %s\n", newname, fxp_error());
	    sfree(newname);
	    return 0;
	}
	sfree(newname);

	finalfname = newcanon;
    } else {
	finalfname = ctx->dstfname;
    }

    req = fxp_rename_send(srcfname, finalfname);
    pktin = sftp_wait_for_reply(req);
    result = fxp_rename_recv(pktin, req);

    error = result ? NULL : fxp_error();

    if (error) {
	printf("mv %s %s: %s\n", srcfname, finalfname, error);
	ret = 0;
    } else {
	printf("%s -> %s\n", srcfname, finalfname);
	ret = 1;
    }

    sfree(newcanon);
    return ret;
}

int sftp_cmd_mv(struct sftp_command *cmd)
{
    struct sftp_context_mv actx, *ctx = &actx;
    int i, ret;

    if (back == NULL) {
	not_connected();
	return 0;
    }

    if (cmd->nwords < 3) {
	printf("mv: expects two filenames\n");
	return 0;
    }

    ctx->dstfname = canonify(cmd->words[cmd->nwords-1]);
    if (!ctx->dstfname) {
	printf("%s: canonify: %s\n", ctx->dstfname, fxp_error());
	return 0;
    }

    /*
     * If there's more than one source argument, or one source
     * argument which is a wildcard, we _require_ that the
     * destination is a directory.
     */
    ctx->dest_is_dir = check_is_dir(ctx->dstfname);
    if ((cmd->nwords > 3 || is_wildcard(cmd->words[1])) && !ctx->dest_is_dir) {
	printf("mv: multiple or wildcard arguments require the destination"
	       " to be a directory\n");
	sfree(ctx->dstfname);
	return 0;
    }

    /*
     * Now iterate over the source arguments.
     */
    ret = 1;
    for (i = 1; i < cmd->nwords-1; i++)
	ret &= wildcard_iterate(cmd->words[i], sftp_action_mv, ctx);

    sfree(ctx->dstfname);
    return ret;
}

struct sftp_context_chmod {
    unsigned attrs_clr, attrs_xor;
};

static int sftp_action_chmod(void *vctx, char *fname)
{
    struct fxp_attrs attrs;
    struct sftp_packet *pktin;
    struct sftp_request *req;
    int result;
    unsigned oldperms, newperms;
    struct sftp_context_chmod *ctx = (struct sftp_context_chmod *)vctx;

    req = fxp_stat_send(fname);
    pktin = sftp_wait_for_reply(req);
    result = fxp_stat_recv(pktin, req, &attrs);

    if (!result || !(attrs.flags & SSH_FILEXFER_ATTR_PERMISSIONS)) {
	printf("get attrs for %s: %s\n", fname,
	       result ? "file permissions not provided" : fxp_error());
	return 0;
    }

    attrs.flags = SSH_FILEXFER_ATTR_PERMISSIONS;   /* perms _only_ */
    oldperms = attrs.permissions & 07777;
    attrs.permissions &= ~ctx->attrs_clr;
    attrs.permissions ^= ctx->attrs_xor;
    newperms = attrs.permissions & 07777;

    if (oldperms == newperms)
	return 1;		       /* no need to do anything! */

    req = fxp_setstat_send(fname, attrs);
    pktin = sftp_wait_for_reply(req);
    result = fxp_setstat_recv(pktin, req);

    if (!result) {
	printf("set attrs for %s: %s\n", fname, fxp_error());
	return 0;
    }

    printf("%s: %04o -> %04o\n", fname, oldperms, newperms);

    return 1;
}

int sftp_cmd_chmod(struct sftp_command *cmd)
{
    char *mode;
    int i, ret;
    struct sftp_context_chmod actx, *ctx = &actx;

    if (back == NULL) {
	not_connected();
	return 0;
    }

    if (cmd->nwords < 3) {
	printf("chmod: expects a mode specifier and a filename\n");
	return 0;
    }

    /*
     * Attempt to parse the mode specifier in cmd->words[1]. We
     * don't support the full horror of Unix chmod; instead we
     * support a much simpler syntax in which the user can either
     * specify an octal number, or a comma-separated sequence of
     * [ugoa]*[-+=][rwxst]+. (The initial [ugoa] sequence may
     * _only_ be omitted if the only attribute mentioned is t,
     * since all others require a user/group/other specification.
     * Additionally, the s attribute may not be specified for any
     * [ugoa] specifications other than exactly u or exactly g.
     */
    ctx->attrs_clr = ctx->attrs_xor = 0;
    mode = cmd->words[1];
    if (mode[0] >= '0' && mode[0] <= '9') {
	if (mode[strspn(mode, "01234567")]) {
	    printf("chmod: numeric file modes should"
		   " contain digits 0-7 only\n");
	    return 0;
	}
	ctx->attrs_clr = 07777;
	sscanf(mode, "%o", &ctx->attrs_xor);
	ctx->attrs_xor &= ctx->attrs_clr;
    } else {
	while (*mode) {
	    char *modebegin = mode;
	    unsigned subset, perms;
	    int action;

	    subset = 0;
	    while (*mode && *mode != ',' &&
		   *mode != '+' && *mode != '-' && *mode != '=') {
		switch (*mode) {
		  case 'u': subset |= 04700; break; /* setuid, user perms */
		  case 'g': subset |= 02070; break; /* setgid, group perms */
		  case 'o': subset |= 00007; break; /* just other perms */
		  case 'a': subset |= 06777; break; /* all of the above */
		  default:
		    printf("chmod: file mode '%.*s' contains unrecognised"
			   " user/group/other specifier '%c'\n",
			   (int)strcspn(modebegin, ","), modebegin, *mode);
		    return 0;
		}
		mode++;
	    }
	    if (!*mode || *mode == ',') {
		printf("chmod: file mode '%.*s' is incomplete\n",
		       (int)strcspn(modebegin, ","), modebegin);
		return 0;
	    }
	    action = *mode++;
	    if (!*mode || *mode == ',') {
		printf("chmod: file mode '%.*s' is incomplete\n",
		       (int)strcspn(modebegin, ","), modebegin);
		return 0;
	    }
	    perms = 0;
	    while (*mode && *mode != ',') {
		switch (*mode) {
		  case 'r': perms |= 00444; break;
		  case 'w': perms |= 00222; break;
		  case 'x': perms |= 00111; break;
		  case 't': perms |= 01000; subset |= 01000; break;
		  case 's':
		    if ((subset & 06777) != 04700 &&
			(subset & 06777) != 02070) {
			printf("chmod: file mode '%.*s': set[ug]id bit should"
			       " be used with exactly one of u or g only\n",
			       (int)strcspn(modebegin, ","), modebegin);
			return 0;
		    }
		    perms |= 06000;
		    break;
		  default:
		    printf("chmod: file mode '%.*s' contains unrecognised"
			   " permission specifier '%c'\n",
			   (int)strcspn(modebegin, ","), modebegin, *mode);
		    return 0;
		}
		mode++;
	    }
	    if (!(subset & 06777) && (perms &~ subset)) {
		printf("chmod: file mode '%.*s' contains no user/group/other"
		       " specifier and permissions other than 't' \n",
		       (int)strcspn(modebegin, ","), modebegin);
		return 0;
	    }
	    perms &= subset;
	    switch (action) {
	      case '+':
		ctx->attrs_clr |= perms;
		ctx->attrs_xor |= perms;
		break;
	      case '-':
		ctx->attrs_clr |= perms;
		ctx->attrs_xor &= ~perms;
		break;
	      case '=':
		ctx->attrs_clr |= subset;
		ctx->attrs_xor |= perms;
		break;
	    }
	    if (*mode) mode++;	       /* eat comma */
	}
    }

    ret = 1;
    for (i = 2; i < cmd->nwords; i++)
	ret &= wildcard_iterate(cmd->words[i], sftp_action_chmod, ctx);

    return ret;
}

static int sftp_cmd_open(struct sftp_command *cmd)
{
    int portnumber;

    if (back != NULL) {
	printf("psftp: already connected\n");
	return 0;
    }

    if (cmd->nwords < 2) {
	printf("open: expects a host name\n");
	return 0;
    }

    if (cmd->nwords > 2) {
	portnumber = atoi(cmd->words[2]);
	if (portnumber == 0) {
	    printf("open: invalid port number\n");
	    return 0;
	}
    } else
	portnumber = 0;

    if (psftp_connect(cmd->words[1], NULL, portnumber)) {
	back = NULL;		       /* connection is already closed */
	return -1;		       /* this is fatal */
    }
    do_sftp_init();
    return 1;
}

static int sftp_cmd_lcd(struct sftp_command *cmd)
{
    char *currdir, *errmsg;

    if (cmd->nwords < 2) {
	printf("lcd: expects a local directory name\n");
	return 0;
    }

    errmsg = psftp_lcd(cmd->words[1]);
    if (errmsg) {
	printf("lcd: unable to change directory: %s\n", errmsg);
	sfree(errmsg);
	return 0;
    }

    currdir = psftp_getcwd();
    printf("New local directory is %s\n", currdir);
    sfree(currdir);

    return 1;
}

static int sftp_cmd_lpwd(struct sftp_command *cmd)
{
    char *currdir;

    currdir = psftp_getcwd();
    printf("Current local directory is %s\n", currdir);
    sfree(currdir);

    return 1;
}

static int sftp_cmd_pling(struct sftp_command *cmd)
{
    int exitcode;

    exitcode = system(cmd->words[1]);
    return (exitcode == 0);
}

static int sftp_cmd_help(struct sftp_command *cmd);

static struct sftp_cmd_lookup {
    char *name;
    /*
     * For help purposes, there are two kinds of command:
     * 
     *  - primary commands, in which `longhelp' is non-NULL. In
     *    this case `shorthelp' is descriptive text, and `longhelp'
     *    is longer descriptive text intended to be printed after
     *    the command name.
     * 
     *  - alias commands, in which `longhelp' is NULL. In this case
     *    `shorthelp' is the name of a primary command, which
     *    contains the help that should double up for this command.
     */
    int listed;			       /* do we list this in primary help? */
    char *shorthelp;
    char *longhelp;
    int (*obey) (struct sftp_command *);
} sftp_lookup[] = {
    /*
     * List of sftp commands. This is binary-searched so it MUST be
     * in ASCII order.
     */
    {
	"!", TRUE, "run a local command",
	    "<command>\n"
	    /* FIXME: this example is crap for non-Windows. */
	    "  Runs a local command. For example, \"!del myfile\".\n",
	    sftp_cmd_pling
    },
    {
	"bye", TRUE, "finish your SFTP session",
	    "\n"
	    "  Terminates your SFTP session and quits the PSFTP program.\n",
	    sftp_cmd_quit
    },
    {
	"cd", TRUE, "change your remote working directory",
	    " [ <new working directory> ]\n"
	    "  Change the remote working directory for your SFTP session.\n"
	    "  If a new working directory is not supplied, you will be\n"
	    "  returned to your home directory.\n",
	    sftp_cmd_cd
    },
    {
	"chmod", TRUE, "change file permissions and modes",
	    " <modes> <filename-or-wildcard> [ <filename-or-wildcard>... ]\n"
	    "  Change the file permissions on one or more remote files or\n"
	    "  directories.\n"
	    "  <modes> can be any octal Unix permission specifier.\n"
	    "  Alternatively, <modes> can include the following modifiers:\n"
	    "    u+r     make file readable by owning user\n"
	    "    u+w     make file writable by owning user\n"
	    "    u+x     make file executable by owning user\n"
	    "    u-r     make file not readable by owning user\n"
	    "    [also u-w, u-x]\n"
	    "    g+r     make file readable by members of owning group\n"
	    "    [also g+w, g+x, g-r, g-w, g-x]\n"
	    "    o+r     make file readable by all other users\n"
	    "    [also o+w, o+x, o-r, o-w, o-x]\n"
	    "    a+r     make file readable by absolutely everybody\n"
	    "    [also a+w, a+x, a-r, a-w, a-x]\n"
	    "    u+s     enable the Unix set-user-ID bit\n"
	    "    u-s     disable the Unix set-user-ID bit\n"
	    "    g+s     enable the Unix set-group-ID bit\n"
	    "    g-s     disable the Unix set-group-ID bit\n"
	    "    +t      enable the Unix \"sticky bit\"\n"
	    "  You can give more than one modifier for the same user (\"g-rwx\"), and\n"
	    "  more than one user for the same modifier (\"ug+w\"). You can\n"
	    "  use commas to separate different modifiers (\"u+rwx,g+s\").\n",
	    sftp_cmd_chmod
    },
    {
	"close", TRUE, "finish your SFTP session but do not quit PSFTP",
	    "\n"
	    "  Terminates your SFTP session, but does not quit the PSFTP\n"
	    "  program. You can then use \"open\" to start another SFTP\n"
	    "  session, to the same server or to a different one.\n",
	    sftp_cmd_close
    },
    {
	"del", TRUE, "delete files on the remote server",
	    " <filename-or-wildcard> [ <filename-or-wildcard>... ]\n"
	    "  Delete a file or files from the server.\n",
	    sftp_cmd_rm
    },
    {
	"delete", FALSE, "del", NULL, sftp_cmd_rm
    },
    {
	"dir", TRUE, "list remote files",
	    " [ <directory-name> ]/[ <wildcard> ]\n"
	    "  List the contents of a specified directory on the server.\n"
	    "  If <directory-name> is not given, the current working directory\n"
	    "  is assumed.\n"
	    "  If <wildcard> is given, it is treated as a set of files to\n"
	    "  list; otherwise, all files are listed.\n",
	    sftp_cmd_ls
    },
    {
	"exit", TRUE, "bye", NULL, sftp_cmd_quit
    },
    {
	"get", TRUE, "download a file from the server to your local machine",
	    " [ -r ] [ -- ] <filename> [ <local-filename> ]\n"
	    "  Downloads a file on the server and stores it locally under\n"
	    "  the same name, or under a different one if you supply the\n"
	    "  argument <local-filename>.\n"
	    "  If -r specified, recursively fetch a directory.\n",
	    sftp_cmd_get
    },
    {
	"help", TRUE, "give help",
	    " [ <command> [ <command> ... ] ]\n"
	    "  Give general help if no commands are specified.\n"
	    "  If one or more commands are specified, give specific help on\n"
	    "  those particular commands.\n",
	    sftp_cmd_help
    },
    {
	"lcd", TRUE, "change local working directory",
	    " <local-directory-name>\n"
	    "  Change the local working directory of the PSFTP program (the\n"
	    "  default location where the \"get\" command will save files).\n",
	    sftp_cmd_lcd
    },
    {
	"lpwd", TRUE, "print local working directory",
	    "\n"
	    "  Print the local working directory of the PSFTP program (the\n"
	    "  default location where the \"get\" command will save files).\n",
	    sftp_cmd_lpwd
    },
    {
	"ls", TRUE, "dir", NULL,
	    sftp_cmd_ls
    },
    {
	"mget", TRUE, "download multiple files at once",
	    " [ -r ] [ -- ] <filename-or-wildcard> [ <filename-or-wildcard>... ]\n"
	    "  Downloads many files from the server, storing each one under\n"
	    "  the same name it has on the server side. You can use wildcards\n"
	    "  such as \"*.c\" to specify lots of files at once.\n"
	    "  If -r specified, recursively fetch files and directories.\n",
	    sftp_cmd_mget
    },
    {
	"mkdir", TRUE, "create directories on the remote server",
	    " <directory-name> [ <directory-name>... ]\n"
	    "  Creates directories with the given names on the server.\n",
	    sftp_cmd_mkdir
    },
    {
	"mput", TRUE, "upload multiple files at once",
	    " [ -r ] [ -- ] <filename-or-wildcard> [ <filename-or-wildcard>... ]\n"
	    "  Uploads many files to the server, storing each one under the\n"
	    "  same name it has on the client side. You can use wildcards\n"
	    "  such as \"*.c\" to specify lots of files at once.\n"
	    "  If -r specified, recursively store files and directories.\n",
	    sftp_cmd_mput
    },
    {
	"mv", TRUE, "move or rename file(s) on the remote server",
	    " <source> [ <source>... ] <destination>\n"
	    "  Moves or renames <source>(s) on the server to <destination>,\n"
	    "  also on the server.\n"
	    "  If <destination> specifies an existing directory, then <source>\n"
	    "  may be a wildcard, and multiple <source>s may be given; all\n"
	    "  source files are moved into <destination>.\n"
	    "  Otherwise, <source> must specify a single file, which is moved\n"
	    "  or renamed so that it is accessible under the name <destination>.\n",
	    sftp_cmd_mv
    },
    {
	"open", TRUE, "connect to a host",
	    " [<user>@]<hostname> [<port>]\n"
	    "  Establishes an SFTP connection to a given host. Only usable\n"
	    "  when you are not already connected to a server.\n",
	    sftp_cmd_open
    },
    {
	"put", TRUE, "upload a file from your local machine to the server",
	    " [ -r ] [ -- ] <filename> [ <remote-filename> ]\n"
	    "  Uploads a file to the server and stores it there under\n"
	    "  the same name, or under a different one if you supply the\n"
	    "  argument <remote-filename>.\n"
	    "  If -r specified, recursively store a directory.\n",
	    sftp_cmd_put
    },
    {
	"pwd", TRUE, "print your remote working directory",
	    "\n"
	    "  Print the current remote working directory for your SFTP session.\n",
	    sftp_cmd_pwd
    },
    {
	"quit", TRUE, "bye", NULL,
	    sftp_cmd_quit
    },
    {
	"reget", TRUE, "continue downloading files",
	    " [ -r ] [ -- ] <filename> [ <local-filename> ]\n"
	    "  Works exactly like the \"get\" command, but the local file\n"
	    "  must already exist. The download will begin at the end of the\n"
	    "  file. This is for resuming a download that was interrupted.\n"
	    "  If -r specified, resume interrupted \"get -r\".\n",
	    sftp_cmd_reget
    },
    {
	"ren", TRUE, "mv", NULL,
	    sftp_cmd_mv
    },
    {
	"rename", FALSE, "mv", NULL,
	    sftp_cmd_mv
    },
    {
	"reput", TRUE, "continue uploading files",
	    " [ -r ] [ -- ] <filename> [ <remote-filename> ]\n"
	    "  Works exactly like the \"put\" command, but the remote file\n"
	    "  must already exist. The upload will begin at the end of the\n"
	    "  file. This is for resuming an upload that was interrupted.\n"
	    "  If -r specified, resume interrupted \"put -r\".\n",
	    sftp_cmd_reput
    },
    {
	"rm", TRUE, "del", NULL,
	    sftp_cmd_rm
    },
    {
	"rmdir", TRUE, "remove directories on the remote server",
	    " <directory-name> [ <directory-name>... ]\n"
	    "  Removes the directory with the given name on the server.\n"
	    "  The directory will not be removed unless it is empty.\n"
	    "  Wildcards may be used to specify multiple directories.\n",
	    sftp_cmd_rmdir
    }
};

const struct sftp_cmd_lookup *lookup_command(char *name)
{
    int i, j, k, cmp;

    i = -1;
    j = sizeof(sftp_lookup) / sizeof(*sftp_lookup);
    while (j - i > 1) {
	k = (j + i) / 2;
	cmp = strcmp(name, sftp_lookup[k].name);
	if (cmp < 0)
	    j = k;
	else if (cmp > 0)
	    i = k;
	else {
	    return &sftp_lookup[k];
	}
    }
    return NULL;
}

static int sftp_cmd_help(struct sftp_command *cmd)
{
    int i;
    if (cmd->nwords == 1) {
	/*
	 * Give short help on each command.
	 */
	int maxlen;
	maxlen = 0;
	for (i = 0; i < sizeof(sftp_lookup) / sizeof(*sftp_lookup); i++) {
	    int len;
	    if (!sftp_lookup[i].listed)
		continue;
	    len = strlen(sftp_lookup[i].name);
	    if (maxlen < len)
		maxlen = len;
	}
	for (i = 0; i < sizeof(sftp_lookup) / sizeof(*sftp_lookup); i++) {
	    const struct sftp_cmd_lookup *lookup;
	    if (!sftp_lookup[i].listed)
		continue;
	    lookup = &sftp_lookup[i];
	    printf("%-*s", maxlen+2, lookup->name);
	    if (lookup->longhelp == NULL)
		lookup = lookup_command(lookup->shorthelp);
	    printf("%s\n", lookup->shorthelp);
	}
    } else {
	/*
	 * Give long help on specific commands.
	 */
	for (i = 1; i < cmd->nwords; i++) {
	    const struct sftp_cmd_lookup *lookup;
	    lookup = lookup_command(cmd->words[i]);
	    if (!lookup) {
		printf("help: %s: command not found\n", cmd->words[i]);
	    } else {
		printf("%s", lookup->name);
		if (lookup->longhelp == NULL)
		    lookup = lookup_command(lookup->shorthelp);
		printf("%s", lookup->longhelp);
	    }
	}
    }
    return 1;
}

/* ----------------------------------------------------------------------
 * Command line reading and parsing.
 */
struct sftp_command *sftp_getcmd(FILE *fp, int mode, int modeflags)
{
    char *line;
    struct sftp_command *cmd;
    char *p, *q, *r;
    int quoting;

    cmd = snew(struct sftp_command);
    cmd->words = NULL;
    cmd->nwords = 0;
    cmd->wordssize = 0;

    line = NULL;

    if (fp) {
	if (modeflags & 1)
	    printf("psftp> ");
	line = fgetline(fp);
    } else {
	line = ssh_sftp_get_cmdline("psftp> ", back == NULL);
    }

    if (!line || !*line) {
	cmd->obey = sftp_cmd_quit;
	if ((mode == 0) || (modeflags & 1))
	    printf("quit\n");
        sfree(line);
	return cmd;		       /* eof */
    }

    line[strcspn(line, "\r\n")] = '\0';

    if (modeflags & 1) {
	printf("%s\n", line);
    }

    p = line;
    while (*p && (*p == ' ' || *p == '\t'))
	p++;

    if (*p == '!') {
	/*
	 * Special case: the ! command. This is always parsed as
	 * exactly two words: one containing the !, and the second
	 * containing everything else on the line.
	 */
	cmd->nwords = cmd->wordssize = 2;
	cmd->words = sresize(cmd->words, cmd->wordssize, char *);
	cmd->words[0] = dupstr("!");
	cmd->words[1] = dupstr(p+1);
    } else if (*p == '#') {
	/*
	 * Special case: comment. Entire line is ignored.
	 */
	cmd->nwords = cmd->wordssize = 0;
    } else {

	/*
	 * Parse the command line into words. The syntax is:
	 *  - double quotes are removed, but cause spaces within to be
	 *    treated as non-separating.
	 *  - a double-doublequote pair is a literal double quote, inside
	 *    _or_ outside quotes. Like this:
	 *
	 *      firstword "second word" "this has ""quotes"" in" and""this""
	 *
	 * becomes
	 *
	 *      >firstword<
	 *      >second word<
	 *      >this has "quotes" in<
	 *      >and"this"<
	 */
	while (1) {
	    /* skip whitespace */
	    while (*p && (*p == ' ' || *p == '\t'))
		p++;
            /* terminate loop */
            if (!*p)
                break;
	    /* mark start of word */
	    q = r = p;		       /* q sits at start, r writes word */
	    quoting = 0;
	    while (*p) {
		if (!quoting && (*p == ' ' || *p == '\t'))
		    break;		       /* reached end of word */
		else if (*p == '"' && p[1] == '"')
		    p += 2, *r++ = '"';    /* a literal quote */
		else if (*p == '"')
		    p++, quoting = !quoting;
		else
		    *r++ = *p++;
	    }
	    if (*p)
		p++;		       /* skip over the whitespace */
	    *r = '\0';
	    if (cmd->nwords >= cmd->wordssize) {
		cmd->wordssize = cmd->nwords + 16;
		cmd->words = sresize(cmd->words, cmd->wordssize, char *);
	    }
	    cmd->words[cmd->nwords++] = dupstr(q);
	}
    }

    sfree(line);

    /*
     * Now parse the first word and assign a function.
     */

    if (cmd->nwords == 0)
	cmd->obey = sftp_cmd_null;
    else {
	const struct sftp_cmd_lookup *lookup;
	lookup = lookup_command(cmd->words[0]);
	if (!lookup)
	    cmd->obey = sftp_cmd_unknown;
	else
	    cmd->obey = lookup->obey;
    }

    return cmd;
}

static int do_sftp_init(void)
{
    struct sftp_packet *pktin;
    struct sftp_request *req;

    /*
     * Do protocol initialisation. 
     */
    if (!fxp_init()) {
	fprintf(stderr,
		"Fatal: unable to initialise SFTP: %s\n", fxp_error());
	return 1;		       /* failure */
    }

    /*
     * Find out where our home directory is.
     */
    req = fxp_realpath_send(".");
    pktin = sftp_wait_for_reply(req);
    homedir = fxp_realpath_recv(pktin, req);

    if (!homedir) {
	fprintf(stderr,
		"Warning: failed to resolve home directory: %s\n",
		fxp_error());
	homedir = dupstr(".");
    } else {
	printf("Remote working directory is %s\n", homedir);
    }
    pwd = dupstr(homedir);
    return 0;
}

void do_sftp_cleanup()
{
    char ch;
    if (back) {
	back->special(backhandle, TS_EOF);
        sent_eof = TRUE;
	sftp_recvdata(&ch, 1);
	back->free(backhandle);
	sftp_cleanup_request();
	back = NULL;
	backhandle = NULL;
    }
    if (pwd) {
	sfree(pwd);
	pwd = NULL;
    }
    if (homedir) {
	sfree(homedir);
	homedir = NULL;
    }
}

int do_sftp(int mode, int modeflags, char *batchfile)
{
    FILE *fp;
    int ret;

    /*
     * Batch mode?
     */
    if (mode == 0) {

        /* ------------------------------------------------------------------
         * Now we're ready to do Real Stuff.
         */
        while (1) {
	    struct sftp_command *cmd;
	    cmd = sftp_getcmd(NULL, 0, 0);
	    if (!cmd)
		break;
	    ret = cmd->obey(cmd);
	    if (cmd->words) {
		int i;
		for(i = 0; i < cmd->nwords; i++)
		    sfree(cmd->words[i]);
		sfree(cmd->words);
	    }
	    sfree(cmd);
	    if (ret < 0)
		break;
	}
    } else {
        fp = fopen(batchfile, "r");
        if (!fp) {
	    printf("Fatal: unable to open %s\n", batchfile);
	    return 1;
        }
	ret = 0;
        while (1) {
	    struct sftp_command *cmd;
	    cmd = sftp_getcmd(fp, mode, modeflags);
	    if (!cmd)
		break;
	    ret = cmd->obey(cmd);
	    if (ret < 0)
		break;
	    if (ret == 0) {
		if (!(modeflags & 2))
		    break;
	    }
        }
	fclose(fp);
	/*
	 * In batch mode, and if exit on command failure is enabled,
	 * any command failure causes the whole of PSFTP to fail.
	 */
	if (ret == 0 && !(modeflags & 2)) return 2;
    }
    return 0;
}

/* ----------------------------------------------------------------------
 * Dirty bits: integration with PuTTY.
 */

static int verbose = 0;

/*
 *  Print an error message and perform a fatal exit.
 */
void fatalbox(char *fmt, ...)
{
    char *str, *str2;
    va_list ap;
    va_start(ap, fmt);
    str = dupvprintf(fmt, ap);
    str2 = dupcat("Fatal: ", str, "\n", NULL);
    sfree(str);
    va_end(ap);
    fputs(str2, stderr);
    sfree(str2);

    cleanup_exit(1);
}
void modalfatalbox(char *fmt, ...)
{
    char *str, *str2;
    va_list ap;
    va_start(ap, fmt);
    str = dupvprintf(fmt, ap);
    str2 = dupcat("Fatal: ", str, "\n", NULL);
    sfree(str);
    va_end(ap);
    fputs(str2, stderr);
    sfree(str2);

    cleanup_exit(1);
}
void nonfatal(char *fmt, ...)
{
    char *str, *str2;
    va_list ap;
    va_start(ap, fmt);
    str = dupvprintf(fmt, ap);
    str2 = dupcat("Error: ", str, "\n", NULL);
    sfree(str);
    va_end(ap);
    fputs(str2, stderr);
    sfree(str2);
}
void connection_fatal(void *frontend, char *fmt, ...)
{
    char *str, *str2;
    va_list ap;
    va_start(ap, fmt);
    str = dupvprintf(fmt, ap);
    str2 = dupcat("Fatal: ", str, "\n", NULL);
    sfree(str);
    va_end(ap);
    fputs(str2, stderr);
    sfree(str2);

    cleanup_exit(1);
}

void ldisc_send(void *handle, char *buf, int len, int interactive)
{
    /*
     * This is only here because of the calls to ldisc_send(NULL,
     * 0) in ssh.c. Nothing in PSFTP actually needs to use the
     * ldisc as an ldisc. So if we get called with any real data, I
     * want to know about it.
     */
    assert(len == 0);
}

/*
 * In psftp, all agent requests should be synchronous, so this is a
 * never-called stub.
 */
void agent_schedule_callback(void (*callback)(void *, void *, int),
			     void *callback_ctx, void *data, int len)
{
    assert(!"We shouldn't be here");
}

/*
 * Receive a block of data from the SSH link. Block until all data
 * is available.
 *
 * To do this, we repeatedly call the SSH protocol module, with our
 * own trap in from_backend() to catch the data that comes back. We
 * do this until we have enough data.
 */

static unsigned char *outptr;	       /* where to put the data */
static unsigned outlen;		       /* how much data required */
static unsigned char *pending = NULL;  /* any spare data */
static unsigned pendlen = 0, pendsize = 0;	/* length and phys. size of buffer */
int from_backend(void *frontend, int is_stderr, const char *data, int datalen)
{
    unsigned char *p = (unsigned char *) data;
    unsigned len = (unsigned) datalen;

    /*
     * stderr data is just spouted to local stderr and otherwise
     * ignored.
     */
    if (is_stderr) {
	if (len > 0)
	    if (fwrite(data, 1, len, stderr) < len)
		/* oh well */;
	return 0;
    }

    /*
     * If this is before the real session begins, just return.
     */
    if (!outptr)
	return 0;

    if ((outlen > 0) && (len > 0)) {
	unsigned used = outlen;
	if (used > len)
	    used = len;
	memcpy(outptr, p, used);
	outptr += used;
	outlen -= used;
	p += used;
	len -= used;
    }

    if (len > 0) {
	if (pendsize < pendlen + len) {
	    pendsize = pendlen + len + 4096;
	    pending = sresize(pending, pendsize, unsigned char);
	}
	memcpy(pending + pendlen, p, len);
	pendlen += len;
    }

    return 0;
}
int from_backend_untrusted(void *frontend_handle, const char *data, int len)
{
    /*
     * No "untrusted" output should get here (the way the code is
     * currently, it's all diverted by FLAG_STDERR).
     */
    assert(!"Unexpected call to from_backend_untrusted()");
    return 0; /* not reached */
}
int from_backend_eof(void *frontend)
{
    /*
     * We expect to be the party deciding when to close the
     * connection, so if we see EOF before we sent it ourselves, we
     * should panic.
     */
    if (!sent_eof) {
        connection_fatal(frontend,
                         "Received unexpected end-of-file from SFTP server");
    }
    return FALSE;
}
int sftp_recvdata(char *buf, int len)
{
    outptr = (unsigned char *) buf;
    outlen = len;

    /*
     * See if the pending-input block contains some of what we
     * need.
     */
    if (pendlen > 0) {
	unsigned pendused = pendlen;
	if (pendused > outlen)
	    pendused = outlen;
	memcpy(outptr, pending, pendused);
	memmove(pending, pending + pendused, pendlen - pendused);
	outptr += pendused;
	outlen -= pendused;
	pendlen -= pendused;
	if (pendlen == 0) {
	    pendsize = 0;
	    sfree(pending);
	    pending = NULL;
	}
	if (outlen == 0)
	    return 1;
    }

    while (outlen > 0) {
	if (back->exitcode(backhandle) >= 0 || ssh_sftp_loop_iteration() < 0)
	    return 0;		       /* doom */
    }

    return 1;
}
int sftp_senddata(char *buf, int len)
{
    back->send(backhandle, buf, len);
    return 1;
}

/*
 *  Short description of parameters.
 */
static void usage(void)
{
    printf("PuTTY Secure File Transfer (SFTP) client\n");
    printf("%s\n", ver);
    printf("Usage: psftp [options] [user@]host\n");
    printf("Options:\n");
    printf("  -V        print version information and exit\n");
    printf("  -pgpfp    print PGP key fingerprints and exit\n");
    printf("  -b file   use specified batchfile\n");
    printf("  -bc       output batchfile commands\n");
    printf("  -be       don't stop batchfile processing if errors\n");
    printf("  -v        show verbose messages\n");
    printf("  -load sessname  Load settings from saved session\n");
    printf("  -l user   connect with specified username\n");
    printf("  -P port   connect to specified port\n");
    printf("  -pw passw login with specified password\n");
    printf("  -1 -2     force use of particular SSH protocol version\n");
    printf("  -4 -6     force use of IPv4 or IPv6\n");
    printf("  -C        enable compression\n");
    printf("  -i key    private key file for user authentication\n");
    printf("  -noagent  disable use of Pageant\n");
    printf("  -agent    enable use of Pageant\n");
    printf("  -hostkey aa:bb:cc:...\n");
    printf("            manually specify a host key (may be repeated)\n");
    printf("  -batch    disable all interactive prompts\n");
    printf("  -sshlog file\n");
    printf("  -sshrawlog file\n");
    printf("            log protocol details to a file\n");
    cleanup_exit(1);
}

static void version(void)
{
  printf("psftp: %s\n", ver);
  cleanup_exit(1);
}

/*
 * Connect to a host.
 */
static int psftp_connect(char *userhost, char *user, int portnumber)
{
    char *host, *realhost;
    const char *err;
    void *logctx;

    /* Separate host and username */
    host = userhost;
    host = strrchr(host, '@');
    if (host == NULL) {
	host = userhost;
    } else {
	*host++ = '\0';
	if (user) {
	    printf("psftp: multiple usernames specified; using \"%s\"\n",
		   user);
	} else
	    user = userhost;
    }

    /*
     * If we haven't loaded session details already (e.g., from -load),
     * try looking for a session called "host".
     */
    if (!loaded_session) {
	/* Try to load settings for `host' into a temporary config */
	Conf *conf2 = conf_new();
	conf_set_str(conf2, CONF_host, "");
	do_defaults(host, conf2);
	if (conf_get_str(conf2, CONF_host)[0] != '\0') {
	    /* Settings present and include hostname */
	    /* Re-load data into the real config. */
	    do_defaults(host, conf);
	} else {
	    /* Session doesn't exist or mention a hostname. */
	    /* Use `host' as a bare hostname. */
	    conf_set_str(conf, CONF_host, host);
	}
        conf_free(conf2);
    } else {
	/* Patch in hostname `host' to session details. */
	conf_set_str(conf, CONF_host, host);
    }

    /*
     * Force use of SSH. (If they got the protocol wrong we assume the
     * port is useless too.)
     */
    if (conf_get_int(conf, CONF_protocol) != PROT_SSH) {
        conf_set_int(conf, CONF_protocol, PROT_SSH);
        conf_set_int(conf, CONF_port, 22);
    }

    /*
     * If saved session / Default Settings says SSH-1 (`1 only' or `1'),
     * then change it to SSH-2, on the grounds that that's more likely to
     * work for SFTP. (Can be overridden with `-1' option.)
     * But if it says `2 only' or `2', respect which.
     */
    if ((conf_get_int(conf, CONF_sshprot) & ~1) != 2)   /* is it 2 or 3? */
	conf_set_int(conf, CONF_sshprot, 2);

    /*
     * Enact command-line overrides.
     */
    cmdline_run_saved(conf);

    /*
     * Muck about with the hostname in various ways.
     */
    {
	char *hostbuf = dupstr(conf_get_str(conf, CONF_host));
	char *host = hostbuf;
	char *p, *q;

	/*
	 * Trim leading whitespace.
	 */
	host += strspn(host, " \t");

	/*
	 * See if host is of the form user@host, and separate out
	 * the username if so.
	 */
	if (host[0] != '\0') {
	    char *atsign = strrchr(host, '@');
	    if (atsign) {
		*atsign = '\0';
		conf_set_str(conf, CONF_username, host);
		host = atsign + 1;
	    }
	}

	/*
	 * Remove any remaining whitespace.
	 */
	p = hostbuf;
	q = host;
	while (*q) {
	    if (*q != ' ' && *q != '\t')
		*p++ = *q;
	    q++;
	}
	*p = '\0';

	conf_set_str(conf, CONF_host, hostbuf);
	sfree(hostbuf);
    }

    /* Set username */
    if (user != NULL && user[0] != '\0') {
	conf_set_str(conf, CONF_username, user);
    }

    if (portnumber)
	conf_set_int(conf, CONF_port, portnumber);

    /*
     * Disable scary things which shouldn't be enabled for simple
     * things like SCP and SFTP: agent forwarding, port forwarding,
     * X forwarding.
     */
    conf_set_int(conf, CONF_x11_forward, 0);
    conf_set_int(conf, CONF_agentfwd, 0);
    conf_set_int(conf, CONF_ssh_simple, TRUE);
    {
	char *key;
	while ((key = conf_get_str_nthstrkey(conf, CONF_portfwd, 0)) != NULL)
	    conf_del_str_str(conf, CONF_portfwd, key);
    }

    /* Set up subsystem name. */
    conf_set_str(conf, CONF_remote_cmd, "sftp");
    conf_set_int(conf, CONF_ssh_subsys, TRUE);
    conf_set_int(conf, CONF_nopty, TRUE);

    /*
     * Set up fallback option, for SSH-1 servers or servers with the
     * sftp subsystem not enabled but the server binary installed
     * in the usual place. We only support fallback on Unix
     * systems, and we use a kludgy piece of shellery which should
     * try to find sftp-server in various places (the obvious
     * systemwide spots /usr/lib and /usr/local/lib, and then the
     * user's PATH) and finally give up.
     * 
     *   test -x /usr/lib/sftp-server && exec /usr/lib/sftp-server
     *   test -x /usr/local/lib/sftp-server && exec /usr/local/lib/sftp-server
     *   exec sftp-server
     * 
     * the idea being that this will attempt to use either of the
     * obvious pathnames and then give up, and when it does give up
     * it will print the preferred pathname in the error messages.
     */
    conf_set_str(conf, CONF_remote_cmd2,
		 "test -x /usr/lib/sftp-server &&"
		 " exec /usr/lib/sftp-server\n"
		 "test -x /usr/local/lib/sftp-server &&"
		 " exec /usr/local/lib/sftp-server\n"
		 "exec sftp-server");
    conf_set_int(conf, CONF_ssh_subsys2, FALSE);

    back = &ssh_backend;

    err = back->init(NULL, &backhandle, conf,
		     conf_get_str(conf, CONF_host),
		     conf_get_int(conf, CONF_port),
		     &realhost, 0,
		     conf_get_int(conf, CONF_tcp_keepalives));
    if (err != NULL) {
	fprintf(stderr, "ssh_init: %s\n", err);
	return 1;
    }
    logctx = log_init(NULL, conf);
    back->provide_logctx(backhandle, logctx);
    console_provide_logctx(logctx);
    while (!back->sendok(backhandle)) {
	if (back->exitcode(backhandle) >= 0)
	    return 1;
	if (ssh_sftp_loop_iteration() < 0) {
	    fprintf(stderr, "ssh_init: error during SSH connection setup\n");
	    return 1;
	}
    }
    if (verbose && realhost != NULL)
	printf("Connected to %s\n", realhost);
    if (realhost != NULL)
	sfree(realhost);
    return 0;
}

void cmdline_error(char *p, ...)
{
    va_list ap;
    fprintf(stderr, "psftp: ");
    va_start(ap, p);
    vfprintf(stderr, p, ap);
    va_end(ap);
    fprintf(stderr, "\n       try typing \"psftp -h\" for help\n");
    exit(1);
}

const int share_can_be_downstream = TRUE;
const int share_can_be_upstream = FALSE;

/*
 * Main program. Parse arguments etc.
 */
int psftp_main(int argc, char *argv[])
{
    int i, ret;
    int portnumber = 0;
    char *userhost, *user;
    int mode = 0;
    int modeflags = 0;
    char *batchfile = NULL;

    flags = FLAG_STDERR | FLAG_INTERACTIVE
#ifdef FLAG_SYNCAGENT
	| FLAG_SYNCAGENT
#endif
	;
    cmdline_tooltype = TOOLTYPE_FILETRANSFER;
    sk_init();

    userhost = user = NULL;

    /* Load Default Settings before doing anything else. */
    conf = conf_new();
    do_defaults(NULL, conf);
    loaded_session = FALSE;

    for (i = 1; i < argc; i++) {
	int ret;
	if (argv[i][0] != '-') {
            if (userhost)
                usage();
            else
                userhost = dupstr(argv[i]);
	    continue;
	}
	ret = cmdline_process_param(argv[i], i+1<argc?argv[i+1]:NULL, 1, conf);
	if (ret == -2) {
	    cmdline_error("option \"%s\" requires an argument", argv[i]);
	} else if (ret == 2) {
	    i++;	       /* skip next argument */
	} else if (ret == 1) {
	    /* We have our own verbosity in addition to `flags'. */
	    if (flags & FLAG_VERBOSE)
		verbose = 1;
	} else if (strcmp(argv[i], "-h") == 0 ||
		   strcmp(argv[i], "-?") == 0 ||
                   strcmp(argv[i], "--help") == 0) {
	    usage();
        } else if (strcmp(argv[i], "-pgpfp") == 0) {
            pgp_fingerprints();
            return 1;
	} else if (strcmp(argv[i], "-V") == 0 ||
                   strcmp(argv[i], "--version") == 0) {
	    version();
	} else if (strcmp(argv[i], "-batch") == 0) {
	    console_batch_mode = 1;
	} else if (strcmp(argv[i], "-b") == 0 && i + 1 < argc) {
	    mode = 1;
	    batchfile = argv[++i];
	} else if (strcmp(argv[i], "-bc") == 0) {
	    modeflags = modeflags | 1;
	} else if (strcmp(argv[i], "-be") == 0) {
	    modeflags = modeflags | 2;
	} else if (strcmp(argv[i], "--") == 0) {
	    i++;
	    break;
	} else {
	    cmdline_error("unknown option \"%s\"", argv[i]);
	}
    }
    argc -= i;
    argv += i;
    back = NULL;

    /*
     * If the loaded session provides a hostname, and a hostname has not
     * otherwise been specified, pop it in `userhost' so that
     * `psftp -load sessname' is sufficient to start a session.
     */
    if (!userhost && conf_get_str(conf, CONF_host)[0] != '\0') {
	userhost = dupstr(conf_get_str(conf, CONF_host));
    }

    /*
     * If a user@host string has already been provided, connect to
     * it now.
     */
    if (userhost) {
	int ret;
	ret = psftp_connect(userhost, user, portnumber);
	sfree(userhost);
	if (ret)
	    return 1;
	if (do_sftp_init())
	    return 1;
    } else {
	printf("psftp: no hostname specified; use \"open host.name\""
	       " to connect\n");
    }

    ret = do_sftp(mode, modeflags, batchfile);

    if (back != NULL && back->connected(backhandle)) {
	char ch;
	back->special(backhandle, TS_EOF);
        sent_eof = TRUE;
	sftp_recvdata(&ch, 1);
    }
    do_sftp_cleanup();
    random_save_seed();
    cmdline_cleanup();
    console_provide_logctx(NULL);
    sk_cleanup();

    return ret;
}
