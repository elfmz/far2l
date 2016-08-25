/*
 * pinger.c: centralised module that deals with sending TS_PING
 * keepalives, to avoid replicating this code in multiple backends.
 */

#include "putty.h"

struct pinger_tag {
    int interval;
    int pending;
    unsigned long next;
    Backend *back;
    void *backhandle;
};

static void pinger_schedule(Pinger pinger);

static void pinger_timer(void *ctx, unsigned long now)
{
    Pinger pinger = (Pinger)ctx;

    if (pinger->pending && now == pinger->next) {
	pinger->back->special(pinger->backhandle, TS_PING);
	pinger->pending = FALSE;
	pinger_schedule(pinger);
    }
}

static void pinger_schedule(Pinger pinger)
{
    int next;

    if (!pinger->interval) {
	pinger->pending = FALSE;       /* cancel any pending ping */
	return;
    }

    next = schedule_timer(pinger->interval * TICKSPERSEC,
			  pinger_timer, pinger);
    if (!pinger->pending || next < pinger->next) {
	pinger->next = next;
	pinger->pending = TRUE;
    }
}

Pinger pinger_new(Conf *conf, Backend *back, void *backhandle)
{
    Pinger pinger = snew(struct pinger_tag);

    pinger->interval = conf_get_int(conf, CONF_ping_interval);
    pinger->pending = FALSE;
    pinger->back = back;
    pinger->backhandle = backhandle;
    pinger_schedule(pinger);

    return pinger;
}

void pinger_reconfig(Pinger pinger, Conf *oldconf, Conf *newconf)
{
    int newinterval = conf_get_int(newconf, CONF_ping_interval);
    if (conf_get_int(oldconf, CONF_ping_interval) != newinterval) {
	pinger->interval = newinterval;
	pinger_schedule(pinger);
    }
}

void pinger_free(Pinger pinger)
{
    expire_timer_context(pinger);
    sfree(pinger);
}
