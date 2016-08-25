#include "putty.h"
#ifndef NO_GSSAPI
#include "pgssapi.h"
#include "sshgss.h"
#include "sshgssc.h"

/* Unix code to set up the GSSAPI library list. */

#if !defined NO_LIBDL && !defined NO_GSSAPI

const int ngsslibs = 4;
const char *const gsslibnames[4] = {
    "libgssapi (Heimdal)",
    "libgssapi_krb5 (MIT Kerberos)",
    "libgss (Sun)",
    "User-specified GSSAPI library",
};
const struct keyvalwhere gsslibkeywords[] = {
    { "libgssapi", 0, -1, -1 },
    { "libgssapi_krb5", 1, -1, -1 },
    { "libgss", 2, -1, -1 },
    { "custom", 3, -1, -1 },
};

/*
 * Run-time binding against a choice of GSSAPI implementations. We
 * try loading several libraries, and produce an entry in
 * ssh_gss_libraries[] for each one.
 */

static void gss_init(struct ssh_gss_library *lib, void *dlhandle,
		     int id, const char *msg)
{
    lib->id = id;
    lib->gsslogmsg = msg;
    lib->handle = dlhandle;

#define BIND_GSS_FN(name) \
    lib->u.gssapi.name = (t_gss_##name) dlsym(dlhandle, "gss_" #name)

    BIND_GSS_FN(delete_sec_context);
    BIND_GSS_FN(display_status);
    BIND_GSS_FN(get_mic);
    BIND_GSS_FN(import_name);
    BIND_GSS_FN(init_sec_context);
    BIND_GSS_FN(release_buffer);
    BIND_GSS_FN(release_cred);
    BIND_GSS_FN(release_name);

#undef BIND_GSS_FN

    ssh_gssapi_bind_fns(lib);
}

/* Dynamically load gssapi libs. */
struct ssh_gss_liblist *ssh_gss_setup(Conf *conf)
{
    void *gsslib;
    char *gsspath;
    struct ssh_gss_liblist *list = snew(struct ssh_gss_liblist);

    list->libraries = snewn(4, struct ssh_gss_library);
    list->nlibraries = 0;

    /* Heimdal's GSSAPI Library */
    if ((gsslib = dlopen("libgssapi.so.2", RTLD_LAZY)) != NULL)
	gss_init(&list->libraries[list->nlibraries++], gsslib,
		 0, "Using GSSAPI from libgssapi.so.2");

    /* MIT Kerberos's GSSAPI Library */
    if ((gsslib = dlopen("libgssapi_krb5.so.2", RTLD_LAZY)) != NULL)
	gss_init(&list->libraries[list->nlibraries++], gsslib,
		 1, "Using GSSAPI from libgssapi_krb5.so.2");

    /* Sun's GSSAPI Library */
    if ((gsslib = dlopen("libgss.so.1", RTLD_LAZY)) != NULL)
	gss_init(&list->libraries[list->nlibraries++], gsslib,
		 2, "Using GSSAPI from libgss.so.1");

    /* User-specified GSSAPI library */
    gsspath = conf_get_filename(conf, CONF_ssh_gss_custom)->path;
    if (*gsspath && (gsslib = dlopen(gsspath, RTLD_LAZY)) != NULL)
	gss_init(&list->libraries[list->nlibraries++], gsslib,
		 3, dupprintf("Using GSSAPI from user-specified"
			      " library '%s'", gsspath));

    return list;
}

void ssh_gss_cleanup(struct ssh_gss_liblist *list)
{
    int i;

    /*
     * dlopen and dlclose are defined to employ reference counting
     * in the case where the same library is repeatedly dlopened, so
     * even in a multiple-sessions-per-process context it's safe to
     * naively dlclose everything here without worrying about
     * destroying it under the feet of another SSH instance still
     * using it.
     */
    for (i = 0; i < list->nlibraries; i++) {
	dlclose(list->libraries[i].handle);
	if (list->libraries[i].id == 3) {
	    /* The 'custom' id involves a dynamically allocated message.
	     * Note that we must cast away the 'const' to free it. */
	    sfree((char *)list->libraries[i].gsslogmsg);
	}
    }
    sfree(list->libraries);
    sfree(list);
}

#elif !defined NO_GSSAPI

const int ngsslibs = 1;
const char *const gsslibnames[1] = {
    "static",
};
const struct keyvalwhere gsslibkeywords[] = {
    { "static", 0, -1, -1 },
};

/*
 * Link-time binding against GSSAPI. Here we just construct a single
 * library structure containing pointers to the functions we linked
 * against.
 */

#include <gssapi/gssapi.h>

/* Dynamically load gssapi libs. */
struct ssh_gss_liblist *ssh_gss_setup(Conf *conf)
{
    struct ssh_gss_liblist *list = snew(struct ssh_gss_liblist);

    list->libraries = snew(struct ssh_gss_library);
    list->nlibraries = 1;

    list->libraries[0].gsslogmsg = "Using statically linked GSSAPI";

#define BIND_GSS_FN(name) \
    list->libraries[0].u.gssapi.name = (t_gss_##name) gss_##name

    BIND_GSS_FN(delete_sec_context);
    BIND_GSS_FN(display_status);
    BIND_GSS_FN(get_mic);
    BIND_GSS_FN(import_name);
    BIND_GSS_FN(init_sec_context);
    BIND_GSS_FN(release_buffer);
    BIND_GSS_FN(release_cred);
    BIND_GSS_FN(release_name);

#undef BIND_GSS_FN

    ssh_gssapi_bind_fns(&list->libraries[0]);

    return list;
}

void ssh_gss_cleanup(struct ssh_gss_liblist *list)
{
    sfree(list->libraries);
    sfree(list);
}

#endif /* NO_LIBDL */

#endif /* NO_GSSAPI */
