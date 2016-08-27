/*
 * Unix networking abstraction.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <sys/un.h>
#include <pwd.h>
#include <grp.h>

#define DEFINE_PLUG_METHOD_MACROS
#include "putty.h"
#include "network.h"
#include "tree234.h"

/* Solaris needs <sys/sockio.h> for SIOCATMARK. */
#ifndef SIOCATMARK
#include <sys/sockio.h>
#endif

#ifndef X11_UNIX_PATH
# define X11_UNIX_PATH "/tmp/.X11-unix/X"
#endif

/* 
 * Access to sockaddr types without breaking C strict aliasing rules.
 */
union sockaddr_union {
    struct sockaddr_storage storage;
    struct sockaddr sa;
    struct sockaddr_in sin;
#ifndef NO_IPV6
    struct sockaddr_in6 sin6;
#endif
    struct sockaddr_un su;
};

/*
 * We used to typedef struct Socket_tag *Socket.
 *
 * Since we have made the networking abstraction slightly more
 * abstract, Socket no longer means a tcp socket (it could mean
 * an ssl socket).  So now we must use Actual_Socket when we know
 * we are talking about a tcp socket.
 */
typedef struct Socket_tag *Actual_Socket;

/*
 * Mutable state that goes with a SockAddr: stores information
 * about where in the list of candidate IP(v*) addresses we've
 * currently got to.
 */
typedef struct SockAddrStep_tag SockAddrStep;
struct SockAddrStep_tag {
#ifndef NO_IPV6
    struct addrinfo *ai;	       /* steps along addr->ais */
#endif
    int curraddr;
};

struct Socket_tag {
    struct socket_function_table *fn;
    /* the above variable absolutely *must* be the first in this structure */
    const char *error;
    int s;
    Plug plug;
    bufchain output_data;
    int connected;		       /* irrelevant for listening sockets */
    int writable;
    int frozen; /* this causes readability notifications to be ignored */
    int localhost_only;		       /* for listening sockets */
    char oobdata[1];
    int sending_oob;
    int oobpending;		       /* is there OOB data available to read? */
    int oobinline;
    enum { EOF_NO, EOF_PENDING, EOF_SENT } outgoingeof;
    int incomingeof;
    int pending_error;		       /* in case send() returns error */
    int listener;
    int nodelay, keepalive;            /* for connect()-type sockets */
    int privport, port;                /* and again */
    SockAddr addr;
    SockAddrStep step;
    /*
     * We sometimes need pairs of Socket structures to be linked:
     * if we are listening on the same IPv6 and v4 port, for
     * example. So here we define `parent' and `child' pointers to
     * track this link.
     */
    Actual_Socket parent, child;
};

struct SockAddr_tag {
    int refcount;
    const char *error;
    enum { UNRESOLVED, UNIX, IP } superfamily;
#ifndef NO_IPV6
    struct addrinfo *ais;	       /* Addresses IPv6 style. */
#else
    unsigned long *addresses;	       /* Addresses IPv4 style. */
    int naddresses;
#endif
    char hostname[512];		       /* Store an unresolved host name. */
};

/*
 * Which address family this address belongs to. AF_INET for IPv4;
 * AF_INET6 for IPv6; AF_UNSPEC indicates that name resolution has
 * not been done and a simple host name is held in this SockAddr
 * structure.
 */
#ifndef NO_IPV6
#define SOCKADDR_FAMILY(addr, step) \
    ((addr)->superfamily == UNRESOLVED ? AF_UNSPEC : \
     (addr)->superfamily == UNIX ? AF_UNIX : \
     (step).ai ? (step).ai->ai_family : AF_INET)
#else
/* Here we gratuitously reference 'step' to avoid gcc warnings about
 * 'set but not used' when compiling -DNO_IPV6 */
#define SOCKADDR_FAMILY(addr, step) \
    ((addr)->superfamily == UNRESOLVED ? AF_UNSPEC : \
     (addr)->superfamily == UNIX ? AF_UNIX : \
     (step).curraddr ? AF_INET : AF_INET)
#endif

/*
 * Start a SockAddrStep structure to step through multiple
 * addresses.
 */
#ifndef NO_IPV6
#define START_STEP(addr, step) \
    ((step).ai = (addr)->ais, (step).curraddr = 0)
#else
#define START_STEP(addr, step) \
    ((step).curraddr = 0)
#endif

static tree234 *sktree;

static void uxsel_tell(Actual_Socket s);

static int cmpfortree(void *av, void *bv)
{
    Actual_Socket a = (Actual_Socket) av, b = (Actual_Socket) bv;
    int as = a->s, bs = b->s;
    if (as < bs)
	return -1;
    if (as > bs)
	return +1;
    if (a < b)
       return -1;
    if (a > b)
       return +1;
    return 0;
}

static int cmpforsearch(void *av, void *bv)
{
    Actual_Socket b = (Actual_Socket) bv;
    int as = *(int *)av, bs = b->s;
    if (as < bs)
	return -1;
    if (as > bs)
	return +1;
    return 0;
}

void sk_init(void)
{
    sktree = newtree234(cmpfortree);
    uxsel_init();
}

void sk_cleanup(void)
{
    Actual_Socket s;
    int i;

    if (sktree) {
	for (i = 0; (s = index234(sktree, i)) != NULL; i++) {
	    close(s->s);
	}
    }
}

