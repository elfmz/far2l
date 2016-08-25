/*
 * uxstore.c: Unix-specific implementation of the interface defined
 * in storage.h.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <ctype.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include "putty.h"
#include "storage.h"
#include "tree234.h"

#ifdef PATH_MAX
#define FNLEN PATH_MAX
#else
#define FNLEN 1024 /* XXX */
#endif

enum {
    INDEX_DIR, INDEX_HOSTKEYS, INDEX_HOSTKEYS_TMP, INDEX_RANDSEED,
    INDEX_SESSIONDIR, INDEX_SESSION,
};

static const char hex[16] = "0123456789ABCDEF";

static char *mungestr(const char *in)
{
    char *out, *ret;

    if (!in || !*in)
        in = "Default Settings";

    ret = out = snewn(3*strlen(in)+1, char);

    while (*in) {
        /*
         * There are remarkably few punctuation characters that
         * aren't shell-special in some way or likely to be used as
         * separators in some file format or another! Hence we use
         * opt-in for safe characters rather than opt-out for
         * specific unsafe ones...
         */
	if (*in!='+' && *in!='-' && *in!='.' && *in!='@' && *in!='_' &&
            !(*in >= '0' && *in <= '9') &&
            !(*in >= 'A' && *in <= 'Z') &&
            !(*in >= 'a' && *in <= 'z')) {
	    *out++ = '%';
	    *out++ = hex[((unsigned char) *in) >> 4];
	    *out++ = hex[((unsigned char) *in) & 15];
	} else
	    *out++ = *in;
	in++;
    }
    *out = '\0';
    return ret;
}

static char *unmungestr(const char *in)
{
    char *out, *ret;
    out = ret = snewn(strlen(in)+1, char);
    while (*in) {
	if (*in == '%' && in[1] && in[2]) {
	    int i, j;

	    i = in[1] - '0';
	    i -= (i > 9 ? 7 : 0);
	    j = in[2] - '0';
	    j -= (j > 9 ? 7 : 0);

	    *out++ = (i << 4) + j;
	    in += 3;
	} else {
	    *out++ = *in++;
	}
    }
    *out = '\0';
    return ret;
}

static char *make_filename(int index, const char *subname)
{
    char *env, *tmp, *ret;

    /*
     * Allow override of the PuTTY configuration location, and of
     * specific subparts of it, by means of environment variables.
     */
    if (index == INDEX_DIR) {
	struct passwd *pwd;

	env = getenv("PUTTYDIR");
	if (env)
	    return dupstr(env);
	env = getenv("HOME");
	if (env)
	    return dupprintf("%s/.putty", env);
	pwd = getpwuid(getuid());
	if (pwd && pwd->pw_dir)
	    return dupprintf("%s/.putty", pwd->pw_dir);
	return dupstr("/.putty");
    }
    if (index == INDEX_SESSIONDIR) {
	env = getenv("PUTTYSESSIONS");
	if (env)
	    return dupstr(env);
	tmp = make_filename(INDEX_DIR, NULL);
	ret = dupprintf("%s/sessions", tmp);
	sfree(tmp);
	return ret;
    }
    if (index == INDEX_SESSION) {
        char *munged = mungestr(subname);
	tmp = make_filename(INDEX_SESSIONDIR, NULL);
	ret = dupprintf("%s/%s", tmp, munged);
	sfree(tmp);
	sfree(munged);
	return ret;
    }
    if (index == INDEX_HOSTKEYS) {
	env = getenv("PUTTYSSHHOSTKEYS");
	if (env)
	    return dupstr(env);
	tmp = make_filename(INDEX_DIR, NULL);
	ret = dupprintf("%s/sshhostkeys", tmp);
	sfree(tmp);
	return ret;
    }
    if (index == INDEX_HOSTKEYS_TMP) {
	tmp = make_filename(INDEX_HOSTKEYS, NULL);
	ret = dupprintf("%s.tmp", tmp);
	sfree(tmp);
	return ret;
    }
    if (index == INDEX_RANDSEED) {
	env = getenv("PUTTYRANDOMSEED");
	if (env)
	    return dupstr(env);
	tmp = make_filename(INDEX_DIR, NULL);
	ret = dupprintf("%s/randomseed", tmp);
	sfree(tmp);
	return ret;
    }
    tmp = make_filename(INDEX_DIR, NULL);
    ret = dupprintf("%s/ERROR", tmp);
    sfree(tmp);
    return ret;
}

