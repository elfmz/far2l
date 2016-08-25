/*
 * Routines to refuse to do cryptographic interaction with proxies
 * in PuTTY. This is a stub implementation of the same interfaces
 * provided by cproxy.c, for use in PuTTYtel.
 */

#include <assert.h>
#include <ctype.h>
#include <string.h>

#define DEFINE_PLUG_METHOD_MACROS
#include "putty.h"
#include "network.h"
#include "proxy.h"

void proxy_socks5_offerencryptedauth(char * command, int * len)
{
    /* For telnet, don't add any new encrypted authentication routines */
}

int proxy_socks5_handlechap (Proxy_Socket p)
{

    plug_closing(p->plug, "Proxy error: Trying to handle a SOCKS5 CHAP request"
		 " in telnet-only build",
		 PROXY_ERROR_GENERAL, 0);
    return 1;
}

int proxy_socks5_selectchap(Proxy_Socket p)
{
    plug_closing(p->plug, "Proxy error: Trying to handle a SOCKS5 CHAP request"
		 " in telnet-only build",
		 PROXY_ERROR_GENERAL, 0);
    return 1;
}