SockAddr sk_namelookup(const char *host, char **canonicalname, int address_family)
{
    SockAddr ret = snew(struct SockAddr_tag);
#ifndef NO_IPV6
    struct addrinfo hints;
    int err;
#else
    unsigned long a;
    struct hostent *h = NULL;
    int n;
#endif
    char realhost[8192];

    /* Clear the structure and default to IPv4. */
    memset(ret, 0, sizeof(struct SockAddr_tag));
    ret->superfamily = UNRESOLVED;
    *realhost = '\0';
    ret->error = NULL;
    ret->refcount = 1;

#ifndef NO_IPV6
    hints.ai_flags = AI_CANONNAME;
    hints.ai_family = (address_family == ADDRTYPE_IPV4 ? AF_INET :
		       address_family == ADDRTYPE_IPV6 ? AF_INET6 :
		       AF_UNSPEC);
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;
    hints.ai_addrlen = 0;
    hints.ai_addr = NULL;
    hints.ai_canonname = NULL;
    hints.ai_next = NULL;
    {
        char *trimmed_host = host_strduptrim(host); /* strip [] on literals */
        err = getaddrinfo(trimmed_host, NULL, &hints, &ret->ais);
        sfree(trimmed_host);
    }
    if (err != 0) {
	ret->error = gai_strerror(err);
	return ret;
    }
    ret->superfamily = IP;
    *realhost = '\0';
    if (ret->ais->ai_canonname != NULL)
	strncat(realhost, ret->ais->ai_canonname, sizeof(realhost) - 1);
    else
	strncat(realhost, host, sizeof(realhost) - 1);
#else
    if ((a = inet_addr(host)) == (unsigned long)(in_addr_t)(-1)) {
	/*
	 * Otherwise use the IPv4-only gethostbyname... (NOTE:
	 * we don't use gethostbyname as a fallback!)
	 */
	if (ret->superfamily == UNRESOLVED) {
	    /*debug(("Resolving \"%s\" with gethostbyname() (IPv4 only)...\n", host)); */
	    if ( (h = gethostbyname(host)) )
		ret->superfamily = IP;
	}
	if (ret->superfamily == UNRESOLVED) {
	    ret->error = (h_errno == HOST_NOT_FOUND ||
			  h_errno == NO_DATA ||
			  h_errno == NO_ADDRESS ? "Host does not exist" :
			  h_errno == TRY_AGAIN ?
			  "Temporary name service failure" :
			  "gethostbyname: unknown error");
	    return ret;
	}
	/* This way we are always sure the h->h_name is valid :) */
	strncpy(realhost, h->h_name, sizeof(realhost));
	for (n = 0; h->h_addr_list[n]; n++);
	ret->addresses = snewn(n, unsigned long);
	ret->naddresses = n;
	for (n = 0; n < ret->naddresses; n++) {
	    memcpy(&a, h->h_addr_list[n], sizeof(a));
	    ret->addresses[n] = ntohl(a);
	}
    } else {
	/*
	 * This must be a numeric IPv4 address because it caused a
	 * success return from inet_addr.
	 */
	ret->superfamily = IP;
	strncpy(realhost, host, sizeof(realhost));
	ret->addresses = snew(unsigned long);
	ret->naddresses = 1;
	ret->addresses[0] = ntohl(a);
    }
#endif
    realhost[lenof(realhost)-1] = '\0';
    *canonicalname = snewn(1+strlen(realhost), char);
    strcpy(*canonicalname, realhost);
    return ret;
}

SockAddr sk_nonamelookup(const char *host)
{
    SockAddr ret = snew(struct SockAddr_tag);
    ret->error = NULL;
    ret->superfamily = UNRESOLVED;
    strncpy(ret->hostname, host, lenof(ret->hostname));
    ret->hostname[lenof(ret->hostname)-1] = '\0';
#ifndef NO_IPV6
    ret->ais = NULL;
#else
    ret->addresses = NULL;
#endif
    ret->refcount = 1;
    return ret;
}

static int sk_nextaddr(SockAddr addr, SockAddrStep *step)
{
#ifndef NO_IPV6
    if (step->ai && step->ai->ai_next) {
	step->ai = step->ai->ai_next;
	return TRUE;
    } else
	return FALSE;
#else
    if (step->curraddr+1 < addr->naddresses) {
	step->curraddr++;
	return TRUE;
    } else {
	return FALSE;
    }
#endif    
}

void sk_getaddr(SockAddr addr, char *buf, int buflen)
{
    if (addr->superfamily == UNRESOLVED || addr->superfamily == UNIX) {
	strncpy(buf, addr->hostname, buflen);
	buf[buflen-1] = '\0';
    } else {
#ifndef NO_IPV6
	if (getnameinfo(addr->ais->ai_addr, addr->ais->ai_addrlen, buf, buflen,
			NULL, 0, NI_NUMERICHOST) != 0) {
	    buf[0] = '\0';
	    strncat(buf, "<unknown>", buflen - 1);
	}
#else
	struct in_addr a;
	SockAddrStep step;
	START_STEP(addr, step);
	assert(SOCKADDR_FAMILY(addr, step) == AF_INET);
	a.s_addr = htonl(addr->addresses[0]);
	strncpy(buf, inet_ntoa(a), buflen);
	buf[buflen-1] = '\0';
#endif
    }
}

int sk_addr_needs_port(SockAddr addr)
{
    if (addr->superfamily == UNRESOLVED || addr->superfamily == UNIX) {
        return FALSE;
    } else {
        return TRUE;
    }
}

int sk_hostname_is_local(const char *name)
{
    return !strcmp(name, "localhost") ||
	   !strcmp(name, "::1") ||
	   !strncmp(name, "127.", 4);
}

#define ipv4_is_loopback(addr) \
    (((addr).s_addr & htonl(0xff000000)) == htonl(0x7f000000))

static int sockaddr_is_loopback(struct sockaddr *sa)
{
    union sockaddr_union *u = (union sockaddr_union *)sa;
    switch (u->sa.sa_family) {
      case AF_INET:
	return ipv4_is_loopback(u->sin.sin_addr);
#ifndef NO_IPV6
      case AF_INET6:
	return IN6_IS_ADDR_LOOPBACK(&u->sin6.sin6_addr);
#endif
      case AF_UNIX:
	return TRUE;
      default:
	return FALSE;
    }
}