void *open_settings_w(const char *sessionname, char **errmsg)
{
    char *filename;
    FILE *fp;

    *errmsg = NULL;

    /*
     * Start by making sure the .putty directory and its sessions
     * subdir actually exist.
     */
    filename = make_filename(INDEX_DIR, NULL);
    if (mkdir(filename, 0700) < 0 && errno != EEXIST) {
        *errmsg = dupprintf("Unable to save session: mkdir(\"%s\") "
                            "returned '%s'", filename, strerror(errno));
        sfree(filename);
        return NULL;
    }
    sfree(filename);

    filename = make_filename(INDEX_SESSIONDIR, NULL);
    if (mkdir(filename, 0700) < 0 && errno != EEXIST) {
        *errmsg = dupprintf("Unable to save session: mkdir(\"%s\") "
                            "returned '%s'", filename, strerror(errno));
        sfree(filename);
        return NULL;
    }
    sfree(filename);

    filename = make_filename(INDEX_SESSION, sessionname);
    fp = fopen(filename, "w");
    if (!fp) {
        *errmsg = dupprintf("Unable to save session: open(\"%s\") "
                            "returned '%s'", filename, strerror(errno));
	sfree(filename);
	return NULL;                   /* can't open */
    }
    sfree(filename);
    return fp;
}

void write_setting_s(void *handle, const char *key, const char *value)
{
    FILE *fp = (FILE *)handle;
    fprintf(fp, "%s=%s\n", key, value);
}

void write_setting_i(void *handle, const char *key, int value)
{
    FILE *fp = (FILE *)handle;
    fprintf(fp, "%s=%d\n", key, value);
}

void close_settings_w(void *handle)
{
    FILE *fp = (FILE *)handle;
    fclose(fp);
}

/*
 * Reading settings, for the moment, is done by retrieving X
 * resources from the X display. When we introduce disk files, I
 * think what will happen is that the X resources will override
 * PuTTY's inbuilt defaults, but that the disk files will then
 * override those. This isn't optimal, but it's the best I can
 * immediately work out.
 * FIXME: the above comment is a bit out of date. Did it happen?
 */

struct skeyval {
    const char *key;
    const char *value;
};

static tree234 *xrmtree = NULL;

int keycmp(void *av, void *bv)
{
    struct skeyval *a = (struct skeyval *)av;
    struct skeyval *b = (struct skeyval *)bv;
    return strcmp(a->key, b->key);
}

void provide_xrm_string(char *string)
{
    char *p, *q, *key;
    struct skeyval *xrms, *ret;

    p = q = strchr(string, ':');
    if (!q) {
	fprintf(stderr, "pterm: expected a colon in resource string"
		" \"%s\"\n", string);
	return;
    }
    q++;
    while (p > string && p[-1] != '.' && p[-1] != '*')
	p--;
    xrms = snew(struct skeyval);
    key = snewn(q-p, char);
    memcpy(key, p, q-p);
    key[q-p-1] = '\0';
    xrms->key = key;
    while (*q && isspace((unsigned char)*q))
	q++;
    xrms->value = dupstr(q);

    if (!xrmtree)
	xrmtree = newtree234(keycmp);

    ret = add234(xrmtree, xrms);
    if (ret) {
	/* Override an existing string. */
	del234(xrmtree, ret);
	add234(xrmtree, xrms);
    }
}

const char *get_setting(const char *key)
{
    struct skeyval tmp, *ret;
    tmp.key = key;
    if (xrmtree) {
	ret = find234(xrmtree, &tmp, NULL);
	if (ret)
	    return ret->value;
    }
    return x_get_default(key);
}

