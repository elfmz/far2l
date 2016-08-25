#ifndef PUTTY_UNIX_H
#define PUTTY_UNIX_H

#ifdef HAVE_CONFIG_H
# include "uxconfig.h" /* Space to hide it from mkfiles.pl */
#endif

#include <stdio.h>		       /* for FILENAME_MAX */
#include <stdint.h>		       /* C99 int types */
#ifndef NO_LIBDL
#include <dlfcn.h>		       /* Dynamic library loading */
#endif /*  NO_LIBDL */
#include "charset.h"

struct Filename {
    char *path;
};
FILE *f_open(const struct Filename *, char const *, int);

struct FontSpec {
    char *name;    /* may be "" to indicate no selected font at all */
};
struct FontSpec *fontspec_new(const char *name);

typedef void *Context;                 /* FIXME: probably needs changing */

extern Backend pty_backend;

typedef uint32_t uint32; /* C99: uint32_t defined in stdint.h */
#define PUTTY_UINT32_DEFINED

/*
 * Under GTK, we send MA_CLICK _and_ MA_2CLK, or MA_CLICK _and_
 * MA_3CLK, when a button is pressed for the second or third time.
 */
#define MULTICLICK_ONLY_EVENT 0

/*
 * Under GTK, there is no context help available.
 */
#define HELPCTX(x) P(NULL)
#define FILTER_KEY_FILES NULL          /* FIXME */
#define FILTER_DYNLIB_FILES NULL       /* FIXME */

/*
 * Under X, selection data must not be NUL-terminated.
 */
#define SELECTION_NUL_TERMINATED 0

/*
 * Under X, copying to the clipboard terminates lines with just LF.
 */
#define SEL_NL { 10 }

/* Simple wraparound timer function */
unsigned long getticks(void);	       /* based on gettimeofday(2) */
#define GETTICKCOUNT getticks
#define TICKSPERSEC    1000	       /* we choose to use milliseconds */
#define CURSORBLINK     450	       /* no standard way to set this */

#define WCHAR wchar_t
#define BYTE unsigned char

/*
 * Unix-specific global flag
 *
 * FLAG_STDERR_TTY indicates that standard error might be a terminal and
 * might get its configuration munged, so anything trying to output plain
 * text (i.e. with newlines in it) will need to put it back into cooked
 * mode first.  Applications setting this flag should also call
 * stderr_tty_init() before messing with any terminal modes, and can call
 * premsg() before outputting text to stderr and postmsg() afterwards.
 */
#define FLAG_STDERR_TTY 0x1000

/* Things pty.c needs from pterm.c */
char *get_x_display(void *frontend);
int font_dimension(void *frontend, int which);/* 0 for width, 1 for height */
long get_windowid(void *frontend);
int frontend_is_utf8(void *frontend);

/* Things gtkdlg.c needs from pterm.c */
void *get_window(void *frontend);      /* void * to avoid depending on gtk.h */

/* Things pterm.c needs from gtkdlg.c */
int do_config_box(const char *title, Conf *conf,
		  int midsession, int protcfginfo);
void fatal_message_box(void *window, char *msg);
void nonfatal_message_box(void *window, char *msg);
void about_box(void *window);
void *eventlogstuff_new(void);
void showeventlog(void *estuff, void *parentwin);
void logevent_dlg(void *estuff, const char *string);
int reallyclose(void *frontend);
#ifdef MAY_REFER_TO_GTK_IN_HEADERS
int messagebox(GtkWidget *parentwin, char *title,
               char *msg, int minwid, int selectable, ...);
int string_width(char *text);
#endif

/* Things pterm.c needs from {ptermm,uxputty}.c */
char *make_default_wintitle(char *hostname);
int process_nonoption_arg(char *arg, Conf *conf, int *allow_launch);

/* pterm.c needs this special function in xkeysym.c */
int keysym_to_unicode(int keysym);

/* Things uxstore.c needs from pterm.c */
char *x_get_default(const char *key);

/* Things uxstore.c provides to pterm.c */
void provide_xrm_string(char *string);

/* Things provided by uxcons.c */
struct termios;
void stderr_tty_init(void);
void premsg(struct termios *);
void postmsg(struct termios *);

/* The interface used by uxsel.c */
void uxsel_init(void);
typedef int (*uxsel_callback_fn)(int fd, int event);
void uxsel_set(int fd, int rwx, uxsel_callback_fn callback);
void uxsel_del(int fd);
int select_result(int fd, int event);
int first_fd(int *state, int *rwx);
int next_fd(int *state, int *rwx);
/* The following are expected to be provided _to_ uxsel.c by the frontend */
int uxsel_input_add(int fd, int rwx);  /* returns an id */
void uxsel_input_remove(int id);

/* uxcfg.c */
struct controlbox;
void unix_setup_config_box(struct controlbox *b, int midsession, int protocol);

/* gtkcfg.c */
void gtk_setup_config_box(struct controlbox *b, int midsession, void *window);

/*
 * In the Unix Unicode layer, DEFAULT_CODEPAGE is a special value
 * which causes mb_to_wc and wc_to_mb to call _libc_ rather than
 * libcharset. That way, we can interface the various charsets
 * supported by libcharset with the one supported by mbstowcs and
 * wcstombs (which will be the character set in which stuff read
 * from the command line or config files is assumed to be encoded).
 */
#define DEFAULT_CODEPAGE 0xFFFF
#define CP_UTF8 CS_UTF8		       /* from libcharset */

#define strnicmp strncasecmp
#define stricmp strcasecmp

/* BSD-semantics version of signal(), and another helpful function */
void (*putty_signal(int sig, void (*func)(int)))(int);
void block_signal(int sig, int block_it);

/* uxmisc.c */
void cloexec(int);
void noncloexec(int);
int nonblock(int);
int no_nonblock(int);

/*
 * Exports from unicode.c.
 */
struct unicode_data;
int init_ucs(struct unicode_data *ucsdata, char *line_codepage,
	     int utf8_override, int font_charset, int vtmode);

/*
 * Spare function exported directly from uxnet.c.
 */
void *sk_getxdmdata(void *sock, int *lenp);

/*
 * General helpful Unix stuff: more helpful version of the FD_SET
 * macro, which also handles maxfd.
 */
#define FD_SET_MAX(fd, max, set) do { \
    FD_SET(fd, &set); \
    if (max < fd + 1) max = fd + 1; \
} while (0)

/*
 * Exports from winser.c.
 */
extern Backend serial_backend;

/*
 * uxpeer.c, wrapping getsockopt(SO_PEERCRED).
 */
int so_peercred(int fd, int *pid, int *uid, int *gid);

#endif