int sk_address_is_local(SockAddr addr)
{
    if (addr->superfamily == UNRESOLVED)
	return 0;                      /* we don't know; assume not */
    else if (addr->superfamily == UNIX)
	return 1;
    else {
#ifndef NO_IPV6
	return sockaddr_is_loopback(addr->ais->ai_addr);
#else
	struct in_addr a;
	SockAddrStep step;
	START_STEP(addr, step);
	assert(SOCKADDR_FAMILY(addr, step) == AF_INET);
	a.s_addr = htonl(addr->addresses[0]);
	return ipv4_is_loopback(a);
#endif
    }
}

int sk_address_is_special_local(SockAddr addr)
{
    return addr->superfamily == UNIX;
}

int sk_addrtype(SockAddr addr)
{
    SockAddrStep step;
    int family;
    START_STEP(addr, step);
    family = SOCKADDR_FAMILY(addr, step);

    return (family == AF_INET ? ADDRTYPE_IPV4 :
#ifndef NO_IPV6
	    family == AF_INET6 ? ADDRTYPE_IPV6 :
#endif
	    ADDRTYPE_NAME);
}

void sk_addrcopy(SockAddr addr, char *buf)
{
    SockAddrStep step;
    int family;
    START_STEP(addr, step);
    family = SOCKADDR_FAMILY(addr, step);

#ifndef NO_IPV6
    if (family == AF_INET)
	memcpy(buf, &((struct sockaddr_in *)step.ai->ai_addr)->sin_addr,
	       sizeof(struct in_addr));
    else if (family == AF_INET6)
	memcpy(buf, &((struct sockaddr_in6 *)step.ai->ai_addr)->sin6_addr,
	       sizeof(struct in6_addr));
    else
	assert(FALSE);
#else
    struct in_addr a;

    assert(family == AF_INET);
    a.s_addr = htonl(addr->addresses[step.curraddr]);
    memcpy(buf, (char*) &a.s_addr, 4);
#endif
}

void sk_addr_free(SockAddr addr)
{
    if (--addr->refcount > 0)
	return;
#ifndef NO_IPV6
    if (addr->ais != NULL)
	freeaddrinfo(addr->ais);
#else
    sfree(addr->addresses);
#endif
    sfree(addr);
}

SockAddr sk_addr_dup(SockAddr addr)
{
    addr->refcount++;
    return addr;
}

static Plug sk_tcp_plug(Socket sock, Plug p)
{
    Actual_Socket s = (Actual_Socket) sock;
    Plug ret = s->plug;
    if (p)
	s->plug = p;
    return ret;
}

static void sk_tcp_flush(Socket s)
{
    /*
     * We send data to the socket as soon as we can anyway,
     * so we don't need to do anything here.  :-)
     */
}

static void sk_tcp_close(Socket s);
static int sk_tcp_write(Socket s, const char *data, int len);
static int sk_tcp_write_oob(Socket s, const char *data, int len);
static void sk_tcp_write_eof(Socket s);
static void sk_tcp_set_frozen(Socket s, int is_frozen);
static char *sk_tcp_peer_info(Socket s);
static const char *sk_tcp_socket_error(Socket s);

static struct socket_function_table tcp_fn_table = {
    sk_tcp_plug,
    sk_tcp_close,
    sk_tcp_write,
    sk_tcp_write_oob,
    sk_tcp_write_eof,
    sk_tcp_flush,
    sk_tcp_set_frozen,
    sk_tcp_socket_error,
    sk_tcp_peer_info,
};

static Socket sk_tcp_accept(accept_ctx_t ctx, Plug plug)
{
    int sockfd = ctx.i;
    Actual_Socket ret;

    /*
     * Create Socket structure.
     */
    ret = snew(struct Socket_tag);
    ret->fn = &tcp_fn_table;
    ret->error = NULL;
    ret->plug = plug;
    bufchain_init(&ret->output_data);
    ret->writable = 1;		       /* to start with */
    ret->sending_oob = 0;
    ret->frozen = 1;
    ret->localhost_only = 0;	       /* unused, but best init anyway */
    ret->pending_error = 0;
    ret->oobpending = FALSE;
    ret->outgoingeof = EOF_NO;
    ret->incomingeof = FALSE;
    ret->listener = 0;
    ret->parent = ret->child = NULL;
    ret->addr = NULL;
    ret->connected = 1;

    ret->s = sockfd;

    if (ret->s < 0) {
	ret->error = strerror(errno);
	return (Socket) ret;
    }

    ret->oobinline = 0;

    uxsel_tell(ret);
    add234(sktree, ret);

    return (Socket) ret;
}

