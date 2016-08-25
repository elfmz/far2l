/*
 * "Raw" backend.
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "putty.h"

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

#define RAW_MAX_BACKLOG 4096

typedef struct raw_backend_data {
    const struct plug_function_table *fn;
    /* the above field _must_ be first in the structure */

    Socket s;
    int closed_on_socket_error;
    int bufsize;
    void *frontend;
    int sent_console_eof, sent_socket_eof;
} *Raw;

static void raw_size(void *handle, int width, int height);

static void c_write(Raw raw, char *buf, int len)
{
    int backlog = from_backend(raw->frontend, 0, buf, len);
    sk_set_frozen(raw->s, backlog > RAW_MAX_BACKLOG);
}

static void raw_log(Plug plug, int type, SockAddr addr, int port,
		    const char *error_msg, int error_code)
{
    Raw raw = (Raw) plug;
    char addrbuf[256], *msg;

    sk_getaddr(addr, addrbuf, lenof(addrbuf));

    if (type == 0)
	msg = dupprintf("Connecting to %s port %d", addrbuf, port);
    else
	msg = dupprintf("Failed to connect to %s: %s", addrbuf, error_msg);

    logevent(raw->frontend, msg);
    sfree(msg);
}

static void raw_check_close(Raw raw)
{
    /*
     * Called after we send EOF on either the socket or the console.
     * Its job is to wind up the session once we have sent EOF on both.
     */
    if (raw->sent_console_eof && raw->sent_socket_eof) {
        if (raw->s) {
            sk_close(raw->s);
            raw->s = NULL;
            notify_remote_exit(raw->frontend);
        }
    }
}

static int raw_closing(Plug plug, const char *error_msg, int error_code,
		       int calling_back)
{
    Raw raw = (Raw) plug;

    if (error_msg) {
        /* A socket error has occurred. */
        if (raw->s) {
            sk_close(raw->s);
            raw->s = NULL;
            raw->closed_on_socket_error = TRUE;
            notify_remote_exit(raw->frontend);
        }
        logevent(raw->frontend, error_msg);
        connection_fatal(raw->frontend, "%s", error_msg);
    } else {
        /* Otherwise, the remote side closed the connection normally. */
        if (!raw->sent_console_eof && from_backend_eof(raw->frontend)) {
            /*
             * The front end wants us to close the outgoing side of the
             * connection as soon as we see EOF from the far end.
             */
            if (!raw->sent_socket_eof) {
                if (raw->s)
                    sk_write_eof(raw->s);
                raw->sent_socket_eof= TRUE;
            }
        }
        raw->sent_console_eof = TRUE;
        raw_check_close(raw);
    }
    return 0;
}

static int raw_receive(Plug plug, int urgent, char *data, int len)
{
    Raw raw = (Raw) plug;
    c_write(raw, data, len);
    return 1;
}

static void raw_sent(Plug plug, int bufsize)
{
    Raw raw = (Raw) plug;
    raw->bufsize = bufsize;
}

/*
 * Called to set up the raw connection.
 * 
 * Returns an error message, or NULL on success.
 *
 * Also places the canonical host name into `realhost'. It must be
 * freed by the caller.
 */
