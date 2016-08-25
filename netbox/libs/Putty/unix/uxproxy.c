/*
 * uxproxy.c: Unix implementation of platform_new_connection(),
 * supporting an OpenSSH-like proxy command.
 */

#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#define DEFINE_PLUG_METHOD_MACROS
#include "tree234.h"
#include "putty.h"
#include "network.h"
#include "proxy.h"

typedef struct Socket_localproxy_tag * Local_Proxy_Socket;

struct Socket_localproxy_tag {
    const struct socket_function_table *fn;
    /* the above variable absolutely *must* be the first in this structure */

    int to_cmd, from_cmd;	       /* fds */

    char *error;

    Plug plug;

    bufchain pending_output_data;
    bufchain pending_input_data;
    enum { EOF_NO, EOF_PENDING, EOF_SENT } outgoingeof;
};

static int localproxy_select_result(int fd, int event);

/*
 * Trees to look up the pipe fds in.
 */
static tree234 *localproxy_by_fromfd, *localproxy_by_tofd;
static int localproxy_fromfd_cmp(void *av, void *bv)
{
    Local_Proxy_Socket a = (Local_Proxy_Socket)av;
    Local_Proxy_Socket b = (Local_Proxy_Socket)bv;
    if (a->from_cmd < b->from_cmd)
	return -1;
    if (a->from_cmd > b->from_cmd)
	return +1;
    return 0;
}
static int localproxy_fromfd_find(void *av, void *bv)
{
    int a = *(int *)av;
    Local_Proxy_Socket b = (Local_Proxy_Socket)bv;
    if (a < b->from_cmd)
	return -1;
    if (a > b->from_cmd)
	return +1;
    return 0;
}
static int localproxy_tofd_cmp(void *av, void *bv)
{
    Local_Proxy_Socket a = (Local_Proxy_Socket)av;
    Local_Proxy_Socket b = (Local_Proxy_Socket)bv;
    if (a->to_cmd < b->to_cmd)
	return -1;
    if (a->to_cmd > b->to_cmd)
	return +1;
    return 0;
}
static int localproxy_tofd_find(void *av, void *bv)
{
    int a = *(int *)av;
    Local_Proxy_Socket b = (Local_Proxy_Socket)bv;
    if (a < b->to_cmd)
	return -1;
    if (a > b->to_cmd)
	return +1;
    return 0;
}

/* basic proxy socket functions */

static Plug sk_localproxy_plug (Socket s, Plug p)
{
    Local_Proxy_Socket ps = (Local_Proxy_Socket) s;
    Plug ret = ps->plug;
    if (p)
	ps->plug = p;
    return ret;
}

static void sk_localproxy_close (Socket s)
{
    Local_Proxy_Socket ps = (Local_Proxy_Socket) s;

    if (ps->to_cmd >= 0) {
        del234(localproxy_by_tofd, ps);
        uxsel_del(ps->to_cmd);
        close(ps->to_cmd);
    }

    del234(localproxy_by_fromfd, ps);
    uxsel_del(ps->from_cmd);
    close(ps->from_cmd);

    bufchain_clear(&ps->pending_input_data);
    bufchain_clear(&ps->pending_output_data);
    sfree(ps);
}

static int localproxy_try_send(Local_Proxy_Socket ps)
{
    int sent = 0;

    while (bufchain_size(&ps->pending_output_data) > 0) {
	void *data;
	int len, ret;

	bufchain_prefix(&ps->pending_output_data, &data, &len);
	ret = write(ps->to_cmd, data, len);
	if (ret < 0 && errno != EWOULDBLOCK) {
	    /* We're inside the Unix frontend here, so we know
	     * that the frontend handle is unnecessary. */
	    logevent(NULL, strerror(errno));
	    fatalbox("%s", strerror(errno));
	} else if (ret <= 0) {
	    break;
	} else {
	    bufchain_consume(&ps->pending_output_data, ret);
	    sent += ret;
	}
    }

    if (ps->outgoingeof == EOF_PENDING) {
        del234(localproxy_by_tofd, ps);
        close(ps->to_cmd);
        uxsel_del(ps->to_cmd);
        ps->to_cmd = -1;
        ps->outgoingeof = EOF_SENT;
    }

    if (bufchain_size(&ps->pending_output_data) == 0)
	uxsel_del(ps->to_cmd);
    else
	uxsel_set(ps->to_cmd, 2, localproxy_select_result);

    return sent;
}

static int sk_localproxy_write (Socket s, const char *data, int len)
{
    Local_Proxy_Socket ps = (Local_Proxy_Socket) s;

    assert(ps->outgoingeof == EOF_NO);

    bufchain_add(&ps->pending_output_data, data, len);

    localproxy_try_send(ps);

    return bufchain_size(&ps->pending_output_data);
}

static int sk_localproxy_write_oob (Socket s, const char *data, int len)
{
    /*
     * oob data is treated as inband; nasty, but nothing really
     * better we can do
     */
    return sk_localproxy_write(s, data, len);
}