static int try_connect(Actual_Socket sock)
{
    int s;
    union sockaddr_union u;
    const union sockaddr_union *sa;
    int err = 0;
    short localport;
    int salen, family;

    /*
     * Remove the socket from the tree before we overwrite its
     * internal socket id, because that forms part of the tree's
     * sorting criterion. We'll add it back before exiting this
     * function, whether we changed anything or not.
     */
    del234(sktree, sock);

    if (sock->s >= 0)
        close(sock->s);

    plug_log(sock->plug, 0, sock->addr, sock->port, NULL, 0);

    /*
     * Open socket.
     */
    family = SOCKADDR_FAMILY(sock->addr, sock->step);
    assert(family != AF_UNSPEC);
    s = socket(family, SOCK_STREAM, 0);
    sock->s = s;

    if (s < 0) {
	err = errno;
	goto ret;
    }

    cloexec(s);

    if (sock->oobinline) {
	int b = TRUE;
	if (setsockopt(s, SOL_SOCKET, SO_OOBINLINE,
                       (void *) &b, sizeof(b)) < 0) {
            err = errno;
            close(s);
            goto ret;
        }
    }

    if (sock->nodelay) {
	int b = TRUE;
	if (setsockopt(s, IPPROTO_TCP, TCP_NODELAY,
                       (void *) &b, sizeof(b)) < 0) {
            err = errno;
            close(s);
            goto ret;
        }
    }

    if (sock->keepalive) {
	int b = TRUE;
	if (setsockopt(s, SOL_SOCKET, SO_KEEPALIVE,
                       (void *) &b, sizeof(b)) < 0) {
            err = errno;
            close(s);
            goto ret;
        }
    }

    /*
     * Bind to local address.
     */
    if (sock->privport)
	localport = 1023;	       /* count from 1023 downwards */
    else
	localport = 0;		       /* just use port 0 (ie kernel picks) */

    /* BSD IP stacks need sockaddr_in zeroed before filling in */
    memset(&u,'\0',sizeof(u));

    /* We don't try to bind to a local address for UNIX domain sockets.  (Why
     * do we bother doing the bind when localport == 0 anyway?) */
    if (family != AF_UNIX) {
	/* Loop round trying to bind */
	while (1) {
	    int retcode;

#ifndef NO_IPV6
	    if (family == AF_INET6) {
		/* XXX use getaddrinfo to get a local address? */
		u.sin6.sin6_family = AF_INET6;
		u.sin6.sin6_addr = in6addr_any;
		u.sin6.sin6_port = htons(localport);
		retcode = bind(s, &u.sa, sizeof(u.sin6));
	    } else
#endif
	    {
		assert(family == AF_INET);
		u.sin.sin_family = AF_INET;
		u.sin.sin_addr.s_addr = htonl(INADDR_ANY);
		u.sin.sin_port = htons(localport);
		retcode = bind(s, &u.sa, sizeof(u.sin));
	    }
	    if (retcode >= 0) {
		err = 0;
		break;		       /* done */
	    } else {
		err = errno;
		if (err != EADDRINUSE) /* failed, for a bad reason */
		  break;
	    }
	    
	    if (localport == 0)
	      break;		       /* we're only looping once */
	    localport--;
	    if (localport == 0)
	      break;		       /* we might have got to the end */
	}
	
	if (err)
	    goto ret;
    }

    /*
     * Connect to remote address.
     */
    switch(family) {
#ifndef NO_IPV6
      case AF_INET:
	/* XXX would be better to have got getaddrinfo() to fill in the port. */
	((struct sockaddr_in *)sock->step.ai->ai_addr)->sin_port =
	    htons(sock->port);
	sa = (const union sockaddr_union *)sock->step.ai->ai_addr;
	salen = sock->step.ai->ai_addrlen;
	break;
      case AF_INET6:
	((struct sockaddr_in *)sock->step.ai->ai_addr)->sin_port =
	    htons(sock->port);
	sa = (const union sockaddr_union *)sock->step.ai->ai_addr;
	salen = sock->step.ai->ai_addrlen;
	break;
#else
      case AF_INET:
	u.sin.sin_family = AF_INET;
	u.sin.sin_addr.s_addr = htonl(sock->addr->addresses[sock->step.curraddr]);
	u.sin.sin_port = htons((short) sock->port);
	sa = &u;
	salen = sizeof u.sin;
	break;
#endif
      case AF_UNIX:
	assert(sock->port == 0);       /* to catch confused people */
	assert(strlen(sock->addr->hostname) < sizeof u.su.sun_path);
	u.su.sun_family = AF_UNIX;
	strcpy(u.su.sun_path, sock->addr->hostname);
	sa = &u;
	salen = sizeof u.su;
	break;

      default:
	assert(0 && "unknown address family");
	exit(1); /* XXX: GCC doesn't understand assert() on some systems. */
    }

    nonblock(s);

    if ((connect(s, &(sa->sa), salen)) < 0) {
	if ( errno != EINPROGRESS ) {
	    err = errno;
	    goto ret;
	}
    } else {
	/*
	 * If we _don't_ get EWOULDBLOCK, the connect has completed
	 * and we should set the socket as connected and writable.
	 */
	sock->connected = 1;
	sock->writable = 1;
    }

    uxsel_tell(sock);

    ret:

    /*
     * No matter what happened, put the socket back in the tree.
     */
    add234(sktree, sock);

    if (err)
	plug_log(sock->plug, 1, sock->addr, sock->port, strerror(err), err);
    return err;
}

Socket putty_sk_new(SockAddr addr, int port, int privport, int oobinline,
	      int nodelay, int keepalive, Plug plug)
{
    Actual_Socket ret;
    int err;

    /*
     * Create Socket structure.
     */
    ret = snew(struct Socket_tag);
    ret->fn = &tcp_fn_table;
    ret->error = NULL;
    ret->plug = plug;
    bufchain_init(&ret->output_data);
    ret->connected = 0;		       /* to start with */
    ret->writable = 0;		       /* to start with */
    ret->sending_oob = 0;
    ret->frozen = 0;
    ret->localhost_only = 0;	       /* unused, but best init anyway */
    ret->pending_error = 0;
    ret->parent = ret->child = NULL;
    ret->oobpending = FALSE;
    ret->outgoingeof = EOF_NO;
    ret->incomingeof = FALSE;
    ret->listener = 0;
    ret->addr = addr;
    START_STEP(ret->addr, ret->step);
    ret->s = -1;
    ret->oobinline = oobinline;
    ret->nodelay = nodelay;
    ret->keepalive = keepalive;
    ret->privport = privport;
    ret->port = port;

    err = 0;
    do {
        err = try_connect(ret);
    } while (err && sk_nextaddr(ret->addr, &ret->step));

    if (err)
        ret->error = strerror(err);

    return (Socket) ret;
}