void *open_settings_r(const char *sessionname)
{
    char *filename;
    FILE *fp;
    char *line;
    tree234 *ret;

    filename = make_filename(INDEX_SESSION, sessionname);
    fp = fopen(filename, "r");
    sfree(filename);
    if (!fp)
	return NULL;		       /* can't open */

    ret = newtree234(keycmp);

    while ( (line = fgetline(fp)) ) {
        char *value = strchr(line, '=');
        struct skeyval *kv;

        if (!value) {
            sfree(line);
            continue;
        }
        *value++ = '\0';
        value[strcspn(value, "\r\n")] = '\0';   /* trim trailing NL */

        kv = snew(struct skeyval);
        kv->key = dupstr(line);
        kv->value = dupstr(value);
        add234(ret, kv);

        sfree(line);
    }

    fclose(fp);

    return ret;
}

char *read_setting_s(void *handle, const char *key)
{
    tree234 *tree = (tree234 *)handle;
    const char *val;
    struct skeyval tmp, *kv;

    tmp.key = key;
    if (tree != NULL &&
        (kv = find234(tree, &tmp, NULL)) != NULL) {
        val = kv->value;
        assert(val != NULL);
    } else
        val = get_setting(key);

    if (!val)
	return NULL;
    else
	return dupstr(val);
}

int read_setting_i(void *handle, const char *key, int defvalue)
{
    tree234 *tree = (tree234 *)handle;
    const char *val;
    struct skeyval tmp, *kv;

    tmp.key = key;
    if (tree != NULL &&
        (kv = find234(tree, &tmp, NULL)) != NULL) {
        val = kv->value;
        assert(val != NULL);
    } else
        val = get_setting(key);

    if (!val)
	return defvalue;
    else
	return atoi(val);
}

FontSpec *read_setting_fontspec(void *handle, const char *name)
{
    /*
     * In GTK1-only PuTTY, we used to store font names simply as a
     * valid X font description string (logical or alias), under a
     * bare key such as "Font".
     * 
     * In GTK2 PuTTY, we have a prefix system where "client:"
     * indicates a Pango font and "server:" an X one; existing
     * configuration needs to be reinterpreted as having the
     * "server:" prefix, so we change the storage key from the
     * provided name string (e.g. "Font") to a suffixed one
     * ("FontName").
     */
    char *suffname = dupcat(name, "Name", NULL);
    char *tmp;

    if ((tmp = read_setting_s(handle, suffname)) != NULL) {
        FontSpec *fs = fontspec_new(tmp);
	sfree(suffname);
	sfree(tmp);
	return fs;		       /* got new-style name */
    }
    sfree(suffname);

    /* Fall back to old-style name. */
    tmp = read_setting_s(handle, name);
    if (tmp && *tmp) {
        char *tmp2 = dupcat("server:", tmp, NULL);
        FontSpec *fs = fontspec_new(tmp2);
	sfree(tmp2);
	sfree(tmp);
	return fs;
    } else {
	sfree(tmp);
	return NULL;
    }
}
Filename *read_setting_filename(void *handle, const char *name)
{
    char *tmp = read_setting_s(handle, name);
    if (tmp) {
        Filename *ret = filename_from_str(tmp);
	sfree(tmp);
	return ret;
    } else
	return NULL;
}

void write_setting_fontspec(void *handle, const char *name, FontSpec *fs)
{
    /*
     * read_setting_fontspec had to handle two cases, but when
     * writing our settings back out we simply always generate the
     * new-style name.
     */
    char *suffname = dupcat(name, "Name", NULL);
    write_setting_s(handle, suffname, fs->name);
    sfree(suffname);
}
void write_setting_filename(void *handle, const char *name, Filename *result)
{
    write_setting_s(handle, name, result->path);
}

