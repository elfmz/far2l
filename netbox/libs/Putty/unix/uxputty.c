/*
 * Unix PuTTY main program.
 */

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <gdk/gdk.h>

#include "putty.h"
#include "storage.h"

/*
 * Stubs to avoid uxpty.c needing to be linked in.
 */
const int use_pty_argv = FALSE;
char **pty_argv;		       /* never used */

/*
 * Clean up and exit.
 */
void cleanup_exit(int code)
{
    /*
     * Clean up.
     */
    sk_cleanup();
    random_save_seed();
    exit(code);
}

Backend *select_backend(Conf *conf)
{
    Backend *back = backend_from_proto(conf_get_int(conf, CONF_protocol));
    assert(back != NULL);
    return back;
}

int cfgbox(Conf *conf)
{
    char *title = dupcat(appname, " Configuration", NULL);
    int ret = do_config_box(title, conf, 0, 0);
    sfree(title);
    return ret;
}

static int got_host = 0;

const int use_event_log = 1, new_session = 1, saved_sessions = 1;

int process_nonoption_arg(char *arg, Conf *conf, int *allow_launch)
{
    char *p, *q = arg;

    if (got_host) {
        /*
         * If we already have a host name, treat this argument as a
         * port number. NB we have to treat this as a saved -P
         * argument, so that it will be deferred until it's a good
         * moment to run it.
         */
        int ret = cmdline_process_param("-P", arg, 1, conf);
        assert(ret == 2);
    } else if (!strncmp(q, "telnet:", 7)) {
        /*
         * If the hostname starts with "telnet:",
         * set the protocol to Telnet and process
         * the string as a Telnet URL.
         */
        char c;

        q += 7;
        if (q[0] == '/' && q[1] == '/')
            q += 2;
        conf_set_int(conf, CONF_protocol, PROT_TELNET);
        p = q;
        p += host_strcspn(p, ":/");
        c = *p;
        if (*p)
            *p++ = '\0';
        if (c == ':')
            conf_set_int(conf, CONF_port, atoi(p));
        else
            conf_set_int(conf, CONF_port, -1);
	conf_set_str(conf, CONF_host, q);
        got_host = 1;
    } else {
        /*
         * Otherwise, treat this argument as a host name.
         */
        p = arg;
        while (*p && !isspace((unsigned char)*p))
            p++;
        if (*p)
            *p++ = '\0';
        conf_set_str(conf, CONF_host, q);
        got_host = 1;
    }
    if (got_host)
	*allow_launch = TRUE;
    return 1;
}

char *make_default_wintitle(char *hostname)
{
    return dupcat(hostname, " - ", appname, NULL);
}

/*
 * X11-forwarding-related things suitable for Gtk app.
 */

char *platform_get_x_display(void) {
    const char *display;
    /* Try to take account of --display and what have you. */
    if (!(display = gdk_get_display()))
	/* fall back to traditional method */
	display = getenv("DISPLAY");
    return dupstr(display);
}

const int share_can_be_downstream = TRUE;
const int share_can_be_upstream = TRUE;

int main(int argc, char **argv)
{
    extern int pt_main(int argc, char **argv);
    int ret;

    sk_init();
    flags = FLAG_VERBOSE | FLAG_INTERACTIVE;
    default_protocol = be_default_protocol;
    /* Find the appropriate default port. */
    {
	Backend *b = backend_from_proto(default_protocol);
	default_port = 0; /* illegal */
	if (b)
	    default_port = b->default_port;
    }
    ret = pt_main(argc, argv);
    cleanup_exit(ret);
    return ret;             /* not reached, but placates optimisers */
}