Socket sk_newlistener(char *srcaddr, int port, Plug plug, int local_host_only, int orig_address_family)
{
    int s;
#ifndef NO_IPV6
    struct addrinfo hints, *ai = NULL;
    char portstr[6];
#endif
    union sockaddr_union u;
    union sockaddr_union *addr;
    int addrlen;
    Actual_Socket ret;
    int retcode;
    int address_family;
    int on = 1;

    /*
     * Create Socket structure.
     */
    ret = snew(struct Socket_tag);
    ret->fn = &tcp_fn_table;
    ret->error = NULL;
    ret->plug = plug;
    bufchain_init(&ret->output_data);
    ret->writable = 0;		       /* to start with */
    ret->sending_oob = 0;
    ret->frozen = 0;
    ret->localhost_only = local_host_only;
    ret->pending_error = 0;
    ret->parent = ret->child = NULL;
    ret->oobpending = FALSE;
    ret->outgoingeof = EOF_NO;
    ret->incomingeof = FALSE;
    ret->listener = 1;
    ret->addr = NULL;
    ret->s = -1;

    /*
     * Translate address_family from platform-independent constants
     * into local reality.
     */
    address_family = (orig_address_family == ADDRTYPE_IPV4 ? AF_INET :
#ifndef NO_IPV6
		      orig_address_family == ADDRTYPE_IPV6 ? AF_INET6 :
#endif
		      AF_UNSPEC);

#ifndef NO_IPV6
    /* Let's default to IPv6.
     * If the stack doesn't support IPv6, we will fall back to IPv4. */
    if (address_family == AF_UNSPEC) address_family = AF_INET6;
#else
    /* No other choice, default to IPv4 */
    if (address_family == AF_UNSPEC)  address_family = AF_INET;
#endif

    /*
     * Open socket.
     */
    s = socket(address_family, SOCK_STREAM, 0);

#ifndef NO_IPV6
    /* If the host doesn't support IPv6 try fallback to IPv4. */
    if (s < 0 && address_family == AF_INET6) {
    	address_family = AF_INET;
    	s = socket(address_family, SOCK_STREAM, 0);
    }
#endif

    if (s < 0) {
	ret->error = strerror(errno);
	return (Socket) ret;
    }

    cloexec(s);

    ret->oobinline = 0;

    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR,
                   (const char *)&on, sizeof(on)) < 0) {
        ret->error = strerror(errno);
        close(s);
        return (Socket) ret;
    }

    retcode = -1;
    addr = NULL; addrlen = -1;         /* placate optimiser */

    if (srcaddr != NULL) {
#ifndef NO_IPV6
        hints.ai_flags = AI_NUMERICHOST;
        hints.ai_family = address_family;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = 0;
        hints.ai_addrlen = 0;
        hints.ai_addr = NULL;
        hints.ai_canonname = NULL;
        hints.ai_next = NULL;
	assert(port >= 0 && port <= 99999);
        sprintf(portstr, "%d", port);
        {
            char *trimmed_addr = host_strduptrim(srcaddr);
            retcode = getaddrinfo(trimmed_addr, portstr, &hints, &ai);
            sfree(trimmed_addr);
        }
	if (retcode == 0) {
	    addr = (union sockaddr_union *)ai->ai_addr;
	    addrlen = ai->ai_addrlen;
	}
#else
        memset(&u,'\0',sizeof u);
        u.sin.sin_family = AF_INET;
        u.sin.sin_port = htons(port);
        u.sin.sin_addr.s_addr = inet_addr(srcaddr);
        if (u.sin.sin_addr.s_addr != (in_addr_t)(-1)) {
            /* Override localhost_only with specified listen addr. */
            ret->localhost_only = ipv4_is_loopback(u.sin.sin_addr);
        }
        addr = &u;
        addrlen = sizeof(u.sin);
        retcode = 0;
#endif
    }

    if (retcode != 0) {
        memset(&u,'\0',sizeof u);
#ifndef NO_IPV6
        if (address_family == AF_INET6) {
            u.sin6.sin6_family = AF_INET6;
            u.sin6.sin6_port = htons(port);
            if (local_host_only)
                u.sin6.sin6_addr = in6addr_loopback;
            else
                u.sin6.sin6_addr = in6addr_any;
            addr = &u;
            addrlen = sizeof(u.sin6);
        } else
#endif
        {
            u.sin.sin_family = AF_INET;
            u.sin.sin_port = htons(port);
	    if (local_host_only)
		u.sin.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	    else
		u.sin.sin_addr.s_addr = htonl(INADDR_ANY);
            addr = &u;
            addrlen = sizeof(u.sin);
        }
    }

    retcode = bind(s, &addr->sa, addrlen);

#ifndef NO_IPV6
    if (ai)
        freeaddrinfo(ai);
#endif

    if (retcode < 0) {
        close(s);
	ret->error = strerror(errno);
	return (Socket) ret;
    }

    if (listen(s, SOMAXCONN) < 0) {
        close(s);
	ret->error = strerror(errno);
	return (Socket) ret;
    }

#ifndef NO_IPV6
    /*
     * If we were given ADDRTYPE_UNSPEC, we must also create an
     * IPv4 listening socket and link it to this one.
     */
    if (address_family == AF_INET6 && orig_address_family == ADDRTYPE_UNSPEC) {
        Actual_Socket other;

        other = (Actual_Socket) sk_newlistener(srcaddr, port, plug,
                                               local_host_only, ADDRTYPE_IPV4);

        if (other) {
            if (!other->error) {
                other->parent = ret;
                ret->child = other;
            } else {
                /* If we couldn't create a listening socket on IPv4 as well
                 * as IPv6, we must return an error overall. */
                close(s);
                sfree(ret);
                return (Socket) other;
            }
        }
    }
#endif

    ret->s = s;

    uxsel_tell(ret);
    add234(sktree, ret);

    return (Socket) ret;
}

static void sk_tcp_close(Socket sock)
{
    Actual_Socket s = (Actual_Socket) sock;

    if (s->child)
        sk_tcp_close((Socket)s->child);

    uxsel_del(s->s);
    del234(sktree, s);
    close(s->s);
    if (s->addr)
        sk_addr_free(s->addr);
    sfree(s);
}