void close_settings_r(void *handle)
{
    tree234 *tree = (tree234 *)handle;
    struct skeyval *kv;

    if (!tree)
        return;

    while ( (kv = index234(tree, 0)) != NULL) {
        del234(tree, kv);
        sfree((char *)kv->key);
        sfree((char *)kv->value);
        sfree(kv);
    }

    freetree234(tree);
}

void del_settings(const char *sessionname)
{
    char *filename;
    filename = make_filename(INDEX_SESSION, sessionname);
    unlink(filename);
    sfree(filename);
}

void *enum_settings_start(void)
{
    DIR *dp;
    char *filename;

    filename = make_filename(INDEX_SESSIONDIR, NULL);
    dp = opendir(filename);
    sfree(filename);

    return dp;
}

char *enum_settings_next(void *handle, char *buffer, int buflen)
{
    DIR *dp = (DIR *)handle;
    struct dirent *de;
    struct stat st;
    char *fullpath;
    int maxlen, thislen, len;
    char *unmunged;

    fullpath = make_filename(INDEX_SESSIONDIR, NULL);
    maxlen = len = strlen(fullpath);

    while ( (de = readdir(dp)) != NULL ) {
        thislen = len + 1 + strlen(de->d_name);
	if (maxlen < thislen) {
	    maxlen = thislen;
	    fullpath = sresize(fullpath, maxlen+1, char);
	}
	fullpath[len] = '/';
	strncpy(fullpath+len+1, de->d_name, thislen - (len+1));
	fullpath[thislen] = '\0';

        if (stat(fullpath, &st) < 0 || !S_ISREG(st.st_mode))
            continue;                  /* try another one */

        unmunged = unmungestr(de->d_name);
        strncpy(buffer, unmunged, buflen);
        buffer[buflen-1] = '\0';
        sfree(unmunged);
	sfree(fullpath);
        return buffer;
    }

    sfree(fullpath);
    return NULL;
}

void enum_settings_finish(void *handle)
{
    DIR *dp = (DIR *)handle;
    closedir(dp);
}

/*
 * Lines in the host keys file are of the form
 * 
 *   type@port:hostname keydata
 * 
 * e.g.
 * 
 *   rsa@22:foovax.example.org 0x23,0x293487364395345345....2343
 */
int verify_host_key(const char *hostname, int port,
		    const char *keytype, const char *key)
{
    FILE *fp;
    char *filename;
    char *line;
    int ret;

    filename = make_filename(INDEX_HOSTKEYS, NULL);
    fp = fopen(filename, "r");
    sfree(filename);
    if (!fp)
	return 1;		       /* key does not exist */

    ret = 1;
    while ( (line = fgetline(fp)) ) {
	int i;
	char *p = line;
	char porttext[20];

	line[strcspn(line, "\n")] = '\0';   /* strip trailing newline */

	i = strlen(keytype);
	if (strncmp(p, keytype, i))
	    goto done;
	p += i;

	if (*p != '@')
	    goto done;
	p++;

	sprintf(porttext, "%d", port);
	i = strlen(porttext);
	if (strncmp(p, porttext, i))
	    goto done;
	p += i;

	if (*p != ':')
	    goto done;
	p++;

	i = strlen(hostname);
	if (strncmp(p, hostname, i))
	    goto done;
	p += i;

	if (*p != ' ')
	    goto done;
	p++;

	/*
	 * Found the key. Now just work out whether it's the right
	 * one or not.
	 */
	if (!strcmp(p, key))
	    ret = 0;		       /* key matched OK */
	else
	    ret = 2;		       /* key mismatch */

	done:
	sfree(line);
	if (ret != 1)
	    break;
    }

    fclose(fp);
    return ret;
}