static const char *raw_init(void *frontend_handle, void **backend_handle,
			    Conf *conf,
			    char *host, int port, char **realhost, int nodelay,
			    int keepalive)
{
    static const struct plug_function_table fn_table = {
	raw_log,
	raw_closing,
	raw_receive,
	raw_sent
    };
    SockAddr addr;
    const char *err;
    Raw raw;
    int addressfamily;
    char *loghost;

    raw = snew(struct raw_backend_data);
    raw->fn = &fn_table;
    raw->s = NULL;
    raw->closed_on_socket_error = FALSE;
    *backend_handle = raw;
    raw->sent_console_eof = raw->sent_socket_eof = FALSE;
    raw->bufsize = 0;

    raw->frontend = frontend_handle;

    addressfamily = conf_get_int(conf, CONF_addressfamily);
    /*
     * Try to find host.
     */
    {
	char *buf;
	buf = dupprintf("Looking up host \"%s\"%s", host,
			(addressfamily == ADDRTYPE_IPV4 ? " (IPv4)" :
			 (addressfamily == ADDRTYPE_IPV6 ? " (IPv6)" :
			  "")));
	logevent(raw->frontend, buf);
	sfree(buf);
    }
    addr = name_lookup(host, port, realhost, conf, addressfamily);
    if ((err = sk_addr_error(addr)) != NULL) {
	sk_addr_free(addr);
	return err;
    }

    if (port < 0)
	port = 23;		       /* default telnet port */

    /*
     * Open socket.
     */
    raw->s = new_connection(addr, *realhost, port, 0, 1, nodelay, keepalive,
			    (Plug) raw, conf);
    if ((err = sk_socket_error(raw->s)) != NULL)
	return err;

    loghost = conf_get_str(conf, CONF_loghost);
    if (*loghost) {
	char *colon;

	sfree(*realhost);
	*realhost = dupstr(loghost);

	colon = host_strrchr(*realhost, ':');
	if (colon)
	    *colon++ = '\0';
    }

    return NULL;
}

static void raw_free(void *handle)
{
    Raw raw = (Raw) handle;

    if (raw->s)
	sk_close(raw->s);
    sfree(raw);
}

/*
 * Stub routine (we don't have any need to reconfigure this backend).
 */
static void raw_reconfig(void *handle, Conf *conf)
{
}

/*
 * Called to send data down the raw connection.
 */
static int raw_send(void *handle, char *buf, int len)
{
    Raw raw = (Raw) handle;

    if (raw->s == NULL)
	return 0;

    raw->bufsize = sk_write(raw->s, buf, len);

    return raw->bufsize;
}

/*
 * Called to query the current socket sendability status.
 */
static int raw_sendbuffer(void *handle)
{
    Raw raw = (Raw) handle;
    return raw->bufsize;
}

/*
 * Called to set the size of the window
 */
static void raw_size(void *handle, int width, int height)
{
    /* Do nothing! */
    return;
}

/*
 * Send raw special codes. We only handle outgoing EOF here.
 */
static void raw_special(void *handle, Telnet_Special code)
{
    Raw raw = (Raw) handle;
    if (code == TS_EOF && raw->s) {
        sk_write_eof(raw->s);
        raw->sent_socket_eof= TRUE;
        raw_check_close(raw);
    }

    return;
}

/*
 * Return a list of the special codes that make sense in this
 * protocol.
 */
static const struct telnet_special *raw_get_specials(void *handle)
{
    return NULL;
}

static int raw_connected(void *handle)
{
    Raw raw = (Raw) handle;
    return raw->s != NULL;
}

static int raw_sendok(void *handle)
{
    return 1;
}

static void raw_unthrottle(void *handle, int backlog)
{
    Raw raw = (Raw) handle;
    sk_set_frozen(raw->s, backlog > RAW_MAX_BACKLOG);
}

static int raw_ldisc(void *handle, int option)
{
    if (option == LD_EDIT || option == LD_ECHO)
	return 1;
    return 0;
}

static void raw_provide_ldisc(void *handle, void *ldisc)
{
    /* This is a stub. */
}

static void raw_provide_logctx(void *handle, void *logctx)
{
    /* This is a stub. */
}

static int raw_exitcode(void *handle)
{
    Raw raw = (Raw) handle;
    if (raw->s != NULL)
        return -1;                     /* still connected */
    else if (raw->closed_on_socket_error)
        return INT_MAX;     /* a socket error counts as an unclean exit */
    else
        /* Exit codes are a meaningless concept in the Raw protocol */
        return 0;
}

/*
 * cfg_info for Raw does nothing at all.
 */
static int raw_cfg_info(void *handle)
{
    return 0;
}

Backend raw_backend = {
    raw_init,
    raw_free,
    raw_reconfig,
    raw_send,
    raw_sendbuffer,
    raw_size,
    raw_special,
    raw_get_specials,
    raw_connected,
    raw_exitcode,
    raw_sendok,
    raw_ldisc,
    raw_provide_ldisc,
    raw_provide_logctx,
    raw_unthrottle,
    raw_cfg_info,
    "raw",
    PROT_RAW,
    0
};