void *sk_getxdmdata(void *sock, int *lenp)
{
    Actual_Socket s = (Actual_Socket) sock;
    union sockaddr_union u;
    socklen_t addrlen;
    char *buf;
    static unsigned int unix_addr = 0xFFFFFFFF;

    /*
     * We must check that this socket really _is_ an Actual_Socket.
     */
    if (s->fn != &tcp_fn_table)
	return NULL;		       /* failure */

    addrlen = sizeof(u);
    if (getsockname(s->s, &u.sa, &addrlen) < 0)
	return NULL;
    switch(u.sa.sa_family) {
      case AF_INET:
	*lenp = 6;
	buf = snewn(*lenp, char);
	PUT_32BIT_MSB_FIRST(buf, ntohl(u.sin.sin_addr.s_addr));
	PUT_16BIT_MSB_FIRST(buf+4, ntohs(u.sin.sin_port));
	break;
#ifndef NO_IPV6
    case AF_INET6:
	*lenp = 6;
	buf = snewn(*lenp, char);
	if (IN6_IS_ADDR_V4MAPPED(&u.sin6.sin6_addr)) {
	    memcpy(buf, u.sin6.sin6_addr.s6_addr + 12, 4);
	    PUT_16BIT_MSB_FIRST(buf+4, ntohs(u.sin6.sin6_port));
	} else
	    /* This is stupid, but it's what XLib does. */
	    memset(buf, 0, 6);
	break;
#endif
      case AF_UNIX:
	*lenp = 6;
	buf = snewn(*lenp, char);
	PUT_32BIT_MSB_FIRST(buf, unix_addr--);
        PUT_16BIT_MSB_FIRST(buf+4, getpid());
	break;

	/* XXX IPV6 */

      default:
	return NULL;
    }

    return buf;
}

/*
 * Deal with socket errors detected in try_send().
 */
static void socket_error_callback(void *vs)
{
    Actual_Socket s = (Actual_Socket)vs;

    /*
     * Just in case other socket work has caused this socket to vanish
     * or become somehow non-erroneous before this callback arrived...
     */
    if (!find234(sktree, s, NULL) || !s->pending_error)
        return;

    /*
     * An error has occurred on this socket. Pass it to the plug.
     */
    plug_closing(s->plug, strerror(s->pending_error), s->pending_error, 0);
}

/*
 * The function which tries to send on a socket once it's deemed
 * writable.
 */
void try_send(Actual_Socket s)
{
    while (s->sending_oob || bufchain_size(&s->output_data) > 0) {
	int nsent;
	int err;
	void *data;
	int len, urgentflag;

	if (s->sending_oob) {
	    urgentflag = MSG_OOB;
	    len = s->sending_oob;
	    data = &s->oobdata;
	} else {
	    urgentflag = 0;
	    bufchain_prefix(&s->output_data, &data, &len);
	}
	nsent = send(s->s, data, len, urgentflag);
	noise_ultralight(nsent);
	if (nsent <= 0) {
	    err = (nsent < 0 ? errno : 0);
	    if (err == EWOULDBLOCK) {
		/*
		 * Perfectly normal: we've sent all we can for the moment.
		 */
		s->writable = FALSE;
		return;
	    } else {
		/*
		 * We unfortunately can't just call plug_closing(),
		 * because it's quite likely that we're currently
		 * _in_ a call from the code we'd be calling back
		 * to, so we'd have to make half the SSH code
		 * reentrant. Instead we flag a pending error on
		 * the socket, to be dealt with (by calling
		 * plug_closing()) at some suitable future moment.
		 */
		s->pending_error = err;
                /*
                 * Immediately cease selecting on this socket, so that
                 * we don't tight-loop repeatedly trying to do
                 * whatever it was that went wrong.
                 */
                uxsel_tell(s);
                /*
                 * Arrange to be called back from the top level to
                 * deal with the error condition on this socket.
                 */
                queue_toplevel_callback(socket_error_callback, s);
		return;
	    }
	} else {
	    if (s->sending_oob) {
		if (nsent < len) {
		    memmove(s->oobdata, s->oobdata+nsent, len-nsent);
		    s->sending_oob = len - nsent;
		} else {
		    s->sending_oob = 0;
		}
	    } else {
		bufchain_consume(&s->output_data, nsent);
	    }
	}
    }

    /*
     * If we reach here, we've finished sending everything we might
     * have needed to send. Send EOF, if we need to.
     */
    if (s->outgoingeof == EOF_PENDING) {
        shutdown(s->s, SHUT_WR);
        s->outgoingeof = EOF_SENT;
    }

    /*
     * Also update the select status, because we don't need to select
     * for writing any more.
     */
    uxsel_tell(s);
}

static int sk_tcp_write(Socket sock, const char *buf, int len)
{
    Actual_Socket s = (Actual_Socket) sock;

    assert(s->outgoingeof == EOF_NO);

    /*
     * Add the data to the buffer list on the socket.
     */
    bufchain_add(&s->output_data, buf, len);

    /*
     * Now try sending from the start of the buffer list.
     */
    if (s->writable)
	try_send(s);

    /*
     * Update the select() status to correctly reflect whether or
     * not we should be selecting for write.
     */
    uxsel_tell(s);

    return bufchain_size(&s->output_data);
}

static int sk_tcp_write_oob(Socket sock, const char *buf, int len)
{
    Actual_Socket s = (Actual_Socket) sock;

    assert(s->outgoingeof == EOF_NO);

    /*
     * Replace the buffer list on the socket with the data.
     */
    bufchain_clear(&s->output_data);
    assert(len <= sizeof(s->oobdata));
    memcpy(s->oobdata, buf, len);
    s->sending_oob = len;

    /*
     * Now try sending from the start of the buffer list.
     */
    if (s->writable)
	try_send(s);

    /*
     * Update the select() status to correctly reflect whether or
     * not we should be selecting for write.
     */
    uxsel_tell(s);

    return s->sending_oob;
}

static void sk_tcp_write_eof(Socket sock)
{
    Actual_Socket s = (Actual_Socket) sock;

    assert(s->outgoingeof == EOF_NO);

    /*
     * Mark the socket as pending outgoing EOF.
     */
    s->outgoingeof = EOF_PENDING;

    /*
     * Now try sending from the start of the buffer list.
     */
    if (s->writable)
	try_send(s);

    /*
     * Update the select() status to correctly reflect whether or
     * not we should be selecting for write.
     */
    uxsel_tell(s);
}