static void sk_localproxy_write_eof (Socket s)
{
    Local_Proxy_Socket ps = (Local_Proxy_Socket) s;

    assert(ps->outgoingeof == EOF_NO);
    ps->outgoingeof = EOF_PENDING;

    localproxy_try_send(ps);
}

static void sk_localproxy_flush (Socket s)
{
    /* Local_Proxy_Socket ps = (Local_Proxy_Socket) s; */
    /* do nothing */
}

static void sk_localproxy_set_frozen (Socket s, int is_frozen)
{
    Local_Proxy_Socket ps = (Local_Proxy_Socket) s;

    if (is_frozen)
	uxsel_del(ps->from_cmd);
    else
	uxsel_set(ps->from_cmd, 1, localproxy_select_result);
}

static const char * sk_localproxy_socket_error (Socket s)
{
    Local_Proxy_Socket ps = (Local_Proxy_Socket) s;
    return ps->error;
}

static int localproxy_select_result(int fd, int event)
{
    Local_Proxy_Socket s;
    char buf[20480];
    int ret;

    if (!(s = find234(localproxy_by_fromfd, &fd, localproxy_fromfd_find)) &&
	!(s = find234(localproxy_by_tofd, &fd, localproxy_tofd_find)) )
	return 1;		       /* boggle */

    if (event == 1) {
	assert(fd == s->from_cmd);
	ret = read(fd, buf, sizeof(buf));
	if (ret < 0) {
	    return plug_closing(s->plug, strerror(errno), errno, 0);
	} else if (ret == 0) {
	    return plug_closing(s->plug, NULL, 0, 0);
	} else {
	    return plug_receive(s->plug, 0, buf, ret);
	}
    } else if (event == 2) {
	assert(fd == s->to_cmd);
	if (localproxy_try_send(s))
	    plug_sent(s->plug, bufchain_size(&s->pending_output_data));
	return 1;
    }

    return 1;
}

Socket platform_new_connection(SockAddr addr, char *hostname,
			       int port, int privport,
			       int oobinline, int nodelay, int keepalive,
			       Plug plug, Conf *conf)
{
    char *cmd;

    static const struct socket_function_table socket_fn_table = {
	sk_localproxy_plug,
	sk_localproxy_close,
	sk_localproxy_write,
	sk_localproxy_write_oob,
	sk_localproxy_write_eof,
	sk_localproxy_flush,
	sk_localproxy_set_frozen,
	sk_localproxy_socket_error,
        NULL, /* peer_info */
    };

    Local_Proxy_Socket ret;
    int to_cmd_pipe[2], from_cmd_pipe[2], pid;

    if (conf_get_int(conf, CONF_proxy_type) != PROXY_CMD)
	return NULL;

    cmd = format_telnet_command(addr, port, conf);

    ret = snew(struct Socket_localproxy_tag);
    ret->fn = &socket_fn_table;
    ret->plug = plug;
    ret->error = NULL;
    ret->outgoingeof = EOF_NO;

    bufchain_init(&ret->pending_input_data);
    bufchain_init(&ret->pending_output_data);

    /*
     * Create the pipes to the proxy command, and spawn the proxy
     * command process.
     */
    if (pipe(to_cmd_pipe) < 0 ||
	pipe(from_cmd_pipe) < 0) {
	ret->error = dupprintf("pipe: %s", strerror(errno));
        sfree(cmd);
	return (Socket)ret;
    }
    cloexec(to_cmd_pipe[1]);
    cloexec(from_cmd_pipe[0]);

    pid = fork();

    if (pid < 0) {
	ret->error = dupprintf("fork: %s", strerror(errno));
        sfree(cmd);
	return (Socket)ret;
    } else if (pid == 0) {
	close(0);
	close(1);
	dup2(to_cmd_pipe[0], 0);
	dup2(from_cmd_pipe[1], 1);
	close(to_cmd_pipe[0]);
	close(from_cmd_pipe[1]);
	noncloexec(0);
	noncloexec(1);
	execl("/bin/sh", "sh", "-c", cmd, (void *)NULL);
	_exit(255);
    }

    sfree(cmd);

    close(to_cmd_pipe[0]);
    close(from_cmd_pipe[1]);

    ret->to_cmd = to_cmd_pipe[1];
    ret->from_cmd = from_cmd_pipe[0];

    if (!localproxy_by_fromfd)
	localproxy_by_fromfd = newtree234(localproxy_fromfd_cmp);
    if (!localproxy_by_tofd)
	localproxy_by_tofd = newtree234(localproxy_tofd_cmp);

    add234(localproxy_by_fromfd, ret);
    add234(localproxy_by_tofd, ret);

    uxsel_set(ret->from_cmd, 1, localproxy_select_result);

    /* We are responsible for this and don't need it any more */
    sk_addr_free(addr);

    return (Socket) ret;
}