void store_host_key(const char *hostname, int port,
		    const char *keytype, const char *key)
{
    FILE *rfp, *wfp;
    char *newtext, *line;
    int headerlen;
    char *filename, *tmpfilename;

    /*
     * Open both the old file and a new file.
     */
    tmpfilename = make_filename(INDEX_HOSTKEYS_TMP, NULL);
    wfp = fopen(tmpfilename, "w");
    if (!wfp && errno == ENOENT) {
        char *dir;

        dir = make_filename(INDEX_DIR, NULL);
        if (mkdir(dir, 0700) < 0) {
            nonfatal("Unable to store host key: mkdir(\"%s\") "
                     "returned '%s'", dir, strerror(errno));
            sfree(dir);
            sfree(tmpfilename);
            return;
        }
	sfree(dir);

        wfp = fopen(tmpfilename, "w");
    }
    if (!wfp) {
        nonfatal("Unable to store host key: open(\"%s\") "
                 "returned '%s'", tmpfilename, strerror(errno));
        sfree(tmpfilename);
        return;
    }
    filename = make_filename(INDEX_HOSTKEYS, NULL);
    rfp = fopen(filename, "r");

    newtext = dupprintf("%s@%d:%s %s\n", keytype, port, hostname, key);
    headerlen = 1 + strcspn(newtext, " ");   /* count the space too */

    /*
     * Copy all lines from the old file to the new one that _don't_
     * involve the same host key identifier as the one we're adding.
     */
    if (rfp) {
        while ( (line = fgetline(rfp)) ) {
            if (strncmp(line, newtext, headerlen))
                fputs(line, wfp);
            sfree(line);
        }
        fclose(rfp);
    }

    /*
     * Now add the new line at the end.
     */
    fputs(newtext, wfp);

    fclose(wfp);

    if (rename(tmpfilename, filename) < 0) {
        nonfatal("Unable to store host key: rename(\"%s\",\"%s\")"
                 " returned '%s'", tmpfilename, filename,
                 strerror(errno));
    }

    sfree(tmpfilename);
    sfree(filename);
    sfree(newtext);
}

void read_random_seed(noise_consumer_t consumer)
{
    int fd;
    char *fname;

    fname = make_filename(INDEX_RANDSEED, NULL);
    fd = open(fname, O_RDONLY);
    sfree(fname);
    if (fd >= 0) {
	char buf[512];
	int ret;
	while ( (ret = read(fd, buf, sizeof(buf))) > 0)
	    consumer(buf, ret);
	close(fd);
    }
}

void write_random_seed(void *data, int len)
{
    int fd;
    char *fname;

    fname = make_filename(INDEX_RANDSEED, NULL);
    /*
     * Don't truncate the random seed file if it already exists; if
     * something goes wrong half way through writing it, it would
     * be better to leave the old data there than to leave it empty.
     */
    fd = open(fname, O_CREAT | O_WRONLY, 0600);
    if (fd < 0) {
        if (errno != ENOENT) {
            nonfatal("Unable to write random seed: open(\"%s\") "
                     "returned '%s'", fname, strerror(errno));
            sfree(fname);
            return;
        }
	char *dir;

	dir = make_filename(INDEX_DIR, NULL);
	if (mkdir(dir, 0700) < 0) {
            nonfatal("Unable to write random seed: mkdir(\"%s\") "
                     "returned '%s'", dir, strerror(errno));
            sfree(fname);
            sfree(dir);
            return;
        }
	sfree(dir);

	fd = open(fname, O_CREAT | O_WRONLY, 0600);
        if (fd < 0) {
            nonfatal("Unable to write random seed: open(\"%s\") "
                     "returned '%s'", fname, strerror(errno));
            sfree(fname);
            return;
        }
    }

    while (len > 0) {
	int ret = write(fd, data, len);
	if (ret < 0) {
            nonfatal("Unable to write random seed: write "
                     "returned '%s'", strerror(errno));
            break;
        }
	len -= ret;
	data = (char *)data + len;
    }

    close(fd);
    sfree(fname);
}

void cleanup_all(void)
{
}

#ifdef MPEXT

void putty_mungestr(const char *in, char *out)
{
  char *o = mungestr(in);
  strcpy(out, o);
  free(o);
}

void putty_unmungestr(const char *in, char *out, int outlen)
{
  char *o = unmungestr(in);
  strncpy(out, o, outlen);
  free(o);
}

#endif