static int net_select_result(int fd, int event)
{
    int ret;
    char buf[20480];		       /* nice big buffer for plenty of speed */
    Actual_Socket s;
    u_long atmark;

    /* Find the Socket structure */
    s = find234(sktree, &fd, cmpforsearch);
    if (!s)
	return 1;		       /* boggle */

    noise_ultralight(event);

    switch (event) {
      case 4:			       /* exceptional */
	if (!s->oobinline) {
	    /*
	     * On a non-oobinline socket, this indicates that we
	     * can immediately perform an OOB read and get back OOB
	     * data, which we will send to the back end with
	     * type==2 (urgent data).
	     */
	    ret = recv(s->s, buf, sizeof(buf), MSG_OOB);
	    noise_ultralight(ret);
	    if (ret <= 0) {
                return plug_closing(s->plug,
				    ret == 0 ? "Internal networking trouble" :
				    strerror(errno), errno, 0);
	    } else {
                /*
                 * Receiving actual data on a socket means we can
                 * stop falling back through the candidate
                 * addresses to connect to.
                 */
                if (s->addr) {
                    sk_addr_free(s->addr);
                    s->addr = NULL;
                }
		return plug_receive(s->plug, 2, buf, ret);
	    }
	    break;
	}

	/*
	 * If we reach here, this is an oobinline socket, which
	 * means we should set s->oobpending and then deal with it
	 * when we get called for the readability event (which
	 * should also occur).
	 */
	s->oobpending = TRUE;
        break;
      case 1: 			       /* readable; also acceptance */
	if (s->listener) {
	    /*
	     * On a listening socket, the readability event means a
	     * connection is ready to be accepted.
	     */
	    union sockaddr_union su;
	    socklen_t addrlen = sizeof(su);
            accept_ctx_t actx;
	    int t;  /* socket of connection */

	    memset(&su, 0, addrlen);
	    t = accept(s->s, &su.sa, &addrlen);
	    if (t < 0) {
		break;
	    }

            nonblock(t);
            actx.i = t;

	    if ((!s->addr || s->addr->superfamily != UNIX) &&
                s->localhost_only && !sockaddr_is_loopback(&su.sa)) {
		close(t);	       /* someone let nonlocal through?! */
	    } else if (plug_accepting(s->plug, sk_tcp_accept, actx)) {
		close(t);	       /* denied or error */
	    }
	    break;
	}

	/*
	 * If we reach here, this is not a listening socket, so
	 * readability really means readability.
	 */

	/* In the case the socket is still frozen, we don't even bother */
	if (s->frozen)
	    break;

	/*
	 * We have received data on the socket. For an oobinline
	 * socket, this might be data _before_ an urgent pointer,
	 * in which case we send it to the back end with type==1
	 * (data prior to urgent).
	 */
	if (s->oobinline && s->oobpending) {
	    atmark = 1;
	    if (ioctl(s->s, SIOCATMARK, &atmark) == 0 && atmark)
		s->oobpending = FALSE; /* clear this indicator */
	} else
	    atmark = 1;

	ret = recv(s->s, buf, s->oobpending ? 1 : sizeof(buf), 0);
	noise_ultralight(ret);
	if (ret < 0) {
	    if (errno == EWOULDBLOCK) {
		break;
	    }
	}
	if (ret < 0) {
            /*
             * An error at this point _might_ be an error reported
             * by a non-blocking connect(). So before we return a
             * panic status to the user, let's just see whether
             * that's the case.
             */
            int err = errno;
	    if (s->addr) {
		plug_log(s->plug, 1, s->addr, s->port, strerror(err), err);
		while (s->addr && sk_nextaddr(s->addr, &s->step)) {
		    err = try_connect(s);
		}
	    }
            if (err != 0)
                return plug_closing(s->plug, strerror(err), err, 0);
	} else if (0 == ret) {
            s->incomingeof = TRUE;     /* stop trying to read now */
            uxsel_tell(s);
	    return plug_closing(s->plug, NULL, 0, 0);
	} else {
            /*
             * Receiving actual data on a socket means we can
             * stop falling back through the candidate
             * addresses to connect to.
             */
            if (s->addr) {
                sk_addr_free(s->addr);
                s->addr = NULL;
            }
	    return plug_receive(s->plug, atmark ? 0 : 1, buf, ret);
	}
	break;
      case 2:			       /* writable */
	if (!s->connected) {
	    /*
	     * select() reports a socket as _writable_ when an
	     * asynchronous connection is completed.
	     */
	    s->connected = s->writable = 1;
	    uxsel_tell(s);
	    break;
	} else {
	    int bufsize_before, bufsize_after;
	    s->writable = 1;
	    bufsize_before = s->sending_oob + bufchain_size(&s->output_data);
	    try_send(s);
	    bufsize_after = s->sending_oob + bufchain_size(&s->output_data);
	    if (bufsize_after < bufsize_before)
		plug_sent(s->plug, bufsize_after);
	}
	break;
    }

    return 1;
}

/*
 * Special error values are returned from sk_namelookup and sk_new
 * if there's a problem. These functions extract an error message,
 * or return NULL if there's no problem.
 */
const char *sk_addr_error(SockAddr addr)
{
    return addr->error;
}
static const char *sk_tcp_socket_error(Socket sock)
{
    Actual_Socket s = (Actual_Socket) sock;
    return s->error;
}

static void sk_tcp_set_frozen(Socket sock, int is_frozen)
{
    Actual_Socket s = (Actual_Socket) sock;
    if (s->frozen == is_frozen)
	return;
    s->frozen = is_frozen;
    uxsel_tell(s);
}

