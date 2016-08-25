/*
 * ux_x11.c: fetch local auth data for X forwarding.
 */

#include <ctype.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <errno.h>

#include "putty.h"
#include "ssh.h"
#include "network.h"

void platform_get_x11_auth(struct X11Display *disp, Conf *conf)
{
    char *xauthfile;
    int needs_free;

    /*
     * Find the .Xauthority file.
     */
    needs_free = FALSE;
    xauthfile = getenv("XAUTHORITY");
    if (!xauthfile) {
	xauthfile = getenv("HOME");
	if (xauthfile) {
	    xauthfile = dupcat(xauthfile, "/.Xauthority", NULL);
	    needs_free = TRUE;
	}
    }

    if (xauthfile) {
	x11_get_auth_from_authfile(disp, xauthfile);
	if (needs_free)
	    sfree(xauthfile);
    }
}

const int platform_uses_x11_unix_by_default = TRUE;
