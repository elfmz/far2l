#include "putty.h"
#ifndef NO_GSSAPI

/* For platforms not supporting GSSAPI */

struct ssh_gss_liblist *ssh_gss_setup(Conf *conf)
{
    struct ssh_gss_liblist *list = snew(struct ssh_gss_liblist *);
    list->libraries = NULL;
    list->nlibraries = 0;
    return list;
}

void ssh_gss_cleanup(struct ssh_gss_liblist *list)
{
    sfree(list);
}

#endif /* NO_GSSAPI */