static char *sk_tcp_peer_info(Socket sock)
{
    Actual_Socket s = (Actual_Socket) sock;
    union sockaddr_union addr;
    socklen_t addrlen = sizeof(addr);
#ifndef NO_IPV6
    char buf[INET6_ADDRSTRLEN];
#endif

    if (getpeername(s->s, &addr.sa, &addrlen) < 0)
        return NULL;
    if (addr.storage.ss_family == AF_INET) {
        return dupprintf
            ("%s:%d",
             inet_ntoa(addr.sin.sin_addr),
             (int)ntohs(addr.sin.sin_port));
#ifndef NO_IPV6
    } else if (addr.storage.ss_family == AF_INET6) {
        return dupprintf
            ("[%s]:%d",
             inet_ntop(AF_INET6, &addr.sin6.sin6_addr, buf, sizeof(buf)),
             (int)ntohs(addr.sin6.sin6_port));
#endif
    } else if (addr.storage.ss_family == AF_UNIX) {
        /*
         * For Unix sockets, the source address is unlikely to be
         * helpful. Instead, we try SO_PEERCRED and try to get the
         * source pid.
         */
        int pid, uid, gid;
        if (so_peercred(s->s, &pid, &uid, &gid)) {
            char uidbuf[64], gidbuf[64];
            sprintf(uidbuf, "%d", uid);
            sprintf(gidbuf, "%d", gid);
            struct passwd *pw = getpwuid(uid);
            struct group *gr = getgrgid(gid);
            return dupprintf("pid %d (%s:%s)", pid,
                             pw ? pw->pw_name : uidbuf,
                             gr ? gr->gr_name : gidbuf);
        }
        return NULL;
    } else {
        return NULL;
    }
}

static void uxsel_tell(Actual_Socket s)
{
    int rwx = 0;
    if (!s->pending_error) {
        if (s->listener) {
            rwx |= 1;                  /* read == accept */
        } else {
            if (!s->connected)
                rwx |= 2;              /* write == connect */
            if (s->connected && !s->frozen && !s->incomingeof)
                rwx |= 1 | 4;          /* read, except */
            if (bufchain_size(&s->output_data))
                rwx |= 2;              /* write */
        }
    }
    uxsel_set(s->s, rwx, net_select_result);
}

int net_service_lookup(char *service)
{
    struct servent *se;
    se = getservbyname(service, NULL);
    if (se != NULL)
	return ntohs(se->s_port);
    else
	return 0;
}

char *get_hostname(void)
{
    int len = 128;
    char *hostname = NULL;
    do {
	len *= 2;
	hostname = sresize(hostname, len, char);
	if ((gethostname(hostname, len) < 0) &&
	    (errno != ENAMETOOLONG)) {
	    sfree(hostname);
	    hostname = NULL;
	    break;
	}
    } while (strlen(hostname) >= len-1);
    return hostname;
}

SockAddr platform_get_x11_unix_address(const char *sockpath, int displaynum)
{
    SockAddr ret = snew(struct SockAddr_tag);
    int n;

    memset(ret, 0, sizeof *ret);
    ret->superfamily = UNIX;
    /*
     * In special circumstances (notably Mac OS X Leopard), we'll
     * have been passed an explicit Unix socket path.
     */
    if (sockpath) {
	n = snprintf(ret->hostname, sizeof ret->hostname,
		     "%s", sockpath);
    } else {
	n = snprintf(ret->hostname, sizeof ret->hostname,
		     "%s%d", X11_UNIX_PATH, displaynum);
    }

    if (n < 0)
	ret->error = "snprintf failed";
    else if (n >= sizeof ret->hostname)
	ret->error = "X11 UNIX name too long";

#ifndef NO_IPV6
    ret->ais = NULL;
#else
    ret->addresses = NULL;
    ret->naddresses = 0;
#endif
    ret->refcount = 1;
    return ret;
}

SockAddr unix_sock_addr(const char *path)
{
    SockAddr ret = snew(struct SockAddr_tag);
    int n;

    memset(ret, 0, sizeof *ret);
    ret->superfamily = UNIX;
    n = snprintf(ret->hostname, sizeof ret->hostname, "%s", path);

    if (n < 0)
	ret->error = "snprintf failed";
    else if (n >= sizeof ret->hostname)
	ret->error = "socket pathname too long";

#ifndef NO_IPV6
    ret->ais = NULL;
#else
    ret->addresses = NULL;
    ret->naddresses = 0;
#endif
    ret->refcount = 1;
    return ret;
}

Socket new_unix_listener(SockAddr listenaddr, Plug plug)
{
    int s;
    union sockaddr_union u;
    union sockaddr_union *addr;
    int addrlen;
    Actual_Socket ret;
    int retcode;

    /*
     * Create Socket structure.
     */
    ret = snew(struct Socket_tag);
    ret->fn = &tcp_fn_table;
    ret->error = NULL;
    ret->plug = plug;
    bufchain_init(&ret->output_data);
    ret->writable = 0;		       /* to start with */
    ret->sending_oob = 0;
    ret->frozen = 0;
    ret->localhost_only = TRUE;
    ret->pending_error = 0;
    ret->parent = ret->child = NULL;
    ret->oobpending = FALSE;
    ret->outgoingeof = EOF_NO;
    ret->incomingeof = FALSE;
    ret->listener = 1;
    ret->addr = listenaddr;
    ret->s = -1;

    assert(listenaddr->superfamily == UNIX);

    /*
     * Open socket.
     */
    s = socket(AF_UNIX, SOCK_STREAM, 0);
    if (s < 0) {
	ret->error = strerror(errno);
	return (Socket) ret;
    }

    cloexec(s);

    ret->oobinline = 0;

    memset(&u, '\0', sizeof(u));
    u.su.sun_family = AF_UNIX;
    strncpy(u.su.sun_path, listenaddr->hostname, sizeof(u.su.sun_path)-1);
    addr = &u;
    addrlen = sizeof(u.su);

    if (unlink(u.su.sun_path) < 0 && errno != ENOENT) {
        close(s);
	ret->error = strerror(errno);
	return (Socket) ret;
    }

    retcode = bind(s, &addr->sa, addrlen);
    if (retcode < 0) {
        close(s);
	ret->error = strerror(errno);
	return (Socket) ret;
    }

    if (listen(s, SOMAXCONN) < 0) {
        close(s);
	ret->error = strerror(errno);
	return (Socket) ret;
    }

    ret->s = s;

    uxsel_tell(ret);
    add234(sktree, ret);

    return (Socket) ret;
}
